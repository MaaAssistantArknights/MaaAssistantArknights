#ifdef WITH_THRIFT

#include "MaaThriftController.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4245 4706)
#endif
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Config/GeneralConfig.h"
#include "Utils/NoWarningCV.h"

asst::MaaThriftController::~MaaThriftController()
{
    LogTraceFunction;
    close();
}

bool asst::MaaThriftController::connect(
    [[maybe_unused]] const std::string& adb_path,
    [[maybe_unused]] const std::string& address,
    const std::string& config)
{
    LogTraceFunction;

    clear_info();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "config", config },
              } },
        };
    };

    auto config_error = [&]() {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigError" },
            { "config", config },
        };
        callback(AsstMsg::ConnectionInfo, info);
    };

    static const std::unordered_map<std::string, ThriftControllerTypeEnum> type_map = {
        { "Socket", ThriftControllerTypeEnum::MaaThriftControllerType_Socket },
        { "UnixDomainSocket", ThriftControllerTypeEnum::MaaThriftControllerType_UnixDomainSocket },
    };

    using namespace apache::thrift;

    std::shared_ptr<transport::TSocket> socket;

    auto param_json = json::parse(config);
    if (!param_json.has_value()) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigError" },
            { "config", config },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    auto type = param_json->at("type").as_string();

    switch (type_map.at(type)) {
    case ThriftControllerTypeEnum::MaaThriftControllerType_Socket:
        if (param_json->at("param").is_object()) {
            auto host = param_json->at("param").at("host").as_string();
            auto port = param_json->at("param").at("port").as_integer();
            socket = std::make_shared<transport::TSocket>(host, port);
        }
        else {
            config_error();
            return false;
        }
        break;
    case ThriftControllerTypeEnum::MaaThriftControllerType_UnixDomainSocket:
        if (param_json->at("param").is_string()) {
            auto path = param_json->at("param").as_string();
            socket = std::make_shared<transport::TSocket>(path);
        }
        else {
            config_error();
            return false;
        }
        break;
    default:
        config_error();
        return false;
    }

    transport_ = std::make_shared<transport::TBufferedTransport>(socket);
    auto protocol = std::make_shared<protocol::TBinaryProtocol>(transport_);

    client_ = std::make_shared<ThriftController::ThriftControllerClient>(protocol);

    try {
        transport_->open();
    }
    catch (const std::exception& e) {
        Log.error("Cannot open transport: {}", e.what());
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "TransportError" },
            { "config", config },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    auto connect_ret = false;
    try {
        connect_ret = client_->connect();
    }
    catch (const std::exception& e) {
        Log.error("Cannot connect to MaaThriftController:", e.what());
        return false;
    }

    if (!connect_ret) {
        Log.error("Cannot connect to MaaThriftController");
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConnectError" },
            { "config", config },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    m_config = config;

    if (need_exit()) {
        return false;
    }

    /* uuid */
    {
        try {
            client_->get_uuid(m_uuid);
        }
        catch (const std::exception& e) {
            Log.error("Cannot get uuid:", e.what());
            return false;
        }

        if (!m_uuid.empty()) {
            json::value info = get_info_json() | json::object {
                { "what", "UuidGot" },
                { "why", "" },
            };
            info["details"]["uuid"] = m_uuid;
            callback(AsstMsg::ConnectionInfo, info);
        }
        else {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Uuid Empty" },
            };
            callback(AsstMsg::ConnectionInfo, info);
        }
    }

    if (need_exit()) {
        return false;
    }

    /* display */
    {
        ThriftController::Size screen_res;
        try {
            client_->get_screen_res(screen_res);
        }
        catch (const std::exception& e) {
            Log.error("Cannot get screen resolution:", e.what());
            return false;
        }
        auto width = screen_res.width;
        auto height = screen_res.height;

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
        m_screen_size = { width, height };
    }

    if (need_exit()) {
        return false;
    }

    {
        try {
            m_support_features = static_cast<ControlFeat::Feat>(client_->get_support_features());
        }
        catch (const std::exception& e) {
            Log.error("Cannot get support features:", e.what());
            return false;
        }
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

bool asst::MaaThriftController::inited() const noexcept
{
    return m_inited;
}

void asst::MaaThriftController::set_swipe_with_pause(bool enable) noexcept
{
    m_swipe_with_pause_enabled = enable;
}

const std::string& asst::MaaThriftController::get_uuid() const
{
    return m_uuid;
}

bool asst::MaaThriftController::screencap(cv::Mat& image_payload, [[maybe_unused]] bool allow_reconnect)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    ThriftController::CustomImage img;
    try {
        client_->screencap(img);
    }
    catch (const std::exception& e) {
        Log.error("Cannot get screencap:", e.what());
        return false;
    }

    if (img.data.empty()) {
        Log.error("client_->screencap() return empty buffer");
        return false;
    }
    cv::Mat orig_mat(img.size.height, img.size.width, img.type, img.data.data());
    orig_mat.copyTo(image_payload);
    return true;
}

