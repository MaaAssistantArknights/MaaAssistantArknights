#ifdef _WIN32

#include "Win32Controller.h"

#include <algorithm>
#include <chrono>
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
    if (method & Win32Screencap::FramePool) {
        return "FramePool";
    }
    if (method & Win32Screencap::DXGI_DesktopDup) {
        return "DXGI_DesktopDup";
    }
    if (method & Win32Screencap::DXGI_DesktopDup_Window) {
        return "DXGI_DesktopDup_Window";
    }
    if (method & Win32Screencap::PrintWindow) {
        return "PrintWindow";
    }
    if (method & Win32Screencap::ScreenDC) {
        return "ScreenDC";
    }
    return "Win32";
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

    // 截图前把鼠标移走，避免光标出现在截图中影响识别
    if (m_screen_size.second > 0) {
        const bool with_window_pos =
            (m_mouse_method & (Win32Input::SendMessageWithWindowPos | Win32Input::PostMessageWithWindowPos)) != 0;
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
        }
    }

    auto start_time = std::chrono::steady_clock::now();
    if (!unit_screencap(image_payload)) {
        return false;
    }

    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

    if (m_screen_size.first == 0) {
        m_screen_size = { image_payload.cols, image_payload.rows };
    }

    m_screencap_cost.emplace_back(cost.count());
    ++m_screencap_times;
    if (m_screencap_cost.size() > 30) {
        m_screencap_cost.pop_front();
    }

    if (!m_screencap_initial_cost_emitted) {
        m_screencap_initial_cost_emitted = true;
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "FastestWayToScreencap" },
            { "details",
              json::object {
                  { "method", get_win32_screencap_method_name(m_screencap_method) },
                  { "cost", cost.count() },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    if (m_screencap_times == 1 || m_screencap_times % 10 == 0) {
        long long min_cost = *std::min_element(m_screencap_cost.begin(), m_screencap_cost.end());
        long long max_cost = *std::max_element(m_screencap_cost.begin(), m_screencap_cost.end());
        long long avg_cost = std::accumulate(m_screencap_cost.begin(), m_screencap_cost.end(), 0ll) /
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
