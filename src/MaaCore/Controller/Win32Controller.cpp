#ifdef _WIN32

#include "Win32Controller.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <thread>

#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{
namespace
{
constexpr LONG CursorAvoidancePadding = 8;
constexpr int NormalWindowParkingAttempts = 3;
constexpr auto WindowMoveSettleTimeout = std::chrono::milliseconds(250);
constexpr auto WindowPosTrackingSettleDelay = std::chrono::milliseconds(40);

struct WindowParkingCandidate
{
    LONG left = 0;
    LONG top = 0;
    long long overflow = 0;
    long long movement = 0;
    bool preferred = false;
};

LONG fit_window_axis(LONG current, LONG bound_begin, LONG bound_end, LONG size)
{
    const LONG available = bound_end - bound_begin;
    if (size >= available) {
        return bound_begin;
    }
    return std::clamp(current, bound_begin, bound_end - size);
}

long long rect_overflow(const RECT& rect, const RECT& bounds)
{
    return static_cast<long long>(std::max(bounds.left - rect.left, 0L)) +
           static_cast<long long>(std::max(bounds.top - rect.top, 0L)) +
           static_cast<long long>(std::max(rect.right - bounds.right, 0L)) +
           static_cast<long long>(std::max(rect.bottom - bounds.bottom, 0L));
}

bool get_client_screen_rect(HWND hwnd, RECT& rect)
{
    RECT client_rect = {};
    if (!GetClientRect(hwnd, &client_rect)) {
        return false;
    }

    POINT top_left = { client_rect.left, client_rect.top };
    POINT bottom_right = { client_rect.right, client_rect.bottom };
    if (!ClientToScreen(hwnd, &top_left) || !ClientToScreen(hwnd, &bottom_right)) {
        return false;
    }

    rect = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };
    return true;
}

std::optional<WindowParkingCandidate> make_parking_candidate(
    LONG left,
    LONG top,
    bool preferred,
    const POINT& cursor,
    const RECT& window_rect,
    const RECT& client_rect,
    const RECT& monitor_rect,
    LONG cursor_clearance)
{
    RECT predicted_window = window_rect;
    OffsetRect(&predicted_window, left - window_rect.left, top - window_rect.top);

    RECT predicted_client = client_rect;
    OffsetRect(&predicted_client, left - window_rect.left, top - window_rect.top);
    InflateRect(&predicted_client, cursor_clearance, cursor_clearance);
    if (PtInRect(&predicted_client, cursor)) {
        return std::nullopt;
    }

    return WindowParkingCandidate {
        .left = left,
        .top = top,
        .overflow = rect_overflow(predicted_window, monitor_rect),
        .movement = std::abs(static_cast<long long>(left) - window_rect.left) +
                    std::abs(static_cast<long long>(top) - window_rect.top),
        .preferred = preferred,
    };
}

void select_parking_candidate(
    std::optional<WindowParkingCandidate>& best,
    std::optional<WindowParkingCandidate> candidate)
{
    if (!candidate) {
        return;
    }
    if (!best || candidate->overflow < best->overflow ||
        (candidate->overflow == best->overflow && candidate->preferred && !best->preferred) ||
        (candidate->overflow == best->overflow && candidate->preferred == best->preferred &&
         candidate->movement < best->movement)) {
        best = candidate;
    }
}

HWND find_mouse_target(HWND root, const POINT& cursor)
{
    HWND target = root;
    while (target && IsWindow(target)) {
        POINT local = cursor;
        if (!ScreenToClient(target, &local)) {
            break;
        }

        HWND child = ChildWindowFromPointEx(target, local, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
        if (!child || child == target) {
            break;
        }
        target = child;
    }
    return target;
}

void send_mouse_leave(HWND root, HWND target)
{
    auto send = [](HWND hwnd) {
        if (!hwnd || !IsWindow(hwnd)) {
            return;
        }
        DWORD_PTR result = 0;
        SendMessageTimeoutW(hwnd, WM_MOUSELEAVE, 0, 0, SMTO_ABORTIFHUNG, 25, &result);
    };

    send(target);
    if (target != root) {
        send(root);
    }
}
}