bool asst::MaaThriftController::start_game(const std::string& client_type)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    if (client_type.empty()) {
        return false;
    }
    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        return false;
    }

    try {
        return client_->start_game(*package_name);
    }
    catch (const std::exception& e) {
        Log.error("Cannot start game:", e.what());
        return false;
    }
}

bool asst::MaaThriftController::stop_game()
{
    return false;
}

bool asst::MaaThriftController::click(const Point& p)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    ThriftController::ClickParam click_param;
    click_param.point.x = p.x;
    click_param.point.y = p.y;

    return client_->click(click_param);
}

bool asst::MaaThriftController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    if (!support_precise_swipe()) {
        ThriftController::SwipeParam swipe_param;
        swipe_param.point1.x = p1.x;
        swipe_param.point1.y = p1.y;
        swipe_param.point2.x = p2.x;
        swipe_param.point2.y = p2.y;
        swipe_param.duration = duration;
        try {
            return client_->swipe(swipe_param);
        }
        catch (const std::exception& e) {
            Log.error("Cannot swipe:", e.what());
            return false;
        }
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    const auto width = m_screen_size.first;
    const auto height = m_screen_size.second;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, width - 1);
        y1 = std::clamp(y1, 0, height - 1);
    }

    toucher_down(p1);

    constexpr int TimeInterval = DefaultSwipeDelay;

    auto cubic_spline = [](double slope_0, double slope_1, double t) {
        const double a = slope_0;
        const double b = -(2 * slope_0 + slope_1 - 3);
        const double c = -(-slope_0 - slope_1 + 2);
        return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
    }; // TODO: move this to math.hpp

    bool need_pause = with_pause && use_swipe_with_pause();
    const auto& opt = Config.get_options();
    std::future<void> pause_future;
    auto progressive_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) {
        for (int cur_time = TimeInterval; cur_time < _duration; cur_time += TimeInterval) {
            double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / duration);
            int cur_x = static_cast<int>(std::lerp(_x1, _x2, progress));
            int cur_y = static_cast<int>(std::lerp(_y1, _y2, progress));
            if (need_pause && std::sqrt(std::pow(cur_x - _x1, 2) + std::pow(cur_y - _y1, 2)) >
                                  opt.swipe_with_pause_required_distance) {
                need_pause = false;
                pause_future = std::async(std::launch::async, [&]() { press_esc(); });
            }
            if (cur_x < 0 || cur_x > width || cur_y < 0 || cur_y > height) {
                continue;
            }
            toucher_move({ cur_x, cur_y });
        }
        if (_x2 >= 0 && _x2 <= width && _y2 >= 0 && _y2 <= height) {
            toucher_move({ _x2, _y2 });
        }
    };

    progressive_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration);

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay)); // 停留终点
        progressive_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
    }
    bool ret = toucher_up(p2);
    return ret;
}

