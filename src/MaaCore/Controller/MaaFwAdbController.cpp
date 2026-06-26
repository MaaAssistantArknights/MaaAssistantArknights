#include "MaaFwAdbController.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <thread>

#include "Common/AsstMsg.h"
#include "Config/GeneralConfig.h"
#include "Controller/MaaFwControlUnitInterface.h"
#include "Controller/SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{
namespace
{
struct MaaFwAdbConfig
{
    std::string json = "{}";
    uint64_t screencap_methods = MaaAdbScreencapMethod::Default;
    uint64_t input_methods = MaaAdbInputMethod::AdbShell | MaaAdbInputMethod::EmulatorExtras;
    std::string screencap_method_name;
};

int get_mumu_index(const std::string& address)
{
    auto pos = address.find(":");
    if (pos == std::string::npos) {
        LogError << "address is invalid" << VAR(address);
        return 0;
    }

    std::string port_str = address.substr(pos + 1);
    if (port_str.empty() ||
        !std::ranges::all_of(port_str, [](unsigned char c) -> bool { return std::isdigit(c); })) {
        LogError << "port is invalid" << VAR(port_str);
        return 0;
    }

    int port = std::stoi(port_str);
    int mumu_index = 0;
    if (port >= 16384) {
        mumu_index = (port - 16384) / 32;
    }
    else if (port == 7555) {
        LogWarn << "Port 7555 is deprecated for MuMu6, please use 16384 or above.";
    }
    else if (port >= 5555) {
        mumu_index = (port - 5555) / 2;
    }
    LogInfo << VAR(port_str) << VAR(port) << VAR(mumu_index);
    return mumu_index;
}

int get_ld_index(const std::string& address)
{
    constexpr std::string_view EmulatorPrefix = "emulator-";
    constexpr int BaseEmulatorPort = 5554;
    if (address.starts_with(EmulatorPrefix)) {
        std::string port_str = address.substr(EmulatorPrefix.size());
        if (port_str.empty() ||
            !std::ranges::all_of(port_str, [](unsigned char c) -> bool { return std::isdigit(c); })) {
            LogError << "emulator port is invalid" << VAR(port_str);
            return 0;
        }

        int port = std::stoi(port_str);
        int ld_index = (port - BaseEmulatorPort) / 2;
        LogInfo << VAR(port_str) << VAR(port) << VAR(ld_index);
        return ld_index;
    }

    constexpr std::string_view LocalhostPrefix = "127.0.0.1:";
    constexpr int BaseAdbPort = 5555;
    if (address.starts_with(LocalhostPrefix)) {
        std::string port_str = address.substr(LocalhostPrefix.size());
        if (port_str.empty() ||
            !std::ranges::all_of(port_str, [](unsigned char c) -> bool { return std::isdigit(c); })) {
            LogError << "adb port is invalid" << VAR(port_str);
            return 0;
        }

        int port = std::stoi(port_str);
        int ld_index = (port - BaseAdbPort) / 2;
        LogInfo << VAR(port_str) << VAR(port) << VAR(ld_index);
        return ld_index;
    }

    LogError << "address is invalid or unsupported" << VAR(address);
    return 0;
}

MaaFwAdbConfig make_maa_fw_adb_config(const std::string& config, const std::string& address)
{
    if (config == "AVD") {
        return { R"({"extras":{"avd":{"enable":true}}})" };
    }

    auto adb_cfg = Config.get_adb_cfg(config);
    if (!adb_cfg || adb_cfg->extras.empty()) {
        return {};
    }

    const auto& extras = adb_cfg->extras;
    if (config == "MuMuEmulator12") {
        std::string mumu_path = extras.get("path", "");
        if (mumu_path.empty()) {
            return {};
        }

        json::object mumu_config {
            { "enable", true },
            { "path", std::move(mumu_path) },
            { "index", extras.contains("index") ? extras.get("index", 0) : get_mumu_index(address) },
        };

        json::value fw_config = json::object {
            { "extras",
              json::object {
                  { "mumu", std::move(mumu_config) },
              } },
        };

        return {
            fw_config.to_string(),
            MaaAdbScreencapMethod::EmulatorExtras,
            MaaAdbInputMethod::AdbShell | MaaAdbInputMethod::EmulatorExtras,
            "MumuExtras",
        };
    }

    if (config == "LDPlayer") {
        std::string ld_path = extras.get("path", "");
        if (ld_path.empty()) {
            return {};
        }

        json::object ld_config {
            { "enable", true },
            { "path", std::move(ld_path) },
            { "index", extras.contains("index") ? extras.get("index", 0) : get_ld_index(address) },
            { "pid", extras.get("pid", 0) },
        };

        json::value fw_config = json::object {
            { "extras",
              json::object {
                  { "ld", std::move(ld_config) },
              } },
        };

        return {
            fw_config.to_string(),
            MaaAdbScreencapMethod::EmulatorExtras,
            MaaAdbInputMethod::AdbShell,
            "LDExtras",
        };
    }

    return {};
}
} // namespace

