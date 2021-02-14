#include "connection.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

struct BaseConnection::Impl {
    int sock{-1};
};

static BaseConnection Connection;
#ifdef MSG_NOSIGNAL
static int MsgFlags = MSG_NOSIGNAL;
#else
static int MsgFlags = 0;
#endif

static const char* GetTempPath()
{
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}

BaseConnection::BaseConnection() : p(new BaseConnection::Impl) {}

BaseConnection::~BaseConnection()
{
    Close();
}

bool BaseConnection::Open()
{
    assert(this->p);
    const char* tempPath = GetTempPath();
    this->p->sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->p->sock == -1) {
        return false;
    }
    fcntl(this->p->sock, F_SETFL, O_NONBLOCK);
#ifdef SO_NOSIGPIPE
    int optval = 1;
    setsockopt(this->p->sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
#endif

    static sockaddr_un PipeAddr {};
    PipeAddr.sun_family = AF_UNIX;
    for (int pipeNum = 0; pipeNum < 10; ++pipeNum) {
        snprintf(
          PipeAddr.sun_path, sizeof(PipeAddr.sun_path), "%s/discord-ipc-%d", tempPath, pipeNum);
        int err = connect(this->p->sock, (const sockaddr*)&PipeAddr, sizeof(PipeAddr));
        if (err == 0) {
            this->isOpen = true;
            return true;
        }
    }
    this->Close();
    return false;
}

bool BaseConnection::Close()
{
    if (this->p->sock == -1) {
        return false;
    }
    close(this->p->sock);
    this->p->sock = -1;
    this->isOpen = false;
    return true;
}

bool BaseConnection::Write(const void* data, size_t length)
{
    if (this->p->sock == -1) {
        return false;
    }

    ssize_t sentBytes = send(this->p->sock, data, length, MsgFlags);
    if (sentBytes < 0) {
        Close();
    }
    return sentBytes == (ssize_t)length;
}

bool BaseConnection::Read(void* data, size_t length)
{
    if (this->p->sock == -1) {
        return false;
    }

    int res = (int)recv(this->p->sock, data, length, MsgFlags);
    if (res < 0) {
        if (errno == EAGAIN) {
            return false;
        }
        Close();
    }
    else if (res == 0) {
        Close();
    }
    return res == (int)length;
}
