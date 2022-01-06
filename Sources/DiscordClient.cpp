#include "DiscordClient.h"

#include <dpp/dpp.h>
#include <StreamDeckSDK/EPLJSONUtils.h>
#include <StreamDeckSDK/ESDLogger.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <optional>

using namespace DiscordPayloads;

namespace {

template<class T>
void resolve_future(asio::io_context& ctx, std::future<T>& f) {
	if (!f.valid()) {
    return;
  }

  while(f.wait_for(std::chrono::seconds::zero()) != std::future_status::ready) {
		ctx.poll();
	}
}


template<class T>
class PubSubDataImpl : public PubSubData<T> {
  public:
    typedef typename PubSubDataImpl<T>::Subscriber Subscriber;

    virtual void subscribe(Subscriber sub) override {
      mSubscribers.push_back(std::move(sub));
    }

    virtual operator bool() const override {
      return mData.has_value();
    }

    virtual const T* const get() const override {
      return &mData.value();
    }

    void set(const T& data) {
      mData = data;
      ESDDebug("PubSub data {}", nlohmann::json(data).dump());
      for (const auto& subscriber : mSubscribers) {
        subscriber(data);
      }
    }
  private:
    std::vector<Subscriber> mSubscribers;
    std::optional<T> mData;
};

} // namespace

class DiscordClient::VoiceSettingsImpl final : public PubSubDataImpl<DiscordPayloads::VoiceSettingsResponse> {};
DiscordClient::VoiceSettings& DiscordClient::getVoiceSettings() const {
  return *mVoiceSettings.get();
}
class DiscordClient::CurrentVoiceChannelImpl final : public PubSubDataImpl<DiscordPayloads::VoiceChannelSelect> {};
DiscordClient::CurrentVoiceChannel& DiscordClient::getCurrentVoiceChannel() const {
  return *mCurrentVoiceChannel.get();
}

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
  ESDDebug("DiscordClient::DiscordClient()");
  mRunning = std::make_shared<bool>(false);
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
  ESDDebug("DiscordClient::~DiscordClient()");
  *mRunning = false;
  if (mConnection) {
    mConnection->Close();
  }
  if (mWorker.valid()) {
    ESDDebug("Waiting for worker...");
    resolve_future(*mIOContext, mWorker);
    ESDDebug("Worker shut down.");
  }
}

asio::awaitable<std::vector<Guild>> DiscordClient::coGetGuilds() {
  const auto data = co_await commandImpl<GetGuildsResponse>("GET_GUILDS", nlohmann::json {});
  co_return data.guilds;
}

asio::awaitable<std::vector<Channel>> DiscordClient::coGetChannels(
  const std::string& guild_id
) {
  const auto data = co_await commandImpl<GetChannelsResponse>("GET_CHANNELS", nlohmann::json { {"guild_id", guild_id  }});
  co_return data.channels;
}

void DiscordClient::initializeInCurrentThread() {
  assert(!mWorker.valid());
  const auto running = mRunning;
  *running = true;
  mWorker = asio::co_spawn(
    *mIOContext,
    [this, running]() -> asio::awaitable<void> {
      co_await initialize();
      ESDDebug("Starting worker loop");
      while(*running) {
        nlohmann::json msg;
        const auto success = co_await mConnection->AsyncRead(&msg);
        if (!success) {
          ESDDebug("Failed to read message, closing connection");
          mConnection->Close();
          break;
        }
        processDiscordRPCMessage(msg);
      }
      ESDDebug("Worker loop stopped");
    },
    asio::use_future
  );
}

