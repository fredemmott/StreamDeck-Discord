#pragma once

#include <functional>
#include <string>

class DiscordClientThread;
class RpcConnection;

class DiscordClient {
 public:
  enum class RpcState {
    UNINITIALIZED,
    CONNECTING,
    REQUESTING_USER_PERMISSION,
    REQUESTING_ACCESS_TOKEN,
    AUTHENTICATING_WITH_ACCESS_TOKEN,
    REQUESTING_VOICE_STATE,
    READY,
    CONNECTION_FAILED,
    AUTHENTICATION_FAILED,
    DISCONNECTED
  };
  struct State {
    RpcState rpcState;
    bool isDeafened;
    bool isMuted;
  };
  typedef std::function<void(const State&)> StateCallback;
  struct Credentials {
    std::string accessToken;
    std::string refreshToken;
  };
  typedef std::function<void(const Credentials&)> CredentialsCallback;

  DiscordClient(
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

  // Easy mode...
  void initializeWithBackgroundThread();

  // ... or, call these
  void initialize();
  bool processEvents();

 private:
  RpcConnection* mConnection;
  State mState;
  Credentials mCredentials;
  StateCallback mReadyCallback;
  StateCallback mStateCallback;
  CredentialsCallback mCredentialsCallback;
  std::string mAppId;
  std::string mAppSecret;
  DiscordClientThread* mProcessingThread;

  Credentials getOAuthCredentials(
    const std::string& grantType,
    const std::string& secretType,
    const std::string& secret);
  void setRpcState(RpcState state);
  void setRpcState(RpcState oldState, RpcState newState);
  std::string getNextNonce();
  bool processInitializationEvents();
  void startAuthenticationWithNewAccessToken();
};
