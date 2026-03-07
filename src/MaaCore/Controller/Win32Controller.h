#pragma once

#ifdef _WIN32

#include <memory>
#include <string>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "Win32ControlUnitLoader.h"

namespace asst
{
class Assistant;

class Win32Controller : public ControllerAPI, private InstHelper
{
public:
    Win32Controller(const AsstCallback& callback, Assistant* inst);
    virtual ~Win32Controller() override;

    Win32Controller(const Win32Controller&) = delete;
    Win32Controller& operator=(const Win32Controller&) = delete;
    Win32Controller(Win32Controller&&) = delete;
    Win32Controller& operator=(Win32Controller&&) = delete;

    // 绑定到窗口（替代 connect）
    bool attach(
        void* hwnd,
        Win32ScreencapMethod screencap_method,
        Win32InputMethod mouse_method,
        Win32InputMethod keyboard_method);

public: // ControllerAPI 接口
    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override { return 0; }

    virtual size_t get_version() const noexcept override { return 0; }

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;
    virtual bool input(const std::string& text) override;
    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;
    virtual ControlFeat::Feat support_features() const noexcept override;

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    virtual void move_cursor_out_of_view() noexcept override;

private:
    void callback(AsstMsg msg, const json::value& details);

    // 封装 MaaWin32ControlUnit 的调用
    bool unit_connect();
    bool unit_screencap(cv::Mat& image);
    bool unit_click(int x, int y);
    bool unit_swipe(int x1, int y1, int x2, int y2, int duration);
    bool unit_touch_down(int contact, int x, int y, int pressure);
    bool unit_touch_move(int contact, int x, int y, int pressure);
    bool unit_touch_up(int contact);
    bool unit_input_text(const std::string& text);
    bool unit_click_key(int key);

private:
    static constexpr int DefaultSwipeDelay = 10; // ms

    AsstCallback m_callback = nullptr;
    std::unique_ptr<Win32ControlUnitLoader> m_loader;
    void* m_unit_handle = nullptr;
    void* m_hwnd = nullptr;

    bool m_inited = false;
    std::string m_uuid;
    std::pair<int, int> m_screen_size = { 0, 0 };

    Win32ScreencapMethod m_screencap_method = Win32Screencap::None;
    Win32InputMethod m_mouse_method = Win32Input::None;
    Win32InputMethod m_keyboard_method = Win32Input::None;
};
} // namespace asst

#endif // _WIN32
