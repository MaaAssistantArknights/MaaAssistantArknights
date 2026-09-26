#pragma once

#if defined(__APPLE__) && ASST_WITH_MAC_NATIVE

#include "ControllerAPI.h"

#include <memory>

namespace asst
{
class MacNativeController final : public ControllerAPI
{
public:
    MacNativeController();
    ~MacNativeController() override;

    bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    bool inited() const noexcept override;
    const std::string& get_uuid() const override;
    size_t get_pipe_data_size() const noexcept override;
    size_t get_version() const noexcept override;
    bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;
    bool start_game(const std::string& client_type) override;
    bool stop_game(const std::string& client_type) override;
    bool click(const Point& p) override;
    bool input(const std::string& text) override;
    bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        SwipeExtraDirection extra_swipe = SwipeExtraDirection::None,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;
    bool inject_input_event(const InputEvent& event) override;
    bool press_esc() override;
    ControlFeat::Feat support_features() const noexcept override;
    std::pair<int, int> get_screen_res() const noexcept override;
    void back_to_home() noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace asst

#endif // defined(__APPLE__) && ASST_WITH_MAC_NATIVE
