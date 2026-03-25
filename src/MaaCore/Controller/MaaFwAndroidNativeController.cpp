#ifdef __ANDROID__

#include "MaaFwAndroidNativeController.h"

#include <thread>

#include "Common/AsstMsg.h"
#include "Config/GeneralConfig.h"
#include "Controller/MaaFwControlUnitInterface.h"
#include "Utils/Logger.hpp"

namespace asst
{

MaaFwAndroidNativeController::MaaFwAndroidNativeController(const AsstCallback& callback, Assistant* inst) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

MaaFwAndroidNativeController::~MaaFwAndroidNativeController()
{
    LogTraceFunction;

    if (m_unit_handle && m_destroy_func) {
        LogInfo << "Cleaning up MaaAndroidNativeControlUnit";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
    }
}

bool MaaFwAndroidNativeController::init_library()
{
    if (m_get_version_func && m_create_func && m_destroy_func) {
        LogInfo << "MaaAndroidNativeControlUnit library already loaded";
        return true;
    }
    if (!load_library("MaaAndroidNativeControlUnit")) {
        LogError << "Failed to load MaaAndroidNativeControlUnit library";
        return false;
    }

    m_get_version_func = get_function<GetVersionFunc>("MaaAndroidNativeControlUnitGetVersion");
    m_create_func = get_function<CreateFunc>("MaaAndroidNativeControlUnitCreate");
    m_destroy_func = get_function<DestroyFunc>("MaaAndroidNativeControlUnitDestroy");
    m_attach_thread_func = get_function<AttachThreadFunc>("MaaAndroidNativeControlUnitAttachThread");
    m_detach_thread_func = get_function<DetachThreadFunc>("MaaAndroidNativeControlUnitDetachThread");

    if (!m_get_version_func || !m_create_func || !m_destroy_func || !m_attach_thread_func || !m_detach_thread_func) {
        LogError << "Failed to get function pointers from MaaAndroidNativeControlUnit library";
        return false;
    }

    LogInfo << "MaaAndroidNativeControlUnit library version:" << m_get_version_func();

    return true;
}

bool MaaFwAndroidNativeController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config)
{
    LogTraceFunction;

    m_inited = false;
    m_uuid.clear();
    m_screen_size = { 0, 0 };

    auto get_info_json = [&]() -> json::object {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "config", config },
              } },
        };
    };

    if (!init_library()) {
        return false;
    }

    if (m_unit_handle && m_destroy_func) {
        LogInfo << "Cleaning up the old connection and reconnecting";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
    }

    // config 直接透传给 MaaAndroidNativeControlUnit
    // config_json 需包含: library_path, screen_resolution.{width,height}, display_id(可选), force_stop(可选)
    m_unit_handle = m_create_func(config.c_str());

    if (!m_unit_handle) {
        LogError << "Failed to create MaaAndroidNativeControlUnit";
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaAndroidNativeControlUnit creation failed" },
            } | get_info_json());
        return false;
    }

    if (!m_unit_handle->connect()) {
        LogError << "MaaAndroidNativeControlUnit failed to connect";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaAndroidNativeControlUnit failed to connect" },
            } | get_info_json());
        return false;
    }

    if (!m_unit_handle->request_uuid(m_uuid)) {
        LogWarn << "Failed to get UUID from MaaAndroidNativeControlUnit";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaAndroidNativeControlUnit failed to get UUID" },
            } | get_info_json());
        return false;
    }

    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "what", "UuidGot" },
            { "why", "" },
            { "details",
              json::object {
                  { "uuid", m_uuid },
              } },
        } | get_info_json());

    // 尝试截图以获取屏幕分辨率
    cv::Mat image;
    if (!m_unit_handle->screencap(image) || image.cols == 0 || image.rows == 0) {
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ResolutionError" },
                { "why", "Get resolution failed" },
            } | get_info_json());
        return false;
    }
    m_screen_size = { image.cols, image.rows };
    LogInfo << "Connected to AndroidNative. Screen size:" << m_screen_size.first << "x" << m_screen_size.second;
    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "what", "ResolutionGot" },
            { "why", "" },
            { "width", m_screen_size.first },
            { "height", m_screen_size.second },
        } | get_info_json());

    m_inited = true;
    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "what", "Connected" },
            { "why", "" },
        } | get_info_json());
    return true;
}

bool MaaFwAndroidNativeController::inited() const noexcept
{
    return m_inited && m_unit_handle && m_unit_handle->connected();
}

const std::string& MaaFwAndroidNativeController::get_uuid() const
{
    return m_uuid;
}

bool MaaFwAndroidNativeController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    if (!m_unit_handle->screencap(image_payload)) {
        LogWarn << "MaaAndroidNativeControlUnit screencap failed";
        return false;
    }

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    return true;
}

bool MaaFwAndroidNativeController::start_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->start_app(*package_name);
}

bool MaaFwAndroidNativeController::stop_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->stop_app(*package_name);
}

bool MaaFwAndroidNativeController::click(const Point& p)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }
    return m_unit_handle->click(p.x, p.y);
}

bool MaaFwAndroidNativeController::input(const std::string& text)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }
    return m_unit_handle->input_text(text);
}

bool MaaFwAndroidNativeController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe [[maybe_unused]],
    double slope_in [[maybe_unused]],
    double slope_out [[maybe_unused]],
    bool with_pause [[maybe_unused]])
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    return m_unit_handle->swipe(p1.x, p1.y, p2.x, p2.y, duration);
}

bool MaaFwAndroidNativeController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return m_unit_handle->touch_down(event.pointerId, event.point.x, event.point.y, 0);
    case InputEvent::Type::TOUCH_MOVE:
        return m_unit_handle->touch_move(event.pointerId, event.point.x, event.point.y, 0);
    case InputEvent::Type::TOUCH_UP:
        return m_unit_handle->touch_up(event.pointerId);
    case InputEvent::Type::TOUCH_RESET:
        return true;
    case InputEvent::Type::KEY_DOWN:
        return m_unit_handle->key_down(event.keycode);
    case InputEvent::Type::KEY_UP:
        return m_unit_handle->key_up(event.keycode);
    case InputEvent::Type::WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    case InputEvent::Type::COMMIT:
        return true;
    default:
        LogError << "unknown input event type" << VAR(static_cast<int>(event.type));
        return false;
    }
}

bool MaaFwAndroidNativeController::press_esc()
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    return m_unit_handle->click_key(111); // KEYCODE_ESCAPE
}

ControlFeat::Feat MaaFwAndroidNativeController::support_features() const noexcept
{
    return ControlFeat::PRECISE_SWIPE;
}

std::pair<int, int> MaaFwAndroidNativeController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void MaaFwAndroidNativeController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

void* MaaFwAndroidNativeController::attach_thread()
{
    if (!m_unit_handle || !m_attach_thread_func) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized or attach_thread not available";
        return nullptr;
    }

    return m_attach_thread_func(m_unit_handle);
}

int MaaFwAndroidNativeController::detach_thread(void* env)
{
    if (!m_unit_handle || !m_detach_thread_func) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized or detach_thread not available";
        return -1;
    }

    return m_detach_thread_func(m_unit_handle, env);
}

} // namespace asst

#endif // __ANDROID__
