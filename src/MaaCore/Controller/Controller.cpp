#include "Controller.h"

#include "Utils/Platform.hpp"

#ifdef _WIN32
#include "ControllerWin32.h"
#else
#include "ControllerPosix.h"
#endif
#include "ControllerAdbLite.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <regex>
#include <utility>
#include <vector>

#include "Assistant.h"
#include "Common/AsstConf.h"
#include "Utils/NoWarningCV.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4068)
#endif
#include <zlib/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Common/AsstTypes.h"
#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/WorkingDir.hpp"

asst::Controller::Controller(const AsstCallback& callback, Assistant* inst)
    : InstHelper(inst), m_callback(callback), m_rand_engine(std::random_device {}())
{
    LogTraceFunction;

#ifdef _WIN32
    m_native_controller = std::make_shared<ControllerWin32>(inst);
#else
    m_native_controller = std::make_shared<ControllerPosix>(inst);
#endif
    m_adblite_controller = std::make_shared<ControllerAdbLite>(m_native_controller);

    m_controller_map[PlatformController::Native] = m_native_controller;
    m_controller_map[PlatformController::AdbLite] = m_adblite_controller;

    m_support_socket = current_controller()->m_support_socket;

    if (!m_support_socket) {
        Log.error("socket not supported");
    }
}

asst::Controller::~Controller()
{
    LogTraceFunction;

    release_minitouch();
    make_instance_inited(false);
    kill_adb_daemon();
}

std::pair<int, int> asst::Controller::get_scale_size() const noexcept
{
    return m_scale_size;
}

std::optional<std::string> asst::Controller::call_command(const std::string& cmd, int64_t timeout, bool allow_reconnect,
                                                          bool recv_by_socket)
{
    using namespace std::chrono_literals;
    using namespace std::chrono;
    // LogTraceScope(std::string(__FUNCTION__) + " | `" + cmd + "`");

    std::string pipe_data;
    std::string sock_data;
    asst::platform::single_page_buffer<char> pipe_buffer;
    asst::platform::single_page_buffer<char> sock_buffer;

    auto start_time = steady_clock::now();
    std::unique_lock<std::mutex> callcmd_lock(m_callcmd_mutex);

    std::optional<int> exit_res;

    exit_res = current_controller()->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout, start_time);

    if (!exit_res) {
        return std::nullopt;
    }
    const int exit_ret = exit_res.value();

    callcmd_lock.unlock();

    auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    Log.info("Call `", cmd, "` ret", exit_ret, ", cost", duration, "ms , stdout size:", pipe_data.size(),
             ", socket size:", sock_data.size());
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        Log.trace("stdout output:", Logger::separator::newline, pipe_data);
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        Log.trace("socket output:", Logger::separator::newline, sock_data);
    }
    // 直接 return，避免走到下面的 else if 里的 make_instance_inited(false) 关闭 adb 连接，
    // 导致停止后再开始任务还需要重连一次
    if (need_exit()) {
        return std::nullopt;
    }

    if (!exit_ret) {
        return recv_by_socket ? sock_data : pipe_data;
    }
    else if (inited() && allow_reconnect) {
        // 之前可以运行，突然运行不了了，这种情况多半是 adb 炸了。所以重新连接一下
        json::value reconnect_info = json::object {
            { "uuid", m_uuid },
            { "what", "Reconnecting" },
            { "why", "" },
            { "details",
              json::object {
                  { "reconnect", m_adb.connect },
                  { "cmd", cmd },
              } },
        };
        static constexpr int ReconnectTimes = 5;
        for (int i = 0; i < ReconnectTimes; ++i) {
            if (need_exit()) {
                break;
            }
            reconnect_info["details"]["times"] = i;
            callback(AsstMsg::ConnectionInfo, reconnect_info);

            sleep(10 * 1000);
            if (need_exit()) {
                break;
            }
            auto reconnect_ret = call_command(m_adb.connect, 60LL * 1000, false /* 禁止重连避免无限递归 */);
            if (need_exit()) {
                break;
            }
            bool is_reconnect_success = false;
            if (reconnect_ret) {
                auto& reconnect_str = reconnect_ret.value();
                is_reconnect_success = reconnect_str.find("error") == std::string::npos;
            }
            if (is_reconnect_success) {
                if (m_minitouch_enabled && call_and_hup_minitouch()) {
                    Log.error("reconnected with minitouch");
                    m_minitouch_available = true;
                }
                else {
                    Log.error("reconnected without minitouch");
                    m_minitouch_available = false;
                }
                auto recall_ret = call_command(cmd, timeout, false /* 禁止重连避免无限递归 */, recv_by_socket);
                if (recall_ret) {
                    // 重连并成功执行了
                    reconnect_info["what"] = "Reconnected";
                    callback(AsstMsg::ConnectionInfo, reconnect_info);
                    return recall_ret;
                }
            }
        }
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "Disconnect" },
            { "why", "Reconnect failed" },
            { "details",
              json::object {
                  { "cmd", m_adb.connect },
              } },
        };
        make_instance_inited(false); // 重连失败，释放
        callback(AsstMsg::ConnectionInfo, info);
    }

    return std::nullopt;
}

