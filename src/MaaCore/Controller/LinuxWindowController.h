#pragma once

#if !defined(_WIN32) && ASST_WITH_X11

#include <memory>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

// X11 头文件定义了 Status/True/False/None/Bool/Success/Always 等宏，
// 与 MAA 代码（如 asst::Status）冲突，在所有 X11 头文件之后统一解除定义
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef None
#undef None
#endif
#ifdef Success
#undef Success
#endif
#ifdef Always
#undef Always
#endif

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
class Assistant;

// Linux (X11) 窗口控制器：绑定到指定标题的窗口，通过 X11 合成事件（XSendEvent）进行后台输入。
// 与 Win32Controller 类似，输入不移动光标、不抢占焦点；截图通过 XGetImage 获取窗口内容。
class LinuxWindowController : public ControllerAPI, private InstHelper
{
public:
    LinuxWindowController(const AsstCallback& callback, Assistant* inst);
    virtual ~LinuxWindowController() override;

    LinuxWindowController(const LinuxWindowController&) = delete;
    LinuxWindowController& operator=(const LinuxWindowController&) = delete;
    LinuxWindowController(LinuxWindowController&&) = delete;
    LinuxWindowController& operator=(LinuxWindowController&&) = delete;

    // 绑定到窗口（替代 connect）
    // window_name: 目标窗口标题（完全匹配）
    // focus_for_keys: 发送按键前是否将输入焦点切换到目标窗口（Unity 等游戏仅在窗口聚焦时响应键盘）
    bool attach(const std::string& window_name, bool focus_for_keys);

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

private:
    bool find_window(const std::string& window_name);
    std::string get_window_name(Window window) const;
    bool refresh_geometry();
    bool capture_window(cv::Mat& image_payload);

    void send_button(int type, int x, int y, unsigned int button);
    void send_motion(int x, int y, unsigned int state);
    void send_key(KeySym keysym, bool press);
    bool ensure_focus();

private:
    Display* m_display = nullptr;
    Window m_window = 0;
    int m_width = 0;
    int m_height = 0;

    bool m_focus_for_keys = false;
    bool m_inited = false;
    std::string m_uuid;
};
} // namespace asst

#endif // !defined(_WIN32) && ASST_WITH_X11
