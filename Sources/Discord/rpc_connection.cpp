#include "rpc_connection.h"

#include "StreamDeckSDK/EPLJSONUtils.h"

#include <algorithm>
#include <atomic>
#include "../pch.h"

static const int RpcVersion = 1;
static RpcConnection Instance;

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

/*static*/ RpcConnection* RpcConnection::Create(
  const std::string& applicationId) {
  Instance.connection = BaseConnection::Create();
  Instance.appId = applicationId;
  return &Instance;
}

/*static*/ void RpcConnection::Destroy(RpcConnection*& c) {
  c->Close();
  if (c->connection) {
    BaseConnection::Destroy(c->connection);
  }
  c = nullptr;
}

void RpcConnection::Open() {
  if (state == State::Connected) {
    return;
  }

  if (state == State::Disconnected && !connection->Open()) {
    return;
  }

  if (state == State::SentHandshake) {
    json message;
    MessageFrame frame;
    if (Read(frame)) {
      message = json::parse(frame.message);
      auto cmd = EPLJSONUtils::GetStringByName(message, "cmd");
      auto evt = EPLJSONUtils::GetStringByName(message, "evt");
      if (cmd == "DISPATCH" && evt == "READY") {
        state = State::Connected;
        if (onConnect) {
          onConnect(message);
        }
      }
    }
  } else {
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
    }
  }
}

bool RpcConnection::Write(const MessageFrame& message) {
  return connection->Write(
    &message, sizeof(MessageFrameHeader) + message.length);
}

void RpcConnection::Close() {
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
  Write(json.c_str(), json.length());
}

bool RpcConnection::Write(const void* data, size_t length) {
  DebugPrint("[discord][rpc] sending: %s", data);
  sendFrame.opcode = Opcode::Frame;
  memcpy_s(sendFrame.message, sizeof(sendFrame.message), data, length);
  sendFrame.length = (uint32_t)length;
  if (!connection->Write(&sendFrame, sizeof(MessageFrameHeader) + length)) {
    Close();
    return false;
  }
  return true;
}

bool RpcConnection::Read(json* message) {
  MessageFrame frame;
  if (!Read(frame)) {
    return false;
  }
  *message = json::parse(frame.message);
  return true;
}

bool RpcConnection::Read(MessageFrame& readFrame) {
  if (state != State::Connected && state != State::SentHandshake) {
    return false;
  }
  for (;;) {
    bool didRead = connection->Read(&readFrame, sizeof(MessageFrameHeader));
    if (!didRead) {
      if (!connection->isOpen) {
        lastErrorCode = (int)ErrorCode::PipeClosed;
        lastErrorMessage = "Pipe closed";
        Close();
      }
      return false;
    }

    if (readFrame.length > 0) {
      didRead = connection->Read(readFrame.message, readFrame.length);
      if (!didRead) {
        lastErrorCode = (int)ErrorCode::ReadCorrupt;
        lastErrorMessage = "Partial data in frame";
        Close();
        return false;
      }
      readFrame.message[readFrame.length] = 0;
    }

    switch (readFrame.opcode) {
      case Opcode::Close: {
        auto json = json::parse(readFrame.message);
        lastErrorCode = EPLJSONUtils::GetIntByName(json, "code");
        lastErrorMessage = EPLJSONUtils::GetStringByName(json, "message");
        Close();
        return false;
      }
      case Opcode::Frame:
        DebugPrint("[discord][rpc] received: %s", readFrame.message);
        return true;
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
        return false;
    }
  }
}
