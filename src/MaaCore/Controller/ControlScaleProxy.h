#pragma once

#include <memory>
#include <random>

#include "ControllerAPI.h"

#include "Common/AsstMsg.h"

namespace asst
{
class ControlScaleProxy
{
public:
    using ProxyCallback = std::function<void(const json::object&)>;

public:
    ControlScaleProxy(
        std::shared_ptr<ControllerAPI> controller,
        ControllerType controller_type,
        ProxyCallback proxy_callback);
    ~ControlScaleProxy() = default;

    ControlScaleProxy(const ControlScaleProxy&) = delete;
    ControlScaleProxy(ControlScaleProxy&&) = delete;

    bool click(const Point& p);
    bool click(const Rect& rect);

    bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false);
    bool swipe(
        const Rect& r1,
        const Rect& r2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false);

    bool inject_input_event(InputEvent event);

    std::pair<int, int> get_scale_size() const noexcept;

private:
    Point rand_point_in_rect(const Rect& rect);

    void callback(const json::object& details);

    std::shared_ptr<ControllerAPI> m_controller;
    ControllerType m_controller_type = ControllerType::Minitouch;
    ProxyCallback m_callback = nullptr;

    std::minstd_rand m_rand_engine;

    std::pair<int, int> m_scale_size = { WindowWidthDefault, WindowHeightDefault };
    double m_control_scale = 1.0;
};
}
