#include "MaaFwLinuxController.h"
#include "Config/GeneralConfig.h"
#include "Controller/SwipeHelper.hpp"

#ifdef __linux__

bool asst::MaaFwLinuxController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config)
{
    if (m_unit) {
        m_loader.destroy(m_unit);
        m_unit = nullptr;
    }

    if (!m_loader.loaded()) {
        if (!m_loader.load("MaaLinuxControlUnit")) {
            return false;
        }
    }

    m_unit = m_loader.create(config.c_str());
    if (!m_unit) {
        Log.error("Failed to create control unit");
        return false;
    }

    if (!m_unit->connect()) {
        Log.error("Failed to connect control unit");
        m_loader.destroy(m_unit);
        m_unit = nullptr;
        return false;
    }

    cv::Mat dummy { };
    return screencap(dummy);
}

const std::string& asst::MaaFwLinuxController::get_uuid() const
{
    const static std::string uuid("MaaFwLinuxControllerUUID");
    return uuid;
}

bool asst::MaaFwLinuxController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    if (!m_unit) {
        Log.error("Control unit not initialized");
        return false;
    }

    if (m_screen_size.second > 0 && m_main_screen_recognition) {
        using enum InputEvent::Type;
        // 主界面情况下鼠标移动到窗口中心，等待主界面的视差动画，300ms
        inject_input_event(InputEvent { .type = TOUCH_MOVE, .point = { m_screen_size.first / 2, m_screen_size.second / 2 } });
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (!m_unit->screencap(image_payload) || image_payload.empty()) {
        return false;
    }

    m_screen_size.first = image_payload.cols;
    m_screen_size.second = image_payload.rows;

    return true;
}

bool asst::MaaFwLinuxController::click(const Point& p)
{
    using enum InputEvent::Type;
    return inject_input_event(InputEvent { .type = TOUCH_DOWN, .point = { p.x, p.y } }) &&
           inject_input_event(InputEvent { .type = WAIT_MS, .milisec = 20 }) &&
           inject_input_event(InputEvent { .type = TOUCH_UP, .point = { p.x, p.y } }) && park_cursor();
}

bool asst::MaaFwLinuxController::input(const std::string& text)
{
    if (!m_unit) {
        Log.error("Control unit not initialized");
        return false;
    }

    return m_unit->input_text(text);
}

bool asst::MaaFwLinuxController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    const auto width = m_screen_size.first;
    const auto height = m_screen_size.second;

    if (width > 0 && height > 0) {
        if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height) {
            Log.warn("swipe point1 is out of range", x1, y1);
            x1 = std::clamp(x1, 0, width - 1);
            y1 = std::clamp(y1, 0, height - 1);
        }
    }

    using enum InputEvent::Type;

    if (!inject_input_event(InputEvent { .type = TOUCH_DOWN, .point = { x1, y1 } })) {
        return false;
    }

    const auto& opt = Config.get_options();
    int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    auto bounds_check = [width, height](int x, int y) {
        if (width <= 0 || height <= 0) {
            return true;
        }
        return x >= 0 && x < width && y >= 0 && y < height;
    };

    auto move_func = [this](int x, int y) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DefaultSwipeDelay));
        return inject_input_event(InputEvent { .type = TOUCH_MOVE, .point = { x, y } });
    };

    auto do_swipe = [&](int _x1, int _y1, int _x2, int _y2, int _duration) {
        return interpolate_swipe(
            _x1,
            _y1,
            _x2,
            _y2,
            _duration,
            DefaultSwipeDelay,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    if (!do_swipe(x1, y1, x2, y2, actual_duration)) {
        inject_input_event(InputEvent { .type = TOUCH_UP, .point = { x2, y2 } });
        return false;
    }

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        do_swipe(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
    }

    return inject_input_event(InputEvent { .type = TOUCH_UP, .point = { x2, y2 } }) && park_cursor();
}

bool asst::MaaFwLinuxController::inject_input_event(const InputEvent& event)
{
    if (!m_unit) {
        Log.error("Control unit not initialized");
        return false;
    }

    using enum InputEvent::Type;
    switch (event.type) {
    case TOUCH_DOWN:
        return m_unit->touch_down(event.pointerId, event.point.x, event.point.y, 0);
    case TOUCH_UP:
        return m_unit->touch_up(event.pointerId);
    case TOUCH_MOVE:
        return m_unit->touch_move(event.pointerId, event.point.x, event.point.y, 0);
    case KEY_DOWN:
        return m_unit->key_down(event.keycode);
    case KEY_UP:
        return m_unit->key_up(event.keycode);
    case WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    case TOUCH_RESET:
        [[fallthrough]];
    case COMMIT:
        return true;
    case UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }
}

bool asst::MaaFwLinuxController::press_esc()
{
    static constexpr int KEY_ESC = 1;

    return inject_input_event(InputEvent { .type = InputEvent::Type::KEY_DOWN, .keycode = KEY_ESC }) &&
           inject_input_event(InputEvent { .type = InputEvent::Type::WAIT_MS, .milisec = 20 }) &&
           inject_input_event(InputEvent { .type = InputEvent::Type::KEY_UP, .keycode = KEY_ESC });
}

bool asst::MaaFwLinuxController::park_cursor()
{
    return inject_input_event(InputEvent { .type = InputEvent::Type::WAIT_MS, .milisec = 20 }) &&
           inject_input_event(
               InputEvent { .type = InputEvent::Type::TOUCH_MOVE,
                            .point = { m_screen_size.first - 10, m_screen_size.second - 10 } });
}

#endif
