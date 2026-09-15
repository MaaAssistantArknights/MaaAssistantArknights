#pragma once

#ifdef _WIN32

#include <memory>
#include <optional>
#include <string>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/AsstMsg.h"
#include "CaptureInterest.hpp"
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

    void restore_window_position();

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
        SwipeExtraDirection extra_swipe = SwipeExtraDirection::None,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;
    virtual void set_capture_hint(const CaptureHint& hint) override;
    virtual ControlFeat::Feat support_features() const noexcept override;

    virtual std::pair<int, int> get_screen_res() const noexcept override;

private:
    void callback(AsstMsg msg, const json::value& details);
    // 记录窗口当前位置，任务结束是恢复
    void save_window_position();

    // 真实光标相对截图画面的位置；三态语义见 capture_interest::CursorLocateState
    struct CursorLocation
    {
        capture_interest::CursorLocateState state = capture_interest::CursorLocateState::Unknown;
        Point frame_pos {}; // 仅 state == Inside 时有效
    };

    CursorLocation locate_cursor() const;
    // 本次截图前是否需要挪开真实光标。拿不准时返回 true（与门控引入前一致）
    bool need_relocate(const CursorLocation& location) const;
    // 诊断：本次停靠是否落到了预期位置（DPI / 多显示器下底层坐标换算可能与预期不同）
    bool parked_at_expected_position(const POINT& parked) const;
    // 截图前发现停靠被用户动作打断时记一条 trace（只记录，不重停靠）
    void log_cursor_escaped(const POINT& parked) const;
    // 截图后把光标还给用户：只有它仍停在停靠点（没人动过）才还原
    void restore_cursor_if_untouched(const POINT& original, const POINT& parked);

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

    CaptureHint m_capture_hint;
    RECT m_original_window_rect = { 0, 0, 0, 0 };
    bool m_window_rect_saved = false;
    bool m_park_mismatch_logged = false;
};
} // namespace asst

#endif // _WIN32