Win32Controller::Win32Controller(const AsstCallback& callback, Assistant* inst) :
    InstHelper(inst),
    m_callback(callback),
    m_loader(std::make_unique<Win32ControlUnitLoader>())
{
    LogTraceFunction;
}

Win32Controller::~Win32Controller()
{
    LogTraceFunction;

    if (m_unit_handle && m_loader) {
        m_loader->destroy(m_unit_handle);
        m_unit_handle = nullptr;
    }
    restore_window_position();
}

bool Win32Controller::attach(
    void* hwnd,
    Win32ScreencapMethod screencap_method,
    Win32InputMethod mouse_method,
    Win32InputMethod keyboard_method)
{
    LogTraceFunction;

    m_inited = false;

    // 销毁旧的控制单元
    if (m_unit_handle && m_loader) {
        m_loader->destroy(m_unit_handle);
        m_unit_handle = nullptr;
    }
    restore_window_position();

    m_hwnd = hwnd;
    m_screencap_method = screencap_method;
    m_mouse_method = mouse_method;
    m_keyboard_method = keyboard_method;

    // 加载 DLL
    if (!m_loader->loaded()) {
        auto dll_path = "MaaWin32ControlUnit";
        if (!m_loader->load(dll_path)) {
            Log.error("Failed to load MaaWin32ControlUnit.dll");
            return false;
        }
    }

    // 创建控制单元
    m_unit_handle = m_loader->create(hwnd, screencap_method, mouse_method, keyboard_method);
    if (!m_unit_handle) {
        Log.error("Failed to create Win32ControlUnit");
        return false;
    }

    // 连接
    if (!unit_connect()) {
        Log.error("Failed to connect Win32ControlUnit");
        m_loader->destroy(m_unit_handle);
        m_unit_handle = nullptr;
        return false;
    }

    // 获取 UUID
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit->request_uuid(m_uuid)) {
        std::stringstream ss;
        ss << hwnd;
        m_uuid = ss.str();
    }

    // 尝试截图获取屏幕分辨率
    cv::Mat image;
    if (unit_screencap(image)) {
        m_screen_size = { image.cols, image.rows };
        Log.info("Screen size:", m_screen_size.first, "x", m_screen_size.second);
    }

    m_inited = true;
    return true;
}

bool Win32Controller::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config [[maybe_unused]])
{
    Log.error("Win32Controller does not support connect(), use attach() instead");
    return false;
}

bool Win32Controller::inited() const noexcept
{
    return m_inited && m_unit_handle;
}

const std::string& Win32Controller::get_uuid() const
{
    return m_uuid;
}

bool Win32Controller::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;

    if (should_avoid_window_cursor()) {
        auto hwnd = static_cast<HWND>(m_hwnd);
        constexpr auto PseudoMinimizeMethods = Win32Screencap::FramePool | Win32Screencap::PrintWindow;
        if (hwnd && IsWindow(hwnd) && IsIconic(hwnd) && (m_screencap_method & PseudoMinimizeMethods) != 0) {
            // These capture methods convert a real minimized window into a transparent pseudo-minimized
            // window inside the control unit's screencap call. Discard that transition frame, then park
            // the restored window before producing the image exposed to recognition.
            cv::Mat transition_frame;
            if (!unit_screencap(transition_frame)) {
                return false;
            }
        }

        if (!park_window_away_from_cursor()) {
            Log.warn("Failed to move Win32 window away from the physical cursor; skip this screencap");
            return false;
        }
    }

    if (!unit_screencap(image_payload)) {
        return false;
    }

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    return true;
}