void asst::Controller::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

bool asst::Controller::call_and_hup_minitouch()
{
    LogTraceFunction;
    release_minitouch(true);

    std::string cmd = m_use_maa_touch ? m_adb.call_maatouch : m_adb.call_minitouch;
    Log.info(cmd);

    std::string pipe_str;

    bool call_minitouch_success = false;
    call_minitouch_success = current_controller()->call_and_hup_minitouch(cmd, 3, pipe_str);
    if (!call_minitouch_success) {
        return false;
    }

    Log.info("pipe str", Logger::separator::newline, pipe_str);

    convert_lf(pipe_str);
    size_t s_pos = pipe_str.find('^');
    size_t e_pos = pipe_str.find('\n', s_pos);
    if (s_pos == std::string::npos || e_pos == std::string::npos) {
        Log.error("Failed to find ^ in minitouch pipe");
        release_minitouch(true);
        return false;
    }
    std::string key_info = pipe_str.substr(s_pos + 1, e_pos - s_pos - 1);
    Log.info("minitouch key props", key_info);
    int size_1 = 0, size_2 = 0;
    std::stringstream ss;
    ss << key_info;
    ss >> m_minitouch_props.max_contacts;
    ss >> size_1;
    ss >> size_2;
    ss >> m_minitouch_props.max_pressure;

    // 有些模拟器在竖屏分辨率时，这里的输出是反过来的
    // 考虑到应该没人竖屏玩明日方舟，所以取较大值为 x，较小值为 y
    m_minitouch_props.max_x = std::max(size_1, size_2);
    m_minitouch_props.max_y = std::min(size_1, size_2);

    m_minitouch_props.x_scaling = static_cast<double>(m_minitouch_props.max_x) / m_width;
    m_minitouch_props.y_scaling = static_cast<double>(m_minitouch_props.max_y) / m_height;

    return true;
}

bool asst::Controller::input_to_minitouch(const std::string& cmd)
{
    // Log.debug("Input to minitouch", Logger::separator::newline, cmd);
    return current_controller()->input_to_minitouch(cmd);
}

void asst::Controller::release_minitouch(bool force)
{
    LogTraceFunction;

    if (!m_minitouch_available && !force) {
        return;
    }
    m_minitouch_available = false;

    current_controller()->release_minitouch(force);
}

// 返回值代表是否找到 "\r\n"，函数本身会将所有 "\r\n" 替换为 "\n"
bool asst::Controller::convert_lf(std::string& data)
{
    if (data.empty() || data.size() < 2) {
        return false;
    }
    auto pred = [](const std::string::iterator& cur) -> bool { return *cur == '\r' && *(cur + 1) == '\n'; };
    // find the first of "\r\n"
    auto first_iter = data.end();
    for (auto iter = data.begin(); iter != data.end() - 1; ++iter) {
        if (pred(iter)) {
            first_iter = iter;
            break;
        }
    }
    if (first_iter == data.end()) {
        return false;
    }
    // move forward all non-crlf elements
    auto end_r1_iter = data.end() - 1;
    auto next_iter = first_iter;
    while (++first_iter != end_r1_iter) {
        if (!pred(first_iter)) {
            *next_iter = *first_iter;
            ++next_iter;
        }
    }
    *next_iter = *end_r1_iter;
    ++next_iter;
    data.erase(next_iter, data.end());
    return true;
}

