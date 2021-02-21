#include <StreamDeckSDK/EPLJSONUtils.h>
#include <StreamDeckSDK/ESDLogger.h>
#include "DiscordClient.h"

#include <Rpc.h>
#include <WinInet.h>
#include <shlwapi.h>

#include <memory>
#include <map>
#include <set>
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

struct InternetCallbackContext {
  std::map<DWORD, AwaitablePromise<void>*> promises;
  std::set<DWORD> seen;

  asio::awaitable<void> async_wait_for(
    DWORD status
  ) {
    if (seen.contains(status)) {
      seen.clear();
      co_return;
    }
    AwaitablePromise<void>* old = promises.contains(status) ? promises.at(status) : nullptr;
    auto executor = co_await asio::this_coro::executor;
    AwaitablePromise<void> p(reinterpret_cast<asio::io_context&>(executor.context()));
    promises.insert_or_assign(status, &p);
    co_await p.async_wait();
    if (old) {
      old->resolve();
      promises.insert_or_assign(status, old);
    } else {
      promises.erase(status);
    }
    seen.clear();
  }
};

void CALLBACK internet_status_callback(
  HINTERNET hInternet,
  DWORD_PTR dwContext,
  DWORD dwInternetStatus,
  LPVOID lpvStatusInformation,
  DWORD dwStatusInformationLength
) {
  ESDDebug("Internet status callback: {}", dwInternetStatus);
  auto ctx = reinterpret_cast<InternetCallbackContext*>(dwContext);
  if (ctx->promises.contains(dwInternetStatus)) {
    ctx->promises.at(dwInternetStatus)->resolve();
  }
  ctx->seen.insert(dwInternetStatus);
}

asio::awaitable<nlohmann::json> co_http_post(
  const std::string& host,
  const uint32_t port,
  const std::string& path,
  const DWORD flags,
  const std::string& headers,
  const std::string& post_data
) {
  if (!hInternet) {
    ESDDebug("Calling InternetOpen");
    hInternet = InternetOpenA(
      "com.fredemmott.streamdeck-discord", INTERNET_OPEN_TYPE_PRECONFIG,
      nullptr, nullptr, INTERNET_FLAG_ASYNC);
    InternetSetStatusCallback(hInternet, &internet_status_callback);
  }

  InternetCallbackContext ctx;

  ESDDebug("Calling InternetConnectA");
  auto hConnection = InternetConnectA(
    hInternet, host.c_str(), port, nullptr, nullptr,
    INTERNET_SERVICE_HTTP,
    flags,
    reinterpret_cast<DWORD_PTR>(&ctx));
  co_await ctx.async_wait_for(INTERNET_STATUS_HANDLE_CREATED);
  ESDDebug("Calling HttpOpenRequestA");

  auto hRequest = HttpOpenRequestA(
    hConnection, "POST", path.c_str(), nullptr, nullptr, nullptr,
    flags, reinterpret_cast<DWORD_PTR>(&ctx));

  HttpAddRequestHeadersA(hRequest, headers.c_str(), headers.length(), HTTP_ADDREQ_FLAG_ADD);

  ESDDebug("Sending request");
  HttpSendRequestA(
    hRequest, nullptr, 0, (void*)post_data.c_str(), post_data.length());

  ESDDebug("---Waiting for sendrequest");

  co_await ctx.async_wait_for(INTERNET_STATUS_REQUEST_COMPLETE);

  // I tried using HttpQueryInfo to get Content-Length; it turns out that
  // Discord only send a Content-Length header for error cases.
  std::string response;
  char buf[1024];
  DWORD bytesRead;
  do {
    InternetReadFile(hRequest, buf, sizeof(buf), &bytesRead);
    response += std::string(buf, bytesRead);
  } while (bytesRead > 0);
  ESDDebug("---InternetCloseHandle");
  InternetCloseHandle(hRequest);

  ESDDebug("received response");
  co_return json::parse(response);
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

asio::awaitable<DiscordClient::Credentials> DiscordClient::coGetOAuthCredentials(
  const std::string& grantType,
  const std::string& secretType,
  const std::string& secret) {
  auto json = co_await co_http_post(
    "discordapp.com",
    INTERNET_DEFAULT_HTTPS_PORT,
    "/api/oauth2/token",
    INTERNET_FLAG_SECURE,
    "Content-Type: application/x-www-form-urlencoded\r\nHost: discordapp.com",
    fmt::format(
      "grant_type={}&client_id={}&client_secret={}&{}={}",
      urlencode(grantType),
      urlencode(mAppId),
      urlencode(mAppSecret),
      urlencode(secretType),
      urlencode(secret)
    )
  );
  ESDDebug("Received credentials");
  json["access_token"].get_to(mCredentials.accessToken);
  json["refresh_token"].get_to(mCredentials.refreshToken);
  co_return mCredentials;
}