bool Win32Controller::start_game(const std::string& client_type [[maybe_unused]])
{
    Log.warn("start_game is not supported on Win32Controller");
    return false;
}

bool Win32Controller::stop_game(const std::string& client_type [[maybe_unused]])
{
    LogTraceFunction;

    if (!m_hwnd) {
        Log.info("No window handle available, game may already be closed");
        return true;
    }

    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!IsWindow(hwnd)) {
        Log.info("Invalid or stale window handle, game may already be closed, hwnd:", m_hwnd);
        return true;
    }

    DWORD pid = 0;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (tid == 0) {
        DWORD error = GetLastError();
        Log.error("Failed to get thread/process id from hwnd, hwnd:", m_hwnd, "last_error:", error);
        return false;
    }

    if (pid == 0) {
        Log.error("Failed to get process id from hwnd, hwnd:", m_hwnd);
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (!hProcess) {
        DWORD error = GetLastError();
        Log.error("Failed to open process, pid:", pid, "last_error:", error);
        return false;
    }

    if (PostMessage(hwnd, WM_CLOSE, 0, 0)) {
        DWORD wait_result = WaitForSingleObject(hProcess, 5000);
        if (wait_result == WAIT_OBJECT_0) {
            CloseHandle(hProcess);
            Log.info("Game process closed gracefully, pid:", pid);
            return true;
        }
    }

    BOOL ok = TerminateProcess(hProcess, 0);
    if (!ok) {
        DWORD error = GetLastError();
        CloseHandle(hProcess);
        Log.error("Failed to terminate process, pid:", pid, "last_error:", error);
        return false;
    }

    DWORD wait_result = WaitForSingleObject(hProcess, 5000);
    CloseHandle(hProcess);

    if (wait_result == WAIT_TIMEOUT) {
        Log.error("Terminate process timed out, pid:", pid);
        return false;
    }

    if (wait_result == WAIT_FAILED) {
        DWORD error = GetLastError();
        Log.error("Wait for process termination failed, pid:", pid, "last_error:", error);
        return false;
    }

    Log.info("Game process terminated, pid:", pid);
    return true;
}

bool Win32Controller::click(const Point& p)
{
    LogTraceFunction;
    Log.trace("Win32Controller click:", p);

    // MaaWin32ControlUnit 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick
    // 需要使用 touch_down/touch_up 替代 click
    // down/up 之间保持一小段时间（hold time），模拟器才能识别为一次完整的点击；
    // up 之后再等同样时间，为下一次 click 留出间隔。
    // 与 Minitoucher::DefaultClickDelay（50ms）对齐。
    constexpr int click_delay_ms = 50;

    bool down = unit_touch_down(0, p.x, p.y, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(click_delay_ms));
    bool up = unit_touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(click_delay_ms));
    if (up && should_avoid_window_cursor() && !park_window_away_from_cursor()) {
        Log.warn("Failed to move Win32 window away from the physical cursor after click");
    }

    return up && down;
}

bool Win32Controller::input(const std::string& text)
{
    LogTraceFunction;
    return unit_input_text(text);
}

bool Win32Controller::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    LogTraceFunction;

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    const auto width = m_screen_size.first;
    const auto height = m_screen_size.second;

    // 起点不能在屏幕外，但是终点可以
    if (width > 0 && height > 0) {
        if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height) {
            Log.warn("swipe point1 is out of range", x1, y1);
            x1 = std::clamp(x1, 0, width - 1);
            y1 = std::clamp(y1, 0, height - 1);
        }
    }

    Log.trace("Win32Controller swipe", p1, p2, duration, extra_swipe, slope_in, slope_out);

    // MaaWin32ControlUnit 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick
    // 需要使用 touch_down/touch_move/touch_up 实现滑动
    if (!unit_touch_down(0, x1, y1, 0)) {
        return false;
    }

    const auto& opt = Config.get_options();
    int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    auto bounds_check = [width, height](int x, int y) {
        if (width <= 0 || height <= 0) {
            return true;
        }
        return x >= 0 && x <= width && y >= 0 && y <= height;
    };

    auto move_func = [this](int x, int y) {
        return unit_touch_move(0, x, y, 0);
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
        unit_touch_up(0);
        return false;
    }

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        do_swipe(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
    }

    bool up = unit_touch_up(0);
    if (up && should_avoid_window_cursor()) {
        std::this_thread::sleep_for(WindowPosTrackingSettleDelay);
        if (!park_window_away_from_cursor()) {
            Log.warn("Failed to move Win32 window away from the physical cursor after swipe");
        }
    }
    return up;
}

