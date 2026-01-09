#include "WDAController.h"

#include <algorithm>
#include <sstream>
#include <thread>

#include <boost/asio.hpp>
#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

using boost::asio::ip::tcp;

asst::WDAController::WDAController(
    const AsstCallback& callback,
    Assistant* inst,
    PlatformType type [[maybe_unused]]) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

asst::WDAController::~WDAController()
{
    destroy_session();
}

bool asst::WDAController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address,
    const std::string& config [[maybe_unused]])
{
    LogTraceFunction;

    auto colon_pos = address.find(':');
    if (colon_pos != std::string::npos) {
        m_host = address.substr(0, colon_pos);
        m_port = static_cast<uint16_t>(std::stoi(address.substr(colon_pos + 1)));
    }
    else {
        m_host = address;
        m_port = 8100;
    }

    Log.info("Connecting to WDA at", m_host, ":", m_port);

    if (!check_wda_status()) {
        Log.error("WDA is not running at", address);
        return false;
    }

    if (!create_session()) {
        Log.error("Failed to create WDA session");
        return false;
    }

    if (!fetch_screen_size()) {
        Log.error("Failed to fetch screen size from WDA");
        return false;
    }

    m_connected = true;
    Log.info("WDA connected successfully, screen size:", m_screen_size.first, "x", m_screen_size.second);
    return true;
}

bool asst::WDAController::inited() const noexcept
{
    return m_connected && m_screen_size.first > 0;
}

const std::string& asst::WDAController::get_uuid() const
{
    static const std::string uuid("com.hypergryph.arknights");
    return uuid;
}

size_t asst::WDAController::get_pipe_data_size() const noexcept
{
    return 0;
}

size_t asst::WDAController::get_version() const noexcept
{
    return 0;
}

bool asst::WDAController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    LogTraceFunction;

    auto response = http_get("/screenshot");
    if (!response.success()) {
        Log.error("Screenshot request failed, status:", response.status_code);
        return false;
    }

    auto json_opt = json::parse(response.body);
    if (!json_opt) {
        Log.error("Failed to parse screenshot response");
        return false;
    }

    std::string base64_data;
    if (json_opt->contains("value") && json_opt->at("value").is_string()) {
        base64_data = json_opt->at("value").as_string();
    }
    else {
        Log.error("Screenshot response missing 'value' field");
        return false;
    }

    return decode_base64_png(base64_data, image_payload);
}

bool asst::WDAController::start_game(const std::string& client_type [[maybe_unused]])
{
    Log.info("start_game is not fully supported on WDA");
    return true;
}

bool asst::WDAController::stop_game(const std::string& client_type [[maybe_unused]])
{
    Log.info("stop_game is not fully supported on WDA");
    return true;
}

bool asst::WDAController::click(const Point& p)
{
    Log.trace("WDA click:", p.x, p.y);

    std::string action_json = build_w3c_tap_action(p.x, p.y);
    auto response = http_post("/session/" + m_session_id + "/actions", action_json);

    if (!response.success()) {
        Log.error("WDA click failed, status:", response.status_code);
        return false;
    }

    return true;
}

bool asst::WDAController::input(const std::string& text [[maybe_unused]])
{
    Log.info("InputText is not fully supported on WDA");
    return true;
}

bool asst::WDAController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in [[maybe_unused]],
    double slope_out [[maybe_unused]],
    bool with_pause [[maybe_unused]])
{
    Log.trace("WDA swipe:", p1.x, p1.y, "->", p2.x, p2.y, "duration:", duration);

    int x1 = std::clamp(p1.x, 0, m_screen_size.first - 1);
    int y1 = std::clamp(p1.y, 0, m_screen_size.second - 1);

    const auto& opt = Config.get_options();
    int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    std::string action_json = build_w3c_swipe_action(x1, y1, p2.x, p2.y, actual_duration);
    auto response = http_post("/session/" + m_session_id + "/actions", action_json);

    if (!response.success()) {
        Log.error("WDA swipe failed, status:", response.status_code);
        return false;
    }

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));
        std::string extra_action = build_w3c_swipe_action(
            p2.x, p2.y, p2.x, p2.y - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
        http_post("/session/" + m_session_id + "/actions", extra_action);
    }

    return true;
}