asst::Point asst::Controller::rand_point_in_rect(const Rect& rect)
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

void asst::Controller::random_delay() const
{
    auto& opt = Config.get_options();
    if (opt.control_delay_upper != 0) {
        static std::default_random_engine rand_engine(std::random_device {}());
        static std::uniform_int_distribution<unsigned> rand_uni(opt.control_delay_lower, opt.control_delay_upper);

        sleep(rand_uni(rand_engine));
    }
}

void asst::Controller::clear_info() noexcept
{
    make_instance_inited(false);
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_control_scale = 1.0;
    m_minitouch_available = false;
    m_scale_size = { WindowWidthDefault, WindowHeightDefault };
    m_minitouch_props = decltype(m_minitouch_props)();
    m_screencap_data_general_size = 0;
}

void asst::Controller::close_socket() noexcept
{
    current_controller()->close_socket();
    m_server_started = false;
}

std::optional<unsigned short> asst::Controller::init_socket(const std::string& local_address)
{
    LogTraceFunction;

    return current_controller()->init_socket(local_address);
}

bool asst::Controller::screencap(bool allow_reconnect)
{
    DecodeFunc decode_raw = [&](const std::string& data) -> bool {
        if (data.empty()) {
            return false;
        }
        size_t std_size = 4ULL * m_width * m_height;
        if (data.size() < std_size) {
            return false;
        }
        size_t header_size = data.size() - std_size;
        auto img_data_beg = data.cbegin() + header_size;
        if (std::all_of(data.cbegin(), img_data_beg, std::logical_not<bool> {})) {
            return false;
        }
        cv::Mat temp(m_height, m_width, CV_8UC4, const_cast<char*>(&*img_data_beg));
        if (temp.empty()) {
            return false;
        }
        cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    DecodeFunc decode_raw_with_gzip = [&](const std::string& data) -> bool {
        const std::string raw_data = gzip::decompress(data.data(), data.size());
        return decode_raw(raw_data);
    };

    DecodeFunc decode_encode = [&](const std::string& data) -> bool {
        cv::Mat temp = cv::imdecode({ data.data(), int(data.size()) }, cv::IMREAD_COLOR);
        if (temp.empty()) {
            return false;
        }
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    switch (m_adb.screencap_method) {
    case AdbProperty::ScreencapMethod::UnknownYet: {
        using namespace std::chrono;
        Log.info("Try to find the fastest way to screencap");
        auto min_cost = milliseconds(LLONG_MAX);
        clear_lf_info();

        auto start_time = high_resolution_clock::now();
        if (m_support_socket && m_server_started &&
            screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true)) {
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawByNc;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("RawByNc cost", duration.count(), "ms");
        }
        else {
            Log.info("RawByNc is not supported");
        }
        clear_lf_info();

        start_time = high_resolution_clock::now();
        if (screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect)) {
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawWithGzip;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("RawWithGzip cost", duration.count(), "ms");
        }
        else {
            Log.info("RawWithGzip is not supported");
        }
        clear_lf_info();

        start_time = high_resolution_clock::now();
        if (screencap(m_adb.screencap_encode, decode_encode, allow_reconnect)) {
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::Encode;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("Encode cost", duration.count(), "ms");
        }
        else {
            Log.info("Encode is not supported");
        }
        static const std::unordered_map<AdbProperty::ScreencapMethod, std::string> MethodName = {
            { AdbProperty::ScreencapMethod::UnknownYet, "UnknownYet" },
            { AdbProperty::ScreencapMethod::RawByNc, "RawByNc" },
            { AdbProperty::ScreencapMethod::RawWithGzip, "RawWithGzip" },
            { AdbProperty::ScreencapMethod::Encode, "Encode" },
        };
        Log.info("The fastest way is", MethodName.at(m_adb.screencap_method), ", cost:", min_cost.count(), "ms");
        clear_lf_info();
        return m_adb.screencap_method != AdbProperty::ScreencapMethod::UnknownYet;
    } break;
    case AdbProperty::ScreencapMethod::RawByNc: {
        return screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true);
    } break;
    case AdbProperty::ScreencapMethod::RawWithGzip: {
        return screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect);
    } break;
    case AdbProperty::ScreencapMethod::Encode: {
        return screencap(m_adb.screencap_encode, decode_encode, allow_reconnect);
    } break;
    }

    return false;
}

