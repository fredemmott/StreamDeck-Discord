//==============================================================================
/**
@file	   MyStreamDeckPlugin.cpp

@brief	  Discord Plugin

@copyright  (c) 2018, Corsair Memory, Inc.
      (c) 2019, Frederick Emmott
      This source code is licensed under the MIT-style license found in the
LICENSE file.

**/
//==============================================================================

#include "MyStreamDeckPlugin.h"
#include <atomic>
#include <mutex>

#include "CallbackTimer.h"
#include "DiscordClient.h"
#include "StreamDeckSDK/EPLJSONUtils.h"
#include "StreamDeckSDK/ESDConnectionManager.h"
#include "StreamDeckSDK/ESDLogger.h"

namespace {
const auto MUTE_TOGGLE_ACTION_ID = "com.fredemmott.discord.mute";
const auto MUTE_ON_ACTION_ID = "com.fredemmott.discord.muteon";
const auto MUTE_OFF_ACTION_ID = "com.fredemmott.discord.muteoff";
const auto DEAFEN_TOGGLE_ACTION_ID = "com.fredemmott.discord.deafen";
const auto DEAFEN_ON_ACTION_ID = "com.fredemmott.discord.deafenon";
const auto DEAFEN_OFF_ACTION_ID = "com.fredemmott.discord.deafenoff";

const auto RECONNECT_PI_TOGGLE_ACTION_ID = "com.fredemmott.discord.rpc.reconnect";
const auto REAUTHENTICATE_PI_TOGGLE_ACTION_ID
  = "com.fredemmott.discord.rpc.reauthenticate";
const auto GET_STATE_PI_TOGGLE_ACTION_ID = "com.fredemmott.discord.rpc.getState";
const auto STATE_PI_EVENT_ID = "com.fredemmott.discord.rpc.state";
}// namespace

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG > 201402L, "C++17 not enabled in _MSVC_LANG");
static_assert(_HAS_CXX17, "C++17 feature flag not enabled");
#endif

MyStreamDeckPlugin::MyStreamDeckPlugin() {
  mHaveRequestedGlobalSettings = false;
  mClient = nullptr;
  mTimer = new CallBackTimer();
}

MyStreamDeckPlugin::~MyStreamDeckPlugin() {
  delete mClient;
  delete mTimer;
}

void MyStreamDeckPlugin::UpdateState(bool isMuted, bool isDeafened) {
  if (mConnectionManager != nullptr) {
    std::scoped_lock lock(mVisibleContextsMutex);
    for (const auto& pair : mVisibleContexts) {
      const auto& context = pair.first;
      const auto& action = pair.second;
      if (action == MUTE_TOGGLE_ACTION_ID) {
        mConnectionManager->SetState((isMuted || isDeafened) ? 1 : 0, context);
        continue;
      }
      if (action == DEAFEN_TOGGLE_ACTION_ID) {
        mConnectionManager->SetState(isDeafened ? 1 : 0, context);
        continue;
      }
    }
  }
}

void MyStreamDeckPlugin::KeyUpForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  mConnectionManager->LogMessage("Key Up: " + inAction + " " + inContext);
  std::scoped_lock clientLock(mClientMutex);
  if (!mClient) {
    return;
  }
  const auto clientState = mClient->getState().rpcState;
  if (mClient->getState().rpcState != DiscordClient::RpcState::READY) {
    mConnectionManager->ShowAlertForContext(inContext);
    return;
  }

  const auto oldState = EPLJSONUtils::GetIntByName(inPayload, "state");
  if (inAction == MUTE_TOGGLE_ACTION_ID) {
    mClient->setIsMuted(oldState == 0);
    return;
  }
  if (inAction == MUTE_ON_ACTION_ID) {
    mClient->setIsMuted(1);
    return;
  }
  if (inAction == MUTE_OFF_ACTION_ID) {
    mClient->setIsMuted(0);
    return;
  }
  if (inAction == DEAFEN_TOGGLE_ACTION_ID) {
    mClient->setIsDeafened(oldState == 0);
    return;
  }
  if (inAction == DEAFEN_ON_ACTION_ID) {
    mClient->setIsDeafened(1);
    return;
  }
  if (inAction == DEAFEN_OFF_ACTION_ID) {
    mClient->setIsDeafened(0);
    return;
  }
}

