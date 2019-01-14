//==============================================================================
/**
@file       pch.h

@brief		Precompiled header

@copyright  (c) 2018, Corsair Memory, Inc.
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#ifndef PCH_H
#define PCH_H


//-------------------------------------------------------------------
// C++ headers
//-------------------------------------------------------------------

#include <string>
#include <set>
#include <thread>


//-------------------------------------------------------------------
// Debug logging
//-------------------------------------------------------------------

#if DEBUG
	#include <os/log.h>
	#define DebugPrint(format, ...)	os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_DEFAULT, format, ##__VA_ARGS__)
#else
	#define DebugPrint(...)		while(0)
#endif


//-------------------------------------------------------------------
// json
//-------------------------------------------------------------------

#include "../Vendor/json/src/json.hpp"
using json = nlohmann::json;


//-------------------------------------------------------------------
// websocketpp
//-------------------------------------------------------------------

#define ASIO_STANDALONE


#endif //PCH_H