bool Win32Controller::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return unit_touch_down(event.pointerId, event.point.x, event.point.y, 0);
    case InputEvent::Type::TOUCH_UP: {
        bool up = unit_touch_up(event.pointerId);
        if (up && should_avoid_window_cursor()) {
            std::this_thread::sleep_for(WindowPosTrackingSettleDelay);
            if (!park_window_away_from_cursor()) {
                Log.warn("Failed to move Win32 window away from the physical cursor after touch up");
            }
        }
        return up;
    }
    case InputEvent::Type::TOUCH_MOVE:
        return unit_touch_move(event.pointerId, event.point.x, event.point.y, 0);
    case InputEvent::Type::KEY_DOWN: {
        auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
        return unit ? unit->key_down(event.keycode) : false;
    }
    case InputEvent::Type::KEY_UP: {
        auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
        return unit ? unit->key_up(event.keycode) : false;
    }
    case InputEvent::Type::WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    case InputEvent::Type::TOUCH_RESET:
    case InputEvent::Type::COMMIT:
        return true;
    case InputEvent::Type::UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }
}

bool Win32Controller::press_esc()
{
    LogTraceFunction;
    return unit_click_key(VK_ESCAPE); // VK_ESCAPE = 0x1B, defined in WinUser.h
}

ControlFeat::Feat Win32Controller::support_features() const noexcept
{
    return ControlFeat::PRECISE_SWIPE;
}

std::pair<int, int> Win32Controller::get_screen_res() const noexcept
{
    return m_screen_size;
}

void Win32Controller::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

bool Win32Controller::should_avoid_window_cursor() const noexcept
{
    constexpr auto WindowPosMethods = Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos;
    return m_window_cursor_avoidance.load() && (m_mouse_method & WindowPosMethods) != 0;
}

