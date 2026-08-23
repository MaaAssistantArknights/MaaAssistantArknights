#include "MumuController.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include <algorithm>
#include <chrono>
#include <thread>

#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"

namespace asst
{
bool MumuController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    m_mumu_input_ready = false;
    m_mumu_input_notify = MumuInputNotify::None;
    release_minitouch();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    // 这里不能直接调 MinitouchController::connect()：它在 minitouch 探测失败时会抛出
    // TouchModeNotAvailable 并返回 false，而此时 MuMu 触控可能是好的。
    // 所以自己接管探测顺序：先 adb 连接（顺带 init_mumu_extras），再优先试 MuMu。
    if (!AdbController::connect(adb_path, address, config)) {
        return false;
    }

    // 连接时游戏可能还没开始渲染，此时 display 是 fallback 的桌面，其尺寸与游戏触控
    // 坐标系无关；这种情况下不做 mismatch 判定，推迟到输入入口（try_enable_mumu_input）
    bool deferred = false;

    if (m_mumu_extras.input_available()) {
        // ControlScaleProxy 会把坐标缩放到 get_screen_res() 的尺寸，而 nemu 期望的是
        // display 自身的坐标系。两者不一致时点击会全部偏掉，宁可降级也不要乱点。
        if (!m_mumu_extras.has_target_display()) {
            deferred = true;
            LogInfo << "Target package is not rendering, defer MuMu extras input check";
        }
        else {
            auto [mumu_width, mumu_height] = m_mumu_extras.get_display_size();
            if (mumu_width == m_width && mumu_height == m_height) {
                m_mumu_input_ready = true;
                LogInfo << "MuMu extras input is ready" << VAR(m_width) << VAR(m_height);
                notify_mumu_input_status(MumuInputNotify::Ready);
                return true;
            }
            LogWarn << "MuMu display size mismatches adb screen size" << VAR(mumu_width) << VAR(mumu_height)
                    << VAR(m_width) << VAR(m_height);
        }
    }

    LogInfo << "MuMu extras input is not available, fallback to minitouch";

    // 通知 GUI：触控增强未生效。deferred 场景（游戏尚未渲染）输入入口仍会自动重试，
    // GUI 不得据此判定触控不可用而停止任务
    notify_mumu_input_status(deferred ? MumuInputNotify::Deferred : MumuInputNotify::Unavailable);

