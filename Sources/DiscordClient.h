#pragma once

#include "AwaitablePromise.h"

#include <asio.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <map>

class RpcConnection;

namespace DiscordPayloads {
  enum class VoiceSettingsModeType {
    PUSH_TO_TALK,
    VOICE_ACTIVITY,
  };
  NLOHMANN_JSON_SERIALIZE_ENUM(
    VoiceSettingsModeType, {
    { VoiceSettingsModeType::PUSH_TO_TALK, "PUSH_TO_TALK" },
    { VoiceSettingsModeType::VOICE_ACTIVITY, "VOICE_ACTIVITY" },
  });
  struct VoiceSettingsMode {
    VoiceSettingsModeType type;
  };
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    VoiceSettingsMode, type
  );
  struct VoiceSettingsResponse {
    bool deaf;
    bool mute;
    VoiceSettingsMode mode;
  };
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    VoiceSettingsResponse, deaf, mute, mode
  );
}

class DiscordClient {
 public:
#define DISCORD_CLIENT_RPCSTATES \
  X(UNINITIALIZED) X(CONNECTING) X(REQUESTING_USER_PERMISSION) \
    X(REQUESTING_ACCESS_TOKEN) X(AUTHENTICATING_WITH_ACCESS_TOKEN) \
    X(REQUESTING_VOICE_STATE) X(READY) X(CONNECTION_FAILED) \
    X(AUTHENTICATION_FAILED) X(DISCONNECTED)

  enum class RpcState {
#define X(y) y,
    DISCORD_CLIENT_RPCSTATES
#undef X
  };
  static const char* getRpcStateName(RpcState state);

  struct State {
    RpcState rpcState;
    bool isDeafened;
    bool isMuted;
    bool isPTT;
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

  // Easy mode...
  void initializeInCurrentThread();

  std::string getAppId() const;
  std::string getAppSecret() const;

 private:
  std::unique_ptr<RpcConnection> mConnection;
  State mState;
  Credentials mCredentials;
  StateCallback mReadyCallback;
  StateCallback mStateCallback;
  CredentialsCallback mCredentialsCallback;
  std::string mAppId;
  std::string mAppSecret;
  asio::awaitable<void> initialize();
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

  template<typename TRet, typename TArgs>
  asio::awaitable<TRet> commandImpl(const char* command, const TArgs& args);
};
