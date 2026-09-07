#include "MaaFwController.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <thread>

#include "Config/GeneralConfig.h"
#include "Controller/MaaFwControlUnitInterface.h"
#include "Controller/SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{
MaaFwController::MaaFwController(const AsstCallback& callback, Assistant* inst, PlatformType type [[maybe_unused]]) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

MaaFwController::~MaaFwController()
{
    LogTraceFunction;
    destroy_unit();
}

bool MaaFwController::connect(
    const std::string& adb_path,
    const std::string& address,
    const std::string& config)
{
    LogTraceFunction;

    auto adb_config = Config.get_adb_cfg("MaaFw");
    if (!adb_config) {
        LogError << "connection extras for MaaFw is required but not set";
        return false;
    }

    return connect_with_extras(adb_path, address, config, adb_config->extras);
}

bool MaaFwController::connect_with_extras(
    const std::string& adb_path,
    const std::string& address,
    const std::string& config,
    const json::object& extras)
{
    LogTraceFunction;

    m_inited = false;
    m_uuid.clear();
    m_screen_size = { 0, 0 };

    const std::string library_name = extras.get("library_name", std::string());

    if (library_name == "MaaLinuxControlUnit") {
        m_esc_keycode = 1; // KEY_ESC defined in <linux/input-event-codes.h>
    }
    else {
        m_esc_keycode = 111; // KEYCODE_ESCAPE for Android
    }

    m_target_is_pc = config == "PC";

    auto get_info_json = [&]() -> json::object {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
                  { "extras", extras },
              } },
        };
    };

    if (library_name.empty()) {
        LogError << "MaaFw connection extras missing required field: library_name";
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "library_name missing in connection extras" },
            } | get_info_json());
        return false;
    }

    destroy_unit();

    if (!init_library(library_name)) {
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "Failed to load MaaFw control unit library" },
            } | get_info_json());
        return false;
    }

    // 创建control unit
    if (m_library_name == "MaaAdbControlUnit") {
        auto create_func =
            get_function<MaaFwControlUnitAPI*(const char*, const char*, uint64_t, uint64_t, const char*, const char*)>(
                m_library_name + "Create");
        if (!create_func) {
            LogError << "MaaAdbControlUnitCreate function not found?!";
            return false;
        }
        m_unit_handle = create_func(
            adb_path.c_str(),
            address.c_str(),
            extras.get("screencap_methods", MaaAdbScreencapMethod::Default),
            // 不要加 EmulatorExtras：MaaFramework 的 MuMuPlayerExtras::click/swipe 是空实现，
            // 它靠 get_features() 返回 UseMouseDownAndUpInsteadOfClick 让上层改走 touch_down/up，
            // 而这里的 click()/swipe() 直接透传给 ControlUnit，从不读 features，会全部失败。
            // MuMu 触控走 MumuController（TouchMode::MumuExtras）。
            extras.get("input_methods", MaaAdbInputMethod::AdbShell),
            config == "AVD" ? "{\"extras\":{\"avd\":{\"enable\":true}}}" : "{}",
            // MaaAgentBinary目录
            utils::path_to_utf8_string(ResDir.get() / "minitouch").c_str());
    }
    else {
        // 创建 MaaAndroidNativeControlUnit/MaaLinuxControlUnit 时使用单个字符串参数
        auto create_func = get_function<MaaFwControlUnitAPI*(const char*)>(m_library_name + "Create");
        if (!create_func) {
            LogError << m_library_name << "control unit create function not found?!";
            return false;
        }
        m_unit_handle = create_func(extras.dumps().c_str());
    }

    if (!m_unit_handle) {
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "Failed to create MaaFw control unit" },
            } | get_info_json());
        return false;
    }

    if (!m_unit_handle->connect()) {
        LogError << "MaaFw control unit failed to connect" << VAR(m_library_name);
        destroy_unit();
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaFw control unit failed to connect" },
            } | get_info_json());
        return false;
    }

    if (!m_unit_handle->request_uuid(m_uuid)) {
        LogError << "Failed to get UUID from MaaFw control unit" << VAR(m_library_name);
        destroy_unit();
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaFw control unit failed to get UUID" },
            } | get_info_json());
        return false;
    }

    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "what", "UuidGot" },
            { "why", "" },
        } | get_info_json());

    if (auto screen_resolution = extras.find<json::object>("screen_resolution")) {
        const int width = screen_resolution->get("width", 0);
        const int height = screen_resolution->get("height", 0);
        m_screen_size = { width, height };
        LogInfo << "Using screen resolution set in connection extras:" << width << "x" << height;
    }

    if (m_screen_size.first <= 0 || m_screen_size.second <= 0) {
        // 尝试进行一次截图以获取屏幕分辨率
        LogInfo << "Trying to determine screen size by screencap";
        cv::Mat image;
        if (!m_unit_handle->screencap(image) || image.empty()) {
            LogError << "Failed to get screen size";
            destroy_unit();
            callback(
                AsstMsg::ConnectionInfo,
                json::object {
                    { "what", "ResolutionError" },
                    { "why", "Get resolution failed" },
                } | get_info_json());
            return false;
        }
        m_screen_size = { image.cols, image.rows };
    }

    LogInfo << "MaaFw control unit screen size:" << m_screen_size.first << "x" << m_screen_size.second;
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

