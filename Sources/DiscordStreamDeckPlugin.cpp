//==============================================================================
/**
@file	   DiscordStreamDeckPlugin.cpp

@brief	  Discord Plugin

@copyright  (c) 2018, Corsair Memory, Inc.
      (c) 2019, Frederick Emmott
      This source code is licensed under the MIT-style license found in the
LICENSE file.

**/
//==============================================================================

#include "DiscordStreamDeckPlugin.h"

#include <atomic>
#include <mutex>

#include "DeafenOffAction.h"
#include "DeafenOnAction.h"
#include "DeafenToggleAction.h"
#include "DiscordClient.h"
#include "HangupAction.h"
#include "SelfMuteOffAction.h"
#include "SelfMuteOnAction.h"
#include "SelfMuteToggleAction.h"
#include "PTTOffAction.h"
#include "PTTOnAction.h"
#include "PTTToggleAction.h"
#include "StreamDeckSDK/EPLJSONUtils.h"
#include "StreamDeckSDK/ESDConnectionManager.h"
#include "StreamDeckSDK/ESDLogger.h"

namespace {
const auto RECONNECT_PI_ACTION_ID = "com.fredemmott.discord.rpc.reconnect";
const auto REAUTHENTICATE_PI_ACTION_ID
  = "com.fredemmott.discord.rpc.reauthenticate";
const auto GET_STATE_PI_ACTION_ID = "com.fredemmott.discord.rpc.getState";
const auto STATE_PI_EVENT_ID = "com.fredemmott.discord.rpc.state";
}// namespace

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG > 201402L, "C++17 not enabled in _MSVC_LANG");
static_assert(_HAS_CXX17, "C++17 feature flag not enabled");
#endif

DiscordStreamDeckPlugin::DiscordStreamDeckPlugin() {
  mHaveRequestedGlobalSettings = false;
}

DiscordStreamDeckPlugin::~DiscordStreamDeckPlugin() {
}

void DiscordStreamDeckPlugin::KeyUpForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  ESDLog("Key Up: {} {}", inAction, inContext);
  if (!mClient) {
    return;
  }
  const auto clientState = mClient->getState().rpcState;
  if (mClient->getState().rpcState != DiscordClient::RpcState::READY) {
    mConnectionManager->ShowAlertForContext(inContext);
    return;
  }

  ESDPlugin::KeyUpForAction(inAction, inContext, inPayload, inDeviceID);
}

void DiscordStreamDeckPlugin::WillAppearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  if (!mHaveRequestedGlobalSettings) {
    ESDDebug("Requesting global settings from WillAppear");
    mHaveRequestedGlobalSettings = true;
    mConnectionManager->GetGlobalSettings();
  }
  ESDLog("Action will appear: {} {}", inAction, inContext);
  ESDPlugin::WillAppearForAction(inAction, inContext, inPayload, inDeviceID);
}

void DiscordStreamDeckPlugin::DidReceiveGlobalSettings(const json& inPayload) {
  ESDDebug("Got Global Settings: {}", inPayload.dump().c_str());
  json settings;
  EPLJSONUtils::GetObjectByName(inPayload, "settings", settings);
  Credentials globalSettings = Credentials::fromJSON(settings);
  if (mCredentials == globalSettings) {
    return;
  }
  mCredentials = globalSettings;
  ESDDebug(
    "parsed global settings: oauth: {}; refresh: {}",
    mCredentials.oauthToken.c_str(), mCredentials.refreshToken.c_str());
  ConnectToDiscord();
}

void DiscordStreamDeckPlugin::SendToPlugin(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  const auto event = EPLJSONUtils::GetStringByName(inPayload, "event");
  ESDLog("PI event: {}", event);
  ESDDebug("PI data: {}", inPayload.dump());

  if (event == REAUTHENTICATE_PI_ACTION_ID) {
    mCredentials.oauthToken.clear();
    mCredentials.refreshToken.clear();
    ReconnectToDiscord();
    return;
  }

  if (event == RECONNECT_PI_ACTION_ID) {
    ReconnectToDiscord();
    return;
  }

  if (event == GET_STATE_PI_ACTION_ID) {
    mConnectionManager->SendToPropertyInspector(
      inAction, inContext,
      json{{"event", STATE_PI_EVENT_ID},
           {"state", mClient ? DiscordClient::getRpcStateName(
                       mClient->getState().rpcState)
                             : "no client"}});
    return;
  }
}

void DiscordStreamDeckPlugin::ReconnectToDiscord() {
  mClient.reset();
  ConnectToDiscord();
}

