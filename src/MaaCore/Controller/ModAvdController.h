#pragma once

#include "MaatouchController.h"

namespace asst
{
// ModAVD 宿主侧点击：经 ModAVD 自带的 modavd_extras 库，从模拟器控制台把触控事件发到虚拟触屏，
// 客体里不用起 MaaTouch。结构参照 MumuController。
// 继承 MaatouchController 是为了现成的降级链：库太旧、连接配置不是 ModAVD、显示尺寸对不上时，
// 所有操作落回 MaaTouch（ModAVD 的 SELinux 是 Enforcing，minitouch 用不了）。
// 按键和输入文字库能做就走库（库的接口版本 3 起；按键还要模拟器的控制台有 event key），
// 库做不了或没发出去时照旧走 adb（AdbController）；滑动途中的暂停同理。
class ModAvdController : public MaatouchController
{
public:
    ModAvdController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        MaatouchController(callback, inst, type)
    {
    }

    ModAvdController(const ModAvdController&) = delete;
    ModAvdController(ModAvdController&&) = delete;
    virtual ~ModAvdController() override = default;

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    virtual bool click(const Point& p) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        SwipeExtraDirection extra_swipe = SwipeExtraDirection::None,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool input(const std::string& text) override;

    virtual bool press_esc() override;

    virtual ControlFeat::Feat support_features() const noexcept override;

    ModAvdController& operator=(const ModAvdController&) = delete;
    ModAvdController& operator=(ModAvdController&&) = delete;

protected:
    virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket) override;

    virtual void clear_info() noexcept override;

private:
    // 宿主侧点击是否就绪；未就绪时全部操作降级到 MaaTouch
    bool m_modavd_input_ready = false;

    // 库能发按键：点击就绪，且这台模拟器的控制台收按键
    bool use_modavd_key() const noexcept { return m_modavd_input_ready && m_modavd_extras.key_available(); }
};
} // namespace asst