bool asst::Controller::screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect,
                                 bool by_socket)
{
    if ((!m_support_socket || !m_server_started) && by_socket) [[unlikely]] {
        return false;
    }

    auto ret = call_command(cmd, 20000, allow_reconnect, by_socket);

    if (!ret || ret.value().empty()) [[unlikely]] {
        Log.error("data is empty!");
        return false;
    }
    auto& data = ret.value();
    if (m_screencap_data_general_size && data.size() < m_screencap_data_general_size * 0.1) {
        Log.error("data is too small!");
        return false;
    }

    bool tried_conversion = false;
    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::CRLF) {
        tried_conversion = true;
        if (!convert_lf(data)) [[unlikely]] { // 没找到 "\r\n"
            Log.info("screencap_end_of_line is set to CRLF but no `\\r\\n` found, set it to LF");
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
    }

    if (decode_func(data)) [[likely]] {
        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) [[unlikely]] {
            Log.info("screencap_end_of_line is LF");
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
    }
    else {
        Log.info("data is not empty, but image is empty");

        if (tried_conversion) { // 已经转换过行尾，再次转换 data 不会变化，不必重试
            Log.error("skip retry decoding and decode failed!");
            return false;
        }

        Log.info("try to cvt lf");
        if (!convert_lf(data)) { // 没找到 "\r\n"，data 没有变化，不必重试
            Log.error("no `\\r\\n` found, skip retry decode");
            return false;
        }
        if (!decode_func(data)) {
            Log.error("convert lf and retry decode failed!");
            return false;
        }

        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
            Log.info("screencap_end_of_line is CRLF");
        }
        else {
            Log.info("screencap_end_of_line is changed to CRLF");
        }
        m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;
    }
    m_screencap_data_general_size = data.size();
    return true;
}

void asst::Controller::clear_lf_info()
{
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::UnknownYet;
}

cv::Mat asst::Controller::get_resized_image_cache() const
{
    const static cv::Size d_size(m_scale_size.first, m_scale_size.second);

    std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    if (m_cache_image.empty()) {
        Log.error("image is empty");
        return { d_size, CV_8UC3 };
    }
    cv::Mat resized_mat;
    cv::resize(m_cache_image, resized_mat, d_size, 0.0, 0.0, cv::INTER_AREA);
    return resized_mat;
}

bool asst::Controller::start_game(const std::string& client_type)
{
    if (client_type.empty()) {
        return false;
    }
    auto intent_name = Config.get_intent_name(client_type);
    if (!intent_name) {
        return false;
    }
    std::string cur_cmd = utils::string_replace_all(m_adb.start, "[Intent]", intent_name.value());
    return call_command(cur_cmd).has_value();
}

bool asst::Controller::stop_game()
{
    return call_command(m_adb.stop).has_value();
}

bool asst::Controller::click(const Point& p)
{
    int x = static_cast<int>(p.x * m_control_scale);
    int y = static_cast<int>(p.y * m_control_scale);
    // log.trace("Click, raw:", p.x, p.y, "corr:", x, y);

    return click_without_scale(Point(x, y));
}

bool asst::Controller::click(const Rect& rect)
{
    return click(rand_point_in_rect(rect));
}

bool asst::Controller::click_without_scale(const Point& p)
{
    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    if (m_minitouch_enabled && m_minitouch_available) {
        Log.trace(m_use_maa_touch ? "maatouch" : "minitouch", "click:", p);
        Minitoucher toucher(std::bind(&Controller::input_to_minitouch, this, std::placeholders::_1), m_minitouch_props);
        return toucher.down(p.x, p.y) && toucher.up();
    }
    else {
        std::string cur_cmd =
            utils::string_replace_all(m_adb.click, { { "[x]", std::to_string(p.x) }, { "[y]", std::to_string(p.y) } });
        return call_command(cur_cmd).has_value();
    }
}

bool asst::Controller::click_without_scale(const Rect& rect)
{
    return click_without_scale(rand_point_in_rect(rect));
}

