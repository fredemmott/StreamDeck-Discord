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
#include "Common/EPLJSONUtils.h"
#include "Common/ESDConnectionManager.h"
#include "DiscordClient.h"

namespace {
const auto MUTE_ACTION_ID = "com.fredemmott.discord.mute";
const auto DEAFEN_ACTION_ID = "com.fredemmott.discord.deafen";
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
      if (action == MUTE_ACTION_ID) {
        mConnectionManager->SetState((isMuted || isDeafened) ? 1 : 0, context);
        continue;
      }
      if (action == DEAFEN_ACTION_ID) {
        mConnectionManager->SetState(isDeafened ? 1 : 0, context);
        continue;
      }
    }
  }
}

void MyStreamDeckPlugin::KeyDownForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
}

void MyStreamDeckPlugin::KeyUpForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  if (!mClient) {
    return;
  }
  const auto clientState = mClient->getState().rpcState;
  switch (clientState) {
    case DiscordClient::RpcState::READY:
      break;
    case DiscordClient::RpcState::CONNECTION_FAILED:
    case DiscordClient::RpcState::DISCONNECTED:
      mTimer->stop();
      ConnectToDiscord();
    case DiscordClient::RpcState::AUTHENTICATION_FAILED:
    case DiscordClient::RpcState::CONNECTING:
    case DiscordClient::RpcState::REQUESTING_ACCESS_TOKEN:
    case DiscordClient::RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN:
    case DiscordClient::RpcState::REQUESTING_USER_PERMISSION:
    case DiscordClient::RpcState::UNINITIALIZED:
    case DiscordClient::RpcState::REQUESTING_VOICE_STATE:
      mConnectionManager->ShowAlertForContext(inContext);
      return;
  }
  const auto oldState = EPLJSONUtils::GetIntByName(inPayload, "state");
  if (inAction == MUTE_ACTION_ID) {
    mClient->setIsMuted(oldState == 0);
    return;
  }
  if (inAction == DEAFEN_ACTION_ID) {
    mClient->setIsDeafened(oldState == 0);
    return;
  }
}

void MyStreamDeckPlugin::WillAppearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  // Remember the context
  {
    std::scoped_lock lock(mVisibleContextsMutex);
    mVisibleContexts[inContext] = inAction;
  }
  if (!mHaveRequestedGlobalSettings) {
    mHaveRequestedGlobalSettings = true;
    mConnectionManager->GetGlobalSettings();
  }

  if (mClient) {
    const auto state = EPLJSONUtils::GetIntByName(inPayload, "state");
    const auto discordState = mClient->getState();
    const int desiredState
      = inAction == MUTE_ACTION_ID
          ? ((discordState.isMuted || discordState.isDeafened) ? 1 : 0)
          : (discordState.isDeafened ? 1 : 0);
    if (state != desiredState) {
      mConnectionManager->SetState(desiredState, inContext);
    }
  }

  if (!mLegacyCredentials.isValid()) {
    json settings;
    EPLJSONUtils::GetObjectByName(inPayload, "settings", settings);
    json credentials;
    EPLJSONUtils::GetObjectByName(settings, "credentials", credentials);
    mLegacyCredentials = Credentials::fromJSON(credentials);
    if (mLegacyCredentials.isValid()) {
      ConnectToDiscordLater();
    }
    return;
  }
}

void MyStreamDeckPlugin::WillDisappearForAction(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  // Remove the context
  {
    std::scoped_lock lock(mVisibleContextsMutex);
    mVisibleContexts.erase(inContext);
  }
}

void MyStreamDeckPlugin::MigrateToGlobalSettings() {
  mCredentials = mLegacyCredentials;
  mConnectionManager->SetGlobalSettings(mLegacyCredentials.toJSON());
  {
    std::scoped_lock lock(mVisibleContextsMutex);
    const json emptyPayload({"settings", json({})});
    for (const auto& pair : mVisibleContexts) {
      const auto ctx = pair.first;
      mConnectionManager->SetSettings(emptyPayload, ctx);
    }
  }
}

void MyStreamDeckPlugin::DidReceiveGlobalSettings(const json& inPayload) {
  json settings;
  EPLJSONUtils::GetObjectByName(inPayload, "settings", settings);
  Credentials globalSettings = Credentials::fromJSON(settings);
  if (mCredentials == globalSettings) {
    return;
  }
  mCredentials = globalSettings;
  ConnectToDiscord();
}

void MyStreamDeckPlugin::SendToPlugin(
  const std::string& inAction,
  const std::string& inContext,
  const json& inPayload,
  const std::string& inDeviceID) {
  const auto event = EPLJSONUtils::GetStringByName(inPayload, "event");
}

void MyStreamDeckPlugin::ConnectToDiscord() {
  if (mLegacyCredentials.isValid() && !mCredentials.isValid()) {
    MigrateToGlobalSettings();
  }
  Credentials creds = mCredentials;

  DiscordClient::Credentials credentials;
  credentials.accessToken = creds.oauthToken;
  credentials.refreshToken = creds.refreshToken;
  delete mClient;
  mClient = new DiscordClient(creds.appId, creds.appSecret, credentials);
  mClient->onStateChanged([=](DiscordClient::State state) {
    switch (state.rpcState) {
      case DiscordClient::RpcState::READY:
        mTimer->stop();
        this->UpdateState(state.isMuted, state.isDeafened);
        break;
      case DiscordClient::RpcState::CONNECTION_FAILED:
        ConnectToDiscordLater();
        break;
      case DiscordClient::RpcState::DISCONNECTED:
        ConnectToDiscordLater();
        // fallthrough
      case DiscordClient::RpcState::AUTHENTICATION_FAILED: {
        std::scoped_lock lock(mVisibleContextsMutex);
        for (const auto& pair : mVisibleContexts) {
          const auto ctx = pair.first;
          mConnectionManager->ShowAlertForContext(ctx);
        }
        break;
      }
      default:
        return;
    }
  });
  mClient->onReady([=](DiscordClient::State) {
    std::scoped_lock lock(mVisibleContextsMutex);
    for (const auto& pair : mVisibleContexts) {
      const auto ctx = pair.first;
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
    mConnectionManager->SetGlobalSettings(mCredentials.toJSON());
  });
  mClient->initializeWithBackgroundThread();
}

void MyStreamDeckPlugin::ConnectToDiscordLater() {
  if (mTimer->is_running()) {
    return;
  }
  mTimer->start(1000, [=]() {
    DiscordClient::RpcState state = DiscordClient::RpcState::DISCONNECTED;
    if (mClient) {
      state = mClient->getState().rpcState;
    }
    if (
      state != DiscordClient::RpcState::CONNECTION_FAILED
      && state != DiscordClient::RpcState::DISCONNECTED) {
      return;
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

void MyStreamDeckPlugin::DeviceDidDisconnect(const std::string& inDeviceID) {
  // Nothing to do
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
