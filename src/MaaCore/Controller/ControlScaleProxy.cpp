#include "ControlScaleProxy.h"

#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"

asst::ControlScaleProxy::ControlScaleProxy(
    std::shared_ptr<ControllerAPI> controller,
    ControllerType controller_type,
    ProxyCallback proxy_callback) :
    m_controller(controller),
    m_controller_type(controller_type),
    m_callback(proxy_callback),
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

    try {
        if (width < WindowWidthDefault || height < WindowHeightDefault) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Low screen resolution";
            callback(info);

            // Log detailed error instead of just throwing
            Log.error("Resolution too low:", width, "x", height,
                     "minimum required:", WindowWidthDefault, "x", WindowHeightDefault);
            throw std::runtime_error("Unsupported resolution");
        }
        else if (
            std::fabs(
                static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault) -
                static_cast<double>(width) / static_cast<double>(height)) > 1e-7) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Not 16:9";
            callback(info);

            // Log detailed error
            Log.error("Aspect ratio not 16:9:", width, "x", height,
                     "aspect ratio:", static_cast<double>(width) / height);
            throw std::runtime_error("Unsupported resolution");
        }

        /* calc ratio */
        constexpr double DefaultRatio = static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
        double cur_ratio = static_cast<double>(width) / static_cast<double>(height);

        if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9,按照高度计算缩放
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
    catch (const std::exception& e) {
        Log.error("ControlScaleProxy initialization failed:", e.what());
        throw;  // Re-throw after logging
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

bool asst::ControlScaleProxy::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause,
    bool high_resolution_swipe_fix)
{
    int x1 = static_cast<int>(p1.x * m_control_scale);
    int y1 = static_cast<int>(p1.y * m_control_scale);
    int x2, y2;
    if (high_resolution_swipe_fix) {
        // 保持滑动距离一致：先计算原始的相对偏移，再加到缩放后的起点上
        int dx = p2.x - p1.x; // 不缩放
        int dy = p2.y - p1.y;
        x2 = x1 + dx;
        y2 = y1 + dy;
        Log.trace("High-resolution swipe fix, offset", Point(dx, dy));
    }
    else {
        x2 = static_cast<int>(p2.x * m_control_scale);
        y2 = static_cast<int>(p2.y * m_control_scale);
    }

    Log.trace("Swipe with scaled coordinates", p1, p2, m_control_scale);

    return m_controller->swipe(Point(x1, y1), Point(x2, y2), duration, extra_swipe, slope_in, slope_out, with_pause);
}

bool asst::ControlScaleProxy::swipe(
    const Rect& r1,
    const Rect& r2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause,
    bool high_resolution_swipe_fix)
{
    Point rand_p1, rand_p2;
    bool precise1 = (r1.width == 1 && r1.height == 1);
    bool precise2 = (r2.width == 1 && r2.height == 1);

    if (precise1) {
        rand_p1 = Point(r1.x, r1.y);
    }
    else {
        rand_p1 = rand_point_in_rect(r1);
    }

    if (precise2) {
        rand_p2 = Point(r2.x, r2.y);
    }
    else {
        rand_p2 = rand_point_in_rect(r2);
    }

    if (m_controller_type == ControllerType::Adb && !(precise1 && precise2)) {
        // 只有不是精确点时才做ADB修正
        // 同样的参数 ADB 总是划过头，糊点屎进来
        // 外部调用 swipe(Point, Point) 时，说明是精确要求位置的，不能做这个调整
        // 所以屎没法糊在下面一层，只能糊在这里了（
        const auto& opt = Config.get_options();
        auto x_dist = rand_p1.x - rand_p2.x;
        rand_p2.x = rand_p1.x - static_cast<int>(x_dist * opt.adb_swipe_x_distance_multiplier);
    }

    return swipe(rand_p1, rand_p2, duration, extra_swipe, slope_in, slope_out, with_pause, high_resolution_swipe_fix);
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

asst::Point asst::ControlScaleProxy::rand_point_in_rect(const Rect& r)
{
    // 过小矩形直接返回中心点，避免死循环
    if (r.width <= 2 || r.height <= 2) {
        return { r.x + r.width / 2, r.y + r.height / 2 };
    }

    constexpr double kStdDevFactor = 3.0;

    const double std_dev_x = r.width / kStdDevFactor;
    const double std_dev_y = r.height / kStdDevFactor;

    std::normal_distribution<double> dist_x(r.x + r.width / 2.0, std_dev_x);
    std::normal_distribution<double> dist_y(r.y + r.height / 2.0, std_dev_y);

    // 优先进行有限次拒绝采样
    constexpr int kMaxAttempts = 8;
    for (int i = 0; i < kMaxAttempts; ++i) {
        const int x = static_cast<int>(std::round(dist_x(m_rand_engine)));
        const int y = static_cast<int>(std::round(dist_y(m_rand_engine)));
        if (x < r.x || x >= r.x + r.width || y < r.y || y >= r.y + r.height) {
            continue;
        }

        return { x, y };
    }

    LogWarn << "Too many sampling attempts";
    // 返回中心点
    return { r.x + r.width / 2, r.y + r.height / 2 };
}

void asst::ControlScaleProxy::callback(const json::object& details)
{
    if (m_callback) {
        m_callback(details);
    }
}