asio::awaitable<void> DiscordClient::initialize() {
  if (mAppId.empty() || mAppSecret.empty()) {
    setRpcState(RpcState::UNINITIALIZED, RpcState::AUTHENTICATION_FAILED);
    co_return;
  }

  setRpcState(RpcState::UNINITIALIZED, RpcState::CONNECTING);
  mConnection = std::make_unique<RpcConnection>(mIOContext, mAppId);

  if (!co_await mConnection->AsyncOpen()) {
    setRpcState(RpcState::CONNECTING, RpcState::CONNECTION_FAILED);
    co_return;
  }

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

  ESDLog("Opened connection to Discord, starting authentication");

  if (mCredentials.accessToken.empty()) {
    setRpcState(RpcState::CONNECTING, RpcState::REQUESTING_USER_PERMISSION);
    co_await mConnection->AsyncWrite(
      {{"nonce", getNextNonce()},
       {"cmd", "AUTHORIZE"},
       {"args", {{"client_id", mAppId}, {"scopes", {"identify", "rpc"}}}}});
    co_return;
  }

  setRpcState(RpcState::CONNECTING, RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
  co_await mConnection->AsyncWrite(
    {{"nonce", getNextNonce()},
     {"cmd", "AUTHENTICATE"},
     {"args", {{"access_token", mCredentials.accessToken}}}});
  co_return;

}

void DiscordClient::startAuthenticationWithNewAccessToken() {
  setRpcState(
    RpcState::REQUESTING_ACCESS_TOKEN,
    RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
  writeAndForget(
    {{"nonce", getNextNonce()},
     {"cmd", "AUTHENTICATE"},
     {"args", {"access_token", mCredentials.accessToken}}});
}

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
    // Command response
    // TODO: error handling
    const auto nonce = *parsed.nonce;
    auto promise = mPromises.find(nonce);
    if (promise != mPromises.end()) {
      promise->second.resolve(message["data"]);
      mPromises.erase(nonce);
      return true;
    }
  }

  const auto command = parsed.cmd;
  const auto data = parsed.data;

  if (command == "DISPATCH") {
    const auto event = *parsed.evt;
    if (mSubscriptions.contains(event)) {
      for (const auto& sub : mSubscriptions.at(event)) {
        sub(data);
      }
    }
    // not exiting, may have hard-coded impl below
  }

  const auto event = parsed.evt;

  if (command == "AUTHORIZE") {
    const auto code = data->value("code", "");
    if (code.empty() || event == "ERROR") {
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
        writeAndForget(
          {{"nonce", getNextNonce()},
           {"cmd", "AUTHORIZE"},
           {"args", {{"client_id", mAppId}, {"scopes", {"identify", "rpc"}}}}});
        return true;
      }
      startAuthenticationWithNewAccessToken();
      return true;
    }

    ESDLog("Authenticated/authorized, setting up subscriptions");

    setRpcState(
      RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN,
      RpcState::WAITING_FOR_INITIAL_DATA);
    subscribeImpl("VOICE_SETTINGS_UPDATE", mVoiceSettings);
    subscribeImpl(
      "VOICE_CHANNEL_SELECT",
      mCurrentVoiceChannel,
      [this]() -> asio::awaitable<CurrentVoiceChannel::Data> {
        auto result = co_await commandImpl<nlohmann::json>("GET_SELECTED_VOICE_CHANNEL", nlohmann::json {});
        if (result.is_null()) {
          co_return CurrentVoiceChannel::Data { .channel_id = std::nullopt };
        }
        co_return CurrentVoiceChannel::Data {
          .channel_id = result.at("id").get<std::string>()
        };
      }
    );
    ESDLog("Sent subscription requests");

    asio::co_spawn(
      *mIOContext,
      [this]() -> asio::awaitable<void> {
        ESDLog("Waiting for initial data");
        for (auto& kv: mInitPromises) {
          // MSVC doesn't allow auto [event, p] = kv, or in the for loop
          auto event = kv.first;
          auto p = kv.second;

          ESDLog("- waiting for {}", event);
          co_await p.async_wait();
          ESDLog("- received {}", event);
        }
        ESDLog("Received all initial data");
        setRpcState(RpcState::WAITING_FOR_INITIAL_DATA, RpcState::READY);
      },
      asio::detached
    );
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
  callAndForget("SET_VOICE_SETTINGS", {{"mute", mute}});
}

