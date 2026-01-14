#include "WDAController.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <ranges>
#include <sstream>
#include <thread>

#include <boost/asio.hpp>
#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

using boost::asio::ip::tcp;
using namespace std::chrono;

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

    // 设置极速模式：禁用等待空闲和动画冷却
    if (!configure_fast_mode()) {
        Log.warn("Failed to configure fast mode, operations may be slower");
        // 不返回 false，因为这不是致命错误
    }

    if (!fetch_screen_size()) {
        Log.error("Failed to fetch WDA logical screen size");
        return false;
    }

    m_connected = true;
    Log.info("WDA connected, logical size:", m_wda_logical_size.first, "x", m_wda_logical_size.second,
             "(Physical size will be determined from first screenshot)");
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

    auto start_time = high_resolution_clock::now();

    auto response = http_get("/screenshot");
    if (!response.success()) {
        Log.error("Screenshot request failed, status:", response.status_code);

        // 记录失败的截图
        m_screencap_cost.emplace_back(-1);
        ++m_screencap_times;
        report_screencap_cost();

        return false;
    }

    auto json_opt = json::parse(response.body);
    if (!json_opt) {
        Log.error("Failed to parse screenshot response");

        m_screencap_cost.emplace_back(-1);
        ++m_screencap_times;
        report_screencap_cost();

        return false;
    }

    std::string base64_data;
    if (json_opt->contains("value") && json_opt->at("value").is_string()) {
        base64_data = json_opt->at("value").as_string();
    }
    else {
        Log.error("Screenshot response missing 'value' field");

        m_screencap_cost.emplace_back(-1);
        ++m_screencap_times;
        report_screencap_cost();

        return false;
    }

    bool decode_success = decode_base64_png(base64_data, image_payload);

    // 记录截图耗时
    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
    m_screencap_cost.emplace_back(decode_success ? duration.count() : -1);
    ++m_screencap_times;

    report_screencap_cost();

    return decode_success;
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
    // 三层坐标转换
    int physical_x = p.x + m_crop_offset_x;
    int physical_y = p.y + m_crop_offset_y;

    auto [wda_x, wda_y] = transform_to_wda_coords(p.x, p.y);

    Log.trace("WDA click: MAA(", p.x, ",", p.y, ") -> physical(",
              physical_x, ",", physical_y, ") -> WDA(", wda_x, ",", wda_y, ")");

    std::string action_json = build_w3c_tap_action(wda_x, wda_y);
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
    // Clamp在MAA坐标空间
    int x1 = std::clamp(p1.x, 0, m_screen_size.first - 1);
    int y1 = std::clamp(p1.y, 0, m_screen_size.second - 1);
    int x2 = std::clamp(p2.x, 0, m_screen_size.first - 1);
    int y2 = std::clamp(p2.y, 0, m_screen_size.second - 1);

    // 转换到WDA逻辑坐标
    auto [wda_x1, wda_y1] = transform_to_wda_coords(x1, y1);
    auto [wda_x2, wda_y2] = transform_to_wda_coords(x2, y2);

    Log.trace("WDA swipe: MAA(", x1, ",", y1, "->", x2, ",", y2,
              ") -> WDA(", wda_x1, ",", wda_y1, "->", wda_x2, ",", wda_y2,
              ") duration:", duration);

    const auto& opt = Config.get_options();
    int actual_duration = duration > 0 ? duration : opt.minitouch_swipe_default_duration;

    std::string action_json = build_w3c_swipe_action(wda_x1, wda_y1, wda_x2, wda_y2, actual_duration);
    auto response = http_post("/session/" + m_session_id + "/actions", action_json);

    if (!response.success()) {
        Log.error("WDA swipe failed, status:", response.status_code);
        return false;
    }

    // extra_swipe处理
    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(opt.minitouch_swipe_extra_end_delay));

        // 计算extra swipe的终点（MAA坐标空间）
        int extra_y2 = y2 - opt.minitouch_extra_swipe_dist;
        auto [extra_wda_x2, extra_wda_y2] = transform_to_wda_coords(x2, extra_y2);

        std::string extra_action = build_w3c_swipe_action(
            wda_x2, wda_y2, extra_wda_x2, extra_wda_y2, opt.minitouch_extra_swipe_duration);
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

