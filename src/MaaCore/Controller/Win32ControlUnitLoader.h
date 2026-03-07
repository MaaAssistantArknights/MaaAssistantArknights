#pragma once

#ifdef _WIN32

#include <cstdint>
#include <filesystem>
#include <memory>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/AsstTypes.h"

#include "MaaFwControlUnitInterface.h"

namespace asst
{
// 动态加载 MaaWin32ControlUnit.dll
class Win32ControlUnitLoader
{
public:
    Win32ControlUnitLoader();
    ~Win32ControlUnitLoader();

    Win32ControlUnitLoader(const Win32ControlUnitLoader&) = delete;
    Win32ControlUnitLoader& operator=(const Win32ControlUnitLoader&) = delete;
    Win32ControlUnitLoader(Win32ControlUnitLoader&&) = delete;
    Win32ControlUnitLoader& operator=(Win32ControlUnitLoader&&) = delete;

    // 加载 DLL
    bool load(const std::filesystem::path& dll_path);

    // 卸载 DLL
    void unload();

    // 是否已加载
    bool loaded() const noexcept { return m_module != nullptr; }

    // 获取版本
    const char* get_version() const;

    // 创建控制单元
    void* create(
        void* hwnd,
        Win32ScreencapMethod screencap_method,
        Win32InputMethod mouse_method,
        Win32InputMethod keyboard_method);

    // 销毁控制单元
    void destroy(void* handle);

private:
    void* m_module = nullptr;

    // 函数指针类型
    using GetVersionFunc = const char* (*)();
    using CreateFunc = void* (*)(void*, uint64_t, uint64_t, uint64_t);
    using DestroyFunc = void (*)(void*);

    GetVersionFunc m_get_version = nullptr;
    CreateFunc m_create = nullptr;
    DestroyFunc m_destroy = nullptr;
};
} // namespace asst

#endif // _WIN32
