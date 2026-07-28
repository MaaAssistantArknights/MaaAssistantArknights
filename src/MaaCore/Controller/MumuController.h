#pragma once

#if ASST_WITH_EMULATOR_EXTRAS

#include "MaatouchController.h"

namespace asst
{
// 通过 MuMu 的 external renderer IPC 直接下发触控，绕开 adb。
// 继承 MaatouchController 是为了拿到现成的降级链：MuMu 触控不可用时（模拟器版本过低、
// dll 缺少 input 符号、非 MuMu 连接配置等）所有操作自动落回 maatouch。
class MumuController : public MaatouchController
{
public:
    MumuController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        MaatouchController(callback, inst, type)
    {
    }

    MumuController(const MumuController&) = delete;
    MumuController(MumuController&&) = delete;
    virtual ~MumuController() override = default;

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    virtual bool click(const Point& p) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool input(const std::string& text) override;

    virtual bool press_esc() override;

    virtual ControlFeat::Feat support_features() const noexcept override;

    MumuController& operator=(const MumuController&) = delete;
    MumuController& operator=(MumuController&&) = delete;

protected:
    virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket) override;

    virtual void clear_info() noexcept override;

private:
    // MuMu 触控是否就绪。未就绪时全部操作降级到 MaatouchController
    bool use_mumu_input() const noexcept { return m_mumu_input_ready; }

    bool m_mumu_input_ready = false;
};
} // namespace asst

#endif
