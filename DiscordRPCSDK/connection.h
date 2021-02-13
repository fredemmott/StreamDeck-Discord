#pragma once

// This is to wrap the platform specific kinds of connect/read/write.

#include <memory>
#include <cstdint>

struct BaseConnection final {
    BaseConnection();
    ~BaseConnection();

    bool isOpen{false};
    bool Open();
    bool Close();
    bool Write(const void* data, size_t length);
    bool Read(void* data, size_t length);
    private:
      struct Impl;
      std::unique_ptr<Impl> p;
};
