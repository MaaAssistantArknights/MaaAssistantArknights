#include "ModAvdController.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>

#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"

namespace asst
{
bool ModAvdController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    m_modavd_input_ready = false;
    release_minitouch();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    // 不直接调 MinitouchController::connect()：它在 MaaTouch 探测失败时会报 TouchModeNotAvailable
    // 并返回 false，而此时宿主侧点击可能是好的。先 adb 连接（顺带 init_modavd_extras），再优先用库。
    if (!AdbController::connect(adb_path, address, config)) {
        return false;
    }

    if (m_modavd_extras.touch_available()) {
        // 库收的是显示像素，ControlScaleProxy 缩放到的是 get_screen_res() 的尺寸，两者不一致时点击会偏
        auto size = m_modavd_extras.display_size();
        if (size && size->first == m_width && size->second == m_height) {
            m_modavd_input_ready = true;
            LogInfo << "ModAVD extras input is ready" << VAR(m_width) << VAR(m_height);
            return true;
        }
        LogWarn << "ModAVD display size mismatches adb screen size" << VAR(size.has_value())
                << VAR(size ? size->first : 0) << VAR(size ? size->second : 0) << VAR(m_width) << VAR(m_height);
    }

    LogInfo << "ModAVD extras input is not available, fallback to maatouch";

    m_minitouch_available = probe_minitouch();
    if (!m_minitouch_available) {
        json::value info = json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
            { "what", "TouchModeNotAvailable" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    return true;
}

bool ModAvdController::click(const Point& p)
{
    if (!m_modavd_input_ready) {
        return MaatouchController::click(p);
    }

    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        LogError << "click point out of range" << VAR(p.x) << VAR(p.y);
    }

    LogTrace << "modavd click" << VAR(p.x) << VAR(p.y);

    // 无条件抬手，避免 down 出错后手指卡在屏幕上；down/up 之间保持一小段时间，才算一次完整的点击
    bool down = m_modavd_extras.touch_down(0, p.x, p.y);
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));
    bool up = m_modavd_extras.touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));

    return up && down;
}

bool ModAvdController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    SwipeExtraDirection extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    if (!m_modavd_input_ready) {
        return MaatouchController::swipe(p1, p2, duration, extra_swipe, slope_in, slope_out, with_pause);
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        LogWarn << "swipe point1 is out of range" << VAR(x1) << VAR(y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    LogTrace << "modavd swipe" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration) << VAR(slope_in)
             << VAR(slope_out);

    if (!m_modavd_extras.touch_down(0, x1, y1)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));

    const auto& opt = Config.get_options();

    // 库的调用是同步的，按绝对节拍控制：第 k 步对齐 start + k * SwipeIntervalMs，调用耗时吃进预算
    auto tick_start = std::chrono::steady_clock::now();
    int move_step = 0;
    auto move_func = [this, &tick_start, &move_step](int x, int y) {
        bool ret = m_modavd_extras.touch_move(0, x, y);
        high_res_sleep_until(tick_start + ++move_step * std::chrono::milliseconds(SwipeIntervalMs));
        return ret;
    };

    auto bounds_check = [this](int x, int y) {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    };

    // 暂停发 ESC：库能发按键就直接发；不能就用 adb（和 minitouch 一样异步），要先有 press_esc 命令
    const bool key_by_lib = use_modavd_key();
    bool need_pause = with_pause && m_swipe_with_pause_enabled && (key_by_lib || !m_adb.press_esc.empty());
    std::future<void> pause_future;

    auto pause_check = [&opt](int cur_x, int cur_y, int start_x, int start_y) {
        return std::sqrt(std::pow(cur_x - start_x, 2) + std::pow(cur_y - start_y, 2)) >
               opt.swipe_with_pause_required_distance;
    };

    auto modavd_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
        // 每段滑动各自成段，重置绝对节拍的起点与步计数
        tick_start = std::chrono::steady_clock::now();
        move_step = 0;
        if (need_pause) {
            return interpolate_swipe_with_pause(
                _x1,
                _y1,
                _x2,
                _y2,
                _duration,
                SwipeIntervalMs,
                slope_in,
                slope_out,
                move_func,
                bounds_check,
                pause_check,
                [&]() {
                    need_pause = false;
                    if (key_by_lib) {
                        press_esc();
                    }
                    else {
                        pause_future = std::async(std::launch::async, [this]() { press_esc(); });
                    }
                });
        }
        return interpolate_swipe(
            _x1,
            _y1,
            _x2,
            _y2,
            _duration,
            SwipeIntervalMs,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    // 中途失败也必须抬手，否则手指会一直按在屏幕上，后续操作全部失效
    bool moved = modavd_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration);

    if (moved && extra_swipe != SwipeExtraDirection::None && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        const auto offset = extra_swipe_offset(extra_swipe, opt.minitouch_extra_swipe_dist);
        // extra 是主滑成功后的补偿段，失败不判整体失败，避免上层无谓重试
        if (!modavd_move(x2, y2, x2 + offset.x, y2 + offset.y, opt.minitouch_extra_swipe_duration)) {
            LogWarn << "failed during extra swipe movement";
        }
    }

    const bool up = m_modavd_extras.touch_up(0);
    // 抬起后留出间隔，为下一次输入留出手势结束的时间
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));
    return up && moved;
}