void DiscordStreamDeckPlugin::ConnectToDiscord() {
  ESDDebug("Connecting to Discord");
  if (mClient) {
    auto state = mClient->getState();
    if (state.rpcState == DiscordClient::RpcState::CONNECTING || state.rpcState == DiscordClient::RpcState::READY) {
      ESDDebug("Already connected, aborting.");
      return;
    }
  }
  Credentials creds = mCredentials;

  DiscordClient::Credentials credentials;
  credentials.accessToken = creds.oauthToken;
  credentials.refreshToken = creds.refreshToken;
  {
    const auto piPayload
      = json{{"event", STATE_PI_EVENT_ID}, {"state", "no client"}};
    for (const auto& [ctx, action] : mActions) {
      mConnectionManager->SendToPropertyInspector(
        action->GetAction(), ctx, piPayload);
    }
  }

  mClient = std::make_shared<DiscordClient>(
    mConnectionManager->GetAsioContext(), creds.appId, creds.appSecret, credentials);
  mClient->onStateChanged([=](DiscordClient::State state) {
    const auto piPayload
      = json{{"event", STATE_PI_EVENT_ID},
             {"state", DiscordClient::getRpcStateName(state.rpcState)}};
    {
      for (const auto& [ctx, action] : mActions) {
        mConnectionManager->SendToPropertyInspector(
          action->GetAction(), ctx, piPayload);
      }
    }
    switch (state.rpcState) {
      case DiscordClient::RpcState::READY: {
        for (const auto& [_ctx, action] : mActions) {
          action->SetDiscordClient(mClient);
        }
      }
        return;
      case DiscordClient::RpcState::CONNECTION_FAILED:
        ConnectToDiscordLater();
        return;
      case DiscordClient::RpcState::DISCONNECTED:
        ESDLog("Disconnected from Discord");
        mClient.reset();
        ConnectToDiscordLater();
        // fallthrough
      case DiscordClient::RpcState::AUTHENTICATION_FAILED: {
        for (const auto& [ctx, _action] : mActions) {
          mConnectionManager->ShowAlertForContext(ctx);
        }
        return;
      }
    }
    return;
  });
  mClient->onReady([=](DiscordClient::State state) {
    ESDLog("Discord client ready");
    for (const auto& [ctx, action] : mActions) {
      action->SetDiscordClient(mClient);
      mConnectionManager->ShowOKForContext(ctx);
    }
  });
  mClient->onCredentialsChanged([=](DiscordClient::Credentials credentials) {
    // Copy these in case we're migrating from legacy credentials (per-action)
    // to global (shared between all actions)
    mCredentials.appId = mClient->getAppId();
    mCredentials.appSecret = mClient->getAppSecret();
    mCredentials.oauthToken = credentials.accessToken;
    mCredentials.refreshToken = credentials.refreshToken;
    ESDLog(
      "Got new credentials for app id {} {} an oauth token",
      mCredentials.appId,
      mCredentials.oauthToken.empty() ? "without" : "with"
    );
    mConnectionManager->SetGlobalSettings(mCredentials.toJSON());
  });
  ESDDebug("Initializing");
  mClient->initializeInCurrentThread();
}

void DiscordStreamDeckPlugin::ConnectToDiscordLater() {
  ESDDebug("Will try to connect in 1 second...");
  if (!mConnectLaterTimer) {
    mConnectLaterTimer = std::make_unique<asio::steady_timer>(
      *mConnectionManager->GetAsioContext()
    );
  }
  mConnectLaterTimer->expires_after(std::chrono::seconds(1));
  mConnectLaterTimer->async_wait(
    [this](const asio::error_code& ec) {
      if (ec) {
        return;
      }
      ConnectToDiscord();
    }
  );
}

void DiscordStreamDeckPlugin::DeviceDidConnect(
  const std::string& inDeviceID,
  const json& inDeviceInfo) {
  if (!mHaveRequestedGlobalSettings) {
    mHaveRequestedGlobalSettings = true;
    mConnectionManager->GetGlobalSettings();
  }
}

bool DiscordStreamDeckPlugin::Credentials::isValid() const {
  return !(appId.empty() || appSecret.empty());
}

bool DiscordStreamDeckPlugin::Credentials::operator==(
  const Credentials& other) const {
  return appId == other.appId && appSecret == other.appSecret
         && oauthToken == other.oauthToken
         && refreshToken == other.refreshToken;
}

json DiscordStreamDeckPlugin::Credentials::toJSON() const {
  return json{{"appId", appId},
              {"appSecret", appSecret},
              {"oauthToken", oauthToken},
              {"refreshToken", refreshToken}};
}

std::shared_ptr<ESDAction> DiscordStreamDeckPlugin::GetOrCreateAction(
  const std::string& action,
  const std::string& context) {
  auto it = mActions.find(context);
  if (it != mActions.end()) {
    return it->second;
  }

#define A(T) \
  if (action == T::ACTION_ID) { \
    auto impl = std::make_shared<T>( \
      mConnectionManager, action, context, mClient); \
    mActions.emplace(context, impl); \
    return impl; \
  }

  A(SelfMuteToggleAction);
  A(SelfMuteOnAction);
  A(SelfMuteOffAction);

  A(DeafenToggleAction);
  A(DeafenOnAction);
  A(DeafenOffAction);

  A(PTTToggleAction);
  A(PTTOnAction);
  A(PTTOffAction);

  A(HangupAction);

#undef A

  return nullptr;
}

DiscordStreamDeckPlugin::Credentials DiscordStreamDeckPlugin::Credentials::fromJSON(
  const json& data) {
  Credentials creds;
  creds.appId = EPLJSONUtils::GetStringByName(data, "appId");
  creds.appSecret = EPLJSONUtils::GetStringByName(data, "appSecret");
  creds.oauthToken = EPLJSONUtils::GetStringByName(data, "oauthToken");
  creds.refreshToken = EPLJSONUtils::GetStringByName(data, "refreshToken");
  return creds;
}
