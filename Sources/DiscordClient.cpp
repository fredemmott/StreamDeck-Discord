#include "DiscordClient.h"

#include "Common/EPLJSONUtils.h"
#include "Discord/rpc_connection.h"
#include "Windows/pch.h"

#include <atomic>
#include <chrono>
#include <thread>

class DiscordClientThread
{
public:
	DiscordClientThread(DiscordClient* client) :_execute(false) {
		_client = client;
	}

	~DiscordClientThread()
	{
		if (_execute.load(std::memory_order_acquire))
		{
			stop();
		};
	}

	void stop()
	{
		_execute.store(false, std::memory_order_release);
		if (_thd.joinable())
			_thd.join();
	}

	void start()
	{
		if (_execute.load(std::memory_order_acquire))
		{
			stop();
		};
		_execute.store(true, std::memory_order_release);
		_thd = std::thread([this]()
		{
			while (_execute.load(std::memory_order_acquire))
			{
                if (!_client->processEvents()) {
                    return;
                }
            }
		});
	}

	bool is_running() const noexcept
	{
		return (_execute.load(std::memory_order_acquire) && _thd.joinable());
	}

private:
	std::atomic<bool> _execute;
	std::thread _thd;
	DiscordClient* _client;
};

DiscordClient::DiscordClient(const std::string & appId, const std::string & appSecret, const Credentials & credentials)
{
	mConnection = nullptr;
	mState.rpcState = RpcState::UNINITIALIZED;
	mAppId = appId;
	mAppSecret = appSecret;
	mCredentials = credentials;

	mStateCallback = [](State) {};
	mReadyCallback = mStateCallback;
	mCredentialsCallback = [](Credentials) {};
	mProcessingThread = nullptr;
}

DiscordClient::~DiscordClient()
{
	delete mProcessingThread;
	RpcConnection::Destroy(mConnection);
}

void DiscordClient::initializeWithBackgroundThread() {
	initialize();
	delete mProcessingThread;
	mProcessingThread = new DiscordClientThread(this);
	mProcessingThread->start();
}

void DiscordClient::initialize() {
	if (mAppId.empty() || mAppSecret.empty()) {
		setRpcState(RpcState::UNINITIALIZED, RpcState::AUTHENTICATION_FAILED);
		return;
	}

	setRpcState(RpcState::UNINITIALIZED, RpcState::CONNECTING);
	mConnection = RpcConnection::Create(mAppId);
	mConnection->onDisconnect = [=](int code, const std::string& message) {
		DebugPrint("Disconnected from discord: %d %s", code, message.c_str());
		if (this->mState.rpcState != RpcState::CONNECTION_FAILED && this->mState.rpcState != RpcState::AUTHENTICATION_FAILED) {
			setRpcState(RpcState::DISCONNECTED);
		}
	};
	mConnection->Open();
}

