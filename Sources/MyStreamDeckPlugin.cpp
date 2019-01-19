//==============================================================================
/**
@file	   MyStreamDeckPlugin.cpp

@brief	  Discord Plugin

@copyright  (c) 2018, Corsair Memory, Inc.
			(c) 2019, Frederick Emmott
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#include "MyStreamDeckPlugin.h"
#include <atomic>
#include <mutex>

#include "Common/ESDConnectionManager.h"
#include "Common/EPLJSONUtils.h"
#include "DiscordClient.h"
#include "CallbackTimer.h"

namespace {
	const auto MUTE_ACTION_ID = "com.fredemmott.discord.mute";
	const auto DEAFEN_ACTION_ID = "com.fredemmott.discord.deafen";
}

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG > 201402L, "C++17 not enabled in _MSVC_LANG");
static_assert(_HAS_CXX17, "C++17 feature flag not enabled");
#endif

MyStreamDeckPlugin::MyStreamDeckPlugin()
{
	mClient = nullptr;
	mTimer = new CallBackTimer();
}

MyStreamDeckPlugin::~MyStreamDeckPlugin()
{
	delete mClient;
	delete mTimer;
}

void MyStreamDeckPlugin::UpdateState(bool isMuted, bool isDeafened)
{
	if(mConnectionManager != nullptr)
	{
		std::scoped_lock lock(mVisibleContextsMutex);
		for (const auto& pair: mVisibleContexts)
		{
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

void MyStreamDeckPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
}

void MyStreamDeckPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
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

void MyStreamDeckPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remember the context
	{
		std::scoped_lock lock(mVisibleContextsMutex);
		mVisibleContexts[inContext] = inAction;
	}
	if (!mClient) {
		DebugPrint("Loaded settings: %s", inPayload.dump().c_str());
		json settings;
		EPLJSONUtils::GetObjectByName(inPayload, "settings", settings);
		json credentials;
		EPLJSONUtils::GetObjectByName(settings, "credentials", credentials);
		mAppId = EPLJSONUtils::GetStringByName(credentials, "appId");
		mAppSecret = EPLJSONUtils::GetStringByName(credentials, "appSecret");
		mOAuthToken = EPLJSONUtils::GetStringByName(credentials, "oauthToken");
		mRefreshToken = EPLJSONUtils::GetStringByName(credentials, "refreshToken");
		if (!mAppSecret.empty()) {
			mTimer->stop();
			ConnectToDiscord();
		}
	}
}

void MyStreamDeckPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove the context
	{
		std::scoped_lock lock(mVisibleContextsMutex);
		mVisibleContexts.erase(inContext);
	}
}

void MyStreamDeckPlugin::SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	const auto event = EPLJSONUtils::GetStringByName(inPayload, "event");

	if (event == "getData") {
		json outPayload{
			{ "event", event },
			{ "data", {{"appId", mAppId}, {"appSecret", mAppSecret}} }
		};
		DebugPrint("JSON to PI: %s", outPayload.dump().c_str());
		mConnectionManager->SendToPropertyInspector(inAction, inContext, outPayload);
		return;
	}
	if (event == "saveSettings") {
		json data;
		EPLJSONUtils::GetObjectByName(inPayload, "data", data);
		const auto appId = EPLJSONUtils::GetStringByName(data, "appId");
		const auto appSecret = EPLJSONUtils::GetStringByName(data, "appSecret");
		if (appId == mAppId && appSecret == mAppSecret) {
			return;
		}
		mAppId = appId;
		mAppSecret = appSecret;
		mOAuthToken.clear();
		mRefreshToken.clear();
		const json settings{ {"credentials", {{"appId", appId}, {"appSecret", appSecret}}} };
		if (mClient) {
			delete mClient;
			mClient = nullptr;
		}
		for (const auto& pair: mVisibleContexts)
		{
			const auto context = pair.first;
			mConnectionManager->SetSettings(settings, context);
		}
		if (appId.empty() || appSecret.empty()) {
			return;
		}	
		mTimer->stop();
		ConnectToDiscord();
	}
}

void MyStreamDeckPlugin::ConnectToDiscord() {
	if (mAppId.empty() || mAppSecret.empty()) {
		return;
	}

	DiscordClient::Credentials credentials;
	credentials.accessToken = mOAuthToken;
	credentials.refreshToken = mRefreshToken;
	delete mClient;
	mClient = new DiscordClient(mAppId, mAppSecret, credentials);
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
		case DiscordClient::RpcState::AUTHENTICATION_FAILED:
		{
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
		this->mOAuthToken = credentials.accessToken;
		this->mRefreshToken = credentials.refreshToken;
		const json settings{ {"credentials", {
			  {"appId", this->mAppId},
			  {"appSecret", this->mAppSecret},
			  { "oauthToken", credentials.accessToken },
			  { "refreshToken", credentials.refreshToken }
		}} };
		std::scoped_lock lock(mVisibleContextsMutex);
		for (const auto& pair: mVisibleContexts)
		{
			const auto context = pair.first;
			mConnectionManager->SetSettings(settings, context);
		}

	});
	mClient->initializeWithBackgroundThread();
}

void MyStreamDeckPlugin::ConnectToDiscordLater()
{
	if (mTimer->is_running()) {
		return;
	}
	mTimer->start(1000, [=]() {
		DiscordClient::RpcState state = DiscordClient::RpcState::DISCONNECTED;
		if (mClient) {
			state = mClient->getState().rpcState;
		}
		if (state != DiscordClient::RpcState::CONNECTION_FAILED && state != DiscordClient::RpcState::DISCONNECTED) {
			return;
		}
		ConnectToDiscord();
	});
}

void MyStreamDeckPlugin::DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo)
{
	// Nothing to do
}

void MyStreamDeckPlugin::DeviceDidDisconnect(const std::string& inDeviceID)
{
	// Nothing to do
}
