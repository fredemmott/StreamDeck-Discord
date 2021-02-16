#include "connection.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <StreamDeckSDK/ESDLogger.h>

#ifdef __APPLE__
using asiosock = asio::local::stream_protocol::socket;
#else
using asiosock = asio::windows::stream_handle;
typedef SSIZE_T ssize_t;
#endif

struct BaseConnection::Impl {
    std::shared_ptr<asio::io_context> asioctx;
    std::unique_ptr<asiosock> asiosock;
};

BaseConnection::BaseConnection(const std::shared_ptr<asio::io_context>& asioctx) : p(new BaseConnection::Impl { .asioctx = asioctx }) {
}

BaseConnection::~BaseConnection()
{
    Close();
}

#ifdef __APPLE__

namespace {

static const char* GetTempPath()
{
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}

} // namespace

bool BaseConnection::Open()
{
    const char* tempPath = GetTempPath();
    this->p->asiosock = std::make_unique<asiosock>(*this->p->asioctx);
    char sun_path[2048];
    asio::error_code ec;
    for (int pipeNum = 0; pipeNum < 10; ++pipeNum) {
        snprintf(sun_path, sizeof(sun_path), "%s/discord-ipc-%d", tempPath, pipeNum);
        this->p->asiosock->connect(sun_path, ec);
        if (!ec) {
            this->isOpen = true;
            return true;
        }
    }
    this->Close();
    return false;
}

#else // ifdef __APPLE__

bool BaseConnection::Open()
{
    wchar_t pipeName[]{L"\\\\?\\pipe\\discord-ipc-0"};
    const size_t pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
    pipeName[pipeDigit] = L'0';
    for (;;) {
        ESDDebug(L"Attempting to connect to pipe {}", pipeName);
        auto pipe = ::CreateFileW(
          pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            this->isOpen = true;
            this->p->asiosock = std::make_unique<asiosock>(*this->p->asioctx);
            this->p->asiosock->assign(pipe);
            return true;
        }

        auto lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND) {
            if (pipeName[pipeDigit] < L'9') {
                pipeName[pipeDigit]++;
                continue;
            }
        }
        else if (lastError == ERROR_PIPE_BUSY) {
            if (!WaitNamedPipeW(pipeName, 10000)) {
                return false;
            }
            continue;
        }
        return false;
    }
}


#endif // ifdef __APPLE__

bool BaseConnection::Close()
{
    if (!this->p->asiosock) {
        return false;
    }
    if (!this->p->asiosock->is_open()) {
      return false;
    }
    this->p->asiosock->close();
    this->p->asiosock.reset();
    this->isOpen = false;
    return true;
}

asio::awaitable<bool> BaseConnection::AsyncWrite(const void* data, size_t length)
{
    if (!this->p->asiosock) {
        co_return false;
    }

    asio::error_code ec;
    ssize_t sentBytes = co_await this->p->asiosock->async_write_some(asio::buffer(data, length), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        Close();
    }
    if (sentBytes < 0) {
        Close();
    }
    co_return sentBytes == (ssize_t)length;
}

asio::awaitable<bool> BaseConnection::AsyncRead(void* data, size_t length) {
    assert(this->p->asiosock);
    asio::error_code ec;
    size_t res;
    res = co_await this->p->asiosock->async_read_some(asio::buffer(data, length), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        ESDLog("Error: {} - calling Close", ec.message());
        Close();
        co_return false;
    }
    if (res == 0) {
        Close();
    }
    co_return res == length;
}
