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

  void SendToPlugin(
    const std::string& inAction,
    const std::string& inContext,
    const json& inPayload,
    const std::string& inDeviceID) override;

  void DeviceDidConnect(const std::string& inDeviceID, const json& inDeviceInfo)
    override;
  void DeviceDidDisconnect(const std::string& inDeviceID) override;

 private:
  void UpdateState(bool isMuted, bool isDeafened);

  std::mutex mVisibleContextsMutex;
  std::map<std::string, std::string> mVisibleContexts;

  std::string mAppId;
  std::string mAppSecret;
  std::string mOAuthToken;
  std::string mRefreshToken;

  DiscordClient* mClient;
  CallBackTimer* mTimer;

  void ConnectToDiscord();
  void ConnectToDiscordLater();
};
