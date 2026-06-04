#pragma once

#if defined(__linux__) && !defined(__ANDROID__)

#include <optional>

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Platform/PlatformIO.h"
#include "SwipeHelper.hpp"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"

extern "C"
{
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
}

namespace asst
{

struct X11Controller : public ControllerAPI, protected InstHelper
{
public:
    X11Controller(AsstCallback callback, Assistant* inst, PlatformType) :
        InstHelper(inst),
        m_callback(std::move(callback))
    {
    }

    virtual ~X11Controller() override { XCloseDisplay(m_display); };

    X11Controller(const X11Controller&) = delete;
    X11Controller(X11Controller&&) = delete;
    X11Controller& operator=(const X11Controller&) = delete;
    X11Controller& operator=(X11Controller&&) = delete;

    virtual bool connect(
        const std::string& adb_path [[maybe_unused]],
        const std::string& address,
        const std::string& config [[maybe_unused]]) override;

    virtual bool inited() const noexcept override { return m_display != nullptr && m_window != None; }

    virtual const std::string& get_uuid() const override { return m_uuid; }

    virtual size_t get_pipe_data_size() const noexcept override { return 0; }

    virtual size_t get_version() const noexcept override { return 0; }

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]] = false) override;

    virtual bool start_game(const std::string&) override
    {
        Log.warn("start_game is not supported on X11Controller");
        return false;
    }

    virtual bool stop_game(const std::string& client_type [[maybe_unused]]) override
    {
        // TODO: Implement this pure virtual method.
        return false;
    }

    virtual bool click(const Point& p) override
    {
        if (m_display == nullptr || m_window == None) {
            return false;
        }

        using enum InputEvent::Type;

        bool result = true;

        result &= inject_input_event(InputEvent { .type = TOUCH_DOWN, .point = p });

        // result &= inject_input_event(InputEvent { .type = WAIT_MS, .milisec = 10 });

        result &= inject_input_event(InputEvent { .type = TOUCH_UP, .point = p });

        park_cursor();

        return result;
    }

    virtual bool input(const std::string&) override
    {
        // TODO
        return false;
    }

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override
    {
        if (m_display == nullptr || m_window == None) {
            return false;
        }

        const bool result = send_keysym(XK_Escape, KeyPress) && send_keysym(XK_Escape, KeyRelease);
        return result;
    }

    virtual ControlFeat::Feat support_features() const noexcept override { return ControlFeat::PRECISE_SWIPE; }

    virtual std::pair<int, int> get_screen_res() const noexcept override
    {
        if (auto attrib = get_xwin_attrib(); attrib) {
            return { attrib->width, attrib->height };
        }
        return { 0, 0 };
    }

protected:
    std::optional<XWindowAttributes> get_xwin_attrib() const
    {
        if (m_display == nullptr || m_window == None) {
            return std::nullopt;
        }

        XWindowAttributes attr;
        if (XGetWindowAttributes(m_display, m_window, &attr) == False) {
            Log.error("Failed to get window attributes for window", m_window);
            // set_window(None);
            return std::nullopt;
        }
        m_max_x = attr.width;
        m_max_y = attr.height;
        return attr;
    }

    static void ximg_to_cv(cv::Mat& image_payload, const XImage* ximg)
    {
        if (ximg == nullptr) {
            return;
        }
        if (ximg->bits_per_pixel != 32 || ximg->format != ZPixmap) {
            Log.error("Unsupported image format");
            image_payload.release();
            return;
        }

        cv::Mat view(ximg->height, ximg->width, CV_8UC4, ximg->data, ximg->bytes_per_line);
        cv::cvtColor(view, image_payload, cv::COLOR_RGBA2RGB);
    }

    bool focus_window() const;

private:
    AsstCallback m_callback = nullptr;

    void set_window(Window w)
    {
        m_window = w;
        m_uuid = std::format("{:#x}", w);
    }

    bool park_cursor()
    {
        Point point = { m_max_x - 10, m_max_y - 10 };
        return inject_input_event(
            InputEvent {
                .type = InputEvent::Type::TOUCH_MOVE,
                .point = point,
            });
    }

    bool send_keysym(KeySym keysym, int type) const;

    Display* m_display = nullptr;
    Window m_window = None;
    std::string m_uuid;
    mutable int m_max_x { };
    mutable int m_max_y { };
};

} // namespace asst

#endif
