#ifdef __ANDROID__

#include "AndroidExternalLib.h"

#include "Utils/Logger.hpp"

using namespace asst;

AndroidExternalLib* AndroidExternalLib::s_instance = nullptr;

AndroidExternalLib::AndroidExternalLib()
{
    LogTraceFunction;
    LogInfo << "AndroidExternalLib instance created";
}

AndroidExternalLib::~AndroidExternalLib()
{
    LogTraceFunction;
    if (m_handle) {
        LogInfo << "Unloading library: " << m_library_path;
        dlclose(m_handle);
        m_handle = nullptr;
    }
}

AndroidExternalLib& AndroidExternalLib::instance()
{
    if (!s_instance) {
        s_instance = new AndroidExternalLib();
    }
    return *s_instance;
}

bool AndroidExternalLib::load(const std::string& library_path)
{
    LogTraceFunction;

    if (library_path.empty()) {
        LogError << "Library path is empty";
        return false;
    }

    auto& inst = instance();

    if (inst.m_loaded && inst.m_library_path == library_path) {
        LogInfo << "Library already loaded: " << library_path;
        return true;
    }

    if (inst.load_library(library_path)) {
        LogInfo << "Successfully loaded library: " << library_path;
        return true;
    } else {
        LogError << "Failed to load library: " << library_path;
        return false;
    }
}

bool AndroidExternalLib::load_library(const std::string& library_path)
{
    LogTraceFunction;
    LogInfo << "Loading library: " << library_path;

    if (m_handle && m_library_path == library_path && m_loaded) {
        LogInfo << "Library already loaded: " << library_path;
        return true;
    }

    if (m_handle) {
        LogInfo << "Unloading previous library: " << m_library_path;
        dlclose(m_handle);
        m_handle = nullptr;
        m_loaded = false;
        clear_symbols();
    }

    m_handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (!m_handle) {
        LogError << "Failed to load library: " << dlerror();
        m_loaded = false;
        return false;
    }

    m_library_path = library_path;

    if (!resolve_symbols()) {
        LogError << "Failed to resolve symbols from library";
        dlclose(m_handle);
        m_handle = nullptr;
        clear_symbols();
        return false;
    }

    m_loaded = true;
    LogInfo << "Successfully loaded library and resolved symbols: " << library_path;
    return true;
}

bool AndroidExternalLib::resolve_symbols()
{
    LogTraceFunction;

    if (!m_handle) {
        LogError << "No library handle available";
        return false;
    }

    dlerror();

    // 解析 GetLockedPixels
    m_funcs.get_locked_pixels = (GetLockedPixels_t)dlsym(m_handle, "GetLockedPixels");
    if (!m_funcs.get_locked_pixels) {
        LogError << "Failed to resolve GetLockedPixels: " << dlerror();
        return false;
    }

    // 解析 UnlockPixels
    m_funcs.unlock_pixels = (UnlockPixels_t)dlsym(m_handle, "UnlockPixels");
    if (!m_funcs.unlock_pixels) {
        LogError << "Failed to resolve UnlockPixels: " << dlerror();
        return false;
    }

    // 解析 AttachThread
    m_funcs.attach_thread = (AttachThread_t)dlsym(m_handle, "AttachThread");
    if (!m_funcs.attach_thread) {
        LogError << "Failed to resolve AttachThread: " << dlerror();
        return false;
    }

    // 解析 DetachThread
    m_funcs.detach_thread = (DetachThread_t)dlsym(m_handle, "DetachThread");
    if (!m_funcs.detach_thread) {
        LogError << "Failed to resolve DetachThread: " << dlerror();
        return false;
    }

    // 解析 DispatchInputMessage
    m_funcs.dispatch_input_message = (DispatchInputMessage_t)dlsym(m_handle, "DispatchInputMessage");
    if (!m_funcs.dispatch_input_message) {
        LogError << "Failed to resolve DispatchInputMessage: " << dlerror();
        return false;
    }

    LogInfo << "All symbols resolved successfully";
    return true;
}

void AndroidExternalLib::clear_symbols()
{
    m_funcs.get_locked_pixels = nullptr;
    m_funcs.unlock_pixels = nullptr;
    m_funcs.attach_thread = nullptr;
    m_funcs.detach_thread = nullptr;
    m_funcs.dispatch_input_message = nullptr;
}

bool AndroidExternalLib::is_loaded() const
{
    return m_handle != nullptr && m_loaded &&
           m_funcs.get_locked_pixels != nullptr;
}

