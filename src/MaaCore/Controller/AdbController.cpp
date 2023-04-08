#include "AdbController.h"

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

asst::AdbController::AdbController(const AsstCallback& callback, Assistant* inst, PlatformType type)
    : InstHelper(inst), m_callback(callback)
{
    LogTraceFunction;

    m_platform_io = PlatformFactory::create_platform(inst, type);

    if (!m_platform_io) {
        Log.error("platform not supported");
        throw std::runtime_error("platform not supported");
    }

    m_support_socket = m_platform_io->m_support_socket;

    if (!m_support_socket) {
        Log.error("socket not supported");
    }
}

asst::AdbController::~AdbController()
{
    LogTraceFunction;

    make_instance_inited(false);
    kill_adb_daemon();
}

std::optional<std::string> asst::AdbController::call_command(const std::string& cmd, int64_t timeout,
                                                             bool allow_reconnect, bool recv_by_socket)
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

    exit_res = m_platform_io->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout, start_time);

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

void asst::AdbController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

void asst::AdbController::close_socket() noexcept
{
    m_platform_io->close_socket();
}

std::optional<unsigned short> asst::AdbController::init_socket(const std::string& local_address)
{
    return m_platform_io->init_socket(local_address);
}

void asst::AdbController::clear_info() noexcept
{
    make_instance_inited(false);
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_screen_size = { 0, 0 };
    m_screencap_data_general_size = 0;
}

bool asst::AdbController::inited() const noexcept
{
    return m_inited;
}

bool asst::AdbController::start_game(const std::string& client_type)
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

bool asst::AdbController::stop_game()
{
    return call_command(m_adb.stop).has_value();
}

bool asst::AdbController::click(const Point& p)
{
    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    std::string cur_cmd =
        utils::string_replace_all(m_adb.click, { { "[x]", std::to_string(p.x) }, { "[y]", std::to_string(p.y) } });
    return call_command(cur_cmd).has_value();
}

bool asst::AdbController::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe,
                                [[maybe_unused]] double slope_in, [[maybe_unused]] double slope_out,
                                [[maybe_unused]] bool with_pause)
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

bool asst::AdbController::press_esc()
{
    LogTraceFunction;

    return call_command(m_adb.press_esc).has_value();
}

std::pair<int, int> asst::AdbController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void asst::AdbController::release()
{
    close_socket();

    if (!m_adb.release.empty()) {
        m_adb_release.clear();
        m_platform_io->release_adb(m_adb.release, 20000);
    }
}

void asst::AdbController::kill_adb_daemon()
{
    if (m_instance_count) return;
    if (!m_adb_release.empty()) {
        m_platform_io->release_adb(m_adb_release, 20000);
        m_adb_release.clear();
    }
}

void asst::AdbController::make_instance_inited(bool inited)
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

const std::string& asst::AdbController::get_uuid() const
{
    return m_uuid;
}

bool asst::AdbController::convert_lf(std::string& data)
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

bool asst::AdbController::screencap(cv::Mat& image_payload, bool allow_reconnect)
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
        image_payload = temp;
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
        image_payload = temp;
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

bool asst::AdbController::screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect,
                                    bool by_socket)
{
    if ((!m_support_socket || !m_server_started) && by_socket) [[unlikely]] {
        return false;
    }

    auto ret = call_command(cmd, 20000, allow_reconnect, by_socket);

    if (!ret || ret.value().empty()) [[unlikely]] {
        Log.warn("data is empty!");
        return false;
    }
    auto& data = ret.value();
    if (m_screencap_data_general_size && data.size() < m_screencap_data_general_size * 0.1) {
        Log.warn("data is too small!");
        // return false;
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

bool asst::AdbController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

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
        m_screen_size = { m_width, m_height };
    }

    if (need_exit()) {
        return false;
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
    return true;
}

void asst::AdbController::clear_lf_info()
{
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::UnknownYet;
}
