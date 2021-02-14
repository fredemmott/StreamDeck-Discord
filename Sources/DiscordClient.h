#pragma once

#include <asio.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>

class DiscordClientThread;
class RpcConnection;

class DiscordClient {
  friend class DiscordClientThread;
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
  void initializeWithBackgroundThread();
  void initializeInCurrentThread();

  // ... or, call these
  asio::awaitable<void> initialize();
  bool processDiscordRPCMessage(const nlohmann::json& message);

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
  std::unique_ptr<DiscordClientThread> mProcessingThread;
  std::shared_ptr<asio::io_context> mIOContext;

  Credentials getOAuthCredentials(
    const std::string& grantType,
    const std::string& secretType,
    const std::string& secret);
  void setRpcState(RpcState state);
  void setRpcState(RpcState oldState, RpcState newState);
  std::string getNextNonce();
  void startAuthenticationWithNewAccessToken();

  bool mRunning = false;
};