bool asst::Controller::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe, double slope_in,
                             double slope_out, bool with_pause)
{
    int x1 = static_cast<int>(p1.x * m_control_scale);
    int y1 = static_cast<int>(p1.y * m_control_scale);
    int x2 = static_cast<int>(p2.x * m_control_scale);
    int y2 = static_cast<int>(p2.y * m_control_scale);
    // log.trace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

    return swipe_without_scale(Point(x1, y1), Point(x2, y2), duration, extra_swipe, slope_in, slope_out, with_pause);
}

bool asst::Controller::swipe(const Rect& r1, const Rect& r2, int duration, bool extra_swipe, double slope_in,
                             double slope_out, bool with_pause)
{
    return swipe(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, extra_swipe, slope_in, slope_out,
                 with_pause);
}

bool asst::Controller::swipe_without_scale(const Point& p1, const Point& p2, int duration, bool extra_swipe,
                                           double slope_in, double slope_out, bool with_pause)
{
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    const auto& opt = Config.get_options();
    if (m_minitouch_enabled && m_minitouch_available) {
        Log.trace(m_use_maa_touch ? "maatouch" : "minitouch", "swipe", p1, p2, duration, extra_swipe, slope_in,
                  slope_out);
        Minitoucher toucher(std::bind(&Controller::input_to_minitouch, this, std::placeholders::_1), m_minitouch_props);
        toucher.down(x1, y1);

        constexpr int TimeInterval = Minitoucher::DefaultSwipeDelay;

        auto cubic_spline = [](double slope_0, double slope_1, double t) {
            const double a = slope_0;
            const double b = -(2 * slope_0 + slope_1 - 3);
            const double c = -(-slope_0 - slope_1 + 2);
            return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
        }; // TODO: move this to math.hpp

        bool need_pause = with_pause && support_swipe_with_pause();
        std::future<void> pause_future;
        auto minitouch_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) {
            for (int cur_time = TimeInterval; cur_time < _duration; cur_time += TimeInterval) {
                double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / duration);
                int cur_x = static_cast<int>(std::lerp(_x1, _x2, progress));
                int cur_y = static_cast<int>(std::lerp(_y1, _y2, progress));
                if (need_pause && std::sqrt(std::pow(cur_x - _x1, 2) + std::pow(cur_y - _y1, 2)) >
                                      opt.swipe_with_pause_required_distance) {
                    need_pause = false;
                    if (m_use_maa_touch) {
                        constexpr int EscKeyCode = 111;
                        toucher.key_down(EscKeyCode);
                        toucher.key_up(EscKeyCode, 0);
                    }
                    else {
                        pause_future = std::async(std::launch::async, [&]() { press_esc(); });
                    }
                }
                if (cur_x < 0 || cur_x > m_minitouch_props.max_x || cur_y < 0 || cur_y > m_minitouch_props.max_y) {
                    continue;
                }
                toucher.move(cur_x, cur_y);
            }
            if (_x2 >= 0 && _x2 <= m_minitouch_props.max_x && _y2 >= 0 && _y2 <= m_minitouch_props.max_y) {
                toucher.move(_x2, _y2);
            }
        };

        constexpr int DefaultDuration = 200;
        minitouch_move(x1, y1, x2, y2, duration ? duration : DefaultDuration);

        if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
            constexpr int ExtraEndDelay = 100; // 停留终点
            toucher.wait(ExtraEndDelay);
            minitouch_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
        }
        return toucher.up();
    }
    else {
        std::string duration_str =
            duration <= 0 ? "" : std::to_string(static_cast<int>(duration * opt.adb_swipe_duration_multiplier));
        std::string cur_cmd = utils::string_replace_all(m_adb.swipe, {
                                                                         { "[x1]", std::to_string(x1) },
                                                                         { "[y1]", std::to_string(y1) },
                                                                         { "[x2]", std::to_string(x2) },
                                                                         { "[y2]", std::to_string(y2) },
                                                                         { "[duration]", duration_str },
                                                                     });
        bool ret = call_command(cur_cmd).has_value();

        // 额外的滑动：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
        if (extra_swipe && opt.adb_extra_swipe_duration > 0) {
            std::string extra_cmd = utils::string_replace_all(
                m_adb.swipe, {
                                 { "[x1]", std::to_string(x2) },
                                 { "[y1]", std::to_string(y2) },
                                 { "[x2]", std::to_string(x2) },
                                 { "[y2]", std::to_string(y2 - opt.adb_extra_swipe_dist /* * m_control_scale*/) },
                                 { "[duration]", std::to_string(opt.adb_extra_swipe_duration) },
                             });
            ret &= call_command(extra_cmd).has_value();
        }
        return ret;
    }
}

