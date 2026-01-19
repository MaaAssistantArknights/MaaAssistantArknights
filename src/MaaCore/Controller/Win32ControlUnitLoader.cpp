#ifdef _WIN32

#include "Win32ControlUnitLoader.h"

#include <Windows.h>

#include "Utils/Logger.hpp"

namespace asst
{

Win32ControlUnitLoader::Win32ControlUnitLoader() = default;

Win32ControlUnitLoader::~Win32ControlUnitLoader()
{
    unload();
}

bool Win32ControlUnitLoader::load(const std::filesystem::path& dll_path)
{
    LogTraceFunction;

    if (m_module) {
        Log.warn("DLL already loaded");
        return true;
    }

    std::filesystem::path full_path = dll_path;
    if (!full_path.has_extension()) {
        full_path += ".dll";
    }

    Log.info("Loading", full_path);

    m_module = LoadLibraryW(full_path.wstring().c_str());
    if (!m_module) {
        DWORD error = GetLastError();
        Log.error("Failed to load DLL, error code:", error);
        return false;
    }

    m_get_version = reinterpret_cast<GetVersionFunc>(
        GetProcAddress(static_cast<HMODULE>(m_module), "MaaWin32ControlUnitGetVersion"));
    m_create =
        reinterpret_cast<CreateFunc>(GetProcAddress(static_cast<HMODULE>(m_module), "MaaWin32ControlUnitCreate"));
    m_destroy =
        reinterpret_cast<DestroyFunc>(GetProcAddress(static_cast<HMODULE>(m_module), "MaaWin32ControlUnitDestroy"));

    if (!m_create || !m_destroy) {
        Log.error("Failed to get function pointers from DLL");
        unload();
        return false;
    }

    if (m_get_version) {
        Log.info("MaaWin32ControlUnit version:", m_get_version());
    }

    return true;
}

void Win32ControlUnitLoader::unload()
{
    if (m_module) {
        FreeLibrary(static_cast<HMODULE>(m_module));
        m_module = nullptr;
    }

    m_get_version = nullptr;
    m_create = nullptr;
    m_destroy = nullptr;
}

const char* Win32ControlUnitLoader::get_version() const
{
    if (m_get_version) {
        return m_get_version();
    }
    return nullptr;
}

void* Win32ControlUnitLoader::create(
    void* hwnd,
    Win32ScreencapMethod screencap_method,
    Win32InputMethod mouse_method,
    Win32InputMethod keyboard_method)
{
    LogTraceFunction;

    if (!m_create) {
        Log.error("DLL not loaded or create function not available");
        return nullptr;
    }

    void* handle = m_create(hwnd, screencap_method, mouse_method, keyboard_method);
    if (!handle) {
        Log.error("Failed to create Win32ControlUnit");
        return nullptr;
    }

    Log.info("Created Win32ControlUnit:", reinterpret_cast<void*>(handle));
    return handle;
}

void Win32ControlUnitLoader::destroy(void* handle)
{
    LogTraceFunction;

    if (!m_destroy) {
        Log.error("DLL not loaded or destroy function not available");
        return;
    }

    if (handle) {
        m_destroy(handle);
        Log.info("Destroyed Win32ControlUnit:", reinterpret_cast<void*>(handle));
    }
}

} // namespace asst

#endif // _WIN32