bool Win32Controller::park_window_away_from_cursor()
{
    if (!should_avoid_window_cursor()) {
        return true;
    }

    auto hwnd = static_cast<HWND>(m_hwnd);
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    if (IsIconic(hwnd)) {
        return true;
    }

    std::scoped_lock lock(m_window_move_mutex);
    auto previous_dpi_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    bool result = [&]() {
        for (int attempt = 0; attempt <= NormalWindowParkingAttempts; ++attempt) {
            const bool use_offscreen_fallback = attempt == NormalWindowParkingAttempts;
            POINT cursor = {};
            RECT window_rect = {};
            RECT client_rect = {};
            if (!GetCursorPos(&cursor) || !GetWindowRect(hwnd, &window_rect) ||
                !get_client_screen_rect(hwnd, client_rect)) {
                return false;
            }

            const LONG client_width = client_rect.right - client_rect.left;
            const LONG client_height = client_rect.bottom - client_rect.top;
            const LONG window_width = window_rect.right - window_rect.left;
            const LONG window_height = window_rect.bottom - window_rect.top;
            const LONG client_offset_x = client_rect.left - window_rect.left;
            const LONG client_offset_y = client_rect.top - window_rect.top;
            const LONG cursor_clearance = std::max(
                                              { static_cast<LONG>(GetSystemMetrics(SM_CXCURSOR)),
                                                static_cast<LONG>(GetSystemMetrics(SM_CYCURSOR)),
                                                32L }) +
                                          CursorAvoidancePadding;
            if (client_width <= 0 || client_height <= 0) {
                return false;
            }

            RECT anchor_rect = m_window_position_saved ? m_saved_window_rect : window_rect;
            HMONITOR monitor = MonitorFromRect(&anchor_rect, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitor_info = { sizeof(MONITORINFO) };
            if (!monitor || !GetMonitorInfoW(monitor, &monitor_info)) {
                return false;
            }
            const RECT& monitor_rect = monitor_info.rcMonitor;
            const LONG fitted_left =
                fit_window_axis(window_rect.left, monitor_rect.left, monitor_rect.right, window_width);
            const LONG fitted_top =
                fit_window_axis(window_rect.top, monitor_rect.top, monitor_rect.bottom, window_height);

            RECT cursor_safe_client_rect = client_rect;
            InflateRect(&cursor_safe_client_rect, cursor_clearance, cursor_clearance);
            if (!PtInRect(&cursor_safe_client_rect, cursor)) {
                return true;
            }
            HWND mouse_target = find_mouse_target(hwnd, cursor);

            std::optional<WindowParkingCandidate> best;
            if (!use_offscreen_fallback && m_window_position_saved) {
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        m_saved_window_rect.left,
                        m_saved_window_rect.top,
                        true,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
            }
            if (!use_offscreen_fallback) {
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        window_rect.left,
                        window_rect.top,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        cursor.x + cursor_clearance + 1 - client_offset_x,
                        fitted_top,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        cursor.x - cursor_clearance - client_width - client_offset_x,
                        fitted_top,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        fitted_left,
                        cursor.y + cursor_clearance + 1 - client_offset_y,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        fitted_left,
                        cursor.y - cursor_clearance - client_height - client_offset_y,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
            }
            else {
                const LONG virtual_left = GetSystemMetrics(SM_XVIRTUALSCREEN);
                const LONG virtual_top = GetSystemMetrics(SM_YVIRTUALSCREEN);
                const LONG virtual_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                const LONG virtual_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                if (virtual_width <= 0 || virtual_height <= 0) {
                    return false;
                }

                const RECT virtual_screen = {
                    virtual_left,
                    virtual_top,
                    virtual_left + virtual_width,
                    virtual_top + virtual_height,
                };
                const LONG virtual_fitted_left =
                    fit_window_axis(window_rect.left, virtual_screen.left, virtual_screen.right, window_width);
                const LONG virtual_fitted_top =
                    fit_window_axis(window_rect.top, virtual_screen.top, virtual_screen.bottom, window_height);

                Log.info("Physical cursor kept entering the Win32 client; park the window outside the virtual screen");
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        virtual_screen.right + cursor_clearance + 1 - client_offset_x,
                        virtual_fitted_top,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        virtual_screen.left - cursor_clearance - client_width - client_offset_x,
                        virtual_fitted_top,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        virtual_fitted_left,
                        virtual_screen.bottom + cursor_clearance + 1 - client_offset_y,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
                select_parking_candidate(
                    best,
                    make_parking_candidate(
                        virtual_fitted_left,
                        virtual_screen.top - cursor_clearance - client_height - client_offset_y,
                        false,
                        cursor,
                        window_rect,
                        client_rect,
                        monitor_rect,
                        cursor_clearance));
            }

            if (!best) {
                return false;
            }

            const bool needs_move = std::abs(static_cast<long long>(best->left) - window_rect.left) > 1 ||
                                    std::abs(static_cast<long long>(best->top) - window_rect.top) > 1;
            if (needs_move) {
                const bool saved_now = !m_window_position_saved;
                if (saved_now) {
                    m_saved_window_rect = window_rect;
                    m_window_position_saved = true;
                    m_saved_window_placement.length = sizeof(m_saved_window_placement);
                    m_window_placement_saved = GetWindowPlacement(hwnd, &m_saved_window_placement) != FALSE;
                }
                if (!SetWindowPos(
                        hwnd,
                        nullptr,
                        best->left,
                        best->top,
                        0,
                        0,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS)) {
                    if (saved_now) {
                        m_window_position_saved = false;
                        m_window_placement_saved = false;
                    }
                    continue;
                }
            }

            if (needs_move) {
                bool settled = false;
                const auto settle_deadline = std::chrono::steady_clock::now() + WindowMoveSettleTimeout;
                do {
                    RECT settled_rect = {};
                    if (GetWindowRect(hwnd, &settled_rect) &&
                        std::abs(static_cast<long long>(settled_rect.left) - best->left) <= 1 &&
                        std::abs(static_cast<long long>(settled_rect.top) - best->top) <= 1) {
                        settled = true;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                } while (std::chrono::steady_clock::now() < settle_deadline);
                if (!settled) {
                    continue;
                }
            }

            send_mouse_leave(hwnd, mouse_target);
            if (needs_move) {
                std::this_thread::sleep_for(std::chrono::milliseconds(17));
            }

            POINT current_cursor = {};
            RECT current_client_rect = {};
            if (GetCursorPos(&current_cursor) && get_client_screen_rect(hwnd, current_client_rect)) {
                InflateRect(&current_client_rect, cursor_clearance, cursor_clearance);
                if (!PtInRect(&current_client_rect, current_cursor)) {
                    return true;
                }
            }
        }

        return false;
    }();

    if (previous_dpi_context) {
        SetThreadDpiAwarenessContext(previous_dpi_context);
    }
    return result;
}

void Win32Controller::restore_window_position()
{
    std::scoped_lock lock(m_window_move_mutex);
    auto hwnd = static_cast<HWND>(m_hwnd);
    if (!m_window_position_saved || !hwnd || !IsWindow(hwnd)) {
        m_window_position_saved = false;
        m_window_placement_saved = false;
        return;
    }

    bool restored = false;
    if (IsIconic(hwnd) && m_window_placement_saved) {
        WINDOWPLACEMENT current_placement = { sizeof(WINDOWPLACEMENT) };
        if (GetWindowPlacement(hwnd, &current_placement)) {
            current_placement.rcNormalPosition = m_saved_window_placement.rcNormalPosition;
            restored = SetWindowPlacement(hwnd, &current_placement) != FALSE;
        }
    }

    if (!restored && !SetWindowPos(
                         hwnd,
                         nullptr,
                         m_saved_window_rect.left,
                         m_saved_window_rect.top,
                         0,
                         0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS)) {
        Log.warn("Failed to restore Win32 window position", GetLastError());
    }
    m_window_position_saved = false;
    m_window_placement_saved = false;
}

bool Win32Controller::unit_connect()
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->connect();
}

bool Win32Controller::unit_screencap(cv::Mat& image)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->screencap(image);
}

bool Win32Controller::unit_click(int x, int y)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->click(x, y);
}

bool Win32Controller::unit_swipe(int x1, int y1, int x2, int y2, int duration)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->swipe(x1, y1, x2, y2, duration);
}

bool Win32Controller::unit_touch_down(int contact, int x, int y, int pressure)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->touch_down(contact, x, y, pressure);
}

bool Win32Controller::unit_touch_move(int contact, int x, int y, int pressure)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->touch_move(contact, x, y, pressure);
}

bool Win32Controller::unit_touch_up(int contact)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->touch_up(contact);
}

bool Win32Controller::unit_input_text(const std::string& text)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return unit->input_text(text);
}

bool Win32Controller::unit_click_key(int key)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }

    // MaaWin32ControlUnit 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick
    // 需要使用 key_down/key_up 替代 click_key
    if (!unit->key_down(key)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return unit->key_up(key);
}
} // namespace asst

#endif // _WIN32
