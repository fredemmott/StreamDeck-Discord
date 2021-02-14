#include "DiscordClient.h"

#include <DiscordRPCSDK/rpc_connection.h>
#include <StreamDeckSDK/EPLJSONUtils.h>
#include <StreamDeckSDK/ESDLogger.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <optional>

using namespace DiscordPayloads;

class DiscordClientThread {
 public:
  DiscordClientThread(DiscordClient* client) : _execute(false) {
    _client = client;
  }

  ~DiscordClientThread() {
    if (_execute.load(std::memory_order_acquire)) {
      stop();
    };
  }

  void stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable())
      _thd.join();
  }

  void start() {
    if (_execute.load(std::memory_order_acquire)) {
      stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this]() {
      auto f = asio::co_spawn(
        *_client->mIOContext,
        impl(),
        asio::use_future
      );
      f.wait();
    });
  }

  bool is_running() const noexcept {
    return (_execute.load(std::memory_order_acquire) && _thd.joinable());
  }

 private:
  asio::awaitable<void> impl() {
    while (_execute.load(std::memory_order_acquire)) {
      nlohmann::json buf;
      ESDDebug("Waiting for message in background thread");
      auto success = co_await _client->mConnection->AsyncRead(&buf);
      ESDDebug("Read message? {}", (const char*) (success ? "true" : "false"));
      _client->processDiscordRPCMessage(buf);
      ESDDebug("Processed message!");
    }
  }

  std::atomic<bool> _execute;
  std::thread _thd;
  DiscordClient* _client;
};

const char* DiscordClient::getRpcStateName(RpcState state) {
  switch (state) {
#define X(y) \
  case RpcState::y: \
    return #y;
    DISCORD_CLIENT_RPCSTATES
#undef X
    default:
      return "Invalid state";
  }
}

DiscordClient::DiscordClient(
  const std::shared_ptr<asio::io_context>& ctx,
  const std::string& appId,
  const std::string& appSecret,
  const Credentials& credentials) {
  mState.rpcState = RpcState::UNINITIALIZED;
  mIOContext = ctx;
  mAppId = appId;
  mAppSecret = appSecret;
  mCredentials = credentials;

  mStateCallback = [](State) {};
  mReadyCallback = mStateCallback;
  mCredentialsCallback = [](Credentials) {};
}

DiscordClient::~DiscordClient() {
  mRunning = false;
}

void DiscordClient::initializeWithBackgroundThread() {
  asio::co_spawn(
    *mIOContext,
    [this]() -> asio::awaitable<void> {
      co_await initialize();
      mProcessingThread = std::make_unique<DiscordClientThread>(this);
      mProcessingThread->start();
    },
    asio::detached
  );
}

void DiscordClient::initializeInCurrentThread() {
  asio::co_spawn(
    *mIOContext,
    [this]() -> asio::awaitable<void> {
      co_await initialize();
      mRunning = true;
      while(mRunning) {
        nlohmann::json msg;
        ESDDebug("Waiting for message in main thread");
        auto success = co_await mConnection->AsyncRead(&msg);
        if (!success) {
          ESDDebug("Failed to read message");
          break;
        }
        processDiscordRPCMessage(msg);
        ESDDebug("Processed message");
      }
    },
    asio::detached
  );
}

