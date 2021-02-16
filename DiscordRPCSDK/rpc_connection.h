#pragma once

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>

using json = nlohmann::json;
class BaseConnection;

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
        Connected,
    };

    State state{State::Disconnected};
	std::function<void(const json&)> onConnect = [](const json& message) {};
	std::function<void(int, const std::string&)> onDisconnect = [](int code, const std::string& message) {};
	std::string appId;
    int lastErrorCode{0};
	std::string lastErrorMessage;
    RpcConnection::MessageFrame sendFrame;

    RpcConnection(const std::shared_ptr<asio::io_context>&, const std::string& applicationId);
    ~RpcConnection();

    inline bool IsOpen() const { return state == State::Connected; }

    asio::awaitable<bool> AsyncOpen();
    void Close();

	asio::awaitable<void> AsyncWrite(const json& message);
    // Ban implicit conversions; if you hit this, you're likely calling Write(json.dump()) instead of Write(json)
    template<typename T>
    asio::awaitable<void> AsyncWrite(T) = delete;

	asio::awaitable<bool> AsyncRead(json* message);
private:
    asio::awaitable<bool> AsyncRead(MessageFrame& message);
	asio::awaitable<bool> AsyncWrite(const MessageFrame& message);
    asio::awaitable<bool> AsyncWrite(const void* data, size_t length);
    std::unique_ptr<BaseConnection> connection{nullptr};
    std::shared_ptr<asio::io_context> ioContext;
};
