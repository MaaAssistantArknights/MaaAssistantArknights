#pragma once

#ifdef _WIN32

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "MaaUtils/SafeWindows.hpp"

namespace asst::utils
{
inline bool write_windows_crash_bytes(HANDLE file, const char* value) noexcept
{
    if (value == nullptr) {
        return true;
    }

    std::size_t remaining = std::strlen(value);
    while (remaining > 0) {
        DWORD written = 0;
        const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(remaining, MAXDWORD));
        if (!WriteFile(file, value, requested, &written, nullptr) || written == 0) {
            return false;
        }

        value += written;
        remaining -= written;
    }

    return true;
}

inline bool write_windows_crash_file(const wchar_t* path, const char* reason, const char* detail) noexcept
{
    if (path == nullptr) {
        return false;
    }

    const HANDLE file = CreateFileW(
        path,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool written = write_windows_crash_bytes(file, "=== FATAL ERROR ===\n");
    if (reason != nullptr) {
        written = write_windows_crash_bytes(file, "Reason: ") && written;
        written = write_windows_crash_bytes(file, reason) && written;
        written = write_windows_crash_bytes(file, "\n") && written;
    }
    if (detail != nullptr) {
        written = write_windows_crash_bytes(file, "Detail: ") && written;
        written = write_windows_crash_bytes(file, detail) && written;
        written = write_windows_crash_bytes(file, "\n") && written;
    }
    written = write_windows_crash_bytes(file, "===================\n\n") && written;
    written = FlushFileBuffers(file) && written;
    written = CloseHandle(file) && written;
    return written;
}

inline bool write_windows_crash_file_with_fallback(
    const wchar_t* primary_path,
    const wchar_t* fallback_path,
    const char* reason,
    const char* detail) noexcept
{
    return write_windows_crash_file(primary_path, reason, detail) ||
           write_windows_crash_file(fallback_path, reason, detail);
}

inline const char* format_windows_access_type(ULONG_PTR operation) noexcept
{
    switch (operation) {
    case 0:
        return "read";
    case 1:
        return "write";
    case 8:
        return "execute";
    default:
        return "unknown";
    }
}

inline void format_windows_module_path(HMODULE module, char* destination, std::size_t destination_size) noexcept
{
    if (destination == nullptr || destination_size == 0) {
        return;
    }
    if (destination_size == 1) {
        destination[0] = '\0';
        return;
    }

    wchar_t wide_path[MAX_PATH] {};
    const DWORD wide_length =
        GetModuleFileNameW(module, wide_path, static_cast<DWORD>(sizeof(wide_path) / sizeof(wchar_t)));
    if (wide_length == 0 || wide_length >= sizeof(wide_path) / sizeof(wchar_t)) {
        std::snprintf(destination, destination_size, "<module-base:%p>", static_cast<void*>(module));
        return;
    }

    const int utf8_length = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wide_path,
        static_cast<int>(wide_length),
        destination,
        static_cast<int>(destination_size - 1),
        nullptr,
        nullptr);
    if (utf8_length <= 0) {
        std::snprintf(destination, destination_size, "<module-base:%p>", static_cast<void*>(module));
        return;
    }

    destination[utf8_length] = '\0';
}

inline void format_windows_exception_detail(
    PEXCEPTION_POINTERS exception_info,
    char* destination,
    std::size_t destination_size) noexcept
{
    if (destination == nullptr || destination_size == 0) {
        return;
    }
    if (exception_info == nullptr || exception_info->ExceptionRecord == nullptr) {
        std::snprintf(destination, destination_size, "Exception information was unavailable");
        return;
    }

    const auto* record = exception_info->ExceptionRecord;
    const auto fault_address = reinterpret_cast<std::uintptr_t>(record->ExceptionAddress);
    char module_path[MAX_PATH * 3] = "<unknown>";
    std::uintptr_t module_offset = 0;
    MEMORY_BASIC_INFORMATION memory_info {};
    if (record->ExceptionAddress != nullptr &&
        VirtualQuery(record->ExceptionAddress, &memory_info, sizeof(memory_info)) == sizeof(memory_info)) {
        const auto module = static_cast<HMODULE>(memory_info.AllocationBase);
        format_windows_module_path(module, module_path, sizeof(module_path));
        module_offset = fault_address - reinterpret_cast<std::uintptr_t>(memory_info.AllocationBase);
    }

    const bool is_access_violation =
        record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2;
    const char* access_type =
        is_access_violation ? format_windows_access_type(record->ExceptionInformation[0]) : "not-applicable";
    const void* accessed_address =
        is_access_violation ? reinterpret_cast<const void*>(record->ExceptionInformation[1]) : nullptr;
    std::snprintf(
        destination,
        destination_size,
        "ExceptionCode: 0x%08lX; ExceptionFlags: 0x%08lX; FaultAddress: %p; "
        "FaultingModule: %s; ModuleOffset: 0x%llX; AccessType: %s; AccessedAddress: %p",
        static_cast<unsigned long>(record->ExceptionCode),
        static_cast<unsigned long>(record->ExceptionFlags),
        record->ExceptionAddress,
        module_path,
        static_cast<unsigned long long>(module_offset),
        access_type,
        accessed_address);
}
} // namespace asst::utils

#endif
