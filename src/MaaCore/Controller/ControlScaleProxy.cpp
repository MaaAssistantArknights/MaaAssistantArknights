#include "ControlScaleProxy.h"

#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"

asst::ControlScaleProxy::ControlScaleProxy(std::shared_ptr<ControllerAPI> controller, ControllerType controller_type,
                                           ProxyCallback proxy_callback)
    : m_controller(controller), m_controller_type(controller_type), m_callback(proxy_callback),
      m_rand_engine(std::random_device {}())
{
    auto screen_res = m_controller->get_screen_res();

    int width = screen_res.first;
    int height = screen_res.second;

    auto info = json::object { {
        "details",
        json::object {
            { "width", width },
            { "height", height },
        },
    } };

    if (width < WindowWidthDefault || height < WindowHeightDefault) {
        info["what"] = "UnsupportedResolution";
        info["why"] = "Low screen resolution";
        callback(info);
        throw std::runtime_error("Unsupported resolution");
    }
    else if (std::fabs(static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault) -
                       static_cast<double>(width) / static_cast<double>(height)) > 1e-7) {
        info["what"] = "UnsupportedResolution";
        info["why"] = "Not 16:9";
        callback(info);
        throw std::runtime_error("Unsupported resolution");
    }

    /* calc ratio */
    constexpr double DefaultRatio = static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
    double cur_ratio = static_cast<double>(width) / static_cast<double>(height);

    if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9，按照高度计算缩放
        || std::fabs(cur_ratio - DefaultRatio) < DoubleDiff) {
        int scale_width = static_cast<int>(cur_ratio * WindowHeightDefault);
        m_scale_size = std::make_pair(scale_width, WindowHeightDefault);
        m_control_scale = static_cast<double>(height) / static_cast<double>(WindowHeightDefault);
    }
    else { // 否则可能是偏正方形的屏幕，按宽度计算
        int scale_height = static_cast<int>(WindowWidthDefault / cur_ratio);
        m_scale_size = std::make_pair(WindowWidthDefault, scale_height);
        m_control_scale = static_cast<double>(width) / static_cast<double>(WindowWidthDefault);
    }
}

bool asst::ControlScaleProxy::click(const Point& p)
{
    int x = static_cast<int>(p.x * m_control_scale);
    int y = static_cast<int>(p.y * m_control_scale);

    Log.trace("Click with scaled coordinates", p, m_control_scale);
    
    return m_controller->click(Point(x, y));
}

bool asst::ControlScaleProxy::click(const Rect& rect)
{
    return click(rand_point_in_rect(rect));
}

bool asst::ControlScaleProxy::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe, double slope_in,
                                    double slope_out, bool with_pause)
{
    int x1 = static_cast<int>(p1.x * m_control_scale);
    int y1 = static_cast<int>(p1.y * m_control_scale);
    int x2 = static_cast<int>(p2.x * m_control_scale);
    int y2 = static_cast<int>(p2.y * m_control_scale);

    Log.trace("Swipe with scaled coordinates", p1, p2, m_control_scale);

    return m_controller->swipe(Point(x1, y1), Point(x2, y2), duration, extra_swipe, slope_in, slope_out, with_pause);
}

bool asst::ControlScaleProxy::swipe(const Rect& r1, const Rect& r2, int duration, bool extra_swipe, double slope_in,
                                    double slope_out, bool with_pause)
{
    auto rand_p1 = rand_point_in_rect(r1);
    auto rand_p2 = rand_point_in_rect(r2);

    if (m_controller_type == ControllerType::Adb) {
        // 同样的参数 ADB 总是划过头，糊点屎进来
        // 外部调用 swipe(Point, Point) 时，说明是精确要求位置的，不能做这个调整
        // 所以屎没法糊在下面一层，只能糊在这里了（
        const auto& opt = Config.get_options();
        auto x_dist = rand_p1.x - rand_p2.x;
        rand_p2.x = rand_p1.x - static_cast<int>(x_dist * opt.adb_swipe_x_distance_multiplier);
    }

    return swipe(rand_p1, rand_p2, duration, extra_swipe, slope_in, slope_out, with_pause);
}

bool asst::ControlScaleProxy::inject_input_event(InputEvent event)
{
    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
    case InputEvent::Type::TOUCH_MOVE:
        event.point.x = static_cast<int>(event.point.x * m_control_scale);
        event.point.y = static_cast<int>(event.point.y * m_control_scale);
        break;
    default:
        break;
    }

    return m_controller->inject_input_event(event);
}

std::pair<int, int> asst::ControlScaleProxy::get_scale_size() const noexcept
{
    return m_scale_size;
}

asst::Point asst::ControlScaleProxy::rand_point_in_rect(const Rect& rect)
{
    int x = 0, y = 0;
    if (rect.width == 0) {
        x = rect.x;
    }
    else {
        int x_rand = std::poisson_distribution<int>(rect.width / 2.)(m_rand_engine);

        x = x_rand + rect.x;
    }

    if (rect.height == 0) {
        y = rect.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(rect.height / 2.)(m_rand_engine);
        y = y_rand + rect.y;
    }

    return { x, y };
}

void asst::ControlScaleProxy::callback(const json::object& details)
{
    if (m_callback) {
        m_callback(details);
    }
}
