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

#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <set>
#include <thread>
#include <strsafe.h>


//-------------------------------------------------------------------
// Debug logging
//-------------------------------------------------------------------

#ifdef _DEBUG
	#define DEBUG 1
#else
	#define DEBUG 0
#endif

void __cdecl dbgprintf(const char *format, ...);

#if DEBUG
#define DebugPrint			dbgprintf
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
