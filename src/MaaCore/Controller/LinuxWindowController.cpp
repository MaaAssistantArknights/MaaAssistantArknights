#if !defined(_WIN32) && ASST_WITH_X11

#include "LinuxWindowController.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>

#include <X11/keysym.h>

#include "Config/GeneralConfig.h"
#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"

namespace asst
{
LinuxWindowController::LinuxWindowController(const AsstCallback& callback [[maybe_unused]], Assistant* inst) :
    InstHelper(inst)
{
    LogTraceFunction;
}

LinuxWindowController::~LinuxWindowController()
{
    LogTraceFunction;

    if (m_display != nullptr) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

bool LinuxWindowController::attach(const std::string& window_name, bool focus_for_keys)
{
    LogTraceFunction;

    m_inited = false;
    m_focus_for_keys = focus_for_keys;

    if (m_display != nullptr) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }

    m_display = XOpenDisplay(nullptr);
    if (m_display == nullptr) {
        Log.error("Failed to open X display");
        return false;
    }

    // 静默 X11 错误，避免窗口被销毁等竞态打印大量错误
    XSetErrorHandler([](Display*, XErrorEvent*) -> int { return 0; });

    if (!find_window(window_name)) {
        Log.error("Failed to find window:", window_name);
        return false;
    }

    if (!refresh_geometry()) {
        Log.error("Failed to get window geometry");
        return false;
    }

    // 尝试截图
    cv::Mat image;
    if (!capture_window(image)) {
        Log.error("Failed to capture window");
        return false;
    }

    std::stringstream ss;
    ss << std::hex << m_window;
    m_uuid = ss.str();

    m_inited = true;
    return true;
}

bool LinuxWindowController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config [[maybe_unused]])
{
    Log.error("LinuxWindowController does not support connect(), use attach() instead");
    return false;
}

bool LinuxWindowController::inited() const noexcept
{
    return m_inited && m_display != nullptr && m_window != 0;
}

const std::string& LinuxWindowController::get_uuid() const
{
    return m_uuid;
}

bool LinuxWindowController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;

    if (!inited()) {
        return false;
    }

    return capture_window(image_payload);
}

bool LinuxWindowController::capture_window(cv::Mat& image_payload)
{
    if (!refresh_geometry()) {
        return false;
    }

    XImage* image = XGetImage(m_display, m_window, 0, 0, m_width, m_height, AllPlanes, ZPixmap);
    if (image == nullptr) {
        Log.error("XGetImage failed");
        return false;
    }

    cv::Mat mat(m_height, m_width, CV_8UC3);
    const unsigned long r_mask = image->red_mask;
    const unsigned long g_mask = image->green_mask;
    const unsigned long b_mask = image->blue_mask;

    // 常见 32bpp/24bpp BGRA/BGR（XWayland 默认，小端序），直接按字节拷贝
    const int bytes_per_pixel = image->bits_per_pixel / 8;
    const bool fast_path = image->byte_order == LSBFirst && bytes_per_pixel >= 3 &&
                           r_mask == 0xFF0000 && g_mask == 0xFF00 && b_mask == 0xFF;

    if (fast_path) {
        for (int y = 0; y < m_height; ++y) {
            const uchar* src = reinterpret_cast<const uchar*>(image->data + y * image->bytes_per_line);
            uchar* dst = mat.ptr<uchar>(y);
            for (int x = 0; x < m_width; ++x) {
                dst[x * 3] = src[x * bytes_per_pixel + 0];
                dst[x * 3 + 1] = src[x * bytes_per_pixel + 1];
                dst[x * 3 + 2] = src[x * bytes_per_pixel + 2];
            }
        }
    }
    else {
        const int r_shift = r_mask ? __builtin_ctzl(r_mask) : 0;
        const int g_shift = g_mask ? __builtin_ctzl(g_mask) : 0;
        const int b_shift = b_mask ? __builtin_ctzl(b_mask) : 0;
        for (int y = 0; y < m_height; ++y) {
            uchar* dst = mat.ptr<uchar>(y);
            for (int x = 0; x < m_width; ++x) {
                unsigned long pixel = XGetPixel(image, x, y);
                dst[x * 3] = static_cast<uchar>((pixel & b_mask) >> b_shift);
                dst[x * 3 + 1] = static_cast<uchar>((pixel & g_mask) >> g_shift);
                dst[x * 3 + 2] = static_cast<uchar>((pixel & r_mask) >> r_shift);
            }
        }
    }

    XDestroyImage(image);
    image_payload = std::move(mat);
    return true;
}

bool LinuxWindowController::start_game(const std::string& client_type [[maybe_unused]])
{
    Log.warn("start_game is not supported on LinuxWindowController");
    return false;
}

bool LinuxWindowController::stop_game(const std::string& client_type [[maybe_unused]])
{
    Log.warn("stop_game is not supported on LinuxWindowController");
    return false;
}

