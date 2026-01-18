#pragma once

#ifdef _WIN32

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
// 与 MaaFramework 的 ControlUnitAPI 接口保持 ABI 兼容
// 虚函数表布局必须与 MaaFramework 完全一致
class MaaControlUnitInterface
{
public:
    virtual ~MaaControlUnitInterface() = default;

    virtual bool connect() = 0;
    virtual bool connected() const = 0;

    virtual bool request_uuid(std::string& uuid) = 0;
    virtual uint64_t get_features() const = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool screencap(cv::Mat& image) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool scroll(int dx, int dy) = 0;
};

// 与 MaaFramework 的 MaaControllerFeature 兼容的常量
namespace MaaFeature
{
constexpr uint64_t None = 0;
constexpr uint64_t UseMouseDownAndUpInsteadOfClick = 1ULL << 1;
constexpr uint64_t UseKeyboardDownAndUpInsteadOfClick = 1ULL << 2;
} // namespace MaaFeature

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
