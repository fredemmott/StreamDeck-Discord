#pragma once

#include "connection.h"
#include "../Common/EPLJSONUtils.h"

#include <functional>
#include <string>

// I took this from the buffer size libuv uses for named pipes; I suspect ours would usually be much
// smaller.
constexpr size_t MaxRpcFrameSize = 64 * 1024;

struct RpcConnection {
    enum class ErrorCode : int {
        Success = 0,
        PipeClosed = 1,
        ReadCorrupt = 2,
    };

    enum class Opcode : uint32_t {
        Handshake = 0,
        Frame = 1,
        Close = 2,
        Ping = 3,
        Pong = 4,
    };

    struct MessageFrameHeader {
        Opcode opcode;
        uint32_t length;
    };

    struct MessageFrame : public MessageFrameHeader {
        char message[MaxRpcFrameSize - sizeof(MessageFrameHeader)];
    };

    enum class State : uint32_t {
        Disconnected,
        SentHandshake,
        AwaitingResponse,
        Connected,
    };

    BaseConnection* connection{nullptr};
    State state{State::Disconnected};
	std::function<void(const json&)> onConnect = [](const json& message) {};
	std::function<void(int, const std::string&)> onDisconnect = [](int code, const std::string& message) {};
	std::string appId;
    int lastErrorCode{0};
	std::string lastErrorMessage;
    RpcConnection::MessageFrame sendFrame;

    static RpcConnection* Create(const std::string& applicationId);
    static void Destroy(RpcConnection*&);

    inline bool IsOpen() const { return state == State::Connected; }

    void Open();
    void Close();
	void Write(const json& message);
	bool Read(json* message);
private:
    bool Read(MessageFrame& message);
	bool Write(const MessageFrame& message);
    bool Write(const void* data, size_t length);
};
