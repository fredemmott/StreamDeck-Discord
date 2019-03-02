//==============================================================================
/**
@file       MyStreamDeckPlugin.h

@brief      CPU plugin

@copyright  (c) 2018, Corsair Memory, Inc.
      This source code is licensed under the MIT-style license found in the
LICENSE file.

**/
//==============================================================================

#include <mutex>
#include "Common/ESDBasePlugin.h"

class CallBackTimer;
class DiscordClient;

class MyStreamDeckPlugin : public ESDBasePlugin {
 public:
  MyStreamDeckPlugin();
  virtual ~MyStreamDeckPlugin();

  void KeyDownForAction(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;
  void KeyUpForAction(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;

  void WillAppearForAction(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;
  void WillDisappearForAction(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;
  void DidReceiveGlobalSettings(const json& inPayload) override;

  void SendToPlugin(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;

  void DeviceDidConnect(const std::string& inDeviceID, const json& inDeviceInfo)
    override;
  void DeviceDidDisconnect(const std::string& inDeviceID) override;

 private:
  void MigrateToGlobalSettings();
  void UpdateState(bool isMuted, bool isDeafened);

  std::mutex mVisibleContextsMutex;
  std::map<std::string, std::string> mVisibleContexts;

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
  // Stored (duplicated) for each action. Used with 4.0 SDK
  Credentials mLegacyCredentials;
  // Global configuration; Used with 4.1 SDK
  Credentials mCredentials;

  DiscordClient* mClient;
  CallBackTimer* mTimer;

  void ConnectToDiscord();
  void ConnectToDiscordLater();
};