bool MaaFwController::init_library(const std::string& library_name)
{
    if (m_library_name == library_name && m_get_version_func && m_destroy_func) {
        LogInfo << "MaaFw control unit library already loaded" << VAR(library_name);
        return true;
    }

    if (!m_library_name.empty() && m_library_name != library_name) {
        LogError << "Cannot change MaaFw control unit library on the same controller object" << VAR(m_library_name)
                 << VAR(library_name);
        return false;
    }

    if (!load_library(library_name)) {
        LogError << "Failed to load MaaFw control unit library" << VAR(library_name);
        return false;
    }

    m_library_name = library_name;
    m_get_version_func = get_function<GetVersionFunc>(library_name + "GetVersion");
    m_destroy_func = get_function<DestroyFunc>(library_name + "Destroy");

    if (!m_get_version_func || !m_destroy_func) {
        LogError << "Failed to get function pointers from MaaFw control unit library" << VAR(library_name);
        return false;
    }

    LogInfo << "MaaFw control unit library version:" << m_get_version_func();
    return true;
}

void MaaFwController::destroy_unit()
{
    m_inited = false;
    if (!m_unit_handle) {
        return;
    }

    if (m_destroy_func) {
        LogInfo << "Cleaning up MaaFw control unit" << VAR(m_library_name);
        m_destroy_func(m_unit_handle);
    }
    m_unit_handle = nullptr;
}

bool MaaFwController::inited() const noexcept
{
    return m_inited && m_unit_handle && m_unit_handle->connected();
}

const std::string& MaaFwController::get_uuid() const
{
    return m_uuid;
}

bool MaaFwController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    // PC 端截图前把鼠标移走，避免光标出现在截图中影响识别
    if (m_target_is_pc && m_screen_size.second > 0) {
        if (m_main_screen_recognition) {
            // 主界面情况下鼠标移动到窗口中心，等待主界面的视差动画，300ms
            inject_input_event(
                InputEvent {
                    .type = InputEvent::Type::TOUCH_MOVE,
                    .point = { m_screen_size.first / 2, m_screen_size.second / 2 },
                });
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        else {
            inject_input_event(
                InputEvent {
                    .type = InputEvent::Type::TOUCH_MOVE,
                    .point = { 0, m_screen_size.second - 1 },
                });
            // 游戏自绘光标跟随真实光标，渲染存在帧延迟，等待其画到挪动终点后再截图，避免光标被截进识别区
            std::this_thread::sleep_for(std::chrono::milliseconds(34));
        }
    }

    if (!m_unit_handle->screencap(image_payload) || image_payload.empty()) {
        LogWarn << "MaaFw control unit screencap failed" << VAR(m_library_name);
        return false;
    }

    m_screen_size = { image_payload.cols, image_payload.rows };
    return true;
}