bool DiscordClient::processInitializationEvents() {
	assert(mState.rpcState == RpcState::CONNECTING);
	if (mConnection->state == RpcConnection::State::Disconnected) {
		setRpcState(RpcState::CONNECTING, RpcState::CONNECTION_FAILED);
		return false;
	}

	if (!mConnection->IsOpen()) {
		mConnection->Open();
		return true;
	}

	if (mCredentials.accessToken.empty()) {
		setRpcState(RpcState::CONNECTING, RpcState::REQUESTING_USER_PERMISSION);
		mConnection->Write({
			{ "nonce", getNextNonce() },
			{ "cmd", "AUTHORIZE" },
			{ "args", {
				{ "client_id", mAppId },
				{ "scopes", { "rpc" } }
			}}
			});
		return true;
	}

	setRpcState(RpcState::CONNECTING, RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
	mConnection->Write({
		{ "nonce", getNextNonce() },
		{ "cmd", "AUTHENTICATE" },
		{ "args", {
			{"access_token", mCredentials.accessToken }
		}}
		});
	return true;
}

bool DiscordClient::processEvents() {
	if (mState.rpcState == RpcState::CONNECTING) {
		return processInitializationEvents();

	}

	json message;
	if (!mConnection->Read(&message)) {
		return false;
	}
	const auto command = EPLJSONUtils::GetStringByName(message, "cmd");
	const auto event = EPLJSONUtils::GetStringByName(message, "evt");
	json data;
	const bool haveData = EPLJSONUtils::GetObjectByName(message, "data", data);
	if (command == "AUTHORIZE") {
		const auto code = EPLJSONUtils::GetStringByName(data, "code");
		if (code.empty() || event == "error") {
			setRpcState(RpcState::REQUESTING_USER_PERMISSION, RpcState::AUTHENTICATION_FAILED);
			return false;
		}
		setRpcState(RpcState::REQUESTING_USER_PERMISSION, RpcState::REQUESTING_ACCESS_TOKEN);
		mCredentials = getOAuthCredentialsFromCode(code);
		if (mCredentials.accessToken.empty()) {
			setRpcState(RpcState::REQUESTING_ACCESS_TOKEN, RpcState::AUTHENTICATION_FAILED);
			return false;
		}
		mCredentialsCallback(mCredentials);
		setRpcState(RpcState::REQUESTING_ACCESS_TOKEN, RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN);
		mConnection->Write({
			{ "nonce", getNextNonce() },
			{ "cmd", "AUTHENTICATE" },
			{ "args", { "access_token", mCredentials.accessToken }}
		});
		return true;
	}

	if (command == "AUTHENTICATE") {
		if (event == "ERROR") {
			setRpcState(RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN, RpcState::AUTHENTICATION_FAILED);
			return false;
		}
		setRpcState(RpcState::AUTHENTICATING_WITH_ACCESS_TOKEN, RpcState::REQUESTING_VOICE_STATE);
		mConnection->Write({
			{ "nonce", getNextNonce() },
			{ "cmd", "SUBSCRIBE" },
			{ "evt", "VOICE_SETTINGS_UPDATE" }
		});
		mConnection->Write({
			{ "nonce", getNextNonce() },
			{ "cmd", "GET_VOICE_SETTINGS" },
		});
		return true;
	}

	if (command == "GET_VOICE_SETTINGS" || event == "VOICE_SETTINGS_UPDATE") {
		if (haveData) {
			mState.isMuted = EPLJSONUtils::GetBoolByName(data, "mute");
			mState.isDeafened = EPLJSONUtils::GetBoolByName(data, "deaf");
			if (mState.rpcState == RpcState::REQUESTING_VOICE_STATE) {
				mState.rpcState = RpcState::READY;
				mReadyCallback(mState);
			}
			mStateCallback(mState);
		}
		return true;
	}

	return true;
}

DiscordClient::State DiscordClient::getState() const
{
	return mState;
}

void DiscordClient::onStateChanged(StateCallback cb)
{
	mStateCallback = cb;
}

void DiscordClient::onReady(StateCallback cb)
{
	mReadyCallback = cb;
}

void DiscordClient::onCredentialsChanged(CredentialsCallback cb)
{
	mCredentialsCallback = cb;
}

void DiscordClient::setIsMuted(bool mute)
{
	// Don't update the state locally to avoid displaying mic mute when not muted.
	// 1. We ask discord to mute or unmute
	// 2. discord does that
	// 3. discord tells subscribing apps (including us) about that
	// 4. we update the state when discord says it's changed
	json args;
	args["mute"] = mute;
	mConnection->Write({
		{"nonce", getNextNonce()},
		{"cmd", "SET_VOICE_SETTINGS"},
		{"args", args}
	});
}

void DiscordClient::setIsDeafened(bool deaf)
{
	json args;
	args["deaf"] = deaf;
	mConnection->Write({
		{"nonce", getNextNonce()},
		{"cmd", "SET_VOICE_SETTINGS"},
		{"args", args}
	});
}

void DiscordClient::setRpcState(RpcState state) {
	mState.rpcState = state;
	if (mStateCallback) {
		mStateCallback(mState);
	}
}

void DiscordClient::setRpcState(RpcState oldState, RpcState newState) {
	assert(mState.rpcState == oldState);
	setRpcState(newState);
}
