#ifdef _WIN32

#include "Win32Controller.h"

#include <algorithm>
#include <sstream>
#include <thread>

#include "CaptureInterest.hpp"
#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{
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

    restore_window_position();

    if (m_unit_handle && m_loader) {
        m_loader->destroy(m_unit_handle);
        m_unit_handle = nullptr;
    }
}

bool Win32Controller::attach(
    void* hwnd,
    Win32ScreencapMethod screencap_method,
    Win32InputMethod mouse_method,
    Win32InputMethod keyboard_method)
{
    LogTraceFunction;

    m_inited = false;
    m_hwnd = hwnd;
    m_screencap_method = screencap_method;
    m_mouse_method = mouse_method;
    m_keyboard_method = keyboard_method;

    // 销毁旧的控制单元
    if (m_unit_handle && m_loader) {
        m_loader->destroy(m_unit_handle);
        m_unit_handle = nullptr;
    }

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

    // 诊断用：记下窗口在屏幕上的矩形，便于从日志判断光标是否落在客户区内
    RECT window_rect = { 0, 0, 0, 0 };
    if (GetWindowRect(static_cast<HWND>(m_hwnd), &window_rect)) {
        Log.info(
            "Attached window rect:",
            window_rect.left,
            ",",
            window_rect.top,
            ",",
            window_rect.right,
            ",",
            window_rect.bottom);
    }

    if ((m_mouse_method & (Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos)) != 0 &&
        (m_screencap_method &
         (Win32Screencap::ScreenDC | Win32Screencap::DXGI_DesktopDup | Win32Screencap::DXGI_DesktopDup_Window)) == 0) {
        save_window_position();
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

    const bool with_window_pos =
        (m_mouse_method & (Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos)) != 0;
    // 仅 WithCursorPos 两种方式挪的是真实光标；Seize 本就强制接管鼠标，纯消息模式不动真实光标
    const bool cursor_pos_mode =
        (m_mouse_method & (Win32Input::SendMessageWithCursorPos | Win32Input::PostMessageWithCursorPos)) != 0;
    // 主界面识别必须把光标挪到窗口中心以回正视差；其他界面只在光标会挡到识别区时才挪。
    // 曾经这里是「非主界面一律挪」，在战斗中每秒截图 18 帧的节奏下等于每秒抢占光标 18 次，
    // 挂机期间用户鼠标完全不可用（issue #18229）。
    const bool move_to_center = m_capture_hint.main_screen_recognition;
    // 本次截图的决策输入：光标相对画面的位置（画面外/未知都偏保守）
    const CursorLocation cursor_location = locate_cursor();
    // 非主界面、非 WindowPos 的输入方式（CursorPos 与 Seize 都会真的移动光标）都要把光标挪出识别区，
    // 但只在它确实会挡到本次识别时才挪。注意 save/restore 仍只对 CursorPos 生效（沿用改动前的分工）。
    const bool move_to_corner = !move_to_center && !with_window_pos && need_relocate(cursor_location);
    const bool relocating = cursor_pos_mode && (move_to_center || move_to_corner);

    POINT original_cursor_pos = { 0, 0 };
    POINT parked_cursor_pos = { 0, 0 };
    bool cursor_pos_saved = false;
    bool cursor_parked = false;
    if (relocating && m_screen_size.second > 0) {
        // 挪之前先把用户的位置存下来。不再用 BlockInput 冻结指针来保护这次写入：
        // 还原改为事后观测（见下方 should_restore_cursor），既不丢用户输入，也不会把位置拉错。
        cursor_pos_saved = GetCursorPos(&original_cursor_pos) != 0;
        Log.trace(
            "Screencap saves cursor position:",
            original_cursor_pos.x,
            ",",
            original_cursor_pos.y,
            "cursor:",
            capture_interest::to_string(cursor_location.state),
            cursor_location.frame_pos.x,
            ",",
            cursor_location.frame_pos.y);
    }

    if (m_screen_size.second > 0) {
        if (move_to_center) {
            // 主界面情况下鼠标移动到窗口中心，等待主界面的视差动画，300ms
            unit_touch_move(0, m_screen_size.first / 2, m_screen_size.second / 2, 0);
            if (with_window_pos) {
                unit_touch_up(0);
            }
            cursor_parked = cursor_pos_saved && GetCursorPos(&parked_cursor_pos) != 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        else if (with_window_pos) {
            const bool capture_from_screen =
                (m_screencap_method & (Win32Screencap::ScreenDC | Win32Screencap::DXGI_DesktopDup |
                                       Win32Screencap::DXGI_DesktopDup_Window)) != 0;
            if (!capture_from_screen) {
                // WindowPos输入模式下，非主界面识别把窗口移到屏幕外
                unit_touch_move(0, 0, m_screen_size.second + GetSystemMetrics(SM_CYVIRTUALSCREEN) + 100, 0);
            }
            else {
                // 其他情况光标移到窗口左下角
                unit_touch_move(0, 0, m_screen_size.second - 1, 0);
            }
            unit_touch_up(0);
        }
        else if (move_to_corner) {
            unit_touch_move(0, 0, m_screen_size.second - 1, 0);
            // 记录本次挪动实际落点：还原时以它为准而不是以「期望挪到哪」为准，
            // 这样即使底层坐标换算与预期不同，也不会把光标还原到错误位置
            cursor_parked = cursor_pos_saved && GetCursorPos(&parked_cursor_pos) != 0;
            if (cursor_parked && !m_park_mismatch_logged && !parked_at_expected_position(parked_cursor_pos)) {
                Log.warn(
                    "Screencap parking spot mismatch, actual:",
                    parked_cursor_pos.x,
                    ",",
                    parked_cursor_pos.y,
                    "expected client:",
                    "0,",
                    m_screen_size.second - 1);
                m_park_mismatch_logged = true;
            }
            // 游戏自绘光标跟随真实光标，渲染存在帧延迟，等待其画到挪动终点后再截图，避免光标被截进识别区
            std::this_thread::sleep_for(std::chrono::milliseconds(34));
        }
        else {
            // 光标不会挡到本次识别的区域：不挪光标、不屏蔽输入
            Log.trace(
                "Screencap skips cursor relocation, cursor:",
                capture_interest::to_string(cursor_location.state),
                cursor_location.frame_pos.x,
                ",",
                cursor_location.frame_pos.y);
        }
    }

    // 截图前确认停靠还在：用户抢先动了鼠标时本帧可能带光标，只记录不追
    if (cursor_parked) {
        log_cursor_escaped(parked_cursor_pos);
    }

    bool ret = unit_screencap(image_payload);

    if (cursor_parked) {
        restore_cursor_if_untouched(original_cursor_pos, parked_cursor_pos);
    }

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    return ret;
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
    SwipeExtraDirection extra_swipe,
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
        bool ret = unit_touch_move(0, x, y, 0);
        // Win32 输入（如 Seize 的 SendInput）为异步注入且无内置节拍，不等待会使整段滑动在毫秒级完成，被游戏判定为点击
        std::this_thread::sleep_for(std::chrono::milliseconds(DefaultSwipeDelay));
        return ret;
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

    if (extra_swipe != SwipeExtraDirection::None && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        const auto offset = extra_swipe_offset(extra_swipe, opt.minitouch_extra_swipe_dist);
        do_swipe(x2, y2, x2 + offset.x, y2 + offset.y, opt.minitouch_extra_swipe_duration);
    }

    return unit_touch_up(0);
}

bool Win32Controller::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        return unit_touch_down(event.pointerId, event.point.x, event.point.y, 0);
    case InputEvent::Type::TOUCH_UP:
        return unit_touch_up(event.pointerId);
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

void Win32Controller::set_capture_hint(const CaptureHint& hint)
{
    m_capture_hint = hint;
}

Win32Controller::CursorLocation asst::Win32Controller::locate_cursor() const
{
    CursorLocation location;
    if (m_hwnd == nullptr || m_screen_size.first <= 0 || m_screen_size.second <= 0) {
        return location;
    }

    POINT cursor_pos = { 0, 0 };
    if (!GetCursorPos(&cursor_pos)) {
        return location;
    }

    RECT client_rect = { 0, 0, 0, 0 };
    if (!GetClientRect(static_cast<HWND>(m_hwnd), &client_rect)) {
        return location;
    }

    const int client_width = client_rect.right - client_rect.left;
    const int client_height = client_rect.bottom - client_rect.top;
    if (client_width <= 0 || client_height <= 0) {
        return location;
    }

    if (!ScreenToClient(static_cast<HWND>(m_hwnd), &cursor_pos)) {
        return location;
    }

    // 客户区物理像素与截图画面尺寸可能因 DPI 缩放而不一致，统一归一化到画面坐标
    const int frame_x = cursor_pos.x * m_screen_size.first / client_width;
    const int frame_y = cursor_pos.y * m_screen_size.second / client_height;
    if (frame_x < 0 || frame_y < 0 || frame_x >= m_screen_size.first || frame_y >= m_screen_size.second) {
        // 客户区外：这是实测过的「不污染画面」情形（挂机时鼠标通常就停在 MAA 窗口上）
        location.state = capture_interest::CursorLocateState::Outside;
        return location;
    }

    location.state = capture_interest::CursorLocateState::Inside;
    location.frame_pos = Point { frame_x, frame_y };
    return location;
}

bool asst::Win32Controller::need_relocate(const CursorLocation& location) const
{
    bool hit_interest = false;
    if (location.state == capture_interest::CursorLocateState::Inside) {
        if (m_capture_hint.interests.empty()) {
            // 调用方未指定兴趣区：拿不准，按「必须挪」处理
            hit_interest = true;
        }
        else {
            hit_interest = std::ranges::any_of(m_capture_hint.interests, [&](const Rect& roi) {
                return capture_interest::rect_hit(
                    location.frame_pos.x,
                    location.frame_pos.y,
                    roi.x,
                    roi.y,
                    roi.width,
                    roi.height,
                    capture_interest::DefaultInterestMargin);
            });
        }
    }

    return capture_interest::should_relocate(location.state, hit_interest);
}

void Win32Controller::log_cursor_escaped(const POINT& parked) const
{
    // 截图过程中停靠被用户动作打断：本帧可能带光标（尽力而为的降级）。
    // 不去重停靠 —— 那要再等一轮 34ms 让游戏重绘，事件成本翻倍，且会与正在操作的用户打架。
    POINT cursor_pos = { 0, 0 };
    if (!GetCursorPos(&cursor_pos)) {
        return;
    }
    if (!capture_interest::same_point(cursor_pos.x, cursor_pos.y, parked.x, parked.y)) {
        Log.trace("Screencap cursor escaped the parking spot before capture");
    }
}

void Win32Controller::restore_cursor_if_untouched(const POINT& original, const POINT& parked)
{
    POINT cursor_pos = { 0, 0 };
    if (!GetCursorPos(&cursor_pos)) {
        // 读不到位置：保守不动，避免把用户拽到错误的地方
        return;
    }
    // 只有光标仍停在我们挪去的停靠点（没人动过）才还原；用户自己动过就尊重其当前位置。
    // 这取代了原先用 BlockInput 冻结指针来保证「还原写不被竞争覆盖」的做法：
    // 既不会丢掉用户输入，也不会把光标拉回 45ms 前的位置。
    if (!capture_interest::should_restore_cursor(cursor_pos.x, cursor_pos.y, parked.x, parked.y)) {
        Log.trace("Screencap keeps user cursor position:", cursor_pos.x, ",", cursor_pos.y);
        return;
    }
    if (!SetCursorPos(original.x, original.y)) {
        Log.error("Failed to restore cursor position after screencap, last_error:", GetLastError());
        return;
    }
    Log.trace("Screencap restores cursor position");
}

bool Win32Controller::parked_at_expected_position(const POINT& parked) const
{
    constexpr int ParkVerifyTolerance = 8;
    // 左下角停靠的目标点（客户区坐标），下面会被 ClientToScreen 就地换算成屏幕坐标
    POINT expected_cursor_pos = { 0, m_screen_size.second - 1 };
    if (!ClientToScreen(static_cast<HWND>(m_hwnd), &expected_cursor_pos)) {
        return false;
    }
    return capture_interest::same_point(
        parked.x,
        parked.y,
        expected_cursor_pos.x,
        expected_cursor_pos.y,
        ParkVerifyTolerance);
}

void Win32Controller::save_window_position()
{
    if (m_window_rect_saved || !m_hwnd) {
        return;
    }
    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!IsWindow(hwnd) || !GetWindowRect(hwnd, &m_original_window_rect)) {
        return;
    }
    m_window_rect_saved = true;
}

void Win32Controller::restore_window_position()
{
    LogTraceFunction;
    if (!m_window_rect_saved || !m_hwnd) {
        return;
    }
    // 先结束窗口追踪，避免把窗口移动到错误位置
    if (auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle); unit != nullptr) {
        unit->inactive();
    }
    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (IsWindow(hwnd)) {
        SetWindowPos(
            hwnd,
            nullptr,
            m_original_window_rect.left,
            m_original_window_rect.top,
            0,
            0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    m_window_rect_saved = false;
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