bool asst::WDAController::press_esc()
{
    Log.info("ESC is not supported on iOS/WDA");
    return false;
}

std::pair<int, int> asst::WDAController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void asst::WDAController::back_to_home() noexcept
{
    Log.info("HOME is not supported on WDA");
}

asst::WDAController::HttpResponse asst::WDAController::http_get(const std::string& path)
{
    HttpResponse response;
    try {
        tcp::socket socket(m_context);
        tcp::resolver resolver(m_context);
        boost::asio::connect(socket, resolver.resolve(m_host, std::to_string(m_port)));

        std::string request = "GET " + path + " HTTP/1.1\r\n"
                              "Host: " + m_host + ":" + std::to_string(m_port) + "\r\n"
                              "Accept: application/json\r\n"
                              "Connection: close\r\n\r\n";

        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response_buf;
        boost::system::error_code ec;

        boost::asio::read_until(socket, response_buf, "\r\n", ec);
        std::istream response_stream(&response_buf);
        std::string http_version;
        response_stream >> http_version >> response.status_code;

        boost::asio::read_until(socket, response_buf, "\r\n\r\n", ec);

        std::string header;
        size_t content_length = 0;
        while (std::getline(response_stream, header) && header != "\r") {
            if (header.find("Content-Length:") != std::string::npos ||
                header.find("content-length:") != std::string::npos) {
                auto pos = header.find(':');
                if (pos != std::string::npos) {
                    content_length = std::stoul(header.substr(pos + 1));
                }
            }
        }

        if (content_length > 0) {
            size_t remaining = content_length;
            if (response_buf.size() > 0) {
                std::ostringstream ss;
                ss << &response_buf;
                response.body = ss.str();
                remaining = content_length > response.body.size() ? content_length - response.body.size() : 0;
            }
            if (remaining > 0) {
                std::vector<char> buf(remaining);
                boost::asio::read(socket, boost::asio::buffer(buf), ec);
                response.body.append(buf.data(), buf.size());
            }
        }
        else {
            std::ostringstream ss;
            boost::asio::read(socket, response_buf, ec);
            ss << &response_buf;
            response.body = ss.str();
        }
    }
    catch (const std::exception& e) {
        Log.error("HTTP GET failed:", e.what());
        response.status_code = 0;
    }
    return response;
}

asst::WDAController::HttpResponse asst::WDAController::http_post(const std::string& path, const std::string& json_body)
{
    HttpResponse response;
    try {
        tcp::socket socket(m_context);
        tcp::resolver resolver(m_context);
        boost::asio::connect(socket, resolver.resolve(m_host, std::to_string(m_port)));

        std::string request = "POST " + path + " HTTP/1.1\r\n"
                              "Host: " + m_host + ":" + std::to_string(m_port) + "\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(json_body.size()) + "\r\n"
                              "Accept: application/json\r\n"
                              "Connection: close\r\n\r\n" + json_body;

        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response_buf;
        boost::system::error_code ec;

        boost::asio::read_until(socket, response_buf, "\r\n", ec);
        std::istream response_stream(&response_buf);
        std::string http_version;
        response_stream >> http_version >> response.status_code;

        boost::asio::read_until(socket, response_buf, "\r\n\r\n", ec);
        boost::asio::read(socket, response_buf, ec);

        std::ostringstream ss;
        ss << &response_buf;
        std::string full_response = ss.str();

        auto body_start = full_response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            response.body = full_response.substr(body_start + 4);
        }
        else {
            response.body = full_response;
        }
    }
    catch (const std::exception& e) {
        Log.error("HTTP POST failed:", e.what());
        response.status_code = 0;
    }
    return response;
}

