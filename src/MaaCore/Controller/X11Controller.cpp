#if defined(__linux__) && !defined(__ANDROID__)

#include "X11Controller.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

bool asst::X11Controller::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address,
    const std::string& config [[maybe_unused]])
{
    if (m_display != nullptr) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
    set_window(None);

    XSetErrorHandler([](Display* display, XErrorEvent* e) -> int {
        char buf[1024];
        XGetErrorText(display, e->error_code, buf, sizeof(buf));
        Log.error("X11:", std::string(buf));
        return True;
    });

    m_display = XOpenDisplay(nullptr);
    if (m_display == nullptr) {
        Log.error("Failed to open X display");
        set_window(None);
        return false;
    }

    // e.g. address = "0x3e00003"
    Window w { };
    try {
        w = static_cast<Window>(std::stoull(address, nullptr, 16));
    }
    catch (...) {
        Log.error("Invalid window XID", address);
        return false;
    }

    if (w == 0) {
        Log.error("Invalid window XID", address, "(", w, ")");
        return false;
    }

    set_window(w);
    Log.info("Fetching info from window", m_uuid);
    if (auto attr = get_xwin_attrib(); attr) {
        Log.info("Connected to X display, window, ", "size:", attr->width, "x", attr->height, "rescaling");
        // likely has no effect, you still have to set the resolution in game manually
        if (XMoveResizeWindow(m_display, m_window, attr->x, attr->y, 1280, 720) == False) {
            Log.info("Failed to rescale the window");
            set_window(None);
            return false;
        }
        XFlush(m_display);
        return true;
    }
    set_window(None);
    return false;
}

bool asst::X11Controller::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    using enum InputEvent::Type;

    if (auto attrib = get_xwin_attrib(); attrib) {
        XImage* ximg = XGetImage(m_display, m_window, 0, 0, attrib->width, attrib->height, AllPlanes, ZPixmap);
        if (ximg != nullptr) {
            ximg_to_cv(image_payload, ximg);
            XDestroyImage(ximg);
            return true;
        }
        Log.error("Failed to call XGetImage");
    }
    return false;
}

bool asst::X11Controller::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    const auto [x1, y1] = p1;
    const auto [x2, y2] = p2;

    using enum InputEvent::Type;

    static constexpr int interval = 2;

    auto move_func = [this](int x, int y) {
        inject_input_event(InputEvent { .type = WAIT_MS, .milisec = interval });
        return inject_input_event(InputEvent { .type = TOUCH_MOVE, .point = { x, y } });
    };

    bool res = true;

    res &= inject_input_event(InputEvent { .type = TOUCH_DOWN, .point = p1 });
    res &= interpolate_swipe(x1, y1, x2, y2, duration, interval, slope_in, slope_out, move_func);

    Point end = p2;

    if (extra_swipe) {
        const auto& opt = Config.get_options();
        inject_input_event(InputEvent { .type = WAIT_MS, .milisec = opt.minitouch_swipe_extra_end_delay });
        end.y -= opt.minitouch_extra_swipe_dist;
        res &= interpolate_swipe(x2, y2, end.x, end.y, opt.minitouch_extra_swipe_duration, interval, 0, 0, move_func);
    }

    res &= inject_input_event(InputEvent { .type = TOUCH_UP, .point = end });

    park_cursor();

    return res;
}

bool asst::X11Controller::inject_input_event(const InputEvent& event)
{
    if (m_display == nullptr || m_window == None) {
        return false;
    }

    XButtonEvent bev {
        .display = m_display,
        .window = m_window,
        .root = DefaultRootWindow(m_display),
        .subwindow = None,
        .time = CurrentTime,
        .x = event.point.x,
        .y = event.point.y,
        .x_root = event.point.x,
        .y_root = event.point.y,
        .state = 0,
        .button = Button1,
        .same_screen = True,
    };

    XMotionEvent mev {
        .type = MotionNotify,
        .display = m_display,
        .window = m_window,
        .root = DefaultRootWindow(m_display),
        .subwindow = None,
        .time = CurrentTime,
        .x = event.point.x,
        .y = event.point.y,
        .x_root = event.point.x,
        .y_root = event.point.y,
        .state = 0,
        .same_screen = True,
    };

    using enum InputEvent::Type;

    switch (event.type) {
    case TOUCH_DOWN:
        bev.type = ButtonPress;
        return XSendEvent(bev.display, bev.window, True, ButtonPressMask, reinterpret_cast<XEvent*>(&bev)) == True &&
               XFlush(bev.display) == True;
    case TOUCH_UP:
        bev.type = ButtonRelease;
        return XSendEvent(bev.display, bev.window, True, ButtonReleaseMask, reinterpret_cast<XEvent*>(&bev)) == True &&
               XFlush(bev.display) == True;
    case TOUCH_MOVE:
        return XSendEvent(bev.display, bev.window, True, PointerMotionMask, reinterpret_cast<XEvent*>(&mev)) == True &&
               XFlush(bev.display) == True;
    case TOUCH_RESET:
        return true;
    case KEY_DOWN:
        // return send_keysym(static_cast<KeySym>(event.keycode), KeyPress);
        return false;
    case KEY_UP:
        // return send_keysym(static_cast<KeySym>(event.keycode), KeyRelease);
        return false;
    case WAIT_MS:
        std::this_thread::sleep_for(std::chrono::milliseconds(event.milisec));
        return true;
    case COMMIT:
        return XFlush(bev.display) == True;
    case UNKNOWN:
        Log.error("Unknown input event type");
        return false;
    }

    return false;
}

bool asst::X11Controller::focus_window() const
{
    Atom atom = XInternAtom(m_display, "_NET_ACTIVE_WINDOW", False);

    XEvent ev { };
    ev.xclient.type = ClientMessage;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.window = m_window;
    ev.xclient.message_type = atom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 1;
    ev.xclient.data.l[1] = CurrentTime;
    ev.xclient.data.l[2] = 0;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;

    Status s = XSendEvent(
        m_display,
        DefaultRootWindow(m_display),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &ev);
    return s == True;
}

bool asst::X11Controller::send_keysym(KeySym keysym, int type) const
{
    if (m_display == nullptr) {
        return false;
    }

    focus_window() && XFlush(m_display) == True;

    KeyCode keycode = XKeysymToKeycode(m_display, keysym);

    XKeyEvent kev {
        .type = type,
        .display = m_display,
        .window = m_window,
        .root = DefaultRootWindow(m_display),
        .subwindow = None,
        .time = CurrentTime,
        .x = 0,
        .y = 0,
        .x_root = 0,
        .y_root = 0,
        .state = 0,
        .keycode = keycode,
        .same_screen = True,
    };

    const auto event_mask = type == KeyPress ? KeyPressMask : KeyReleaseMask;
    return XSendEvent(m_display, m_window, True, event_mask, reinterpret_cast<XEvent*>(&kev)) == True &&
           XFlush(m_display) == True;
}

#endif
