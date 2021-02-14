#include "connection.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using asiosock = asio::local::stream_protocol::socket;

struct BaseConnection::Impl {
    std::shared_ptr<asio::io_context> asioctx;
    std::unique_ptr<asiosock> asiosock;
};

static const char* GetTempPath()
{
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}

BaseConnection::BaseConnection(const std::shared_ptr<asio::io_context>& asioctx) : p(new BaseConnection::Impl { .asioctx = asioctx }) {
}

BaseConnection::~BaseConnection()
{
    Close();
}

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

bool BaseConnection::Close()
{
    if (!this->p->asiosock) {
        return false;
    }
    this->p->asiosock->close();
    this->p->asiosock.reset();
    this->isOpen = false;
    return true;
}

bool BaseConnection::Write(const void* data, size_t length)
{
    if (!this->p->asiosock) {
        return false;
    }

    ssize_t sentBytes = this->p->asiosock->write_some(asio::buffer(data, length));
    if (sentBytes < 0) {
        Close();
    }
    return sentBytes == (ssize_t)length;
}

bool BaseConnection::Read(void* data, size_t length)
{
    if (!this->p->asiosock) {
        return false;
    }

    asio::error_code ec;
    int res = this->p->asiosock->receive(asio::buffer(data, length), 0, ec);
    if (ec) {
        if (ec == asio::error::try_again) {
            return false;
        }
        Close();
    }
    else if (res == 0) {
        Close();
    }
    return res == (int)length;
}
