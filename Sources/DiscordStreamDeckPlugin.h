//==============================================================================
/**
@file       DiscordStreamDeckPlugin.h

@brief      CPU plugin

@copyright  (c) 2018, Corsair Memory, Inc.
      This source code is licensed under the MIT-style license found in the
LICENSE file.

**/
//==============================================================================

#include <StreamDeckSDK/ESDPlugin.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <map>

#include "DiscordESDAction.h"

class DiscordClient;

class DiscordStreamDeckPlugin : public ESDPlugin {
 public:
  DiscordStreamDeckPlugin();
  virtual ~DiscordStreamDeckPlugin();

  void KeyUpForAction(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;

  void DidReceiveGlobalSettings(const json& inPayload) override;

  void WillAppearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) override;

  void SendToPlugin(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;

  void ReconnectToDiscord();

  void DeviceDidConnect(const std::string& inDeviceID, const json& inDeviceInfo)
    override;

  virtual std::shared_ptr<ESDAction> GetOrCreateAction(const std::string& action, const std::string& context) override final;

 private:
  std::map<std::string, std::shared_ptr<DiscordESDAction>> mActions;

  struct Credentials {
    std::string appId;
    std::string appSecret;
    std::string oauthToken;
    std::string refreshToken;

    bool isValid() const;
    bool operator==(const Credentials& other) const;
    json toJSON() const;
    static Credentials fromJSON(const json&);
  };
  // Global configuration; Used with 4.1 SDK
  Credentials mCredentials;

  std::shared_ptr<DiscordClient> mClient;
  bool mHaveRequestedGlobalSettings;

  void ConnectToDiscord();
  void ConnectToDiscordLater();
  std::unique_ptr<asio::steady_timer> mConnectLaterTimer;
};
