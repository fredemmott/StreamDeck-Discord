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
  mConnectionManager->LogMessage("Key Up: " + inAction + " " + inContext);
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
  ESDDebug("Received plugin request: {}", inPayload.dump());
  const auto event = EPLJSONUtils::GetStringByName(inPayload, "event");
  mConnectionManager->LogMessage("Property inspector event: " + event);

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
  ESDDebug("Connecting to discord");
  Credentials creds = mCredentials;

  DiscordClient::Credentials credentials;
  credentials.accessToken = creds.oauthToken;
  credentials.refreshToken = creds.refreshToken;
  mClient.reset();
  {
    const auto piPayload
      = json{{"event", STATE_PI_EVENT_ID}, {"state", "no client"}};
    std::scoped_lock lock(mActionsMutex);
    for (const auto& [ctx, action] : mActions) {
      mConnectionManager->SendToPropertyInspector(
        action->GetActionID(), ctx, piPayload);
    }
  }

  mClient = std::make_shared<DiscordClient>(
    mConnectionManager->GetAsioContext(), creds.appId, creds.appSecret, credentials);
  mClient->onStateChanged([=](DiscordClient::State state) {
    std::stringstream logMessage;
    logMessage << "Discord state change: "
               << "rpcState = "
               << DiscordClient::getRpcStateName(state.rpcState)
               << ", muted = " << state.isMuted
               << ", deafened = " << state.isDeafened;

    mConnectionManager->LogMessage(logMessage.str());

    const auto piPayload
      = json{{"event", STATE_PI_EVENT_ID},
             {"state", DiscordClient::getRpcStateName(state.rpcState)}};
    {
      std::scoped_lock lock(mActionsMutex);
      for (const auto& [ctx, action] : mActions) {
        mConnectionManager->SendToPropertyInspector(
          action->GetActionID(), ctx, piPayload);
      }
    }
    switch (state.rpcState) {
      case DiscordClient::RpcState::READY: {
        std::scoped_lock lock(mActionsMutex);
        for (const auto& [_ctx, action] : mActions) {
          action->DiscordStateDidChange(mClient, state);
        }
      }
        return;
      case DiscordClient::RpcState::CONNECTION_FAILED:
        ConnectToDiscordLater();
        return;
      case DiscordClient::RpcState::DISCONNECTED:
        ConnectToDiscordLater();
        // fallthrough
      case DiscordClient::RpcState::AUTHENTICATION_FAILED: {
        std::scoped_lock lock(mActionsMutex);
        for (const auto& [ctx, _action] : mActions) {
          mConnectionManager->ShowAlertForContext(ctx);
        }
        return;
      }
    }
    return;
  });
  mClient->onReady([=](DiscordClient::State state) {
    ESDDebug("Client ready");
    const bool isMuted = state.isMuted || state.isDeafened;
    std::scoped_lock lock(mActionsMutex);
    for (const auto& [ctx, action] : mActions) {
      action->DiscordStateDidChange(mClient, state);
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
    std::stringstream logMessage;
    logMessage << "Got new credentials for app id" << mCredentials.appId
               << " - with oauth token = "
               << (mCredentials.oauthToken.empty() ? "no" : "yes");
    mConnectionManager->LogMessage(logMessage.str());
    mConnectionManager->SetGlobalSettings(mCredentials.toJSON());
  });
  mClient->initializeInCurrentThread();
}

void DiscordStreamDeckPlugin::ConnectToDiscordLater() {
  ESDDebug("ConnectToDiscordLater, ignoring");
  return;
  /*
  mConnectionManager->LogMessage("Will try to connect in 1 second...");

  mTimer->start(1000, [=]() {
    if (mClient) {
      const auto state = mClient->getState().rpcState;
      if (
        state != DiscordClient::RpcState::CONNECTION_FAILED
        && state != DiscordClient::RpcState::DISCONNECTED) {
        return;
      }
    }
    ConnectToDiscord();
  });
  */
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
  std::scoped_lock lock(mActionsMutex);
  auto it = mActions.find(context);
  if (it != mActions.end()) {
    return it->second;
  }

  if (action == SelfMuteToggleAction::ACTION_ID) {
    auto impl = std::make_shared<SelfMuteToggleAction>(
      mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == SelfMuteOnAction::ACTION_ID) {
    auto impl = std::make_shared<SelfMuteOnAction>(
      mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == SelfMuteOffAction::ACTION_ID) {
    auto impl = std::make_shared<SelfMuteOffAction>(
      mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == DeafenToggleAction::ACTION_ID) {
    auto impl = std::make_shared<DeafenToggleAction>(
      mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == DeafenOnAction::ACTION_ID) {
    auto impl
      = std::make_shared<DeafenOnAction>(mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == DeafenOffAction::ACTION_ID) {
    auto impl
      = std::make_shared<DeafenOffAction>(mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == PTTToggleAction::ACTION_ID) {
    auto impl = std::make_shared<PTTToggleAction>(
      mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == PTTOnAction::ACTION_ID) {
    auto impl
      = std::make_shared<PTTOnAction>(mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

  if (action == PTTOffAction::ACTION_ID) {
    auto impl
      = std::make_shared<PTTOffAction>(mConnectionManager, context, mClient);
    mActions.emplace(context, impl);
    return impl;
  }

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
