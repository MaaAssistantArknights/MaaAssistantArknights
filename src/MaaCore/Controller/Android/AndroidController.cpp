#ifdef __ANDROID__

#include "AndroidController.h"

#include <chrono>
#include <thread>

#include "AndroidExternalLib.h"
#include "Common/AsstTypes.h"
#include "Config/GeneralConfig.h"
#include "Controller/SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "opencv2/imgproc.hpp"

using namespace asst;

AndroidController::AndroidController(const AsstCallback& callback, Assistant* inst) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

AndroidController::~AndroidController()
{
    LogTraceFunction;
    std::cout << "~AndroidController" << std::endl;
}

bool AndroidController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;
    LogInfo << "AndroidController connecting, adb_path:" << adb_path << ", address:" << address
            << ", config:" << config;

    if (const auto& lib = AndroidExternalLib::instance(); !lib.is_loaded()) {
        LogInfo << "AndroidExternalLib not loaded";
        return false;
    }

    if (!config.empty()) {
        auto config_opt = json::parse(config);
        if (config_opt.has_value()) {
            auto& config_json = config_opt.value();
            if (config_json.contains("screen_resolution")) {
                auto& res = config_json["screen_resolution"];
                if (res.contains("width") && res.contains("height")) {
                    int width = res.get("width", 1920);
                    int height = res.get("height", 1080);
                    m_screen_resolution = { width, height };
                    LogInfo << "Parsed screen resolution from config:" << width << "x" << height;
                }
            }
            if (config_json.contains("display_id")) {
                m_display_id = config_json.get("display_id", 0);
                LogInfo << "Parsed display_id from config:" << m_display_id;
            }
            if (config_json.contains("force_stop")) {
                m_force_stop = config_json.get("force_stop", false);
                LogInfo << "Parsed force_stop from config:" << m_force_stop;
            }
        }
        else {
            LogWarn << "Failed to parse config as JSON, using default resolution";
            return false;
        }
    }

    m_uuid = "IAndroidController";
    m_inited = true;

    callback(
        AsstMsg::ConnectionInfo,
        json::object { { "uuid", m_uuid },
                       { "details",
                         json::object {
                             { "adb", adb_path },
                             { "address", address },
                             { "config", config },
                         } } });

    LogInfo << "AndroidController connected successfully, uuid:" << m_uuid;
    return true;
}

bool AndroidController::inited() const noexcept
{
    return m_inited;
}

const std::string& AndroidController::get_uuid() const
{
    return m_uuid;
}

size_t AndroidController::get_pipe_data_size() const noexcept
{
    return 0;
}

size_t AndroidController::get_version() const noexcept
{
    return 1;
}

bool AndroidController::screencap(cv::Mat& image, [[maybe_unused]] bool allow_reconnect)
{
    LogTraceFunction;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    const auto& library = AndroidExternalLib::instance();
    const auto info = library.GetLockedPixels();

    if (!info.data || info.width == 0 || info.height == 0) {
        LogError << "GetLockedPixels returned invalid data";
        return false;
    }

    const cv::Mat src(info.height, info.width, CV_8UC4, info.data, info.stride);
    cv::Mat bgr;

    const auto cvt_start = std::chrono::high_resolution_clock::now();
    cv::cvtColor(src, bgr, cv::COLOR_RGBA2BGR);
    auto cvt_end = std::chrono::high_resolution_clock::now();
    auto cvt_duration = std::chrono::duration_cast<std::chrono::microseconds>(cvt_end - cvt_start);
    LogDebug << "cvtColor(RGBA->BGR) took: " << cvt_duration.count() << " microseconds";

    image = bgr;

    int unlock_ret = library.UnlockPixels(info);
    if (unlock_ret == 0) {
        LogInfo << "UnlockPixels Successful " << unlock_ret;
        return true;
    }

    return false;
}

bool AndroidController::start_game(const std::string& client_type)
{
    LogTraceFunction;
    LogInfo << "Starting game with client_type:" << client_type;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        LogError << "Unknown client_type: " << client_type;
        return false;
    }

    auto& library = AndroidExternalLib::instance();

    MethodParam start_param;
    start_param.display_id = m_display_id;
    start_param.method = START_GAME;
    start_param.args.start_game.package_name = package_name->c_str();
    start_param.args.start_game.force_stop = m_force_stop ? 1 : 0;

    if (library.DispatchInputMessage(start_param) != 0) {
        LogError << "Failed to start game with client_type: " << client_type;
        return false;
    }

    LogInfo << "Game started successfully with client_type: " << client_type << ", package: " << *package_name;
    return true;
}

