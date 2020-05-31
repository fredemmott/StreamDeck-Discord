#include <StreamDeckSDK/EPLJSONUtils.h>
#include <StreamDeckSDK/ESDCommon.h>
#include "DiscordClient.h"

#include <Rpc.h>
#include <WinInet.h>
#include <shlwapi.h>
#include <memory>
#include <sstream>

namespace {
HINTERNET hInternet = nullptr;

std::string urlencode(const std::string& in) {
  const char* hex = "0123456789abcdef";
  // this matches x-www-form-url-encoded, which isn't quite the same thing as
  // actual url escaping
  std::string out;
  for (const auto c : in) {
    if (
      (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
      || c == '.' || c == '_') {
      out += c;
      continue;
    }
    out += '%';
    out += hex[c / 16];
    out += hex[c % 16];
  }
  return out;
}
}// namespace

std::string DiscordClient::getNextNonce() {
  UUID uuid;
  UuidCreate(&uuid);
  RPC_CSTR rpcstr;
  UuidToStringA(&uuid, &rpcstr);
  const std::string ret((char*)rpcstr);
  RpcStringFreeA(&rpcstr);
  return ret;
}

DiscordClient::Credentials DiscordClient::getOAuthCredentials(
  const std::string& grantType,
  const std::string& secretType,
  const std::string& secret) {
  if (!hInternet) {
    hInternet = InternetOpenA(
      "com.fredemmott.streamdeck-discord", INTERNET_OPEN_TYPE_PRECONFIG,
      nullptr, nullptr, 0);
  }

  auto hConnection = InternetConnectA(
    hInternet, "discordapp.com", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr,
    INTERNET_SERVICE_HTTP,
    INTERNET_FLAG_SECURE,// HTTPS please
    NULL);
  auto hRequest = HttpOpenRequestA(
    hConnection, "POST", "/api/oauth2/token", nullptr, nullptr, nullptr,
    INTERNET_FLAG_SECURE, NULL);

  const auto headers
    = "Content-Type: application/x-www-form-urlencoded\r\nHost: "
      "discordapp.com";
  HttpAddRequestHeadersA(
    hRequest, headers, -1, HTTP_ADDREQ_FLAG_ADD);
  std::stringstream ss;
  ss << "grant_type=" << urlencode(grantType) << "&" << urlencode(secretType)
     << "=" << urlencode(secret) << "&client_id=" << urlencode(mAppId)
     << "&client_secret=" << urlencode(mAppSecret);
  const auto postData = ss.str();
  DebugPrint("[discord][plugin][wininet] sending: %s", postData.c_str());

  HttpSendRequestA(
    hRequest, nullptr, 0, (void*)postData.c_str(), postData.length());

  // I tried using HttpQueryInfo to get Content-Length; it turns out that
  // Discord only send a Content-Length header for error cases.
  std::string response;
  char buf[1024];
  DWORD bytesRead;
  do {
    InternetReadFile(hRequest, buf, sizeof(buf), &bytesRead);
    response += std::string(buf, bytesRead);
  } while (bytesRead > 0);

  DebugPrint("[discord][plugin][wininet] received: %s", response.c_str());
  const json parsed = json::parse(response);
  mCredentials.accessToken
    = EPLJSONUtils::GetStringByName(parsed, "access_token");
  mCredentials.refreshToken
    = EPLJSONUtils::GetStringByName(parsed, "refresh_token");
  return mCredentials;
}
