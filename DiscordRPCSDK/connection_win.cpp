#include "connection.h"

#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME
#include <assert.h>
#include <windows.h>

struct BaseConnection::Impl {
    HANDLE pipe{INVALID_HANDLE_VALUE};
};

BaseConnection::BaseConnection(const std::shared_ptr<asio::io_context>&)
{
}

BaseConnection::~BaseConnection() {
    this->Close();
}

bool BaseConnection::Open()
{
    wchar_t pipeName[]{L"\\\\?\\pipe\\discord-ipc-0"};
    const size_t pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
    pipeName[pipeDigit] = L'0';
    for (;;) {
        this->p->pipe = ::CreateFileW(
          pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (this->p->pipe != INVALID_HANDLE_VALUE) {
            this->isOpen = true;
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

bool BaseConnection::Close()
{
    ::CloseHandle(this->p->pipe);
    this->p->pipe = INVALID_HANDLE_VALUE;
    this->isOpen = false;
    return true;
}

bool BaseConnection::Write(const void* data, size_t length)
{
    if (length == 0) {
        return true;
    }
    if (this->p->pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    assert(data);
    if (!data) {
        return false;
    }
    const DWORD bytesLength = (DWORD)length;
    DWORD bytesWritten = 0;
    return ::WriteFile(this->p->pipe, data, bytesLength, &bytesWritten, nullptr) == TRUE &&
      bytesWritten == bytesLength;
}

bool BaseConnection::Read(void* data, size_t length)
{
    assert(data);
    if (!data) {
        return false;
    }
    if (this->p->pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD bytesAvailable = 0;
    if (::PeekNamedPipe(this->p->pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
        if (bytesAvailable >= length) {
            DWORD bytesToRead = (DWORD)length;
            DWORD bytesRead = 0;
            if (::ReadFile(this->p->pipe, data, bytesToRead, &bytesRead, nullptr) == TRUE) {
                assert(bytesToRead == bytesRead);
                return true;
            }
            else {
                Close();
            }
        }
    }
    else {
        Close();
    }
    return false;
}