bool AndroidController::stop_game(const std::string& client_type)
{
    LogTraceFunction;
    LogInfo << "Stopping game with client_type:" << client_type;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    auto& library = AndroidExternalLib::instance();

    MethodParam stop_param;
    stop_param.display_id = m_display_id;
    stop_param.method = STOP_GAME;
    stop_param.args.stop_game.client_type = client_type.c_str();

    if (library.DispatchInputMessage(stop_param) != 0) {
        LogError << "Failed to stop game with client_type: " << client_type;
        return false;
    }

    LogInfo << "Game stopped successfully with client_type: " << client_type;
    return true;
}

bool AndroidController::click(const Point& p)
{
    LogTraceFunction;
    LogTrace << "Clicking at point:" << p.x << "," << p.y;
    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    if (p.x < 0 || p.x >= m_screen_resolution.first || p.y < 0 || p.y >= m_screen_resolution.second) {
        LogError << "click point out of range: " << p.x << "," << p.y << " screen:" << m_screen_resolution.first << "x"
                 << m_screen_resolution.second;
    }

    auto& library = AndroidExternalLib::instance();

    // key_down
    MethodParam touch_down_param;
    touch_down_param.display_id = m_display_id;
    touch_down_param.method = TOUCH_DOWN;
    touch_down_param.args.touch.p = { p.x, p.y };

    if (library.DispatchInputMessage(touch_down_param) != 0) {
        LogError << "Failed to send TOUCH_DOWN event";
        return false;
    }
    sleep(20);

    // key_up
    MethodParam touch_up_param;
    touch_up_param.display_id = m_display_id;
    touch_up_param.method = TOUCH_UP;
    touch_up_param.args.touch.p = { p.x, p.y };

    if (library.DispatchInputMessage(touch_up_param) != 0) {
        LogError << "Failed to send TOUCH_UP event";
        return false;
    }

    LogInfo << "Click completed successfully at: " << p.x << "," << p.y;
    return true;
}

bool AndroidController::input(const std::string& text)
{
    LogTraceFunction;
    LogTrace << "Input text:" << text;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    if (text.empty()) {
        LogWarn << "Input text is empty";
        return true;
    }

    auto& library = AndroidExternalLib::instance();

    // input
    MethodParam input_param;
    input_param.display_id = m_display_id;
    input_param.method = INPUT;
    input_param.args.input.text = text.c_str();

    if (library.DispatchInputMessage(input_param) != 0) {
        LogError << "Failed to send INPUT event";
        return false;
    }

    LogInfo << "Input completed successfully, text length: " << text.length();
    return true;
}

bool AndroidController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    LogTraceFunction;
    LogTrace << "AndroidController swipe" << p1 << p2 << duration << extra_swipe << slope_in << slope_out;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
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

    auto& library = AndroidExternalLib::instance();

    // 触摸按下起点
    MethodParam touch_down_param;
    touch_down_param.display_id = m_display_id;
    touch_down_param.method = TOUCH_DOWN;
    touch_down_param.args.touch.p = { x1, y1 };

    if (library.DispatchInputMessage(touch_down_param) != 0) {
        LogError << "Failed to send TOUCH_DOWN event for swipe";
        return false;
    }

    constexpr int TimeInterval = 2; // 类似 Minitoucher::DefaultSwipeDelay

    bool need_pause = with_pause;
    const auto& opt = Config.get_options();

    auto bounds_check = [this](int x, int y) {
        return x >= 0 && x <= m_screen_resolution.first && y >= 0 && y <= m_screen_resolution.second;
    };

    auto move_func = [&](int x, int y) -> bool {
        MethodParam touch_move_param;
        touch_move_param.display_id = m_display_id;
        touch_move_param.method = TOUCH_MOVE;
        touch_move_param.args.touch.p = { x, y };

        if (library.DispatchInputMessage(touch_move_param) != 0) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TimeInterval));
        return true;
    };

    auto android_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
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
        else {
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
        }
    };

    if (!android_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration)) {
        LogError << "Failed during main swipe movement";
        // key_up
        MethodParam touch_up_param;
        touch_up_param.display_id = m_display_id;
        touch_up_param.method = TOUCH_UP;
        touch_up_param.args.touch.p = { x2, y2 };
        library.DispatchInputMessage(touch_up_param);
        return false;
    }

    // 额外滑动逻辑
    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));

        if (!android_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration)) {
            LogWarn << "Failed during extra swipe movement";
        }
    }

    // key_up
    MethodParam touch_up_param;
    touch_up_param.display_id = m_display_id;
    touch_up_param.method = TOUCH_UP;
    touch_up_param.args.touch.p = { x2, y2 };

    if (library.DispatchInputMessage(touch_up_param) != 0) {
        LogError << "Failed to send TOUCH_UP event for swipe";
        return false;
    }

    LogInfo << "AndroidController swipe completed successfully";
    return true;
}

