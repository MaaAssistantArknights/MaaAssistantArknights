#ifdef __ANDROID__

#include "MaaFwAndroidNativeController.h"

#include <cmath>
#include <thread>

#include "Common/AsstMsg.h"
#include "Config/GeneralConfig.h"
#include "Controller/MaaFwControlUnitInterface.h"
#include "Controller/SwipeHelper.hpp"
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

bool MaaFwAndroidNativeController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config)
{
    LogTraceFunction;

    m_inited = false;
    m_uuid.clear();
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

    if (!config.empty()) {
        if (auto config_opt = json::parse(config); config_opt.has_value()) {
            auto& config_json = config_opt.value();
            if (config_json.contains("screen_resolution")) {
                if (const auto& res = config_json["screen_resolution"];
                    res.contains("width") && res.contains("height")) {
                    int width = res.get("width", 1280);
                    int height = res.get("height", 720);
                    m_screen_resolution = { width, height };
                    LogInfo << "Parsed screen resolution from config:" << width << "x" << height;
                }
            }
        }
        else {
            LogError << "Failed to parse config as JSON";
            return false;
        }
    }

    if (m_screen_resolution.first <= 0 || m_screen_resolution.second <= 0) {
        LogError << "screen_resolution not provided or invalid in config, cannot connect";
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "screen_resolution missing in config" },
            } | get_info_json());
        return false;
    }

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

    m_inited = true;
    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "what", "Connected" },
            { "why", "NativeAndroid" },
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

    if (!m_unit_handle->touch_down(0, p.x, p.y, 1)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return m_unit_handle->touch_up(0);
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
    const int duration,
    const bool extra_swipe,
    const double slope_in,
    const double slope_out,
    const bool with_pause)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAndroidNativeControlUnit is not initialized";
        return false;
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_screen_resolution.first || y1 < 0 || y1 >= m_screen_resolution.second) {
        LogWarn << "swipe point1 is out of range" << x1 << y1;
        x1 = std::clamp(x1, 0, m_screen_resolution.first - 1);
        y1 = std::clamp(y1, 0, m_screen_resolution.second - 1);
    }

    // 触摸按下起点
    if (!m_unit_handle->touch_down(0, x1, y1, 1)) {
        LogError << "touch_down failed at swipe start point";
        return false;
    }

    constexpr int TimeInterval = 5; // 类似 Minitoucher::DefaultSwipeDelay

    bool need_pause = with_pause;
    const auto& opt = Config.get_options();

    auto bounds_check = [this](int x, int y) {
        return x >= 0 && x < m_screen_resolution.first && y >= 0 && y < m_screen_resolution.second;
    };

    auto move_func = [&](int x, int y) -> bool {
        if (!m_unit_handle->touch_move(0, x, y, 1)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TimeInterval));
        return true;
    };

    auto do_swipe = [&](const int _x1, const int _y1, const int _x2, const int _y2, const int _duration) -> bool {
        if (need_pause) {
            auto pause_check = [&opt](const int cur_x, const int cur_y, const int start_x, const int start_y) {
                return std::sqrt(std::pow(cur_x - start_x, 2) + std::pow(cur_y - start_y, 2)) >
                       opt.swipe_with_pause_required_distance;
            };

            return interpolate_swipe_with_pause(
                _x1,
                _y1,
                _x2,
                _y2,
                _duration,
                TimeInterval,
                slope_in,
                slope_out,
                move_func,
                bounds_check,
                pause_check,
                [&]() {
                    need_pause = false;
                    press_esc();
                });
        }
        return interpolate_swipe(
            _x1,
            _y1,
            _x2,
            _y2,
            _duration,
            TimeInterval,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    if (!do_swipe(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration)) {
        LogError << "Failed during main swipe movement";
        m_unit_handle->touch_up(0);
        return false;
    }

    // 额外滑动逻辑
    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));

        if (!do_swipe(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration)) {
            LogWarn << "Failed during extra swipe movement";
        }
    }

    m_unit_handle->touch_up(0);
    return true;
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

    constexpr int KEYCODE_ESCAPE = 111;
    if (!m_unit_handle->key_down(KEYCODE_ESCAPE)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return m_unit_handle->key_up(KEYCODE_ESCAPE);
}

ControlFeat::Feat MaaFwAndroidNativeController::support_features() const noexcept
{
    // MaaFwAndroidNativeController 支持精确滑动和暂停滑动功能
    auto feat = ControlFeat::PRECISE_SWIPE;
    feat |= ControlFeat::SWIPE_WITH_PAUSE;
    return feat;
}

std::pair<int, int> MaaFwAndroidNativeController::get_screen_res() const noexcept
{
    return m_screen_resolution;
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

    if (!m_get_version_func || !m_create_func || !m_destroy_func) {
        LogError << "Failed to get function pointers from MaaAndroidNativeControlUnit library";
        return false;
    }

    LogInfo << "MaaAndroidNativeControlUnit library version:" << m_get_version_func();

    return true;
}

void MaaFwAndroidNativeController::callback(const AsstMsg msg, const json::value& details) const
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

} // namespace asst

#endif // __ANDROID__