bool asst::MaaThriftController::inject_input_event(const InputEvent& event)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    bool ret = false;

    ThriftController::InputEvent input_event;
    switch (event.type) {
    case InputEvent::Type::TOUCH_DOWN:
        input_event.type = ThriftController::InputEventType::TOUCH_DOWN;
        break;
    case InputEvent::Type::TOUCH_UP:
        input_event.type = ThriftController::InputEventType::TOUCH_UP;
        break;
    case InputEvent::Type::TOUCH_MOVE:
        input_event.type = ThriftController::InputEventType::TOUCH_MOVE;
        break;
    case InputEvent::Type::TOUCH_RESET:
        input_event.type = ThriftController::InputEventType::TOUCH_RESET;
        break;
    case InputEvent::Type::KEY_DOWN:
        input_event.type = ThriftController::InputEventType::KEY_DOWN;
        break;
    case InputEvent::Type::KEY_UP:
        input_event.type = ThriftController::InputEventType::KEY_UP;
        break;
    case InputEvent::Type::WAIT_MS:
        input_event.type = ThriftController::InputEventType::WAIT_MS;
        break;
    case InputEvent::Type::COMMIT:
        try {
            ret = client_->inject_input_events(m_input_events);
        }
        catch (const std::exception& e) {
            Log.error("Cannot inject input events:", e.what());
        }
        m_input_events.clear();
        return ret;
    case InputEvent::Type::UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }

    input_event.touch.point.x = event.point.x;
    input_event.touch.point.y = event.point.y;
    input_event.touch.contact = event.pointerId;
    input_event.key = event.keycode;
    input_event.wait_ms = event.milisec;
    m_input_events.push_back(input_event);
    return true;
}

bool asst::MaaThriftController::press_esc()
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        Log.error("client_ is not created or transport_ is not open");
        return false;
    }

    try {
        return client_->press_key(111);
    }
    catch (const std::exception& e) {
        Log.error("Cannot press esc:", e.what());
        return false;
    }
}

asst::ControlFeat::Feat asst::MaaThriftController::support_features() const noexcept
{
    return m_support_features;
}

std::pair<int, int> asst::MaaThriftController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void asst::MaaThriftController::back_to_home() noexcept
{
}

void asst::MaaThriftController::clear_info() noexcept
{
    m_inited = false;
    m_screen_size = { 0, 0 };
    m_uuid.clear();
    m_config.clear();
    m_support_features = ControlFeat::NONE;
    m_swipe_with_pause_enabled = false;
    m_input_events.clear();
    close();
    client_.reset();
    transport_.reset();
}

void asst::MaaThriftController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

bool asst::MaaThriftController::support_precise_swipe() const noexcept
{
    return m_support_features & ControlFeat::PRECISE_SWIPE;
}

bool asst::MaaThriftController::support_swipe_with_pause() const noexcept
{
    return m_support_features & ControlFeat::SWIPE_WITH_PAUSE;
}

bool asst::MaaThriftController::use_swipe_with_pause() const noexcept
{
    return m_swipe_with_pause_enabled && support_swipe_with_pause();
}

bool asst::MaaThriftController::toucher_down(const Point& p, const int delay, int contact)
{
    InputEvent event;
    event.type = InputEvent::Type::TOUCH_DOWN;
    event.point = p;
    event.pointerId = contact;
    inject_input_event(event);
    event.type = InputEvent::Type::COMMIT;
    auto ret = inject_input_event(event);
    if (ret) {
        toucher_wait(delay);
    }
    return ret;
}

bool asst::MaaThriftController::toucher_move(const Point& p, const int delay, int contact)
{
    InputEvent event;
    event.type = InputEvent::Type::TOUCH_MOVE;
    event.point = p;
    event.pointerId = contact;
    inject_input_event(event);
    event.type = InputEvent::Type::COMMIT;
    auto ret = inject_input_event(event);
    if (ret) {
        toucher_wait(delay);
    }
    return ret;
}

bool asst::MaaThriftController::toucher_up(const Point& p, const int delay, int contact)
{
    InputEvent event;
    event.type = InputEvent::Type::TOUCH_UP;
    event.point = p;
    event.pointerId = contact;
    inject_input_event(event);
    event.type = InputEvent::Type::COMMIT;
    auto ret = inject_input_event(event);
    if (ret) {
        toucher_wait(delay);
    }
    return ret;
}

void asst::MaaThriftController::toucher_wait(const int delay)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

void asst::MaaThriftController::close()
{
    if (transport_) {
        transport_->close();
    }
}

#endif // WITH_THRIFT