bool AndroidController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;
    LogTrace << "Injecting input event, type:" << static_cast<int>(event.type) << " point:(" << event.point.x << ","
             << event.point.y << ")"
             << " keycode:" << event.keycode;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    auto& library = AndroidExternalLib::instance();
    MethodParam param;
    param.display_id = m_display_id;

    switch (event.type) {
    case InputEvent::Type::KEY_DOWN:
        param.method = KEY_DOWN;
        param.args.key.key_code = event.keycode;
        break;

    case InputEvent::Type::KEY_UP:
        param.method = KEY_UP;
        param.args.key.key_code = event.keycode;
        break;

    case InputEvent::Type::TOUCH_DOWN:
        param.method = TOUCH_DOWN;
        param.args.touch.p = { event.point.x, event.point.y };
        break;

    case InputEvent::Type::TOUCH_UP:
        param.method = TOUCH_UP;
        param.args.touch.p = { event.point.x, event.point.y };
        break;

    case InputEvent::Type::TOUCH_MOVE:
        param.method = TOUCH_MOVE;
        param.args.touch.p = { event.point.x, event.point.y };
        break;

    case InputEvent::Type::TOUCH_RESET:
    case InputEvent::Type::WAIT_MS:
    case InputEvent::Type::COMMIT:
        LogWarn << "InputEvent type not supported by DispatchInputMessage: " << static_cast<int>(event.type);
        return true; // 不支持的事件类型，但不报错

    case InputEvent::Type::UNKNOWN:
    default:
        LogError << "Unknown input event type: " << static_cast<int>(event.type);
        return false;
    }

    if (library.DispatchInputMessage(param) != 0) {
        LogError << "Failed to dispatch input event";
        return false;
    }

    LogInfo << "Input event injected successfully";
    return true;
}

bool AndroidController::press_esc()
{
    LogTraceFunction;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return false;
    }

    auto& library = AndroidExternalLib::instance();

    constexpr int EscKeyCode = 27; // ESC键代码

    // 按键按下
    MethodParam key_down_param;
    key_down_param.display_id = m_display_id;
    key_down_param.method = KEY_DOWN;
    key_down_param.args.key.key_code = EscKeyCode;

    if (library.DispatchInputMessage(key_down_param) != 0) {
        LogError << "Failed to send KEY_DOWN event for ESC";
        return false;
    }

    // 按键抬起
    MethodParam key_up_param;
    key_up_param.display_id = m_display_id;
    key_up_param.method = KEY_UP;
    key_up_param.args.key.key_code = EscKeyCode;

    if (library.DispatchInputMessage(key_up_param) != 0) {
        LogError << "Failed to send KEY_UP event for ESC";
        return false;
    }

    LogInfo << "ESC key pressed successfully";
    return true;
}

ControlFeat::Feat AndroidController::support_features() const noexcept
{
    // AndroidController 支持精确滑动和暂停滑动功能
    auto feat = ControlFeat::PRECISE_SWIPE;
    feat |= ControlFeat::SWIPE_WITH_PAUSE;
    return feat;
}

std::pair<int, int> AndroidController::get_screen_res() const noexcept
{
    return m_screen_resolution;
}

void AndroidController::back_to_home() noexcept
{
    LogTraceFunction;

    if (!m_inited) {
        LogError << "AndroidController not initialized";
        return;
    }
}

void AndroidController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, inst());
    }
}

#endif // __ANDROID__