void MyStreamDeckPlugin::WillAppearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  mConnectionManager->LogMessage("Will appear: " + inAction + " " + inContext);
  // Remember the context
  {
    std::scoped_lock lock(mVisibleContextsMutex);
    mVisibleContexts[inContext] = inAction;
  }
  if (!mHaveRequestedGlobalSettings) {
    ESDDebug("Requesting global settings from WillAppear");
    mHaveRequestedGlobalSettings = true;
    mConnectionManager->GetGlobalSettings();
  }

  std::scoped_lock clientLock(mClientMutex);
  if (mClient) {
    const auto state = EPLJSONUtils::GetIntByName(inPayload, "state");
    const auto discordState = mClient->getState();
    const int desiredState
      = inAction == MUTE_TOGGLE_ACTION_ID
          ? ((discordState.isMuted || discordState.isDeafened) ? 1 : 0)
          : (discordState.isDeafened ? 1 : 0);
    if (state != desiredState) {
      ESDDebug("Overriding state from WillAppear");
      mConnectionManager->SetState(desiredState, inContext);
    }
  }
}

void MyStreamDeckPlugin::WillDisappearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  mConnectionManager->LogMessage(
    "Will disappear: " + inAction + " " + inContext);
  // Remove the context
  {
    std::scoped_lock lock(mVisibleContextsMutex);
    mVisibleContexts.erase(inContext);
  }
}