bool asst::WDAController::configure_fast_mode()
{
    // 设置 WDA 的极速模式：禁用等待空闲和动画冷却
    // 这是实现低延迟点击的关键
    json::object settings;
    settings["settings"] = json::object {
        { "waitForIdleTimeout", 0 },        // 不等待 App 空闲
        { "animationCoolOffTimeout", 0 },   // 点击后不等待动画冷却
    };

    auto response = http_post("/session/" + m_session_id + "/appium/settings", settings.to_string());
    if (!response.success()) {
        Log.warn("Failed to configure fast mode, status:", response.status_code);
        return false;
    }

    Log.info("WDA fast mode configured (waitForIdleTimeout=0, animationCoolOffTimeout=0)");
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
        m_wda_logical_size.first = static_cast<int>(rect->at("width").as_integer());
        m_wda_logical_size.second = static_cast<int>(rect->at("height").as_integer());

        Log.info("WDA logical size:", m_wda_logical_size.first, "x", m_wda_logical_size.second);
        return m_wda_logical_size.first > 0 && m_wda_logical_size.second > 0;
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

std::pair<int, int> asst::WDAController::transform_to_wda_coords(int maa_x, int maa_y) const
{
    // 检查是否已获取物理分辨率和WDA逻辑分辨率
    if (m_physical_screen_size.first == 0 || m_wda_logical_size.first == 0) {
        Log.error("Cannot transform coordinates: physical or logical size not initialized");
        // 降级：直接返回MAA坐标
        return { maa_x, maa_y };
    }

    // Step 1: MAA坐标 → 物理像素坐标
    int physical_x = maa_x + m_crop_offset_x;
    int physical_y = maa_y + m_crop_offset_y;

    // Step 2: 物理像素坐标 → WDA逻辑坐标
    float scale_x = static_cast<float>(m_physical_screen_size.first) /
                    static_cast<float>(m_wda_logical_size.first);
    float scale_y = static_cast<float>(m_physical_screen_size.second) /
                    static_cast<float>(m_wda_logical_size.second);

    int wda_x = static_cast<int>(std::round(physical_x / scale_x));
    int wda_y = static_cast<int>(std::round(physical_y / scale_y));

    // 边界检查
    wda_x = std::clamp(wda_x, 0, m_wda_logical_size.first - 1);
    wda_y = std::clamp(wda_y, 0, m_wda_logical_size.second - 1);

    return { wda_x, wda_y };
}

bool asst::WDAController::decode_base64_png(const std::string& base64_data, cv::Mat& output)
{
    std::string decoded = base64_decode(base64_data);
    if (decoded.empty()) {
        Log.error("Base64 decode failed");
        return false;
    }

    std::vector<uint8_t> png_data(decoded.begin(), decoded.end());
    cv::Mat raw_image = cv::imdecode(png_data, cv::IMREAD_COLOR);

    if (raw_image.empty()) {
        Log.error("PNG decode failed");
        return false;
    }

    int original_width = raw_image.cols;
    int original_height = raw_image.rows;
    double original_aspect = static_cast<double>(original_width) / original_height;
    constexpr double target_aspect = 16.0 / 9.0;

    if (m_physical_screen_size.first == 0) {
        m_physical_screen_size = { original_width, original_height };
        Log.info("Physical screen size detected:", original_width, "x", original_height);
    }

    // ControlScaleProxy 的检查阈值是 1e-7，所以这里也必须更严格
    if (std::abs(original_aspect - target_aspect) < 1e-6) {
        output = raw_image;
        m_crop_offset_x = 0;
        m_crop_offset_y = 0;
        m_screen_size = { original_width, original_height };
        Log.info("Aspect ratio is already 16:9, no crop needed");
        return true;
    }

    if (original_aspect > target_aspect) {
        // 裁剪左右，保持高度
        // 为确保精确的16:9，需要调整高度到能被9整除的数，或宽度到能被16整除的数
        // 选择调整高度以最小化损失
        int adjusted_height = (original_height / 9) * 9;  // 确保能被9整除
        int target_width = (adjusted_height * 16) / 9;     // 精确的16:9

        int crop_y = (original_height - adjusted_height) / 2;
        m_crop_offset_x = (original_width - target_width) / 2;
        m_crop_offset_y = crop_y;

        cv::Rect crop_region(m_crop_offset_x, crop_y, target_width, adjusted_height);
        output = raw_image(crop_region).clone();

        m_screen_size = { target_width, adjusted_height };
        Log.info("Cropped image from", original_width, "x", original_height,
                 "to", target_width, "x", adjusted_height, "(16:9 aspect ratio, offset_x:", m_crop_offset_x, ", offset_y:", m_crop_offset_y, ")");
    }
    else {
        // 裁剪上下，保持宽度
        // 为确保精确的16:9，需要调整宽度到能被16整除的数
        int adjusted_width = (original_width / 16) * 16;   // 确保能被16整除
        int target_height = (adjusted_width * 9) / 16;     // 精确的16:9

        int crop_x = (original_width - adjusted_width) / 2;
        m_crop_offset_x = crop_x;
        m_crop_offset_y = (original_height - target_height) / 2;

        cv::Rect crop_region(crop_x, m_crop_offset_y, adjusted_width, target_height);
        output = raw_image(crop_region).clone();

        m_screen_size = { adjusted_width, target_height };
        Log.info("Cropped image from", original_width, "x", original_height,
                 "to", adjusted_width, "x", target_height, "(16:9 aspect ratio, offset_x:", m_crop_offset_x, ", offset_y:", m_crop_offset_y, ")");
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

void asst::WDAController::report_screencap_cost()
{
    if (m_screencap_cost.size() > 30) {
        m_screencap_cost.pop_front();
    }

    // 每 10 次截图计算一次平均耗时
    if (m_screencap_times >= 10) {
        m_screencap_times = 0;

        auto filtered_cost = m_screencap_cost | std::views::filter([](auto num) { return num > 0; });
        if (filtered_cost.empty()) {
            return;
        }

        // 过滤后的有效截图用时次数
        auto filtered_count = m_screencap_cost.size() - std::ranges::count(m_screencap_cost, -1);
        auto [screencap_cost_min, screencap_cost_max] = std::ranges::minmax(filtered_cost);

        json::value info = json::object {
            { "uuid", get_uuid() },
            { "what", "ScreencapCost" },
            { "details",
              json::object {
                  { "min", screencap_cost_min },
                  { "max", screencap_cost_max },
                  { "avg",
                    filtered_count > 0
                        ? std::accumulate(filtered_cost.begin(), filtered_cost.end(), 0ll) / filtered_count
                        : -1 },
              } },
        };

        if (m_screencap_cost.size() > filtered_count) {
            info["details"]["fault_times"] = m_screencap_cost.size() - filtered_count;
        }

        m_callback(AsstMsg::ConnectionInfo, info, m_inst);
    }
}
