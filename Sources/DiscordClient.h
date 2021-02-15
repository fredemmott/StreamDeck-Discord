#pragma once

#include "AwaitablePromise.h"
#include "DiscordPayloads.h"

#include <asio.hpp>

#include <functional>
#include <string>
#include <map>

class RpcConnection;

template<class T>
class PubSubData {
  public:
    typedef T Data;

    PubSubData() {};
    virtual ~PubSubData() {};
    PubSubData(const PubSubData&) = delete;
    void operator=(const PubSubData&) = delete;

    typedef std::function<void(const T&)> Subscriber;

    virtual const T* const get() const = 0;
    const T* const operator->() const { return get(); }

    virtual operator bool() const = 0;
    virtual void subscribe(Subscriber) = 0;
};

class DiscordClient {
 public:
#define DISCORD_CLIENT_RPCSTATES \
  X(UNINITIALIZED) X(CONNECTING) X(REQUESTING_USER_PERMISSION) \
    X(REQUESTING_ACCESS_TOKEN) X(AUTHENTICATING_WITH_ACCESS_TOKEN) \
    X(WAITING_FOR_INITIAL_DATA) X(READY) \
    X(CONNECTION_FAILED) X(AUTHENTICATION_FAILED) X(DISCONNECTED)

  enum class RpcState {
#define X(y) y,
    DISCORD_CLIENT_RPCSTATES
#undef X
  };
  static const char* getRpcStateName(RpcState state);

  struct State {
    RpcState rpcState;
  };
  typedef std::function<void(const State&)> StateCallback;
  struct Credentials {
    std::string accessToken;
    std::string refreshToken;
  };
  typedef std::function<void(const Credentials&)> CredentialsCallback;

  DiscordClient(
    const std::shared_ptr<asio::io_context>& ioContext,
    const std::string& appId,
    const std::string& appSecret,
    const Credentials& credentials);
  ~DiscordClient();

  State getState() const;
  void onStateChanged(StateCallback);
  void onReady(StateCallback);
  void onCredentialsChanged(CredentialsCallback);

  void setIsMuted(bool);
  void setIsDeafened(bool);
  void setIsPTT(bool);
  void setCurrentVoiceChannel(const std::string& channel_id);

  asio::awaitable<std::vector<DiscordPayloads::Guild>> coGetGuilds();
  inline auto coGetServers() { return coGetGuilds(); }

  // Easy mode...
  void initializeInCurrentThread();

  std::string getAppId() const;
  std::string getAppSecret() const;

  typedef PubSubData<DiscordPayloads::VoiceSettingsResponse> VoiceSettings;
  VoiceSettings& getVoiceSettings() const;

  typedef PubSubData<DiscordPayloads::VoiceChannelSelect> CurrentVoiceChannel;
  CurrentVoiceChannel& getCurrentVoiceChannel() const;

 private:
  asio::awaitable<void> initialize();

  class VoiceSettingsImpl;
  std::unique_ptr<VoiceSettingsImpl> mVoiceSettings;
  class CurrentVoiceChannelImpl;
  std::unique_ptr<CurrentVoiceChannelImpl> mCurrentVoiceChannel;

  std::unique_ptr<RpcConnection> mConnection;
  State mState;
  Credentials mCredentials;
  StateCallback mReadyCallback;
  StateCallback mStateCallback;
  CredentialsCallback mCredentialsCallback;
  std::string mAppId;
  std::string mAppSecret;
  std::future<void> mWorker;
  std::shared_ptr<asio::io_context> mIOContext;

  Credentials getOAuthCredentials(
    const std::string& grantType,
    const std::string& secretType,
    const std::string& secret);
  void setRpcState(RpcState state);
  void setRpcState(RpcState oldState, RpcState newState);
  std::string getNextNonce();
  void startAuthenticationWithNewAccessToken();
  bool processDiscordRPCMessage(const nlohmann::json& message);

  std::shared_ptr<bool> mRunning;
  std::map<std::string, AwaitablePromise<nlohmann::json>> mPromises;
  std::map<std::string, std::vector<std::function<void(const nlohmann::json&)>>> mSubscriptions;
  std::vector<AwaitablePromise<void>> mInitPromises;

  void callAndForget(const char* func, const nlohmann::json& args);
  template<typename TRet, typename TArgs>
  asio::awaitable<TRet> commandImpl(const char* command, TArgs args);

  template<typename TPubSub>
  void subscribeImpl(const char* event, std::unique_ptr<TPubSub>&,
    std::optional<
      std::function<asio::awaitable<typename TPubSub::Data>()>
    > init = std::nullopt
  );
};