void DiscordClient::setIsDeafened(bool deaf) {
  callAndForget("SET_VOICE_SETTINGS", {{"deaf", deaf}});
}

void DiscordClient::setIsPTT(bool isPTT) {
  callAndForget("SET_VOICE_SETTINGS", {
    { "mode", { { "type", (isPTT) ? "PUSH_TO_TALK" : "VOICE_ACTIVITY" } } },
  });
}

void DiscordClient::setCurrentVoiceChannel(const std::string& id) {
  callAndForget("SELECT_VOICE_CHANNEL", {
    { "channel_id", id.empty() ? std::nullopt : std::optional(std::string(id)) }
  });
}

void DiscordClient::callAndForget(const char* command, const nlohmann::json& args) {
  writeAndForget({
    { "nonce", getNextNonce() },
    { "cmd", command },
    { "args", args },
  });
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
asio::awaitable<TRet> DiscordClient::commandImpl(const char* command, TArgs args) {
  auto nonce = getNextNonce();
  nlohmann::json request {
    { "cmd", command },
    { "nonce", nonce },
    { "args", nlohmann::json(args) }
  };
  AwaitablePromise<nlohmann::json> p(*mIOContext);
  mPromises.emplace(nonce, p);
  co_await mConnection->AsyncWrite(request);
  const nlohmann::json json_response = co_await p.async_wait();
  ESDDebug("Finished waiting for command promise");
  ESDDebug("Response: {}", json_response.dump());
  co_return json_response;
}

template<typename TPubSub>
void DiscordClient::subscribeImpl(const char* event, std::unique_ptr<TPubSub>& target,
  std::optional<
    std::function<asio::awaitable<typename TPubSub::Data>()>
   > initial_fetch) {
  if (!target) {
    target = std::make_unique<TPubSub>();
  }

  AwaitablePromise<void> p(*mIOContext);
  mInitPromises.emplace(event, p);

  auto resolve_on_dispatch = initial_fetch == std::nullopt;

  std::string event_copy(event);

  mSubscriptions[event].push_back(
    [event_copy, resolve_on_dispatch, p, &target](const nlohmann::json& data) mutable {
      ESDLog("{}: ----- received event -----", event_copy);
      auto converted = data.get<typename TPubSub::Data>();
      ESDLog("> {}: {}", event_copy, nlohmann::json(converted).dump());

      target->set(converted);

      if (resolve_on_dispatch) {
        p.resolve();
        ESDDebug("Resolved promise");
      }

      ESDLog("> {}: ----- done -----", event_copy);
    }
  );
  nlohmann::json sub {
    { "nonce", getNextNonce() },
    { "cmd", "SUBSCRIBE" },
    { "evt", event }
  };
  ESDDebug("Sending sub {}", sub.dump());
  writeAndForget(sub);
  ESDDebug("Sent sub");

  if (resolve_on_dispatch) {
    return;
  }

  asio::co_spawn(
    *mIOContext,
    [=, &target]() mutable -> asio::awaitable<void> {
      auto data = co_await (*initial_fetch)();
      target->set(data);
      p.resolve();
    },
    asio::detached
  );
}

void DiscordClient::writeAndForget(const nlohmann::json& json) {
  auto copy = json;
  asio::co_spawn(
    *mIOContext,
    [copy, this]() -> asio::awaitable<void> {
      co_await mConnection->AsyncWrite(copy);
    },
    asio::detached
  );
}

DiscordClient::Credentials DiscordClient::getOAuthCredentials(
    const std::string& grantType,
    const std::string& secretType,
    const std::string& secret) {
  ESDDebug("Spawing coroutine");
  auto future = asio::co_spawn(
    *mIOContext,
    [=]() -> asio::awaitable<Credentials> {
      co_return co_await coGetOAuthCredentials(grantType, secretType, secret);
    },
    asio::use_future
  );
  ESDDebug("resolving future");
  resolve_future(*mIOContext, future);
  ESDDebug("resolved");
  return future.get();
}
