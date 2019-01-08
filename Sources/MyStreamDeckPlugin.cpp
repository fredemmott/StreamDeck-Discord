//==============================================================================
/**
@file       MyStreamDeckPlugin.cpp

@brief      CPU plugin

@copyright  (c) 2018, Corsair Memory, Inc.
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#include "MyStreamDeckPlugin.h"
#include <atomic>

#include "MicMuteToggle.h"
#include "Windows/resource.h"
#include <wincrypt.h>
#include "Common/ESDConnectionManager.h"

namespace {
	std::string ResourceAsBase64(int resource) {
		const auto res = FindResource(nullptr, MAKEINTRESOURCE(resource), RT_RCDATA);
		if (!res) {
			return "";
		}
		const auto resHandle = LoadResource(NULL, res);
		if (!resHandle) {
			return "";
		}
		const BYTE* data = (BYTE*)LockResource(resHandle);
		const auto size = SizeofResource(NULL, res);

		DWORD b64size;
		// Find size
		CryptBinaryToStringA(
			data,
			size,
			CRYPT_STRING_BASE64,
			nullptr,
			&b64size
		);
		char* b64data = new char[b64size];
		CryptBinaryToStringA(
			data,
			size,
			CRYPT_STRING_BASE64,
			b64data,
			&b64size
		);
		auto ret = std::string(b64data, b64size);
		delete b64data;
		return ret;
	}
};

class CallBackTimer
{
public:
    CallBackTimer() :_execute(false) { }

    ~CallBackTimer()
    {
        if( _execute.load(std::memory_order_acquire) )
        {
            stop();
        };
    }

    void stop()
    {
        _execute.store(false, std::memory_order_release);
        if(_thd.joinable())
            _thd.join();
    }

    void start(int interval, std::function<void(void)> func)
    {
        if(_execute.load(std::memory_order_acquire))
        {
            stop();
        };
        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, func]()
        {
			CoInitialize(NULL); // initialize COM again for the timer thread
            while (_execute.load(std::memory_order_acquire))
            {
                func();
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
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
};

MyStreamDeckPlugin::MyStreamDeckPlugin()
{
	mMutedImage = ResourceAsBase64(IDM_MUTED);
	mUnmutedImage = ResourceAsBase64(IDM_UNMUTED);
	CoInitialize(NULL); // initialize COM for the main thread
	mTimer = new CallBackTimer();
	mTimer->start(500, [this]()
	{
		this->UpdateTimer();
	});
}

MyStreamDeckPlugin::~MyStreamDeckPlugin()
{
	if (mTimer != nullptr)
	{
		mTimer->stop();

		delete mTimer;
		mTimer = nullptr;
	}
}

void MyStreamDeckPlugin::UpdateTimer()
{
	//
	// Warning: UpdateTimer() is running in the timer thread
	//
	if(mConnectionManager != nullptr)
	{
		mVisibleContextsMutex.lock();
		const bool isMuted = IsMuted();
		for (const std::string& context : mVisibleContexts)
		{
			mConnectionManager->SetImage(isMuted ? mMutedImage : mUnmutedImage, context, kESDSDKTarget_HardwareAndSoftware);
		}
		mVisibleContextsMutex.unlock();
	}
}

void MyStreamDeckPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	SetMuted(MuteBehavior::TOGGLE);
	UpdateTimer();
}

void MyStreamDeckPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}

void MyStreamDeckPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remember the context
	mVisibleContextsMutex.lock();
	mVisibleContexts.insert(inContext);
	mVisibleContextsMutex.unlock();
}

void MyStreamDeckPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove the context
	mVisibleContextsMutex.lock();
	mVisibleContexts.erase(inContext);
	mVisibleContextsMutex.unlock();
}

void MyStreamDeckPlugin::DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo)
{
	// Nothing to do
}

void MyStreamDeckPlugin::DeviceDidDisconnect(const std::string& inDeviceID)
{
	// Nothing to do
}