void MyStreamDeckPlugin::DidReceiveGlobalSettings(const json& inPayload) {
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

void MyStreamDeckPlugin::SendToPlugin(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  ESDDebug("Received plugin request: {}", inPayload.dump().c_str());
  const auto event = EPLJSONUtils::GetStringByName(inPayload, "event");
  mConnectionManager->LogMessage("Property inspector event: " + event);

  if (event == REAUTHENTICATE_PI_TOGGLE_ACTION_ID) {
    mCredentials.oauthToken.clear();
    mCredentials.refreshToken.clear();
    ReconnectToDiscord();
    return;
  }

  if (event == RECONNECT_PI_TOGGLE_ACTION_ID) {
    ReconnectToDiscord();
    return;
  }

  if (event == GET_STATE_PI_TOGGLE_ACTION_ID) {
    std::scoped_lock clientLock(mClientMutex);
    mConnectionManager->SendToPropertyInspector(
      inAction, inContext,
      json{{"event", STATE_PI_EVENT_ID},
           {"state", mClient ? DiscordClient::getRpcStateName(
                       mClient->getState().rpcState)
                             : "no client"}});
    return;
  }
}

void MyStreamDeckPlugin::ReconnectToDiscord() {
  {
    std::scoped_lock clientLock(mClientMutex);
    delete mClient;
    mClient = nullptr;
  }
  ConnectToDiscord();
}

void MyStreamDeckPlugin::ConnectToDiscord() {
  mConnectionManager->LogMessage("Connecting to Discord");
  Credentials creds = mCredentials;

  DiscordClient::Credentials credentials;
  credentials.accessToken = creds.oauthToken;
  credentials.refreshToken = creds.refreshToken;
  std::scoped_lock clientLock(mClientMutex);
  delete mClient;
  {
    const auto piPayload
      = json{{"event", STATE_PI_EVENT_ID}, {"state", "no client"}};
    std::scoped_lock lock(mVisibleContextsMutex);
    for (const auto& pair : mVisibleContexts) {
      const auto ctx = pair.first;
      const auto action = pair.second;
      mConnectionManager->SendToPropertyInspector(action, ctx, piPayload);
    }
  }

  mClient = new DiscordClient(creds.appId, creds.appSecret, credentials);
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
      std::scoped_lock lock(mVisibleContextsMutex);
      for (const auto& pair : mVisibleContexts) {
        const auto ctx = pair.first;
        const auto action = pair.second;
        mConnectionManager->SendToPropertyInspector(action, ctx, piPayload);
      }
    }
    switch (state.rpcState) {
      case DiscordClient::RpcState::READY:
        this->UpdateState(state.isMuted, state.isDeafened);
        return;
      case DiscordClient::RpcState::CONNECTION_FAILED:
        ConnectToDiscordLater();
        return;
      case DiscordClient::RpcState::DISCONNECTED:
        ConnectToDiscordLater();
        // fallthrough
      case DiscordClient::RpcState::AUTHENTICATION_FAILED: {
        std::scoped_lock lock(mVisibleContextsMutex);
        for (const auto& pair : mVisibleContexts) {
          const auto ctx = pair.first;
          mConnectionManager->ShowAlertForContext(ctx);
        }
        return;
      }
    }
    return;
  });
  mClient->onReady([=](DiscordClient::State state) {
    mConnectionManager->LogMessage("Connected to Discord");
    mTimer->stop();
    const bool isMuted = state.isMuted || state.isDeafened;
    std::scoped_lock lock(mVisibleContextsMutex);
    for (const auto& pair : mVisibleContexts) {
      const auto ctx = pair.first;
      const auto action = pair.second;
      if (action == MUTE_TOGGLE_ACTION_ID) {
        mConnectionManager->SetState(isMuted ? 1 : 0, ctx);
      } else if (action == DEAFEN_TOGGLE_ACTION_ID) {
        mConnectionManager->SetState(state.isDeafened ? 1 : 0, ctx);
      }
      mConnectionManager->ShowOKForContext(ctx);
    }
  });
  mClient->onCredentialsChanged([=](DiscordClient::Credentials credentials) {
    std::scoped_lock lock(mClientMutex);
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
  mClient->initializeWithBackgroundThread();
}

void MyStreamDeckPlugin::ConnectToDiscordLater() {
  if (mTimer->is_running()) {
    return;
  }
  mConnectionManager->LogMessage("Will try to connect in 1 second...");

  mTimer->start(1000, [=]() {
    std::scoped_lock lock(mClientMutex);
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
}

void MyStreamDeckPlugin::DeviceDidConnect(
  const std::string& inDeviceID,
  const json& inDeviceInfo) {
  if (!mHaveRequestedGlobalSettings) {
    mHaveRequestedGlobalSettings = true;
    mConnectionManager->GetGlobalSettings();
  }
}

bool MyStreamDeckPlugin::Credentials::isValid() const {
  return !(appId.empty() || appSecret.empty());
}

bool MyStreamDeckPlugin::Credentials::operator==(
  const Credentials& other) const {
  return appId == other.appId && appSecret == other.appSecret
         && oauthToken == other.oauthToken
         && refreshToken == other.refreshToken;
}

json MyStreamDeckPlugin::Credentials::toJSON() const {
  return json{{"appId", appId},
              {"appSecret", appSecret},
              {"oauthToken", oauthToken},
              {"refreshToken", refreshToken}};
}

MyStreamDeckPlugin::Credentials MyStreamDeckPlugin::Credentials::fromJSON(
  const json& data) {
  Credentials creds;
  creds.appId = EPLJSONUtils::GetStringByName(data, "appId");
  creds.appSecret = EPLJSONUtils::GetStringByName(data, "appSecret");
  creds.oauthToken = EPLJSONUtils::GetStringByName(data, "oauthToken");
  creds.refreshToken = EPLJSONUtils::GetStringByName(data, "refreshToken");
  return creds;
}