bool asst::Controller::swipe_without_scale(const Rect& r1, const Rect& r2, int duration, bool extra_swipe, double v0,
                                           double v1, bool with_pause)
{
    return swipe_without_scale(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, extra_swipe, v0, v1,
                               with_pause);
}

bool asst::Controller::press_esc()
{
    LogTraceFunction;

    return call_command(m_adb.press_esc).has_value();
}

bool asst::Controller::support_swipe_with_pause() const noexcept
{
    return m_minitouch_enabled && m_minitouch_available && m_swipe_with_pause_enabled && !m_adb.press_esc.empty();
}

bool asst::Controller::support_precise_swipe() const noexcept
{
    return m_minitouch_enabled && m_minitouch_available;
}

bool asst::Controller::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    release_minitouch();
    clear_info();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        make_instance_inited(true);
        return true;
    }
#endif

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

    auto adb_ret = Config.get_adb_cfg(config);
    if (!adb_ret) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigNotFound" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    const auto& adb_cfg = adb_ret.value();
    std::string display_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    // 里面的值每次执行命令后可能更新，所以要用 lambda 拿最新的
    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(cfg_cmd, {
                                                      { "[Adb]", adb_path },
                                                      { "[AdbSerial]", address },
                                                      { "[DisplayId]", display_id },
                                                      { "[NcPort]", std::to_string(nc_port) },
                                                      { "[NcAddress]", nc_address },
                                                  });
    };

    if (need_exit()) {
        return false;
    }

    /* connect */
    {
        m_adb.connect = cmd_replace(adb_cfg.connect);
        auto connect_ret = call_command(m_adb.connect, 60LL * 1000, false /* adb 连接时不允许重试 */);
        bool is_connect_success = false;
        if (connect_ret) {
            auto& connect_str = connect_ret.value();
            is_connect_success = connect_str.find("error") == std::string::npos;
            if (connect_str.find("daemon started successfully") != std::string::npos &&
                connect_str.find("daemon still not running") == std::string::npos) {
                m_adb_release = cmd_replace(adb_cfg.release);
            }
        }
        if (!is_connect_success) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Connection command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }

    /* get uuid (imei) */
    {
        auto uuid_ret = call_command(cmd_replace(adb_cfg.uuid), 20000, false /* adb 连接时不允许重试 */);
        if (!uuid_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Uuid command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        auto& uuid_str = uuid_ret.value();
        std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });
        m_uuid = std::move(uuid_str);

        json::value info = get_info_json() | json::object {
            { "what", "UuidGot" },
            { "why", "" },
        };
        info["details"]["uuid"] = m_uuid;
        callback(AsstMsg::ConnectionInfo, info);
    }

    if (need_exit()) {
        return false;
    }

    // 按需获取display ID 信息
    if (!adb_cfg.display_id.empty()) {
        auto display_id_ret = call_command(cmd_replace(adb_cfg.display_id));
        if (!display_id_ret) {
            return false;
        }

        auto& display_id_pipe_str = display_id_ret.value();
        convert_lf(display_id_pipe_str);
        auto last = display_id_pipe_str.rfind(':');
        if (last == std::string::npos) {
            return false;
        }

        display_id = display_id_pipe_str.substr(last + 1);
        // 去掉换行
        display_id.pop_back();
    }

    if (need_exit()) {
        return false;
    }

    /* display */
    {
        auto display_ret = call_command(cmd_replace(adb_cfg.display));
        if (!display_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Display command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        std::stringstream display_ss(display_ret.value());
        int size_value1 = 0;
        int size_value2 = 0;
        display_ss >> size_value1 >> size_value2;

        m_width = (std::max)(size_value1, size_value2);
        m_height = (std::min)(size_value1, size_value2);

        json::value info = get_info_json() | json::object {
            { "what", "ResolutionGot" },
            { "why", "" },
        };

        info["details"] |= json::object {
            { "width", m_width },
            { "height", m_height },
        };

        callback(AsstMsg::ConnectionInfo, info);

        if (m_width == 0 || m_height == 0) {
            info["what"] = "ResolutionError";
            info["why"] = "Get resolution failed";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        else if (m_width < WindowWidthDefault || m_height < WindowHeightDefault) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Low screen resolution";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        else if (std::fabs(static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault) -
                           static_cast<double>(m_width) / static_cast<double>(m_height)) > 1e-7) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Not 16:9";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }

    /* calc ratio */
    {
        constexpr double DefaultRatio =
            static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
        double cur_ratio = static_cast<double>(m_width) / static_cast<double>(m_height);

        if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9，按照高度计算缩放
            || std::fabs(cur_ratio - DefaultRatio) < DoubleDiff) {
            int scale_width = static_cast<int>(cur_ratio * WindowHeightDefault);
            m_scale_size = std::make_pair(scale_width, WindowHeightDefault);
            m_control_scale = static_cast<double>(m_height) / static_cast<double>(WindowHeightDefault);
        }
        else { // 否则可能是偏正方形的屏幕，按宽度计算
            int scale_height = static_cast<int>(WindowWidthDefault / cur_ratio);
            m_scale_size = std::make_pair(WindowWidthDefault, scale_height);
            m_control_scale = static_cast<double>(m_width) / static_cast<double>(WindowWidthDefault);
        }
    }

    {
        json::value info = get_info_json() | json::object {
            { "what", "Connected" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    m_adb.click = cmd_replace(adb_cfg.click);
    m_adb.swipe = cmd_replace(adb_cfg.swipe);
    m_adb.press_esc = cmd_replace(adb_cfg.press_esc);
    m_adb.screencap_raw_with_gzip = cmd_replace(adb_cfg.screencap_raw_with_gzip);
    m_adb.screencap_encode = cmd_replace(adb_cfg.screencap_encode);
    m_adb_release = m_adb.release = cmd_replace(adb_cfg.release);
    m_adb.start = cmd_replace(adb_cfg.start);
    m_adb.stop = cmd_replace(adb_cfg.stop);

    if (m_support_socket && !m_server_started) {
        std::string bind_address;
        if (size_t pos = address.rfind(':'); pos != std::string::npos) {
            bind_address = address.substr(0, pos);
        }
        else {
            bind_address = "127.0.0.1";
        }

        // reference from
        // https://github.com/ArknightsAutoHelper/ArknightsAutoHelper/blob/master/automator/connector/ADBConnector.py#L436
        auto nc_address_ret = call_command(cmd_replace(adb_cfg.nc_address));
        if (nc_address_ret && !m_server_started) {
            auto& nc_result_str = nc_address_ret.value();
            if (auto pos = nc_result_str.find(' '); pos != std::string::npos) {
                nc_address = nc_result_str.substr(0, pos);
            }
        }

        auto socket_opt = init_socket(bind_address);
        if (socket_opt) {
            nc_port = socket_opt.value();
            m_adb.screencap_raw_by_nc = cmd_replace(adb_cfg.screencap_raw_by_nc);
            m_server_started = true;
        }
        else {
            m_server_started = false;
        }
    }

    if (need_exit()) {
        return false;
    }

    while (m_minitouch_enabled) {
        m_minitouch_available = false;

        std::string_view touch_program;
        if (m_use_maa_touch) {
            touch_program = "maatouch";
            m_minitouch_props.orientation = 0;
        }
        else {
            std::string abilist = call_command(cmd_replace(adb_cfg.abilist)).value_or(std::string());
            for (const auto& abi : Config.get_options().minitouch_programs_order) {
                if (abilist.find(abi) != std::string::npos) {
                    touch_program = abi;
                    break;
                }
            }
            std::string orientation_str = call_command(cmd_replace(adb_cfg.orientation)).value_or("0");
            if (!orientation_str.empty()) {
                char first = orientation_str.front();
                if (first == '0' || first == '1' || first == '2' || first == '3') {
                    m_minitouch_props.orientation = static_cast<int>(first - '0');
                }
            }
        }
        Log.info("touch_program", touch_program, "orientation", m_minitouch_props.orientation);

        if (touch_program.empty()) break;

        auto minitouch_cmd_rep = [&](const std::string& cfg_cmd) -> std::string {
            using namespace asst::utils::path_literals;
            return utils::string_replace_all(
                cmd_replace(cfg_cmd),
                {
                    { "[minitouchLocalPath]",
                      utils::path_to_utf8_string(ResDir.get() / "minitouch"_p / touch_program / "minitouch"_p) },
                    { "[minitouchWorkingFile]", m_uuid },
                });
        };

        if (!call_command(minitouch_cmd_rep(adb_cfg.push_minitouch))) break;
        if (!call_command(minitouch_cmd_rep(adb_cfg.chmod_minitouch))) break;

        m_adb.call_minitouch = minitouch_cmd_rep(adb_cfg.call_minitouch);
        m_adb.call_maatouch = minitouch_cmd_rep(adb_cfg.call_maatouch);

        if (!call_and_hup_minitouch()) break;

        m_minitouch_available = true;
        break;
    };

    if (m_minitouch_enabled && !m_minitouch_available) {
        json::value info = get_info_json() | json::object {
            { "what", "TouchModeNotAvailable" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    // try to find the fastest way
    if (!screencap()) {
        Log.error("Cannot find a proper way to screencap!");
        return false;
    }

    return true;
}

void asst::Controller::make_instance_inited(bool inited)
{
    Log.trace(__FUNCTION__, "|", inited, ", pre m_inited =", m_inited, ", pre m_instance_count =", m_instance_count);

    if (inited == m_inited) {
        return;
    }
    m_inited = inited;

    if (inited) {
        ++m_instance_count;
    }
    else {
        // 所有实例全部释放，执行最终的 release 函数
        if (!--m_instance_count) {
            release();
        }
    }
}

void asst::Controller::kill_adb_daemon()
{
    if (m_instance_count) return;
    if (!m_adb_release.empty()) {
        m_adblite_controller->release_adb(m_adb_release, 20000);
        m_adb_release.clear();
    }
}

void asst::Controller::release()
{
    close_socket();

    if (!m_adb.release.empty()) {
        m_adb_release.clear();
        m_adblite_controller->release_adb(m_adb.release, 20000);
    }
}

bool asst::Controller::inited() const noexcept
{
    return m_inited;
}

void asst::Controller::set_minitouch_enabled(bool enable, bool maa_touch) noexcept
{
    m_minitouch_enabled = enable;
    m_use_maa_touch = maa_touch;
}

void asst::Controller::set_swipe_with_pause(bool enable) noexcept
{
    m_swipe_with_pause_enabled = enable;
}

void asst::Controller::set_adb_lite_enabled(bool enable) noexcept
{
    m_platform_controller = enable ? PlatformController::AdbLite : PlatformController::Native;
}

const std::string& asst::Controller::get_uuid() const
{
    return m_uuid;
}

cv::Mat asst::Controller::get_image(bool raw)
{
    if (m_scale_size.first == 0 || m_scale_size.second == 0) {
        Log.error("Unknown image size");
        return {};
    }

    // 有些模拟器adb偶尔会莫名其妙截图失败，多试几次
    static constexpr int MaxTryCount = 20;
    bool success = false;
    for (int i = 0; i < MaxTryCount && inited(); ++i) {
        if (need_exit()) {
            break;
        }
        if (screencap()) {
            success = true;
            break;
        }
    }
    while (!success && !need_exit()) {
        if (screencap(true)) {
            break;
        }
        Log.error(__FUNCTION__, "screencap failed!");
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapFailed" },
            { "why", "ScreencapFailed" },
            { "details", json::object {} },
        };
        callback(AsstMsg::ConnectionInfo, info);

        const static cv::Size d_size(m_scale_size.first, m_scale_size.second);
        m_cache_image = cv::Mat(d_size, CV_8UC3);

        break;
    }

    if (raw) {
        std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
        cv::Mat copy = m_cache_image.clone();
        return copy;
    }

    return get_resized_image_cache();
}

cv::Mat asst::Controller::get_image_cache() const
{
    return get_resized_image_cache();
}