bool LinuxWindowController::click(const Point& p)
{
    LogTraceFunction;

    if (!inited()) {
        return false;
    }

    // 与 Win32Controller 对齐：down/up 之间保持一小段时间，游戏才能识别为完整点击
    constexpr int click_delay_ms = 50;

    send_button(ButtonPress, p.x, p.y, Button1);
    std::this_thread::sleep_for(std::chrono::milliseconds(click_delay_ms));
    send_button(ButtonRelease, p.x, p.y, Button1);
    std::this_thread::sleep_for(std::chrono::milliseconds(click_delay_ms));
    return true;
}

bool LinuxWindowController::input(const std::string& text)
{
    LogTraceFunction;

    if (!inited()) {
        return false;
    }

    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7e) {
            continue; // 仅支持可打印 ASCII
        }
        char buf[2] = { static_cast<char>(c), '\0' };
        KeySym keysym = XStringToKeysym(buf);
        if (keysym == NoSymbol) {
            continue;
        }
        const bool need_shift = std::isupper(c) != 0 || std::strchr("~!@#$%^&*()_+{}|:\"<>?", c) != nullptr;
        if (need_shift) {
            send_key(XK_Shift_L, true);
        }
        send_key(keysym, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        send_key(keysym, false);
        if (need_shift) {
            send_key(XK_Shift_L, false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return true;
}

bool LinuxWindowController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    LogTraceFunction;

    if (!inited()) {
        return false;
    }

    int x1 = p1.x;
    int y1 = p1.y;
    const int x2 = p2.x;
    const int y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (m_width > 0 && m_height > 0) {
        if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
            Log.warn("swipe point1 is out of range", x1, y1);
            x1 = std::clamp(x1, 0, m_width - 1);
            y1 = std::clamp(y1, 0, m_height - 1);
        }
    }

    send_button(ButtonPress, x1, y1, Button1);

    const auto& opt = Config.get_options();
    const int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    auto bounds_check = [this](int x, int y) {
        if (m_width <= 0 || m_height <= 0) {
            return true;
        }
        return x >= 0 && x <= m_width && y >= 0 && y <= m_height;
    };

    auto move_func = [this](int x, int y) {
        send_motion(x, y, Button1Mask);
        return true;
    };

    constexpr int DefaultSwipeDelay = 10; // ms

    bool ret = interpolate_swipe(
        x1, y1, x2, y2, actual_duration, DefaultSwipeDelay, slope_in, slope_out, move_func, bounds_check);

    if (ret && extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        interpolate_swipe(
            x2,
            y2,
            x2,
            y2 - opt.minitouch_extra_swipe_dist,
            opt.minitouch_extra_swipe_duration,
            DefaultSwipeDelay,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    }

    send_button(ButtonRelease, x2, y2, Button1);
    return ret;
}

bool LinuxWindowController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;

    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN: {
        unsigned int button = event.pointerId == 1 ? Button2 : (event.pointerId == 2 ? Button3 : Button1);
        send_button(ButtonPress, event.point.x, event.point.y, button);
        return true;
    }
    case InputEvent::Type::TOUCH_UP: {
        unsigned int button = event.pointerId == 1 ? Button2 : (event.pointerId == 2 ? Button3 : Button1);
        send_button(ButtonRelease, event.point.x, event.point.y, button);
        return true;
    }
    case InputEvent::Type::TOUCH_MOVE:
        send_motion(event.point.x, event.point.y, Button1Mask);
        return true;
    case InputEvent::Type::KEY_DOWN:
        send_key(static_cast<KeySym>(event.keycode), true);
        return true;
    case InputEvent::Type::KEY_UP:
        send_key(static_cast<KeySym>(event.keycode), false);
        return true;
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

bool LinuxWindowController::press_esc()
{
    LogTraceFunction;

    if (!inited()) {
        return false;
    }

    send_key(XK_Escape, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_key(XK_Escape, false);
    return true;
}

ControlFeat::Feat LinuxWindowController::support_features() const noexcept
{
    return ControlFeat::PRECISE_SWIPE;
}

std::pair<int, int> LinuxWindowController::get_screen_res() const noexcept
{
    return { m_width, m_height };
}

bool LinuxWindowController::find_window(const std::string& window_name)
{
    const Window root = DefaultRootWindow(m_display);

    Window best = 0;
    unsigned long best_area = 0;

    std::vector<Window> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        const Window current = stack.back();
        stack.pop_back();

        Window root_ret = 0;
        Window parent = 0;
        Window* children = nullptr;
        unsigned int child_count = 0;
        if (!XQueryTree(m_display, current, &root_ret, &parent, &children, &child_count)) {
            continue;
        }

        for (unsigned int i = 0; i < child_count; ++i) {
            stack.push_back(children[i]);
        }
        if (children != nullptr) {
            XFree(children);
        }

        if (current == root) {
            continue;
        }

        if (get_window_name(current) != window_name) {
            continue;
        }

        XWindowAttributes attr;
        if (!XGetWindowAttributes(m_display, current, &attr) || attr.map_state != IsViewable) {
            continue;
        }

        const unsigned long area = static_cast<unsigned long>(attr.width) * attr.height;
        if (area > best_area) {
            best_area = area;
            best = current;
        }
    }

    if (best == 0) {
        Log.error("Window not found:", window_name);
        return false;
    }

    m_window = best;
    return true;
}

std::string LinuxWindowController::get_window_name(Window window) const
{
    // 优先 _NET_WM_NAME (UTF-8)，失败则回退 WM_NAME
    const Atom utf8_string = XInternAtom(m_display, "UTF8_STRING", 0);
    const Atom net_wm_name = XInternAtom(m_display, "_NET_WM_NAME", 0);

    Atom type_ret = 0;
    int format = 0;
    unsigned long item_count = 0;
    unsigned long bytes_after = 0;
    unsigned char* prop = nullptr;
    if (XGetWindowProperty(
            m_display,
            window,
            net_wm_name,
            0,
            1024,
            0,
            utf8_string,
            &type_ret,
            &format,
            &item_count,
            &bytes_after,
            &prop) == 0 &&
        prop != nullptr) {
        std::string name(reinterpret_cast<char*>(prop), item_count);
        XFree(prop);
        if (!name.empty()) {
            return name;
        }
    }

    // 回退到 WM_NAME (STRING)
    if (XGetWindowProperty(
            m_display,
            window,
            XA_WM_NAME,
            0,
            1024,
            0,
            XA_STRING,
            &type_ret,
            &format,
            &item_count,
            &bytes_after,
            &prop) == 0 &&
        prop != nullptr) {
        std::string name(reinterpret_cast<char*>(prop), item_count);
        XFree(prop);
        if (!name.empty()) {
            return name;
        }
    }

    return {};
}

bool LinuxWindowController::refresh_geometry()
{
    XWindowAttributes attr;
    if (!XGetWindowAttributes(m_display, m_window, &attr)) {
        Log.error("XGetWindowAttributes failed");
        return false;
    }
    if (attr.map_state != IsViewable) {
        Log.error("Window is not viewable");
        return false;
    }
    m_width = attr.width;
    m_height = attr.height;
    return true;
}

void LinuxWindowController::send_button(int type, int x, int y, unsigned int button)
{
    XButtonEvent ev = {};
    ev.type = type;
    ev.display = m_display;
    ev.window = m_window;
    ev.root = DefaultRootWindow(m_display);
    ev.subwindow = 0;
    ev.time = CurrentTime;
    ev.x = x;
    ev.y = y;
    ev.x_root = 0;
    ev.y_root = 0;
    ev.same_screen = 1;
    ev.button = button;
    ev.state = (type == ButtonRelease) ? Button1Mask : 0;
    XSendEvent(
        m_display,
        m_window,
        1,
        (type == ButtonPress) ? ButtonPressMask : ButtonReleaseMask,
        reinterpret_cast<XEvent*>(&ev));
    XFlush(m_display);
}

void LinuxWindowController::send_motion(int x, int y, unsigned int state)
{
    XMotionEvent ev = {};
    ev.type = MotionNotify;
    ev.display = m_display;
    ev.window = m_window;
    ev.root = DefaultRootWindow(m_display);
    ev.subwindow = 0;
    ev.time = CurrentTime;
    ev.x = x;
    ev.y = y;
    ev.x_root = 0;
    ev.y_root = 0;
    ev.same_screen = 1;
    ev.state = state;
    ev.is_hint = NotifyNormal;
    XSendEvent(m_display, m_window, 1, PointerMotionMask, reinterpret_cast<XEvent*>(&ev));
    XFlush(m_display);
}

void LinuxWindowController::send_key(KeySym keysym, bool press)
{
    if (m_focus_for_keys) {
        ensure_focus();
    }

    KeyCode keycode = XKeysymToKeycode(m_display, keysym);
    if (keycode == 0) {
        Log.warn("No keycode for keysym:", static_cast<unsigned long>(keysym));
        return;
    }

    XKeyEvent ev = {};
    ev.type = press ? KeyPress : KeyRelease;
    ev.display = m_display;
    ev.window = m_window;
    ev.root = DefaultRootWindow(m_display);
    ev.subwindow = 0;
    ev.time = CurrentTime;
    ev.same_screen = 1;
    ev.keycode = keycode;
    ev.state = 0;
    XSendEvent(m_display, m_window, 1, press ? KeyPressMask : KeyReleaseMask, reinterpret_cast<XEvent*>(&ev));
    XFlush(m_display);
}

bool LinuxWindowController::ensure_focus()
{
    if (m_display == nullptr || m_window == 0) {
        return false;
    }
    XSetInputFocus(m_display, m_window, RevertToParent, CurrentTime);
    XFlush(m_display);
    return true;
}
} // namespace asst

#endif // !defined(_WIN32) && ASST_WITH_X11