asio::awaitable<void> DiscordClient::initialize() {
  if (mAppId.empty() || mAppSecret.empty()) {
    setRpcState(RpcState::UNINITIALIZED, RpcState::AUTHENTICATION_FAILED);
    co_return;
  }

  setRpcState(RpcState::UNINITIALIZED, RpcState::CONNECTING);
  mConnection = std::make_unique<RpcConnection>(mIOContext, mAppId);
  mConnection->onDisconnect = [=](int code, const std::string& message) {
    ESDDebug("disconnected - {} {}", code, message.c_str());
    switch (this->mState.rpcState) {
      case RpcState::CONNECTING:
        setRpcState(RpcState::CONNECTION_FAILED);
        return;
      case RpcState::CONNECTION_FAILED:
      case RpcState::AUTHENTICATION_FAILED:
        return;
      default:
        setRpcState(RpcState::DISCONNECTED);
        return;
    }
  };

  co_await mConnection->AsyncOpen();

  if (mCredentials.accessToken.empty()) {
    setRpcState(RpcState::CONNECTING, RpcState::REQUESTING_USER_PERMISSION);
    mConnection->Write(
      {{"nonce", getNextNonce()},
       {"cmd", "AUTHORIZE"},
       {"args", {{"client_id", mAppId}, {"scopes", {"identify", "rpc"}}}}});
    co_return;
  }

  setRpcState(RpcState::CONNECTING, RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
  mConnection->Write(
    {{"nonce", getNextNonce()},
     {"cmd", "AUTHENTICATE"},
     {"args", {{"access_token", mCredentials.accessToken}}}});
  co_return;

}

void DiscordClient::startAuthenticationWithNewAccessToken() {
  setRpcState(
    RpcState::REQUESTING_ACCESS_TOKEN,
    RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
  mConnection->Write(
    {{"nonce", getNextNonce()},
     {"cmd", "AUTHENTICATE"},
     {"args", {"access_token", mCredentials.accessToken}}});
}

// Should be unneeded in a later release of nlohmann/json
// https://github.com/nlohmann/json/pull/2117
namespace nlohmann {
template <typename T>
struct adl_serializer<std::optional<T>> {
  static void to_json(json& j, const std::optional<T>& opt) {
    if (opt == std::nullopt) {
      j = nullptr;
    } else {
      j = *opt;
    }
  }

  static void from_json(const json& j, std::optional<T>& opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};
}// namespace nlohmann

namespace {
  struct BaseMessage {
    std::string cmd;
    std::optional<std::string> nonce;
    std::optional<std::string> evt;
    std::optional<nlohmann::json> data;
  };
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BaseMessage, cmd, nonce, evt, data);
} // namespace

bool DiscordClient::processDiscordRPCMessage(const nlohmann::json& message) {
  ESDDebug("Received message {}", message.dump());
  auto parsed = message.get<BaseMessage>();
  if (parsed.nonce.has_value()) {
    const auto nonce = *parsed.nonce;
    ESDDebug("Nonce: {}", nonce);
    auto promise = mPromises.find(nonce);
    if (promise != mPromises.end()) {
      ESDDebug("Found promise");
      promise->second.resolve(message["data"]);
      mPromises.erase(nonce);
      return true;
    }
    ESDDebug("No promise");
  }

  const auto command = parsed.cmd;
  const auto event = parsed.evt;
  const auto data = parsed.data;
  if (command == "AUTHORIZE") {
    const auto code = data->value("code", "");
    if (code.empty() || event == "error") {
      setRpcState(
        RpcState::REQUESTING_USER_PERMISSION, RpcState::AUTHENTICATION_FAILED);
      return false;
    }
    setRpcState(
      RpcState::REQUESTING_USER_PERMISSION, RpcState::REQUESTING_ACCESS_TOKEN);
    mCredentials = getOAuthCredentials("authorization_code", "code", code);
    mCredentialsCallback(mCredentials);
    if (mCredentials.accessToken.empty()) {
      setRpcState(
        RpcState::REQUESTING_ACCESS_TOKEN, RpcState::AUTHENTICATION_FAILED);
      return false;
    }
    startAuthenticationWithNewAccessToken();
    return true;
  }

  if (command == "AUTHENTICATE") {
    if (event == "ERROR") {
      mCredentials.accessToken.clear();
      if (mCredentials.refreshToken.empty()) {
        setRpcState(
          RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN,
          RpcState::AUTHENTICATION_FAILED);
        return false;
      }
      setRpcState(
        RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN,
        RpcState::REQUESTING_ACCESS_TOKEN);
      mCredentials = getOAuthCredentials(
        "refresh_token", "refresh_token", mCredentials.refreshToken);
      mCredentialsCallback(mCredentials);
      if (mCredentials.accessToken.empty()) {
        setRpcState(
          RpcState::REQUESTING_ACCESS_TOKEN,
          RpcState::REQUESTING_USER_PERMISSION);
        mConnection->Write(
          {{"nonce", getNextNonce()},
           {"cmd", "AUTHORIZE"},
           {"args", {{"client_id", mAppId}, {"scopes", {"identify", "rpc"}}}}});
        return true;
      }
      startAuthenticationWithNewAccessToken();
      return true;
    }
    setRpcState(
      RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN,
      RpcState::REQUESTING_VOICE_STATE);
    mConnection->Write(
      {{"nonce", getNextNonce()},
       {"cmd", "SUBSCRIBE"},
       {"evt", "VOICE_SETTINGS_UPDATE"}});
    mConnection->Write({
      {"nonce", getNextNonce()},
      {"cmd", "GET_VOICE_SETTINGS"},
    });
    return true;
  }

  if (command == "GET_VOICE_SETTINGS" || event == "VOICE_SETTINGS_UPDATE") {
    if (data) {
      if (mState.rpcState == RpcState::REQUESTING_VOICE_STATE) {
        setRpcState(RpcState::REQUESTING_VOICE_STATE, RpcState::READY);
      }
      ESDDebug("Have data");
      const auto response = data->get<VoiceSettingsResponse>();
      ESDDebug("Decoded");
      mState.isMuted = response.mute;
      mState.isDeafened = response.deaf;

      json mode;
      const bool haveMode = EPLJSONUtils::GetObjectByName(data, "mode", mode);
      ESDDebug("have mode? {}", haveMode);
      if (haveMode) {
        const auto type = EPLJSONUtils::GetStringByName(mode, "type");
        if (type == "PUSH_TO_TALK") {
          mState.isPTT = true;
        } else if (type == "VOICE_ACTIVITY") {
          mState.isPTT = false;
        }
      }
      if (mStateCallback) {
        mStateCallback(mState);
      }
      ESDDebug("All done");
    }
    return true;
  }

  return true;
}

std::string DiscordClient::getAppId() const {
  return mAppId;
}

std::string DiscordClient::getAppSecret() const {
  return mAppSecret;
}

DiscordClient::State DiscordClient::getState() const {
  return mState;
}

void DiscordClient::onStateChanged(StateCallback cb) {
  mStateCallback = cb;
}

void DiscordClient::onReady(StateCallback cb) {
  mReadyCallback = cb;
}

void DiscordClient::onCredentialsChanged(CredentialsCallback cb) {
  mCredentialsCallback = cb;
}

void DiscordClient::setIsMuted(bool mute) {
  // Don't update the state locally to avoid displaying mic mute when not muted.
  // 1. We ask discord to mute or unmute
  // 2. discord does that
  // 3. discord tells subscribing apps (including us) about that
  // 4. we update the state when discord says it's changed
  json args;
  args["mute"] = mute;
  mConnection->Write(
    {{"nonce", getNextNonce()}, {"cmd", "SET_VOICE_SETTINGS"}, {"args", args}});
}

void DiscordClient::setIsDeafened(bool deaf) {
  json args;
  args["deaf"] = deaf;
  ESDDebug("Setting deaf to {}", deaf);
  mConnection->Write(
    {{"nonce", getNextNonce()}, {"cmd", "SET_VOICE_SETTINGS"}, {"args", args}});
}

void DiscordClient::setIsPTT(bool isPTT) {
  json mode;
  mode["type"] = (isPTT) ? "PUSH_TO_TALK" : "VOICE_ACTIVITY";
  json args;
  args["mode"] = mode;
  mConnection->Write(
    {{"nonce", getNextNonce()}, {"cmd", "SET_VOICE_SETTINGS"}, {"args", args}});
}

void DiscordClient::setRpcState(RpcState state) {
  ESDDebug(
    "Changing RPC State: {} => {}", getRpcStateName(mState.rpcState),
    getRpcStateName(state));
  mState.rpcState = state;
  if (mStateCallback) {
    mStateCallback(mState);
  }
  if (state == RpcState::READY && mReadyCallback) {
    mReadyCallback(mState);
  }
}

void DiscordClient::setRpcState(RpcState oldState, RpcState newState) {
  assert(mState.rpcState == oldState);
  setRpcState(newState);
}

template<typename TRet, typename TArgs>
asio::awaitable<TRet> DiscordClient::commandImpl(const char* command, const TArgs& args) {
  auto nonce = getNextNonce();
  nlohmann::json request {
    { "cmd", command },
    { "nonce", nonce },
    { "args", nlohmann::json(args) }
  };
  AwaitablePromise<nlohmann::json> p(mIOContext);
  mPromises.emplace(nonce, p);
  mConnection->Write(request.dump());
  const auto json_response = co_await p.async_wait();
  co_return json_response;
}
