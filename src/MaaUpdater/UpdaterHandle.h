#pragma once

#include <windows.h>

// ---------------------------------------------------------------------------
// RAII wrapper for Windows HANDLE
// ---------------------------------------------------------------------------

class ScopedHandle {
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE h) noexcept : m_handle(h) {}

    ~ScopedHandle() { close(); }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept
        : m_handle(other.m_handle) { other.m_handle = nullptr; }

    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) { close(); m_handle = other.m_handle; other.m_handle = nullptr; }
        return *this;
    }

    void close() noexcept {
        if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }

    [[nodiscard]] HANDLE get() const noexcept { return m_handle; }
    [[nodiscard]] bool valid() const noexcept { return m_handle && m_handle != INVALID_HANDLE_VALUE; }
    explicit operator bool() const noexcept { return valid(); }

private:
    HANDLE m_handle = nullptr;
};