asst::WDAController::HttpResponse asst::WDAController::http_delete(const std::string& path)
{
    HttpResponse response;
    try {
        tcp::socket socket(m_context);
        tcp::resolver resolver(m_context);
        boost::asio::connect(socket, resolver.resolve(m_host, std::to_string(m_port)));

        std::string request = "DELETE " + path + " HTTP/1.1\r\n"
                              "Host: " + m_host + ":" + std::to_string(m_port) + "\r\n"
                              "Accept: application/json\r\n"
                              "Connection: close\r\n\r\n";

        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response_buf;
        boost::system::error_code ec;

        boost::asio::read_until(socket, response_buf, "\r\n", ec);
        std::istream response_stream(&response_buf);
        std::string http_version;
        response_stream >> http_version >> response.status_code;

        boost::asio::read(socket, response_buf, ec);
        std::ostringstream ss;
        ss << &response_buf;
        response.body = ss.str();
    }
    catch (const std::exception& e) {
        Log.error("HTTP DELETE failed:", e.what());
        response.status_code = 0;
    }
    return response;
}

bool asst::WDAController::check_wda_status()
{
    auto response = http_get("/status");
    if (!response.success()) {
        Log.error("WDA status check failed, status:", response.status_code);
        return false;
    }

    auto json_opt = json::parse(response.body);
    if (!json_opt) {
        Log.error("Failed to parse WDA status response");
        return false;
    }

    Log.info("WDA status check passed");
    return true;
}

bool asst::WDAController::create_session()
{
    json::object capabilities;
    capabilities["capabilities"] = json::object {};

    auto response = http_post("/session", capabilities.to_string());
    if (!response.success()) {
        Log.error("Failed to create WDA session, status:", response.status_code);
        return false;
    }

    auto json_opt = json::parse(response.body);
    if (!json_opt) {
        Log.error("Failed to parse session creation response");
        return false;
    }

    if (json_opt->contains("sessionId") && json_opt->at("sessionId").is_string()) {
        m_session_id = json_opt->at("sessionId").as_string();
    }
    else if (json_opt->contains("value") && json_opt->at("value").is_object()) {
        const auto& value = json_opt->at("value");
        if (value.contains("sessionId") && value.at("sessionId").is_string()) {
            m_session_id = value.at("sessionId").as_string();
        }
    }

    if (m_session_id.empty()) {
        Log.error("Session ID not found in response");
        return false;
    }

    Log.info("WDA session created:", m_session_id);
    return true;
}

bool asst::WDAController::fetch_screen_size()
{
    auto response = http_get("/session/" + m_session_id + "/window/rect");
    if (!response.success()) {
        Log.error("Failed to fetch screen size, status:", response.status_code);
        return false;
    }

    auto json_opt = json::parse(response.body);
    if (!json_opt) {
        Log.error("Failed to parse screen size response");
        return false;
    }

    const json::value* rect = nullptr;
    if (json_opt->contains("value") && json_opt->at("value").is_object()) {
        rect = &json_opt->at("value");
    }
    else {
        rect = &(*json_opt);
    }

    if (rect->contains("width") && rect->contains("height")) {
        m_screen_size.first = static_cast<int>(rect->at("width").as_integer());
        m_screen_size.second = static_cast<int>(rect->at("height").as_integer());
        return m_screen_size.first > 0 && m_screen_size.second > 0;
    }

    Log.error("Screen size not found in response");
    return false;
}

bool asst::WDAController::destroy_session()
{
    if (m_session_id.empty()) {
        return true;
    }

    Log.info("Destroying WDA session:", m_session_id);
    auto response = http_delete("/session/" + m_session_id);
    m_session_id.clear();
    m_connected = false;

    return response.success();
}

