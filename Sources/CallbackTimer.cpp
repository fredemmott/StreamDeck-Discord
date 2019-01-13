//==============================================================================
/**
@file       CallbackTimer.cpp

@brief      Periodically run code in another thread.

@copyright  (c) 2018, Corsair Memory, Inc.
			(c) 2019, Frederick Emmott
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================
#include "CallbackTimer.h"

CallBackTimer::CallBackTimer() :_execute(false) { }

CallBackTimer::~CallBackTimer()
{
	if (_execute.load(std::memory_order_acquire))
	{
		stop();
	};
}

void CallBackTimer::stop()
{
	_execute.store(false, std::memory_order_release);
	if (_thd.joinable())
		_thd.join();
}

void CallBackTimer::start(int interval, std::function<void(void)> func)
{
	if (_execute.load(std::memory_order_acquire))
	{
		stop();
	};
	_execute.store(true, std::memory_order_release);
	_thd = std::thread([this, interval, func]()
	{
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (!_execute.load(std::memory_order_acquire)) {
				break;
			}
			func();
		}
	});
}

bool CallBackTimer::is_running() const noexcept
{
	return (_execute.load(std::memory_order_acquire) && _thd.joinable());
}