    m_minitouch_available = probe_minitouch();
    if (!m_minitouch_available) {
        if (deferred) {
            // 游戏渲染后 MuMu 输入仍可能就绪，不因过渡期的 minitouch 探测失败而断开
            LogWarn << "minitouch probe failed while MuMu input check is deferred";
            return true;
        }
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

bool MumuController::try_enable_mumu_input()
{
    if (m_mumu_input_ready) {
        return true;
    }
    if (!m_mumu_extras.input_available() || !m_mumu_extras.has_target_display()) {
        return false;
    }
    auto [mumu_width, mumu_height] = m_mumu_extras.get_display_size();
    if (mumu_width != m_width || mumu_height != m_height) {
        // 游戏已渲染但坐标系不一致，挂起到此为止：这是确认不可用，不再是"尚未判定"，
        // 通知 GUI 终态（保活场景下 GUI 会据此停止任务）
        notify_mumu_input_status(MumuInputNotify::Unavailable);
        return false;
    }

    // 游戏已开始渲染且坐标系一致（典型场景：连接时游戏未启动，由任务拉起后），
    // 此前降级的 minitouch 不再需要
    m_mumu_input_ready = true;
    release_minitouch();
    LogInfo << "MuMu extras input is ready (deferred)" << VAR(m_width) << VAR(m_height);
    notify_mumu_input_status(MumuInputNotify::Ready);
    return true;
}

void MumuController::notify_mumu_input_status(MumuInputNotify status)
{
    if (status == m_mumu_input_notify) {
        return;
    }
    m_mumu_input_notify = status;

    // 通知 GUI：MuMu 触控增强的实际生效状态；deferred 表示尚未判定、等待游戏渲染后自动重试
    json::value input_info = json::object {
        { "uuid", m_uuid },
        { "what", "MuMuExtrasInputStatus" },
        { "details",
          json::object {
              { "available", status == MumuInputNotify::Ready },
              { "deferred", status == MumuInputNotify::Deferred },
          } },
    };
    callback(AsstMsg::ConnectionInfo, input_info);
}

bool MumuController::click(const Point& p)
{
    if (!try_enable_mumu_input()) {
        return MinitouchController::click(p);
    }

    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    Log.trace("mumu click:", p);

    // 无条件抬手，避免 down 出错后手指卡在屏幕上
    // down/up 之间保持一小段时间，模拟器才能识别为一次完整的点击（hold time）。
    // 之后 up 再等同样时间，为下一次 click 留出间隔。与 minitouch 的 down w50、up w50 对齐。
    bool down = m_mumu_extras.touch_down(0, p.x, p.y);
    std::this_thread::sleep_for(std::chrono::milliseconds(Minitoucher::DefaultClickDelay));
    bool up = m_mumu_extras.touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(Minitoucher::DefaultClickDelay));

    return up && down;
}

bool MumuController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    if (!try_enable_mumu_input()) {
        return MinitouchController::swipe(p1, p2, duration, extra_swipe, slope_in, slope_out, with_pause);
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    Log.trace("mumu swipe", p1, p2, duration, extra_swipe, slope_in, slope_out);

    if (!m_mumu_extras.touch_down(0, x1, y1)) {
        return false;
    }

    const auto& opt = Config.get_options();
    constexpr int TimeInterval = Minitoucher::DefaultSwipeDelay;

    auto move_func = [this](int x, int y) {
        // nemu 的调用是同步的，没有 minitouch 那样的 commit/wait 协议，
        // 这里自己按 TimeInterval 节流，否则滑动会瞬间走完
        bool ret = m_mumu_extras.touch_move(0, x, y);
        std::this_thread::sleep_for(std::chrono::milliseconds(TimeInterval));
        return ret;
    };

    auto bounds_check = [this](int x, int y) {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    };

    bool need_pause = with_pause && use_swipe_with_pause();

    auto pause_check = [&opt](int cur_x, int cur_y, int start_x, int start_y) {
        return std::sqrt(std::pow(cur_x - start_x, 2) + std::pow(cur_y - start_y, 2)) >
               opt.swipe_with_pause_required_distance;
    };

    // nemu 直接发按键，不用像 minitouch 那样异步起一个 adb press_esc
    auto pause_action = [this]() {
        constexpr int EscKeyCode = 111;
        m_mumu_extras.key_down(EscKeyCode);
        m_mumu_extras.key_up(EscKeyCode);
    };

    auto mumu_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
        if (need_pause) {
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
                    pause_action();
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

    // 中途失败也必须抬手，否则手指会一直按在屏幕上，后续操作全部失效
    bool moved = mumu_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration);

    if (moved && extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        moved = mumu_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
    }

    return m_mumu_extras.touch_up(0) && moved;
}

bool MumuController::inject_input_event(const InputEvent& event)
{
    if (!try_enable_mumu_input()) {
        return MinitouchController::inject_input_event(event);
    }

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return m_mumu_extras.touch_down(event.pointerId, event.point.x, event.point.y);
    case InputEvent::Type::TOUCH_MOVE:
        return m_mumu_extras.touch_move(event.pointerId, event.point.x, event.point.y);
    case InputEvent::Type::TOUCH_UP:
        return m_mumu_extras.touch_up(event.pointerId);
    case InputEvent::Type::KEY_DOWN:
        return m_mumu_extras.key_down(event.keycode);
    case InputEvent::Type::KEY_UP:
        return m_mumu_extras.key_up(event.keycode);
    case InputEvent::Type::WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    // nemu 每次调用即时生效，没有 minitouch 的批量提交语义
    case InputEvent::Type::TOUCH_RESET:
    case InputEvent::Type::COMMIT:
        return true;
    case InputEvent::Type::UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }
}

bool MumuController::input(const std::string& text)
{
    if (!try_enable_mumu_input()) {
        return MinitouchController::input(text);
    }

    return m_mumu_extras.input_text(text);
}

bool MumuController::press_esc()
{
    if (!try_enable_mumu_input()) {
        return MinitouchController::press_esc();
    }

    constexpr int EscKeyCode = 111;
    return m_mumu_extras.key_down(EscKeyCode) && m_mumu_extras.key_up(EscKeyCode);
}

ControlFeat::Feat MumuController::support_features() const noexcept
{
    if (!use_mumu_input()) {
        return MinitouchController::support_features();
    }

    // nemu 的坐标就是 display 原生坐标，不像 minitouch 要按 max_x/max_y 换算，
    // 暂停也是同步下发的按键，两个特性都能完整支持
    return ControlFeat::PRECISE_SWIPE | ControlFeat::SWIPE_WITH_PAUSE;
}

std::optional<std::string> MumuController::reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket)
{
    if (!use_mumu_input()) {
        return MinitouchController::reconnect(cmd, timeout, recv_by_socket);
    }

    // 走 MuMu 触控时输入不经过 minitouch（即使降级过渡期探测过，恢复时也已释放），
    // 跳过基类的 minitouch 重新拉起，否则会拿着空命令去起交互式 shell。
    // nemu 的连接由 MumuExtras 自己维护
    return AdbController::reconnect(cmd, timeout, recv_by_socket);
}

void MumuController::clear_info() noexcept
{
    MinitouchController::clear_info();
    m_mumu_input_ready = false;
    m_mumu_input_notify = MumuInputNotify::None;
}
} // namespace asst

#endif
