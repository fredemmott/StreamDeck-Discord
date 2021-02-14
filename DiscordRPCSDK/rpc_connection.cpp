#include "rpc_connection.h"

#include "connection.h"

#include "StreamDeckSDK/ESDLogger.h"

#include <algorithm>
#include <atomic>

namespace {
  static const int RpcVersion = 1;
}

#ifdef __APPLE__
namespace {
void strncpy_s(char* dst, const char* src, size_t dstSize) {
  strlcpy(dst, src, dstSize);
}
void memcpy_s(void* dst, size_t dstSize, const void* src, size_t srcSize) {
  assert(dstSize >= srcSize);
  memcpy(dst, src, std::min(dstSize, srcSize));
}
}// namespace
#endif

RpcConnection::RpcConnection(
  const std::shared_ptr<asio::io_context>& ctx,
  const std::string& applicationId
): appId(applicationId), ioContext(ctx), connection(std::make_unique<BaseConnection>(ctx)) {
}

RpcConnection::~RpcConnection() {
}

asio::awaitable<bool> RpcConnection::AsyncOpen() {
  ESDDebug("Opening - state: {}", state);
  if (state != State::Disconnected) {
    co_return false;
  }

  if (!connection->Open()) {
    ESDDebug("Failed to open base connection");
    co_return false;
  }
  ESDDebug("Opened base connection");

  {
    json message;
    message["v"] = RpcVersion;
    message["client_id"] = appId;
    const auto json = message.dump();
    sendFrame.opcode = Opcode::Handshake;
    sendFrame.length = json.length();
    strncpy_s(sendFrame.message, json.c_str(), sizeof(sendFrame.message));

    if (this->Write(sendFrame)) {
      state = State::SentHandshake;
    } else {
      Close();
      co_return false;
    }
  }

  ESDDebug("Sent handshake");

  {
    json message;
    MessageFrame frame;
    if (!co_await AsyncRead(frame)) {
      ESDDebug("Failed to read dispatch frame");
      co_return false;
    }

    message = json::parse(frame.message);
    auto cmd = message.value("cmd", "");
    auto evt = message.value("evt", "");
    if (cmd == "DISPATCH" && evt == "READY") {
      state = State::Connected;
      if (onConnect) {
        onConnect(message);
      }
    }
    ESDDebug("connected");
    co_return true;
  }
}

bool RpcConnection::Write(const MessageFrame& message) {
  return connection->Write(
    &message, sizeof(MessageFrameHeader) + message.length);
}

void RpcConnection::Close() {
  if (state == State::Disconnected) {
    return;
  }
  if (
    onDisconnect
    && (state == State::Connected || state == State::SentHandshake)) {
    onDisconnect(lastErrorCode, lastErrorMessage);
  }
  if (connection) {
    connection->Close();
  }
  state = State::Disconnected;
}

void RpcConnection::Write(const json& message) {
  const auto json = message.dump();
  ESDDebug("Writing raw {}", json);
  Write(json.c_str(), json.length());
}

bool RpcConnection::Write(const void* data, size_t length) {
  sendFrame.opcode = Opcode::Frame;
  memcpy_s(sendFrame.message, sizeof(sendFrame.message), data, length);
  sendFrame.length = (uint32_t)length;
  if (!connection->Write(&sendFrame, sizeof(MessageFrameHeader) + length)) {
    Close();
    return false;
  }
  return true;
}

asio::awaitable<bool> RpcConnection::AsyncRead(json* message) {
  MessageFrame frame;
  if (!co_await AsyncRead(frame)) {
    co_return false;
  }
  *message = json::parse(frame.message);
  co_return true;
}

asio::awaitable<bool> RpcConnection::AsyncRead(MessageFrame& readFrame) {
  if (state != State::Connected && state != State::SentHandshake) {
    co_return false;
  }
  for (;;) {
    bool didRead = co_await connection->AsyncRead(&readFrame, sizeof(MessageFrameHeader));
    if (!didRead) {
      if (!connection->isOpen) {
        lastErrorCode = (int)ErrorCode::PipeClosed;
        lastErrorMessage = "Pipe closed";
        Close();
      }
      co_return false;
    }

    if (readFrame.length > 0) {
      didRead = co_await connection->AsyncRead(readFrame.message, readFrame.length);
      if (!didRead) {
        lastErrorCode = (int)ErrorCode::ReadCorrupt;
        lastErrorMessage = "Partial data in frame";
        Close();
        co_return false;
      }
      readFrame.message[readFrame.length] = 0;
    }

    switch (readFrame.opcode) {
      case Opcode::Close: {
        auto json = json::parse(readFrame.message);
        lastErrorCode = json.value("code", -1);
        lastErrorMessage = json.value("message", "");
        Close();
        co_return false;
      }
      case Opcode::Frame:
        co_return true;
      case Opcode::Ping:
        readFrame.opcode = Opcode::Pong;
        if (!connection->Write(
              &readFrame, sizeof(MessageFrameHeader) + readFrame.length)) {
          Close();
        }
        break;
      case Opcode::Pong:
        break;
      case Opcode::Handshake:
      default:
        // something bad happened
        lastErrorCode = (int)ErrorCode::ReadCorrupt;
        lastErrorMessage = "Bad ipc frame";
        Close();
        co_return false;
    }
  }
}
