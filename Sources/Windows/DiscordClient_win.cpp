#include "../DiscordClient.h"
#include "../Common/EPLJSONUtils.h"
#include "../Windows/pch.h"

#include <Rpc.h>
#include <WinInet.h>
#include <shlwapi.h>
#include <memory>
#include <sstream>

namespace {
	HINTERNET hInternet = nullptr;

	std::string urlencode(const std::string& in) {
		DWORD bufsize = in.length() * 3;
		std::unique_ptr<char> buf(new char[bufsize]);
		UrlEscapeA(
			in.c_str(),
			buf.get(),
			&bufsize,
			NULL
		);
		return std::string(buf.get(), bufsize);
	}
}

std::string DiscordClient::getNextNonce() {
	UUID uuid;
	UuidCreate(&uuid);
	RPC_CSTR rpcstr;
	UuidToStringA(&uuid, &rpcstr);
	const std::string ret((char*) rpcstr);
	RpcStringFreeA(&rpcstr);
	return ret;
}

DiscordClient::Credentials DiscordClient::getOAuthCredentialsFromCode(const std::string& code) {
	if (!hInternet) {
		hInternet = InternetOpen(
			L"com.fredemmott.streamdeck-discord",
			INTERNET_OPEN_TYPE_PRECONFIG,
			nullptr,
			nullptr,
			0
		);
	}

	auto hConnection = InternetConnectA(
		hInternet,
		"discordapp.com",
		INTERNET_DEFAULT_HTTPS_PORT,
		nullptr,
		nullptr,
		INTERNET_SERVICE_HTTP,
		INTERNET_FLAG_SECURE, // HTTPS please
		NULL
	);
	auto hRequest = HttpOpenRequestA(
		hConnection,
		"POST",
		"/api/oauth2/token",
		nullptr,
		nullptr,
		nullptr,
		INTERNET_FLAG_SECURE,
		NULL
	);

	const auto headers = L"Content-Type: application/x-www-form-urlencoded\r\nHost: discordapp.com";
	HttpAddRequestHeaders(hRequest, headers, wcslen(headers), HTTP_ADDREQ_FLAG_ADD);
	std::stringstream ss;
	ss
		<< "grant_type=authorization_code"
		<< "&code=" << urlencode(code)
		<< "&redirect_uri=" << urlencode("https://localhost")
		<< "&client_id=" << urlencode(mAppId)
		<< "&client_secret=" << urlencode(mAppSecret);
	const auto postData = ss.str();
	dbgprintf("Sending to discord www api: %s", postData.c_str());

	HttpSendRequestA(hRequest, nullptr, 0, (void*)postData.c_str(), postData.length());

	// I tried using HttpQueryInfo to get Content-Length; it turns out that Discord only send a Content-Length
	// header for error cases.
	std::string response;
	char buf[1024];
	DWORD bytesRead;
	do {
		InternetReadFile(hRequest, buf, sizeof(buf), &bytesRead);
		response += std::string(buf, bytesRead);
	} while (bytesRead > 0);
	
	dbgprintf("HTTP response from discord: %s", response.c_str());
	const json parsed = json::parse(response);
	mCredentials.accessToken = EPLJSONUtils::GetStringByName(parsed, "access_token");
	mCredentials.refreshToken = EPLJSONUtils::GetStringByName(parsed, "refresh_token");
	return mCredentials;
}
