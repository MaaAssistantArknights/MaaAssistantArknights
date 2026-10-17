#ifdef _WIN32

#include "Win32Controller.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <future>
#include <numeric>
#include <sstream>
#include <thread>

#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

namespace asst
{
static const char* get_win32_screencap_method_name(Win32ScreencapMethod method)
{
    switch (method) {
    case Win32Screencap::GDI:
        return "GDI";
    case Win32Screencap::FramePool:
        return "FramePool";
    case Win32Screencap::DXGI_DesktopDup:
        return "DXGI_DesktopDup";
    case Win32Screencap::DXGI_DesktopDup_Window:
        return "DXGI_DesktopDup_Window";
    case Win32Screencap::PrintWindow:
        return "PrintWindow";
    case Win32Screencap::ScreenDC:
        return "ScreenDC";
    default:
        return "Win32";
    }
}

static bool is_borderless_fullscreen(HWND hwnd)
{
    if ((GetWindowLongPtr(hwnd, GWL_STYLE) & WS_POPUP) == 0) {
        return false;
    }

    RECT window_rect = {};
    MONITORINFO monitor_info = { sizeof(MONITORINFO) };
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!GetWindowRect(hwnd, &window_rect) || !monitor || !GetMonitorInfo(monitor, &monitor_info)) {
        return false;
    }

    const RECT& monitor_rect = monitor_info.rcMonitor;
    return window_rect.left <= monitor_rect.left && window_rect.top <= monitor_rect.top &&
           window_rect.right >= monitor_rect.right && window_rect.bottom >= monitor_rect.bottom;
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

    if (!restore_window_position()) {
        LogError << "Failed to restore the previous Win32 window before attaching a new one";
        return false;
    }
    m_inited = false;
    m_hwnd = hwnd;
    m_screencap_method = screencap_method;
    m_mouse_method = mouse_method;
    m_keyboard_method = keyboard_method;
    m_screencap_cost.clear();
    m_screencap_times = 0;
    m_resize_target = { 0, 0 };
    m_window_size_checked = false;

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
        if (m_auto_resize_window && !resize_window_for_capture(image)) {
            Log.error("Failed to adjust Win32 window to a supported resolution");
            callback(
                AsstMsg::ConnectionInfo,
                json::object {
                    { "uuid", m_uuid },
                    { "what", "UnsupportedResolution" },
                    { "details",
                      json::object {
                          { "width", image.cols },
                          { "height", image.rows },
                          { "auto_resize_failed", true },
                          { "borderless", m_hwnd && is_borderless_fullscreen(static_cast<HWND>(m_hwnd)) },
                      } },
                });
            return false;
        }
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
    return screencap_impl(image_payload, true);
}

bool Win32Controller::screencap_for_preview(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    return screencap_impl(image_payload, false);
}

