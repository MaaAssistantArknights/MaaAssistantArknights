#include "CustomController.h"

#include "Utils/Logger.hpp"

asst::CustomController::CustomController(const AsstCallback& callback, Assistant* inst,
                                         std::shared_ptr<AsstCustomController> custom)
    : InstHelper(inst), m_callback(callback), m_custom(custom)
{
    if (custom == nullptr) {
        throw std::runtime_error("CustomController: custom controller is nullptr");
    }
}

bool asst::CustomController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    /* connect */
    {
        auto connect_ret = m_custom->connect(m_custom.get(), adb_path.c_str(), address.c_str(), config.c_str());

        if (!connect_ret) {
            json::value connection_info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
            };
            callback(AsstMsg::ConnectionInfo, connection_info);
            return false;
        }
    }

    /* get uuid */
    {
        char uuid[64] = { 0 };
        auto uuid_ret = m_custom->get_uuid(m_custom.get(), uuid, sizeof(uuid));
        if (!uuid_ret) {
            json::value connection_info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "get uuid failed" },
            };
            callback(AsstMsg::ConnectionInfo, connection_info);
            return false;
        }

        m_uuid = uuid;
        json::value info = get_info_json() | json::object {
            { "what", "UuidGot" },
            { "why", "" },
        };
        info["details"]["uuid"] = m_uuid;
        callback(AsstMsg::ConnectionInfo, info);
    }

    /* get screen res*/
    {
        int32_t value1 = 0, value2 = 0, width = 0, height = 0;
        auto display_ret = m_custom->get_screen_res(m_custom.get(), &value1, &value2);
        if (!display_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Get Resolution Failed" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        width = std::max(value1, value2);
        height = std::min(value1, value2);

        json::value info = get_info_json() | json::object {
            { "what", "ResolutionGot" },
            { "why", "" },
        };

        info["details"] |= json::object {
            { "width", width },
            { "height", height },
        };

        callback(AsstMsg::ConnectionInfo, info);

        if (width == 0 || height == 0) {
            info["what"] = "ResolutionError";
            info["why"] = "Get resolution failed";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        m_screen_res = { width, height };
    }

    {
        json::value info = get_info_json() | json::object {
            { "what", "Connected" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    return true;
}

bool asst::CustomController::inited() const noexcept
{
    return m_custom->inited(m_custom.get());
}

void asst::CustomController::set_swipe_with_pause(bool enable) noexcept
{
    m_custom->set_swipe_with_pause(m_custom.get(), enable);
}

const std::string& asst::CustomController::get_uuid() const
{
    return m_uuid;
}

bool asst::CustomController::screencap(cv::Mat& image_payload, bool allow_reconnect)
{
    CustomImage* image = nullptr;
    auto ret = m_custom->screencap(m_custom.get(), &image, allow_reconnect);
    if (!ret) {
        return false;
    }
    auto temp = cv::Mat(image->height, image->width, image->pixel_format, image->data);
    image_payload = temp.clone();
    delete image;
    return false;
}

bool asst::CustomController::start_game(const std::string& client_type)
{
    return m_custom->start_game(m_custom.get(), client_type.c_str());
}

bool asst::CustomController::stop_game()
{
    return m_custom->stop_game(m_custom.get());
}

bool asst::CustomController::click(const Point& p)
{
    return m_custom->click(m_custom.get(), p.x, p.y);
}

bool asst::CustomController::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe, double slope_in,
                                   double slope_out, bool with_pause)
{
    return m_custom->swipe(m_custom.get(), p1.x, p1.y, p2.x, p2.y, duration, extra_swipe, slope_in, slope_out,
                           with_pause);
}

bool asst::CustomController::inject_input_event(const InputEvent& event)
{
    AsstInputEvent asst_event = {
        .type = (int)event.type,
        .pointer_id = event.pointerId,
        .x = event.point.x,
        .y = event.point.y,
        .keycode = event.keycode,
        .milis = event.milisec,
    };
    return m_custom->inject_input_event(m_custom.get(), &asst_event);
}

bool asst::CustomController::press_esc()
{
    return m_custom->press_esc(m_custom.get());
}

asst::ControlFeat::Feat asst::CustomController::support_features() const noexcept
{
    return m_custom->support_features(m_custom.get());
}

std::pair<int, int> asst::CustomController::get_screen_res() const noexcept
{
    return m_screen_res;
}

void asst::CustomController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}
