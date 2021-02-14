#pragma once

// This is to wrap the platform specific kinds of connect/read/write.

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <memory>
#include <cstdint>

struct BaseConnection final {
    BaseConnection(const std::shared_ptr<asio::io_context>&);
    ~BaseConnection();

    bool isOpen{false};
    bool Open();
    bool Close();
    bool Write(const void* data, size_t length);
    asio::awaitable<bool> AsyncRead(void* data, size_t length);
    private:
      struct Impl;
      std::unique_ptr<Impl> p;
};