std::string asst::WDAController::build_w3c_tap_action(int x, int y)
{
    json::object actions;
    json::array action_list;

    json::object pointer_action;
    pointer_action["type"] = "pointer";
    pointer_action["id"] = "finger1";

    json::object params;
    params["pointerType"] = "touch";
    pointer_action["parameters"] = std::move(params);

    json::array actions_seq;

    json::object move_action;
    move_action["type"] = "pointerMove";
    move_action["duration"] = 0;
    move_action["x"] = x;
    move_action["y"] = y;
    actions_seq.emplace_back(std::move(move_action));

    json::object down_action;
    down_action["type"] = "pointerDown";
    down_action["button"] = 0;
    actions_seq.emplace_back(std::move(down_action));

    json::object pause_action;
    pause_action["type"] = "pause";
    pause_action["duration"] = 50;
    actions_seq.emplace_back(std::move(pause_action));

    json::object up_action;
    up_action["type"] = "pointerUp";
    up_action["button"] = 0;
    actions_seq.emplace_back(std::move(up_action));

    pointer_action["actions"] = std::move(actions_seq);
    action_list.emplace_back(std::move(pointer_action));
    actions["actions"] = std::move(action_list);

    return actions.to_string();
}

std::string asst::WDAController::build_w3c_swipe_action(int x1, int y1, int x2, int y2, int duration_ms)
{
    json::object actions;
    json::array action_list;

    json::object pointer_action;
    pointer_action["type"] = "pointer";
    pointer_action["id"] = "finger1";

    json::object params;
    params["pointerType"] = "touch";
    pointer_action["parameters"] = std::move(params);

    json::array actions_seq;

    json::object move_start;
    move_start["type"] = "pointerMove";
    move_start["duration"] = 0;
    move_start["x"] = x1;
    move_start["y"] = y1;
    actions_seq.emplace_back(std::move(move_start));

    json::object down_action;
    down_action["type"] = "pointerDown";
    down_action["button"] = 0;
    actions_seq.emplace_back(std::move(down_action));

    json::object move_end;
    move_end["type"] = "pointerMove";
    move_end["duration"] = duration_ms;
    move_end["x"] = x2;
    move_end["y"] = y2;
    actions_seq.emplace_back(std::move(move_end));

    json::object up_action;
    up_action["type"] = "pointerUp";
    up_action["button"] = 0;
    actions_seq.emplace_back(std::move(up_action));

    pointer_action["actions"] = std::move(actions_seq);
    action_list.emplace_back(std::move(pointer_action));
    actions["actions"] = std::move(action_list);

    return actions.to_string();
}

bool asst::WDAController::decode_base64_png(const std::string& base64_data, cv::Mat& output)
{
    std::string decoded = base64_decode(base64_data);
    if (decoded.empty()) {
        Log.error("Base64 decode failed");
        return false;
    }

    std::vector<uint8_t> png_data(decoded.begin(), decoded.end());
    output = cv::imdecode(png_data, cv::IMREAD_COLOR);

    if (output.empty()) {
        Log.error("PNG decode failed");
        return false;
    }

    return true;
}

std::string asst::WDAController::base64_decode(const std::string& encoded)
{
    static constexpr char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    auto find_char = [](char c) -> int {
        const char* p = std::find(base64_chars, base64_chars + 64, c);
        return (p != base64_chars + 64) ? static_cast<int>(p - base64_chars) : -1;
    };

    auto is_base64 = [](char c) -> bool {
        return (std::isalnum(static_cast<unsigned char>(c)) || (c == '+') || (c == '/'));
    };

    std::string decoded;
    size_t in_len = encoded.size();
    size_t i = 0;
    unsigned char char_array_4[4];
    unsigned char char_array_3[3];
    int count = 0;

    while (i < in_len && encoded[i] != '=' && is_base64(encoded[i])) {
        char_array_4[count++] = encoded[i++];
        if (count == 4) {
            for (int j = 0; j < 4; j++) {
                char_array_4[j] = static_cast<unsigned char>(find_char(char_array_4[j]));
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (int j = 0; j < 3; j++) {
                decoded += static_cast<char>(char_array_3[j]);
            }
            count = 0;
        }
    }

    if (count > 0) {
        for (int j = count; j < 4; j++) {
            char_array_4[j] = 0;
        }
        for (int j = 0; j < 4; j++) {
            int idx = find_char(char_array_4[j]);
            char_array_4[j] = (idx >= 0) ? static_cast<unsigned char>(idx) : 0;
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (int j = 0; j < count - 1; j++) {
            decoded += static_cast<char>(char_array_3[j]);
        }
    }

    return decoded;
}