MaaFwAdbController::MaaFwAdbController(
    const AsstCallback& callback,
    Assistant* inst,
    PlatformType type [[maybe_unused]]) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

MaaFwAdbController::~MaaFwAdbController()
{
    LogTraceFunction;

    if (m_unit_handle && m_destroy_func) {
        LogInfo << "Cleaning up MaaAdbControlUnit";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
    }
}

bool MaaFwAdbController::init_library()
{
    if (m_get_version_func && m_create_func && m_destroy_func) {
        LogInfo << "MaaAdbControlUnit library already loaded";
        return true;
    }
    if (!load_library("MaaAdbControlUnit")) {
        LogError << "Failed to load MaaAdbControlUnit library";
        return false;
    }

    m_get_version_func = get_function<GetVersionFunc>("MaaAdbControlUnitGetVersion");
    m_create_func = get_function<CreateFunc>("MaaAdbControlUnitCreate");
    m_destroy_func = get_function<DestroyFunc>("MaaAdbControlUnitDestroy");

    if (!m_get_version_func || !m_create_func || !m_destroy_func) {
        LogError << "Failed to get function pointers from MaaAdbControlUnit library";
        return false;
    }

    LogInfo << "MaaAdbControlUnit library version:" << m_get_version_func();

    return true;
}

bool MaaFwAdbController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
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
                  { "adb", adb_path },
                  { "address", address },
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

    auto maa_fw_config = make_maa_fw_adb_config(config, address);

    m_unit_handle = m_create_func(
        adb_path.c_str(),
        address.c_str(),
        maa_fw_config.screencap_methods,
        maa_fw_config.input_methods,
        maa_fw_config.json.c_str(),
        // MaaAgentBinary目录
        utils::path_to_utf8_string(ResDir.get()).c_str());

    if (!m_unit_handle) {
        LogError << "Failed to create MaaAdbControlUnit";
        return false;
    }

    if (!m_unit_handle->connect()) {
        LogError << "MaaAdbControlUnit failed to connect";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaAdbControlUnit failed to connect" },
            } | get_info_json());
        return false;
    }

    if (!m_unit_handle->request_uuid(m_uuid)) {
        LogWarn << "Failed to get UUID from MaaFwAdbControlUnit";
        m_destroy_func(m_unit_handle);
        m_unit_handle = nullptr;
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "what", "ConnectFailed" },
                { "why", "MaaFwAdbControlUnit failed to get UUID" },
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

    // 尝试进行一次截图以获取屏幕分辨率
    cv::Mat image;
    auto screencap_start = std::chrono::steady_clock::now();
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
    auto screencap_cost = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - screencap_start)
                              .count();
    m_screen_size = { image.cols, image.rows };
    LogInfo << "Connected to ADB. Screen size:" << m_screen_size.first << "x" << m_screen_size.second;
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

    if (!maa_fw_config.screencap_method_name.empty()) {
        callback(
            AsstMsg::ConnectionInfo,
            json::object {
                { "uuid", m_uuid },
                { "what", "FastestWayToScreencap" },
                { "details",
                  json::object {
                      { "method", maa_fw_config.screencap_method_name },
                      { "cost", screencap_cost },
                      { "alternatives",
                        json::array {
                            json::object {
                                { "method", maa_fw_config.screencap_method_name },
                                { "cost", std::to_string(screencap_cost) },
                            },
                        } },
                  } },
            });
    }
    return true;
}