bool ModAvdController::inject_input_event(const InputEvent& event)
{
    if (!m_modavd_input_ready) {
        return MaatouchController::inject_input_event(event);
    }

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return m_modavd_extras.touch_down(event.pointerId, event.point.x, event.point.y);
    case InputEvent::Type::TOUCH_MOVE:
        return m_modavd_extras.touch_move(event.pointerId, event.point.x, event.point.y);
    case InputEvent::Type::TOUCH_UP:
        return m_modavd_extras.touch_up(event.pointerId);
    case InputEvent::Type::TOUCH_RESET:
        return m_modavd_extras.touch_release_all();
    case InputEvent::Type::WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    // 库每次调用即时生效，没有 minitouch 的批量提交语义
    case InputEvent::Type::COMMIT:
        return true;
    // 按键要模拟器的控制台收按键；收不了时没有别的通道能分开发按下和抬起
    case InputEvent::Type::KEY_DOWN:
        if (!use_modavd_key()) {
            LogError << "ModAVD extras input has no key events on this emulator" << VAR(event.keycode);
            return false;
        }
        return m_modavd_extras.key_down(event.keycode);
    case InputEvent::Type::KEY_UP:
        if (!use_modavd_key()) {
            LogError << "ModAVD extras input has no key events on this emulator" << VAR(event.keycode);
            return false;
        }
        return m_modavd_extras.key_up(event.keycode);
    case InputEvent::Type::UNKNOWN:
    default:
        LogError << "unknown input event type";
        return false;
    }
}

bool ModAvdController::input(const std::string& text)
{
    // 库没把文字发出去（没有编辑框可交而文字又不是 ASCII，或库太旧）时什么都没打，改用 adb
    if (m_modavd_input_ready && m_modavd_extras.text_available() && m_modavd_extras.input_text(text)) {
        return true;
    }
    return MaatouchController::input(text);
}

bool ModAvdController::press_esc()
{
    constexpr int EscKeyCode = 111;
    if (use_modavd_key()) {
        const bool down = m_modavd_extras.key_down(EscKeyCode);
        // 按下发出去了就一定抬起，免得键一直按着
        if (down && m_modavd_extras.key_up(EscKeyCode)) {
            return true;
        }
        if (down) {
            return false;
        }
    }
    return MaatouchController::press_esc();
}

ControlFeat::Feat ModAvdController::support_features() const noexcept
{
    if (!m_modavd_input_ready) {
        return MaatouchController::support_features();
    }

    // 坐标就是显示像素，不需要按 max_x/max_y 换算；暂停靠库的按键或 adb 的 press_esc
    auto feat = ControlFeat::PRECISE_SWIPE;
    if (m_swipe_with_pause_enabled && (use_modavd_key() || !m_adb.press_esc.empty())) {
        feat |= ControlFeat::SWIPE_WITH_PAUSE;
    }
    return feat;
}

std::optional<std::string> ModAvdController::reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket)
{
    if (!m_modavd_input_ready) {
        return MaatouchController::reconnect(cmd, timeout, recv_by_socket);
    }

    // 走宿主侧点击时没有 MaaTouch，跳过基类的重新拉起；库的控制台连接由库自己重连
    return AdbController::reconnect(cmd, timeout, recv_by_socket);
}

void ModAvdController::clear_info() noexcept
{
    MaatouchController::clear_info();
    m_modavd_input_ready = false;
}
} // namespace asst