bool MaaFwController::start_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->start_app(*package_name);
}

bool MaaFwController::stop_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->stop_app(*package_name);
}

bool MaaFwController::click(const Point& p)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    if (!(m_unit_handle->get_features() & MaaFeature::UseMouseDownAndUpInsteadOfClick)) {
        return m_unit_handle->click(p.x, p.y);
    }

    if (!m_unit_handle->touch_down(0, p.x, p.y, 1)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(ClickDelay));
    const bool ret = m_unit_handle->touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(ClickDelay));
    return ret;
}

bool MaaFwController::input(const std::string& text)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }
    return m_unit_handle->input_text(text);
}

bool MaaFwController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    if (!(m_unit_handle->get_features() & MaaFeature::UseMouseDownAndUpInsteadOfClick)) {
        return m_unit_handle->swipe(p1.x, p1.y, p2.x, p2.y, duration);
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_screen_size.first || y1 < 0 || y1 >= m_screen_size.second) {
        LogWarn << "swipe point1 is out of range" << x1 << y1;
        x1 = std::clamp(x1, 0, m_screen_size.first - 1);
        y1 = std::clamp(y1, 0, m_screen_size.second - 1);
    }

    // 触摸按下起点
    if (!m_unit_handle->touch_down(0, x1, y1, 1)) {
        LogError << "touch_down failed at swipe start point";
        return false;
    }

    bool need_pause = with_pause;
    const auto& opt = Config.get_options();
    auto bounds_check = [this](int x, int y) {
        return x >= 0 && x < m_screen_size.first && y >= 0 && y < m_screen_size.second;
    };
    auto move_func = [&](int x, int y) -> bool {
        if (!m_unit_handle->touch_move(0, x, y, 1)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SwipeDelay));
        return true;
    };
    auto do_swipe = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
        if (need_pause) {
            auto pause_check = [&opt](int cur_x, int cur_y, int start_x, int start_y) {
                return std::hypot(cur_x - start_x, cur_y - start_y) > opt.swipe_with_pause_required_distance;
            };

            return interpolate_swipe_with_pause(
                _x1,
                _y1,
                _x2,
                _y2,
                _duration,
                SwipeDelay,
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
            SwipeDelay,
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

    const bool result = m_unit_handle->touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(ClickDelay));
    return result;
}

bool MaaFwController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return m_unit_handle->touch_down(event.pointerId, event.point.x, event.point.y, 1);
    case InputEvent::Type::TOUCH_MOVE:
        return m_unit_handle->touch_move(event.pointerId, event.point.x, event.point.y, 1);
    case InputEvent::Type::TOUCH_UP:
        return m_unit_handle->touch_up(event.pointerId);
    case InputEvent::Type::KEY_DOWN:
        return m_unit_handle->key_down(event.keycode);
    case InputEvent::Type::KEY_UP:
        return m_unit_handle->key_up(event.keycode);
    case InputEvent::Type::WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    case InputEvent::Type::TOUCH_RESET:
    case InputEvent::Type::COMMIT:
        return true;
    case InputEvent::Type::UNKNOWN:
    default:
        LogError << "unknown input event type" << VAR(static_cast<int>(event.type));
        return false;
    }
}

bool MaaFwController::press_esc()
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaFw control unit is not initialized";
        return false;
    }

    if (!(m_unit_handle->get_features() & MaaFeature::UseKeyboardDownAndUpInsteadOfClick)) {
        return m_unit_handle->click_key(m_esc_keycode);
    }

    if (!m_unit_handle->key_down(m_esc_keycode)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return m_unit_handle->key_up(m_esc_keycode);
}

ControlFeat::Feat MaaFwController::support_features() const noexcept
{
    // MaaFwController 支持精确滑动和暂停滑动功能
    return ControlFeat::PRECISE_SWIPE | ControlFeat::SWIPE_WITH_PAUSE;
}

std::pair<int, int> MaaFwController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void MaaFwController::callback(AsstMsg msg, const json::value& details) const
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}
} // namespace asst