bool MaaFwAdbController::inited() const noexcept
{
    return m_inited && m_unit_handle && m_unit_handle->connected();
}

const std::string& MaaFwAdbController::get_uuid() const
{
    return m_uuid;
}

bool MaaFwAdbController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    if (!m_unit_handle->screencap(image_payload)) {
        LogWarn << "MaaAdbControlUnit screencap failed";
        return false;
    }

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    return true;
}

bool MaaFwAdbController::start_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->start_app(*package_name);
}

bool MaaFwAdbController::stop_game(const std::string& client_type)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogWarn << "Invalid client_type" << VAR(client_type);
        return false;
    }
    return m_unit_handle->stop_app(*package_name);
}

bool MaaFwAdbController::click(const Point& p)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    if (use_touch_down_up()) {
        if (!m_unit_handle->touch_down(0, p.x, p.y, 1)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return m_unit_handle->touch_up(0);
    }

    return m_unit_handle->click(p.x, p.y);
}

bool MaaFwAdbController::input(const std::string& text)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }
    return m_unit_handle->input_text(text);
}

bool MaaFwAdbController::swipe(
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
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    if (use_touch_down_up()) {
        int x1 = p1.x, y1 = p1.y;
        int x2 = p2.x, y2 = p2.y;

        const auto width = m_screen_size.first;
        const auto height = m_screen_size.second;
        if (width > 0 && height > 0 && (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height)) {
            LogWarn << "swipe point1 is out of range" << x1 << y1;
            x1 = std::clamp(x1, 0, width - 1);
            y1 = std::clamp(y1, 0, height - 1);
        }

        if (!m_unit_handle->touch_down(0, x1, y1, 1)) {
            LogError << "touch_down failed at swipe start point";
            return false;
        }

        constexpr int TimeInterval = 5;
        bool need_pause = with_pause;
        const auto& opt = Config.get_options();

        auto bounds_check = [width, height](int x, int y) {
            if (width <= 0 || height <= 0) {
                return true;
            }
            return x >= 0 && x < width && y >= 0 && y < height;
        };

        auto move_func = [this](int x, int y) -> bool {
            if (!m_unit_handle->touch_move(0, x, y, 1)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(TimeInterval));
            return true;
        };

        auto do_swipe = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
            if (need_pause) {
                auto pause_check = [&opt](int cur_x, int cur_y, int start_x, int start_y) {
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

        if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
            if (!do_swipe(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration)) {
                LogWarn << "Failed during extra swipe movement";
            }
        }

        return m_unit_handle->touch_up(0);
    }

    return m_unit_handle->swipe(p1.x, p1.y, p2.x, p2.y, duration);
}

bool MaaFwAdbController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
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

bool MaaFwAdbController::press_esc()
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    if (use_key_down_up()) {
        constexpr int EscKeyCode = 111;
        if (!m_unit_handle->key_down(EscKeyCode)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return m_unit_handle->key_up(EscKeyCode);
    }

    return m_unit_handle->click_key(111); // KEYCODE_ESCAPE
}

ControlFeat::Feat MaaFwAdbController::support_features() const noexcept
{
    auto feat = ControlFeat::PRECISE_SWIPE;
    if (use_touch_down_up()) {
        feat |= ControlFeat::SWIPE_WITH_PAUSE;
    }
    return feat;
}

std::pair<int, int> MaaFwAdbController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void MaaFwAdbController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

uint64_t MaaFwAdbController::get_maa_fw_features() const noexcept
{
    if (!m_unit_handle) {
        return MaaFeature::None;
    }
    return m_unit_handle->get_features();
}

bool MaaFwAdbController::use_touch_down_up() const noexcept
{
    return get_maa_fw_features() & MaaFeature::UseMouseDownAndUpInsteadOfClick;
}

bool MaaFwAdbController::use_key_down_up() const noexcept
{
    return get_maa_fw_features() & MaaFeature::UseKeyboardDownAndUpInsteadOfClick;
}

} // namespace asst
