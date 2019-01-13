#pragma once

//==============================================================================
/**
@file       CallbackTimer.cpp

@brief      Periodically run code in another thread.

@copyright  (c) 2018, Corsair Memory, Inc.
			(c) 2019, Frederick Emmott
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

class CallBackTimer
{
public:
	CallBackTimer();
	~CallBackTimer();

	void stop();
	void start(int interval, std::function<void(void)> func);
	bool is_running() const noexcept;
private:
	std::atomic<bool> _execute;
	std::thread _thd;
};