#include "MaaFwAdbController.h"

#include <numeric>
#include <thread>

#include "Common/AsstMsg.h"
#include "Config/GeneralConfig.h"
#include "Controller/MaaFwControlUnitInterface.h"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{

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

    m_unit_handle = m_create_func(
        adb_path.c_str(),
        address.c_str(),
        MaaAdbScreencapMethod::Default,
        MaaAdbInputMethod::AdbShell | MaaAdbInputMethod::EmulatorExtras,
        config == "AVD" ? "{\"extras\":{\"avd\":{\"enable\":true}}}" : "{}",
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
    using namespace std::chrono;
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
    }

    if (!m_tested_screencap) {
        image_payload = cv::Mat(); // 清空缓存
        Log.info("Test MaaAdbControlUnit screencap cost");
        auto min_cost = milliseconds(LLONG_MAX);
        auto start_time = steady_clock::now();

        if (!m_unit_handle->screencap(image_payload)) {
            LogWarn << "MaaAdbControlUnit screencap failed";
            return false;
        }

        m_tested_screencap = true;
        auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
        if (duration < min_cost) {
            min_cost = duration;
        }
        Log.info("MaaAdbControlUnit screencap cost", duration.count(), "ms");
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "FastestWayToScreencap" },
            { "details",
              json::object {
                  { "method", "MaaAdbControlUnit" },
                  { "cost", min_cost.count() },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }
    else {
        auto start_time = steady_clock::now();
        bool screencap_ret = (m_unit_handle->screencap(image_payload));
        auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
        m_screencap_cost.emplace_back(screencap_ret ? duration.count() : -1); // 记录截图耗时
        ++m_screencap_times;

        if (m_screencap_cost.size() > 30) {
            m_screencap_cost.pop_front();
        }
        if (m_screencap_times > 9) { // 每 10 次截图计算一次平均耗时
            m_screencap_times = 0;
            auto filtered_cost = m_screencap_cost | std::views::filter([](auto num) { return num > 0; });
            if (filtered_cost.empty()) {
                return screencap_ret;
            }
            // 过滤后的有效截图用时次数
            auto filtered_count = m_screencap_cost.size() - std::ranges::count(m_screencap_cost, -1);
            auto [screencap_cost_min, screencap_cost_max] = std::ranges::minmax(filtered_cost);
            json::value info = json::object {
                { "uuid", m_uuid },
                { "what", "ScreencapCost" },
                { "details",
                  json::object {
                      { "min", screencap_cost_min },
                      { "max", screencap_cost_max },
                      { "avg",
                        filtered_count > 0
                            ? std::accumulate(filtered_cost.begin(), filtered_cost.end(), 0ll) / filtered_count
                            : -1 },
                  } },
            };
            if (m_screencap_cost.size() > filtered_count) {
                info["details"]["fault_times"] = m_screencap_cost.size() - filtered_count;
            }
            callback(AsstMsg::ConnectionInfo, info);
        }
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
    bool extra_swipe [[maybe_unused]],
    double slope_in [[maybe_unused]],
    double slope_out [[maybe_unused]],
    bool with_pause [[maybe_unused]])
{
    LogTraceFunction;
    if (!m_unit_handle) {
        LogWarn << "MaaAdbControlUnit is not initialized";
        return false;
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

    return m_unit_handle->click_key(111); // KEYCODE_ESCAPE
}

ControlFeat::Feat MaaFwAdbController::support_features() const noexcept
{
    return ControlFeat::PRECISE_SWIPE;
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

} // namespace asst