FrameInfo AndroidExternalLib::GetLockedPixels(void) const
{
    LogTraceFunction;

    static constexpr FrameInfo empty_result = {
        0,       // width
        0,       // height
        0,       // stride
        0,       // length
        nullptr, // data
        nullptr  // frame_ref
    };

    if (!is_loaded()) {
        LogError << "Library not properly loaded";
        return empty_result;
    }

    if (!m_funcs.get_locked_pixels) {
        LogError << "GetLockedPixels function not available";
        return empty_result;
    }

    LogInfo << "Calling GetLockedPixels()...";
    FrameInfo result = m_funcs.get_locked_pixels();

    LogInfo << "GetLockedPixels() completed successfully";
    LogDebug << "LockedPixels - width: " << result.width
             << ", height: " << result.height
             << ", stride: " << result.stride
             << ", length: " << result.length
             << ", data: " << result.data
             << ", frame_ref: " << result.frame_ref;

    return result;
}

int AndroidExternalLib::UnlockPixels(FrameInfo info) const
{
    LogTraceFunction;

    if (!is_loaded()) {
        LogError << "Library not properly loaded";
        return -1;
    }

    if (!m_funcs.unlock_pixels) {
        LogError << "UnlockPixels function not available";
        return -1;
    }

    LogInfo << "Calling UnlockPixels()...";
    LogDebug << "Unlocking pixels - data: " << info.data
             << ", frame_ref: " << info.frame_ref;

    int result = m_funcs.unlock_pixels(info);

    if (result == 0) {
        LogInfo << "UnlockPixels() completed successfully";
    } else {
        LogError << "UnlockPixels() failed with code: " << result;
    }

    return result;
}

void* AndroidExternalLib::AttachThread() const
{
    LogTraceFunction;

    if (!is_loaded()) {
        LogError << "Library not properly loaded";
        return nullptr;
    }

    if (!m_funcs.attach_thread) {
        LogError << "AttachThread function not available";
        return nullptr;
    }

    LogInfo << "Calling AttachThread()...";
    void* env = m_funcs.attach_thread();

    if (env) {
        LogInfo << "AttachThread() completed successfully, env: " << env;
    } else {
        LogError << "AttachThread() returned null env";
    }

    return env;
}

int AndroidExternalLib::DetachThread(void* env) const
{
    LogTraceFunction;

    if (!is_loaded()) {
        LogError << "Library not properly loaded";
        return -1;
    }

    if (!m_funcs.detach_thread) {
        LogError << "DetachThread function not available";
        return -1;
    }

    LogInfo << "Calling DetachThread() with env: " << env;
    int result = m_funcs.detach_thread(env);

    if (result == 0) {
        LogInfo << "DetachThread() completed successfully";
    } else {
        LogError << "DetachThread() failed with code: " << result;
    }

    return result;
}

int AndroidExternalLib::DispatchInputMessage(MethodParam param) const
{
    LogTraceFunction;

    if (!is_loaded()) {
        LogError << "Library not properly loaded";
        return -1;
    }

    if (!m_funcs.dispatch_input_message) {
        LogError << "DispatchInputMessage function not available";
        return -1;
    }

    LogInfo << "Calling DispatchInputMessage() with method: " << static_cast<int>(param.method)
            << ", display_id: " << param.display_id;

    switch (param.method) {
        case START_GAME:
            LogDebug << "START_GAME - package_name: " << (param.args.start_game.package_name ? param.args.start_game.package_name : "null")
                     << ", force_stop: " << param.args.start_game.force_stop;
            break;
        case STOP_GAME:
            LogDebug << "STOP_GAME - client_type: " << (param.args.stop_game.client_type ? param.args.stop_game.client_type : "null");
            break;
        case INPUT:
            LogDebug << "INPUT - text: " << (param.args.input.text ? param.args.input.text : "null");
            break;
        case TOUCH_DOWN:
            LogDebug << "TOUCH_DOWN - point: (" << param.args.touch.p.x << ", " << param.args.touch.p.y << ")";
            break;
        case TOUCH_MOVE:
            LogDebug << "TOUCH_MOVE - point: (" << param.args.touch.p.x << ", " << param.args.touch.p.y << ")";
            break;
        case TOUCH_UP:
            LogDebug << "TOUCH_UP - point: (" << param.args.touch.p.x << ", " << param.args.touch.p.y << ")";
            break;
        case KEY_DOWN:
            LogDebug << "KEY_DOWN - key_code: " << param.args.key.key_code;
            break;
        case KEY_UP:
            LogDebug << "KEY_UP - key_code: " << param.args.key.key_code;
            break;
        default:
            LogWarn << "Unknown method type: " << static_cast<int>(param.method);
            break;
    }

    int result = m_funcs.dispatch_input_message(param);

    if (result == 0) {
        LogInfo << "DispatchInputMessage() completed successfully";
    } else {
        LogError << "DispatchInputMessage() failed with code: " << result;
    }

    return result;
}

#endif // __ANDROID__