bool Win32Controller::screencap_impl(cv::Mat& image_payload, bool move_cursor)
{
    LogTraceFunction;

    if (move_cursor && m_auto_resize_window && !ensure_window_size()) {
        return false;
    }
    // 截图前把鼠标移走，避免光标出现在截图中影响识别
    // AnchoredTouch 注入合成触控，全程不移动真实光标，游戏自绘光标也不随合成触控移动，
    // 挪光标逻辑对其无意义，且底层 touch_move 在无活动接触点时会直接失败，整体跳过
    POINT original_cursor_pos = { 0, 0 };
    bool cursor_pos_saved = false;
    bool input_blocked = false;
    // preview 截图（move_cursor == false）与 AnchoredTouch 都不需要挪动窗口/光标，整体跳过
    if (move_cursor && m_screen_size.second > 0 && (m_mouse_method & Win32Input::AnchoredTouch) == 0) {
        const bool with_window_pos =
            (m_mouse_method & (Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos)) != 0;
        // 仅 WithCursorPos 两种方式挪的是真实光标；Seize 本就强制接管鼠标，纯消息模式不动真实光标
        const bool moves_real_cursor =
            (m_main_screen_recognition || !with_window_pos) &&
            (m_mouse_method & (Win32Input::SendMessageWithCursorPos | Win32Input::PostMessageWithCursorPos)) != 0;
        if (moves_real_cursor) {
            // 挪鼠标是孤立 touch_move，没有底层 touch_down 的 BlockInput 保护，这里补上：
            // 阻塞期间用户输入不产生事件，挪动与还原的写入不会被硬件移动竞争覆盖，与底层触控的还原同机制
            input_blocked = BlockInput(TRUE) != 0;
            cursor_pos_saved = GetCursorPos(&original_cursor_pos);
            Log.trace("Screencap saves cursor position:", original_cursor_pos.x, ",", original_cursor_pos.y);
        }
        if (m_main_screen_recognition) {
            // 主界面情况下鼠标移动到窗口中心，等待主界面的视差动画，300ms
            unit_touch_move(0, m_screen_size.first / 2, m_screen_size.second / 2, 0);
            if (with_window_pos) {
                unit_touch_up(0);
            }
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
        else {
            unit_touch_move(0, 0, m_screen_size.second - 1, 0);
            // 游戏自绘光标跟随真实光标，渲染存在帧延迟，等待其画到挪动终点后再截图，避免光标被截进识别区
            std::this_thread::sleep_for(std::chrono::milliseconds(34));
        }
    }

    const auto start_time = std::chrono::steady_clock::now();
    const bool ret = unit_screencap(image_payload);
    const auto cost =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    if (cursor_pos_saved) {
        if (!SetCursorPos(original_cursor_pos.x, original_cursor_pos.y)) {
            Log.error("Failed to restore cursor position after screencap, last_error:", GetLastError());
        }
    }

    if (input_blocked) {
        BlockInput(FALSE);
    }

    if (!ret) {
        return false;
    }

    if (move_cursor && m_auto_resize_window && m_window_resized &&
        (image_payload.cols != m_resize_target.first || image_payload.rows != m_resize_target.second)) {
        LogWarn << "Win32 window capture size changed after adjustment"
                << " actual:" << image_payload.cols << "x" << image_payload.rows << " target:" << m_resize_target.first
                << "x" << m_resize_target.second;
        if (!resize_window_for_capture(image_payload)) {
            return false;
        }
    }

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    const bool is_first_screencap = m_screencap_cost.empty();
    m_screencap_cost.emplace_back(cost);
    if (m_screencap_cost.size() > 30) {
        m_screencap_cost.pop_front();
    }
    m_screencap_times = (m_screencap_times + 1) % 10;

    if (is_first_screencap) {
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "FastestWayToScreencap" },
            { "details",
              json::object {
                  { "method", get_win32_screencap_method_name(m_screencap_method) },
                  { "cost", cost },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    if (is_first_screencap || m_screencap_times == 0) {
        const auto [min_cost, max_cost] = std::ranges::minmax(m_screencap_cost);
        const auto avg_cost = std::accumulate(m_screencap_cost.begin(), m_screencap_cost.end(), 0LL) /
                              static_cast<long long>(m_screencap_cost.size());

        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapCost" },
            { "details",
              json::object {
                  { "min", min_cost },
                  { "avg", avg_cost },
                  { "max", max_cost },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    return true;
}

void Win32Controller::set_auto_resize_window(bool enable) noexcept
{
    m_auto_resize_window = enable;
}

bool Win32Controller::ensure_window_size()
{
    if (!m_auto_resize_window || m_window_size_checked) {
        return true;
    }

    cv::Mat image;
    if (!unit_screencap(image)) {
        LogError << "Failed to capture Win32 window before checking its resolution";
        return false;
    }

    return resize_window_for_capture(image);
}

bool Win32Controller::resize_window_for_capture(cv::Mat& image)
{
    constexpr int AspectWidth = 16;
    constexpr int AspectHeight = 9;
    const auto is_supported = [](int width, int height) {
        return width >= WindowWidthDefault && height >= WindowHeightDefault &&
               static_cast<int64_t>(width) * AspectHeight == static_cast<int64_t>(height) * AspectWidth;
    };

    if (m_resize_target == std::pair<int, int> { 0, 0 } && is_supported(image.cols, image.rows)) {
        m_screen_size = { image.cols, image.rows };
        m_window_size_checked = true;
        return true;
    }

    if (!m_hwnd) {
        return false;
    }
    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!IsWindow(hwnd)) {
        return false;
    }

    const auto style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));
    if (is_borderless_fullscreen(hwnd)) {
        LogError << "Cannot resize a borderless Win32 window; switch the game to windowed mode first"
                 << " hwnd:" << m_hwnd << " capture:" << image.cols << "x" << image.rows;
        return false;
    }

    const auto ex_style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_EXSTYLE));
    const BOOL has_menu = GetMenu(hwnd) != nullptr;
    const UINT dpi = std::max(GetDpiForWindow(hwnd), static_cast<UINT>(USER_DEFAULT_SCREEN_DPI));
    MONITORINFO monitor_info = { sizeof(MONITORINFO) };
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor || !GetMonitorInfo(monitor, &monitor_info)) {
        LogError << "Failed to get the work area for Win32 window, last_error: " << GetLastError();
        return false;
    }
    const int work_width = monitor_info.rcWork.right - monitor_info.rcWork.left;
    const int work_height = monitor_info.rcWork.bottom - monitor_info.rcWork.top;
    const auto fits_work_area = [&](const std::pair<int, int>& target) {
        RECT rect = { 0, 0, target.first, target.second };
        return AdjustWindowRectExForDpi(&rect, style, has_menu, ex_style, dpi) &&
               rect.right - rect.left <= work_width && rect.bottom - rect.top <= work_height;
    };

    if (m_resize_target == std::pair<int, int> { 0, 0 }) {
        const int height_units = std::max(80, static_cast<int>(std::lround(image.rows / 9.0)));
        const int width_units = std::max(80, static_cast<int>(std::lround(image.cols / 16.0)));
        const std::pair<int, int> target_from_height = { height_units * AspectWidth, height_units * AspectHeight };
        const std::pair<int, int> target_from_width = { width_units * AspectWidth, width_units * AspectHeight };
        const auto height_delta =
            std::abs(target_from_height.first - image.cols) + std::abs(target_from_height.second - image.rows);
        const auto width_delta =
            std::abs(target_from_width.first - image.cols) + std::abs(target_from_width.second - image.rows);
        m_resize_target = height_delta <= width_delta ? target_from_height : target_from_width;

        if (!fits_work_area(m_resize_target)) {
            constexpr std::array StandardTargets = {
                std::pair { 2560, 1440 },
                std::pair { 1920, 1080 },
                std::pair { 1600, 900 },
                std::pair { 1280, 720 },
            };
            const auto target = std::ranges::find_if(StandardTargets, [&](const auto& candidate) {
                return candidate.first <= image.cols && candidate.second <= image.rows && fits_work_area(candidate);
            });
            if (target == StandardTargets.end()) {
                LogError << "No supported Win32 window size fits the monitor work area"
                         << " capture:" << image.cols << "x" << image.rows << " work_area:" << work_width << "x"
                         << work_height;
                return false;
            }
            m_resize_target = *target;
        }

        LogInfo << "Selected Win32 resize target"
                << " capture:" << image.cols << "x" << image.rows << " target:" << m_resize_target.first << "x"
                << m_resize_target.second << " work_area:" << work_width << "x" << work_height;
    }

    save_window_position();
    if (!m_window_rect_saved) {
        LogError << "Failed to save Win32 window bounds before resizing";
        return false;
    }
    if (!m_window_resized) {
        m_original_window_placement = { sizeof(WINDOWPLACEMENT) };
        if (!GetWindowPlacement(hwnd, &m_original_window_placement)) {
            LogError << "Failed to save Win32 window placement before resizing, last_error: " << GetLastError();
            return false;
        }
    }
    m_window_resized = true;

    if (IsIconic(hwnd) || IsZoomed(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
        UpdateWindow(hwnd);
    }

    RECT client_rect = {};
    GetClientRect(hwnd, &client_rect);
    LogInfo << "Preparing to resize Win32 window"
            << " hwnd:" << m_hwnd << " borderless:0 dpi:" << dpi << " capture:" << image.cols << "x" << image.rows
            << " target:" << m_resize_target.first << "x" << m_resize_target.second
            << " window_rect:" << m_original_window_rect.left << "," << m_original_window_rect.top << ","
            << m_original_window_rect.right << "," << m_original_window_rect.bottom
            << " client_size:" << client_rect.right - client_rect.left << "x" << client_rect.bottom - client_rect.top
            << " show_cmd:" << m_original_window_placement.showCmd;

    callback(
        AsstMsg::ConnectionInfo,
        json::object {
            { "uuid", m_uuid },
            { "what", "Win32WindowResizeStart" },
            { "details",
              json::object {
                  { "width", image.cols },
                  { "height", image.rows },
                  { "target_width", m_resize_target.first },
                  { "target_height", m_resize_target.second },
              } },
        });

    RECT adjusted_rect = { 0, 0, m_resize_target.first, m_resize_target.second };
    if (!AdjustWindowRectExForDpi(&adjusted_rect, style, has_menu, ex_style, dpi)) {
        LogError << "Failed to calculate Win32 window size, last_error: " << GetLastError();
        restore_window_position();
        return false;
    }

    if (!SetWindowPos(
            hwnd,
            nullptr,
            0,
            0,
            adjusted_rect.right - adjusted_rect.left,
            adjusted_rect.bottom - adjusted_rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING)) {
        LogError << "Failed to resize Win32 window, last_error: " << GetLastError();
        restore_window_position();
        return false;
    }
    UpdateWindow(hwnd);
    for (int attempt = 0; attempt < 10; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const bool captured = unit_screencap(image);
        if (captured && image.cols == m_resize_target.first && image.rows == m_resize_target.second) {
            m_screen_size = { image.cols, image.rows };
            m_window_size_checked = true;
            RECT resized_window_rect = {};
            RECT resized_client_rect = {};
            GetWindowRect(hwnd, &resized_window_rect);
            GetClientRect(hwnd, &resized_client_rect);
            LogInfo << "Adjusted Win32 window"
                    << " capture:" << image.cols << "x" << image.rows << " window_rect:" << resized_window_rect.left
                    << "," << resized_window_rect.top << "," << resized_window_rect.right << ","
                    << resized_window_rect.bottom
                    << " client_size:" << resized_client_rect.right - resized_client_rect.left << "x"
                    << resized_client_rect.bottom - resized_client_rect.top << " attempts:" << attempt + 1;
            callback(
                AsstMsg::ConnectionInfo,
                json::object {
                    { "uuid", m_uuid },
                    { "what", "Win32WindowResizeSuccess" },
                    { "details", json::object { { "width", image.cols }, { "height", image.rows } } },
                });
            return true;
        }

        RECT current_rect = {};
        if (!captured || image.empty() || !GetWindowRect(hwnd, &current_rect)) {
            continue;
        }
        const int corrected_width = current_rect.right - current_rect.left + m_resize_target.first - image.cols;
        const int corrected_height = current_rect.bottom - current_rect.top + m_resize_target.second - image.rows;
        LogTrace << "Correcting Win32 window size"
                 << " attempt:" << attempt + 1 << " capture:" << image.cols << "x" << image.rows
                 << " outer:" << current_rect.right - current_rect.left << "x" << current_rect.bottom - current_rect.top
                 << " next:" << corrected_width << "x" << corrected_height;
        if (corrected_width <= 0 || corrected_height <= 0 ||
            !SetWindowPos(
                hwnd,
                nullptr,
                0,
                0,
                corrected_width,
                corrected_height,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS |
                    SWP_NOSENDCHANGING)) {
            continue;
        }
        UpdateWindow(hwnd);
    }

    LogError << "Win32 window capture size did not become " << m_resize_target.first << "x" << m_resize_target.second
             << ", actual: " << image.cols << "x" << image.rows;
    restore_window_position();
    return false;
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
    bool down = unit_touch_down(0, p.x, p.y, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));
    bool up = unit_touch_up(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));

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
    bool with_pause)
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
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));

    const auto& opt = Config.get_options();
    int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    // pause 的 press_esc 走底层按键注入（key down/up），无 adb 通道前置条件，故直判
    bool need_pause = with_pause;
    std::future<void> pause_future;

    auto bounds_check = [width, height](int x, int y) {
        if (width <= 0 || height <= 0) {
            return true;
        }
        return x >= 0 && x < width && y >= 0 && y < height;
    };

    // Win32 输入（如 Seize 的 SendInput）为异步注入且无内置节拍，不等待会使整段滑动在
    // 毫秒级完成，被游戏判定为点击。按绝对节拍控制：以本段滑动起点为基准，
    // 第 k 步对齐 start + k * interval_ms，调用耗时吃进预算，超时不补立即继续
    // AnchoredTouch 底层 touch_move 每步需等待一帧注入提交（约 12ms 节拍），沿用 2ms 步距
    // 会让实际耗时随步数成倍膨胀，步距与底层节拍对齐后实际耗时≈计划时长
    static constexpr int AnchoredTouchSwipeIntervalMs = 13;
    const int interval_ms =
        (m_mouse_method & Win32Input::AnchoredTouch) != 0 ? AnchoredTouchSwipeIntervalMs : SwipeIntervalMs;
    auto tick_start = std::chrono::steady_clock::now();
    int move_step = 0;
    auto move_func = [this, &tick_start, &move_step, &interval_ms](int x, int y) {
        bool ret = unit_touch_move(0, x, y, 0);
        high_res_sleep_until(tick_start + ++move_step * std::chrono::milliseconds(interval_ms));
        return ret;
    };

    auto pause_check = [&opt](int cur_x, int cur_y, int start_x, int start_y) {
        return std::sqrt(std::pow(cur_x - start_x, 2) + std::pow(cur_y - start_y, 2)) >
               opt.swipe_with_pause_required_distance;
    };

    // press_esc 走底层按键注入，耗时不可控，异步执行以免卡住滑动节拍
    auto pause_action = [this, &pause_future]() {
        pause_future = std::async(std::launch::async, [this]() { press_esc(); });
    };

    auto do_swipe = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
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
                interval_ms,
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
        return interpolate_swipe(
            _x1,
            _y1,
            _x2,
            _y2,
            _duration,
            interval_ms,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    // 中途失败也必须抬手，否则手指会一直按在屏幕上，后续操作全部失效
    if (!do_swipe(x1, y1, x2, y2, actual_duration)) {
        LogWarn << "failed during main swipe movement";
        unit_touch_up(0);
        return false;
    }

    if (extra_swipe != SwipeExtraDirection::None && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        const auto offset = extra_swipe_offset(extra_swipe, opt.minitouch_extra_swipe_dist);
        // extra 是主滑成功后的补偿段，失败不判整体失败，避免上层无谓重试
        if (!do_swipe(x2, y2, x2 + offset.x, y2 + offset.y, opt.minitouch_extra_swipe_duration)) {
            LogWarn << "failed during extra swipe movement";
        }
    }

    const bool up = unit_touch_up(0);
    // 抬起后留出间隔，为下一次输入留出手势结束的时间
    std::this_thread::sleep_for(std::chrono::milliseconds(TouchHoldMs));
    return up;
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
    case InputEvent::Type::KEY_DOWN:
        return unit_key_down(event.keycode);
    case InputEvent::Type::KEY_UP:
        return unit_key_up(event.keycode);
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

void Win32Controller::set_main_screen_recognition(bool on)
{
    m_main_screen_recognition = on;
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

bool Win32Controller::restore_window_position()
{
    LogTraceFunction;
    if (!m_hwnd) {
        return true;
    }

    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (m_window_rect_saved && m_window_resized) {
        RECT current_rect = {};
        GetWindowRect(hwnd, &current_rect);
        LogInfo << "Restoring Win32 window"
                << " hwnd:" << m_hwnd << " resized:" << m_window_resized << " current_rect:" << current_rect.left << ","
                << current_rect.top << "," << current_rect.right << "," << current_rect.bottom
                << " target_rect:" << m_original_window_rect.left << "," << m_original_window_rect.top << ","
                << m_original_window_rect.right << "," << m_original_window_rect.bottom
                << " target_show_cmd:" << m_original_window_placement.showCmd;
    }
    bool minimize_on_restore = false;
    if (IsWindow(hwnd) && (m_screencap_method & (Win32Screencap::PrintWindow | Win32Screencap::FramePool)) != 0) {
        // inactive 会取消截图模块的透明还原，需保留用户当前的最小化意图。
        const auto ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        BYTE alpha = 255;
        DWORD flags = 0;
        const bool pseudo_minimized =
            (ex_style & (WS_EX_LAYERED | WS_EX_TRANSPARENT)) == (WS_EX_LAYERED | WS_EX_TRANSPARENT) &&
            GetLayeredWindowAttributes(hwnd, nullptr, &alpha, &flags) && (flags & LWA_ALPHA) != 0 && alpha == 0;
        minimize_on_restore = IsIconic(hwnd) || pseudo_minimized;
    }

    // 先结束窗口追踪，避免把窗口移动到错误位置
    if (auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle); unit != nullptr) {
        unit->inactive();
    }
    bool restored = !m_window_rect_saved;
    const bool window_valid = IsWindow(hwnd) != 0;
    if (m_window_rect_saved && !window_valid) {
        restored = false;
    }
    else if (window_valid) {
        if (m_window_rect_saved) {
            if (m_window_resized) {
                restored = SetWindowPlacement(hwnd, &m_original_window_placement) != 0;
                if (!restored) {
                    LogError << "Failed to restore window placement, last_error: " << GetLastError();
                }
                if (m_original_window_placement.showCmd != SW_SHOWMAXIMIZED &&
                    m_original_window_placement.showCmd != SW_SHOWMINIMIZED &&
                    m_original_window_placement.showCmd != SW_SHOWMINNOACTIVE) {
                    const bool bounds_restored = SetWindowPos(
                                                     hwnd,
                                                     nullptr,
                                                     m_original_window_rect.left,
                                                     m_original_window_rect.top,
                                                     m_original_window_rect.right - m_original_window_rect.left,
                                                     m_original_window_rect.bottom - m_original_window_rect.top,
                                                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED) != 0;
                    if (!bounds_restored) {
                        LogError << "Failed to restore window bounds, last_error: " << GetLastError();
                    }
                    restored = restored && bounds_restored;
                }
            }
            else {
                restored = SetWindowPos(
                               hwnd,
                               nullptr,
                               m_original_window_rect.left,
                               m_original_window_rect.top,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != 0;
                if (!restored) {
                    LogError << "Failed to restore window position, last_error: " << GetLastError();
                }
            }
        }
        if (minimize_on_restore) {
            ShowWindow(hwnd, SW_SHOWMINNOACTIVE);
        }
        if (restored && m_window_rect_saved && m_window_resized) {
            RECT restored_rect = {};
            RECT restored_client_rect = {};
            WINDOWPLACEMENT restored_placement = { sizeof(WINDOWPLACEMENT) };
            GetWindowRect(hwnd, &restored_rect);
            GetClientRect(hwnd, &restored_client_rect);
            GetWindowPlacement(hwnd, &restored_placement);
            LogInfo << "Restored Win32 window"
                    << " window_rect:" << restored_rect.left << "," << restored_rect.top << "," << restored_rect.right
                    << "," << restored_rect.bottom << " show_cmd:" << restored_placement.showCmd;
            callback(
                AsstMsg::ConnectionInfo,
                json::object {
                    { "uuid", m_uuid },
                    { "what", "Win32WindowResizeRestored" },
                    { "details",
                      json::object {
                          { "width", restored_client_rect.right - restored_client_rect.left },
                          { "height", restored_client_rect.bottom - restored_client_rect.top },
                      } },
                });
        }
    }
    if (restored) {
        if (m_window_resized) {
            m_window_size_checked = false;
        }
        m_window_rect_saved = false;
        m_window_resized = false;
        m_resize_target = { 0, 0 };
    }
    return restored;
}

ControlFeat::Feat Win32Controller::support_features() const noexcept
{
    // Win32 的 touch 坐标即窗口客户区原生坐标，无 minitouch 式的 max_x/max_y 换算；
    // 暂停走底层按键注入，两个特性都能完整支持
    return ControlFeat::PRECISE_SWIPE | ControlFeat::SWIPE_WITH_PAUSE;
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

bool Win32Controller::prepare_window_for_input()
{
    // 后台截图会先把最小化窗口透明还原。必须在移动窗口或发送激活消息之前完成，
    // 否则输入可能先恢复窗口，截图侧就无法感知用户的最小化意图。
    if ((m_screencap_method & (Win32Screencap::PrintWindow | Win32Screencap::FramePool)) != 0 &&
        IsIconic(static_cast<HWND>(m_hwnd))) {
        LogInfo << "Preparing minimized window before input";
        cv::Mat image;
        if (!unit_screencap(image)) {
            LogError << "Failed to prepare minimized window for input";
            return false;
        }
    }
    if ((m_mouse_method & (Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos)) != 0 &&
        (m_screencap_method &
         (Win32Screencap::ScreenDC | Win32Screencap::DXGI_DesktopDup | Win32Screencap::DXGI_DesktopDup_Window)) == 0) {
        // 停止任务后控制器可以复用，下次输入前重新记录位置。
        save_window_position();
    }
    return true;
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
    return prepare_window_for_input() && unit->click(x, y);
}

bool Win32Controller::unit_swipe(int x1, int y1, int x2, int y2, int duration)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return prepare_window_for_input() && unit->swipe(x1, y1, x2, y2, duration);
}

bool Win32Controller::unit_touch_down(int contact, int x, int y, int pressure)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return prepare_window_for_input() && unit->touch_down(contact, x, y, pressure);
}

bool Win32Controller::unit_touch_move(int contact, int x, int y, int pressure)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return prepare_window_for_input() && unit->touch_move(contact, x, y, pressure);
}

bool Win32Controller::unit_touch_up(int contact)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    const bool prepared = prepare_window_for_input();
    // 准备失败也要抬手，避免未结束的手势影响后续操作。
    const bool released = unit->touch_up(contact);
    return prepared && released;
}

bool Win32Controller::unit_input_text(const std::string& text)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    return prepare_window_for_input() && unit->input_text(text);
}

bool Win32Controller::unit_key_down(int key)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }

    return prepare_window_for_input() && unit->key_down(key);
}

bool Win32Controller::unit_key_up(int key)
{
    auto* unit = static_cast<MaaFwControlUnitAPI*>(m_unit_handle);
    if (!unit) {
        return false;
    }
    const bool prepared = prepare_window_for_input();
    const bool released = unit->key_up(key);
    return prepared && released;
}

bool Win32Controller::unit_click_key(int key)
{
    // MaaWin32ControlUnit 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick
    // 需要使用 key_down/key_up 替代 click_key
    if (!unit_key_down(key)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return unit_key_up(key);
}
} // namespace asst

#endif // _WIN32
