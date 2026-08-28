#include "AdbController.h"

#include "Assistant.h"
#include "Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include <cmath>
#include <cstdint>
#include <numeric>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4068)
#endif
#include <zlib/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Common/AsstTypes.h"
#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"

#include <boost/regex.hpp>

asst::AdbController::AdbController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
    InstHelper(inst),
    m_callback(callback)
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

    m_inited = false;
    // 等待异步帧率检测结束，避免线程访问已析构的 this
    if (m_fps_future.valid()) {
        m_fps_future.wait();
    }
    release();
}

std::optional<std::string> asst::AdbController::reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket)
{
    LogTraceFunction;

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
    m_inited = false; // 重连失败，释放
    release();
    callback(AsstMsg::ConnectionInfo, info);

    return std::nullopt;
}

std::optional<std::string> asst::AdbController::call_command(
    const std::string& cmd,
    int64_t timeout,
    bool allow_reconnect,
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

    exit_res = m_platform_io->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout, start_time);

    if (!exit_res) {
        Log.warn("Call `", cmd, "` failed");
        return std::nullopt;
    }
    const int exit_ret = exit_res.value();

    callcmd_lock.unlock();

    m_last_command_duration = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    Log.info(
        "Call `",
        cmd,
        "` ret",
        exit_ret,
        ", cost",
        m_last_command_duration,
        "ms , stdout size:",
        pipe_data.size(),
        ", socket size:",
        sock_data.size());
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        m_pipe_data_size = pipe_data.size();
        Log.trace("stdout output:", Logger::separator::newline, pipe_data);
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        Log.trace("socket output:", Logger::separator::newline, sock_data);
    }
    // 直接 return，避免走到下面的 else if 里的 m_inited = false) 关闭 adb 连接，
    // 导致停止后再开始任务还需要重连一次
    if (need_exit()) {
        return std::nullopt;
    }

    if (!exit_ret) {
        return recv_by_socket ? sock_data : pipe_data;
    }
    else if (inited() && allow_reconnect) {
        // 之前可以运行，突然运行不了了，这种情况多半是 adb 炸了。所以重新连接一下
        return reconnect(cmd, timeout, recv_by_socket);
    }

    return std::nullopt;
}

size_t asst::AdbController::get_pipe_data_size() const noexcept
{
    return m_pipe_data_size;
}

size_t asst::AdbController::get_version() const noexcept
{
    return m_version;
}

void asst::AdbController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

std::optional<int> asst::AdbController::get_mumu_index(const std::string& address)
{
    LogTrace << VAR(address);

    // 新版 MuMu 支持 emulator-xxxx 格式的设备号（如 emulator-5554），
    // 实例索引 = (port - 5554) / 2
    if (address.starts_with("emulator-")) {
        constexpr int base_emulator_port = 5554;
        std::string_view port_sv = std::string_view(address).substr(9); // after "emulator-"
        int port = 0;
        if (!utils::chars_to_number<int, true>(port_sv, port)) {
            Log.error("emulator port is invalid", port_sv);
            return std::nullopt;
        }
        // emulator 控制台端口从 5554 起步进 2（5554, 5556, ...），
        // 奇数端口是 adb 端口而非控制台端口，不在 emulator-xxxx 格式中出现
        if (port < base_emulator_port || (port - base_emulator_port) % 2 != 0) {
            Log.error("emulator port is out of range or not aligned", port);
            return std::nullopt;
        }
        int mumu_index = (port - base_emulator_port) / 2;
        LogInfo << VAR(port_sv) << VAR(port) << VAR(mumu_index);
        return mumu_index;
    }

    auto pos = address.find(":");
    if (pos == std::string::npos) {
        Log.error("address is invalid", address);
        return std::nullopt;
    }

    std::string port_str = address.substr(pos + 1);
    if (port_str.empty() || !std::ranges::all_of(port_str, [](const char& c) -> bool { return std::isdigit(c); })) {
        Log.error("port is invalid", port_str);
        return std::nullopt;
    }
    int port = std::stoi(port_str);
    int mumu_index = 0;
    if (port >= 16384) {
        // port = 16384 + (index % 32) * 32 + ((offset + floor(index/32) * 4) % 32)
        // 设 i = (index % 32) 是 0~31
        // 不考虑 index 超过 256 的情况，设 j = floor(index/32)，只能是 0~7
        // 于是 j * 4 的取值范围是 0, 4, 8, ..., 28，全部小于 32，所以取模后就是它本身
        // offset 的取整为 0~31，但只有端口被占用时才会增加，所以不影响正常情况下的连续性
        // 所以公式简化为：
        //     port = 16384 + (index % 32) * 32 + floor(index/32) * 4 = 16384 + i * 32 + j * 4
        // 设 k = (port - 16384) / 4，则 k = i * 8 + j
        // index = j * 32 + i = (k & 7) * 32 + (k >> 3) = ((k & 7) << 5) + (k >> 3)
        int k = (port - 16384) / 4;
        mumu_index = ((k & 7) << 5) | (k >> 3);
    }
    else if (port == 7555) {
        mumu_index = 0;
        LogWarn << "Port 7555 is deprecated for MuMu6, please use 16384 or above.";
    }
    else if (port >= 5555) {
        mumu_index = (port - 5555) / 2;
    }
    else {
        Log.error("port is not in a valid MuMu range", port);
        return std::nullopt;
    }
    LogInfo << VAR(port_str) << VAR(port) << VAR(mumu_index);
    return mumu_index;
}

void asst::AdbController::init_mumu_extras(const AdbCfg& adb_cfg, const std::string& address)
{
#if !ASST_WITH_EMULATOR_EXTRAS
    std::ignore = adb_cfg;
    std::ignore = address;
    Log.error("MaaCore is not compiled with ASST_WITH_EMULATOR_EXTRAS");
#else
    if (adb_cfg.extras.empty()) {
        LogWarn << "adb_cfg.extras is empty";
        return;
    }

    // extras.client_type 由 GUI 传入；若缺失则回退到 instance option 的 client_type
    std::string client_type = adb_cfg.extras.get("client_type", "");
    if (client_type.empty() && ctrler()) {
        client_type = ctrler()->get_client_type();
    }
    set_mumu_package(client_type);

    std::filesystem::path mumu_path = utils::path(adb_cfg.extras.get("path", ""));
    // 触控需要额外探测 MuMuManager 版本，只截图的用户不该付这份开销
    bool enable_input = adb_cfg.extras.get("touch", false);

    if (adb_cfg.extras.contains("index")) { // MuMu index is provided directly
        m_mumu_extras.init(mumu_path, adb_cfg.extras.get("index", 0), enable_input);
    }
    else {
        auto mumu_index = get_mumu_index(address);
        if (!mumu_index) {
            LogError << "Failed to parse MuMu index from address, skip MumuExtras init" << VAR(address);
            return;
        }
        m_mumu_extras.init(mumu_path, mumu_index.value(), enable_input);
    }
#endif
}

void asst::AdbController::set_mumu_package(const std::string& client_type)
{
#if !ASST_WITH_EMULATOR_EXTRAS
    std::ignore = client_type;
    Log.error("MaaCore is not compiled with ASST_WITH_EMULATOR_EXTRAS");
#else
    // MuMu get_display_id 需要真实包名。client_type 为空时默认官服明日方舟
    const std::string type = client_type.empty() ? "Official" : client_type;
    std::string package_name = Config.get_package_name(type).value_or("com.hypergryph.arknights");
    LogInfo << "MuMu package" << VAR(type) << VAR(package_name);
    m_mumu_extras.set_package_name(package_name);
#endif
}

std::optional<int> asst::AdbController::get_ld_index(const std::string& address)
{
    LogTrace << VAR(address);

    // emulator-5554
    if (address.starts_with("emulator-")) {
        constexpr int base_emulator_port = 5554;
        std::string_view port_sv = std::string_view(address).substr(9); // after "emulator-"
        int port = 0;
        if (!utils::chars_to_number<int, true>(port_sv, port)) {
            Log.error("emulator port is invalid", port_sv);
            return std::nullopt;
        }
        // emulator 控制台端口从 5554 起步进 2（5554, 5556, ...），
        // 奇数端口是 adb 端口而非控制台端口，不在 emulator-xxxx 格式中出现
        if (port < base_emulator_port || (port - base_emulator_port) % 2 != 0) {
            Log.error("emulator port is out of range or not aligned", port);
            return std::nullopt;
        }
        int index = (port - base_emulator_port) / 2;
        LogInfo << VAR(port_sv) << VAR(port) << VAR(index);
        return index;
    }

    // 127.0.0.1:5555
    auto pos = address.find(':');
    if (pos != std::string::npos && address.substr(0, pos) == "127.0.0.1") {
        constexpr int base_adb_port = 5555;
        std::string port_str = address.substr(pos + 1);
        if (port_str.empty() || !std::ranges::all_of(port_str, [](char c) { return std::isdigit(c); })) {
            Log.error("adb port is invalid", port_str);
            return std::nullopt;
        }
        int port = std::stoi(port_str);
        int index = (port - base_adb_port) / 2;
        LogInfo << VAR(port_str) << VAR(port) << VAR(index);
        return index;
    }

    Log.error("address is invalid or unsupported", address);
    return std::nullopt;
}

void asst::AdbController::init_ld_extras(const AdbCfg& adb_cfg, const std::string& address)
{
#if !ASST_WITH_EMULATOR_EXTRAS
    std::ignore = adb_cfg;
    std::ignore = address;
    Log.error("MaaCore is not compiled with ASST_WITH_EMULATOR_EXTRAS");
#else
    if (adb_cfg.extras.empty()) {
        LogWarn << "adb_cfg.extras is empty";
        return;
    }

    std::filesystem::path ld_path = utils::path(adb_cfg.extras.get("path", ""));
    int ld_index;
    if (adb_cfg.extras.contains("index")) {
        ld_index = adb_cfg.extras.get("index", 0);
    }
    else {
        auto ld_index_opt = get_ld_index(address);
        if (!ld_index_opt) {
            LogError << "Failed to parse LD index from address, skip LDExtras init" << VAR(address);
            return;
        }
        ld_index = ld_index_opt.value();
    }
    int ld_pid = adb_cfg.extras.get("pid", 0);
    m_ld_extras.init(ld_path, ld_index, ld_pid, m_width, m_height);
#endif
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
    m_inited = false;
    // 等待可能仍在执行的异步帧率检测
    if (m_fps_future.valid()) {
        m_fps_future.wait();
    }
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_screen_size = { 0, 0 };
    m_last_fps_check_time = {}; // 重置帧率检测计时，重连后立即检测一次
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
    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        return false;
    }

    std::string cur_cmd = utils::string_replace_all(m_adb.start, "[PackageName]", package_name.value());
    bool ret = call_command(cur_cmd).has_value();

    return ret;
}

bool asst::AdbController::stop_game(const std::string& client_type)
{
    if (client_type.empty()) {
        return false;
    }
    auto package_name = Config.get_package_name(client_type);
    if (!package_name) {
        return false;
    }
    std::string cur_cmd = utils::string_replace_all(m_adb.stop, "[PackageName]", package_name.value());
    bool ret = call_command(cur_cmd).has_value();

    return ret;
}

bool asst::AdbController::click(const Point& p)
{
    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    std::string cur_cmd =
        utils::string_replace_all(m_adb.click, { { "[x]", std::to_string(p.x) }, { "[y]", std::to_string(p.y) } });
    bool ret = call_command(cur_cmd).has_value();
    // adb click 没有内置间隔，与 minitouch/maatouch 的 DefaultClickDelay 对齐，避免高频连点丢点
    sleep(50);
    return ret;
}

bool asst::AdbController::input(const std::string& text)
{
    if (text == "") {
        Log.error("empty text");
    }

    std::string cur_cmd = utils::string_replace_all(m_adb.input, { { "[text]", text } });
    return call_command(cur_cmd).has_value();
}

bool asst::AdbController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    [[maybe_unused]] double slope_in,
    [[maybe_unused]] double slope_out,
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
    std::string cur_cmd = utils::string_replace_all(
        m_adb.swipe,
        {
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
            m_adb.swipe,
            {
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

void asst::AdbController::invalidate_connection(std::string_view reason, int width, int height)
{
    // 分辨率被外部修改后，触控倍率与截图校验等整套映射全部过期。
    // 不做原地修补：标记连接失效让任务快速失败，由上层走整体重连，
    // 重连时会重新探测分辨率并重建全部输入映射
    if (m_inited) {
        m_inited = false;
        m_connection_expired = true;
        Log.warn("Resolution changed, connection invalidated.", reason, "width", width, "height", height);
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ResolutionChanged" },
            { "why", std::string(reason) },
            { "details",
              json::object {
                  { "width", width },
                  { "height", height },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }
}

bool asst::AdbController::reprobe_screen_size()
{
    // 连接时执行 display 命令探测分辨率并刷新成员；
    // 解析失败时保留旧值，避免把已知错误写入后续所有换算
    const auto& adb_cfg = m_conn_ctx.adb_cfg;
    auto display_ret = call_command(m_conn_ctx.replace_cmd(adb_cfg.display));
    auto make_info = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", m_conn_ctx.adb_path },
                  { "address", m_conn_ctx.address },
              } },
        };
    };
    if (!display_ret) {
        json::value info = make_info() | json::object {
            { "what", "ResolutionError" },
            { "why", "Display command failed to exec" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }
    std::stringstream display_ss(display_ret.value());
    int size_value1 = 0;
    int size_value2 = 0;
    display_ss >> size_value1 >> size_value2;
    const int width = (std::max)(size_value1, size_value2);
    const int height = (std::min)(size_value1, size_value2);
    if (width == 0 || height == 0) {
        json::value info = make_info() | json::object {
            { "what", "ResolutionError" },
            { "why", "Get resolution failed" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    m_width = width;
    m_height = height;
    m_screen_size = { m_width, m_height };

    json::value info = make_info() | json::object {
        { "what", "ResolutionGot" },
        { "why", "" },
    };
    info["details"] |= json::object {
        { "width", m_width },
        { "height", m_height },
    };
    callback(AsstMsg::ConnectionInfo, info);
    return true;
}

void asst::AdbController::release()
{
    close_socket();

    if (m_kill_adb_on_exit && !m_adb.release.empty()) {
        m_platform_io->release_adb(m_adb.release, 20000);
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
    auto pred = [](const std::string::iterator& cur) -> bool {
        return *cur == '\r' && *(cur + 1) == '\n';
    };
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
    if (m_connection_expired) {
        // 分辨率已被外部修改，等待上层整体重连，不再产出画面
        return false;
    }

    using namespace std::chrono;
    DecodeFunc decode_raw = [&](const std::string& data) -> bool {
        if (data.size() < 8) {
            return false;
        }
        // assuming little endian
        uint32_t w = static_cast<uint32_t>(static_cast<unsigned char>(data[0])) << 0 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 8 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 16 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[3])) << 24;
        uint32_t h = static_cast<uint32_t>(static_cast<unsigned char>(data[4])) << 0 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[5])) << 8 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[6])) << 16 |
                     static_cast<uint32_t>(static_cast<unsigned char>(data[7])) << 24;
        if ((std::max)(int(w), int(h)) != m_width || (std::min)(int(w), int(h)) != m_height) {
            // 截图头与已知分辨率不符，通常为运行中模拟器分辨率被外部修改。
            // 分辨率变化后触控倍率等整套映射全部过期，原地修补无法覆盖所有层，
            // 标记连接失效并通知上层走整体重连，本帧按失败处理
            invalidate_connection("Size from image header", w, h);
            return false;
        }
        size_t std_size = 4ULL * w * h;
        if (data.size() < std_size) {
            return false;
        }
        const size_t header_size = data.size() - std_size; // 12 or 16. ref:
        // https://android.googlesource.com/platform/frameworks/base/+/26a2b97dbe48ee45e9ae70110714048f2f360f97%5E%21/cmds/screencap/screencap.cpp
        auto img_data_beg = data.cbegin() + header_size;
        cv::Mat temp(int(h), int(w), CV_8UC4, const_cast<char*>(&*img_data_beg));
        if (temp.empty()) {
            return false;
        }
        const auto& br = *(temp.end<cv::Vec4b>() - 1);
        if (br[3] != 255) { // only check alpha
            return false;
        }
        cv::cvtColor(temp, temp, cv::COLOR_RGBA2BGR);
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

    image_payload = cv::Mat(); // 清空缓存
    if (m_adb.screencap_method == AdbProperty::ScreencapMethod::UnknownYet) {
        std::vector<std::pair<AdbProperty::ScreencapMethod, std::string>> all_methods_cost;
        auto fastest_method = AdbProperty::ScreencapMethod::UnknownYet;

        Log.info("Try to find the fastest way to screencap");
        auto min_cost = milliseconds(LLONG_MAX);
        clear_lf_info();

        auto start_time = steady_clock::now();
        if (m_support_socket && m_server_started &&
            screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true, 5000) == ScreencapResult::Success) {
            auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
            if (duration < min_cost) {
                fastest_method = AdbProperty::ScreencapMethod::RawByNc;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("RawByNc cost", duration.count(), "ms");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::RawByNc, std::to_string(duration.count()));
        }
        else {
            Log.info("RawByNc is not supported");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::RawByNc, "???");
        }
        clear_lf_info();

        start_time = steady_clock::now();
        if (screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect) ==
            ScreencapResult::Success) {
            auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
            if (duration < min_cost) {
                fastest_method = AdbProperty::ScreencapMethod::RawWithGzip;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("RawWithGzip cost", duration.count(), "ms");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::RawWithGzip, std::to_string(duration.count()));
        }
        else {
            Log.info("RawWithGzip is not supported");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::RawWithGzip, "???");
        }
        clear_lf_info();

        start_time = steady_clock::now();
        if (screencap(m_adb.screencap_encode, decode_encode, allow_reconnect) == ScreencapResult::Success) {
            auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
            if (duration < min_cost) {
                fastest_method = AdbProperty::ScreencapMethod::Encode;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("Encode cost", duration.count(), "ms");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::Encode, std::to_string(duration.count()));
        }
        else {
            Log.info("Encode is not supported");
            all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::Encode, "???");
        }

#if ASST_WITH_EMULATOR_EXTRAS
        if (m_mumu_extras.inited()) {
            start_time = steady_clock::now();
            if (m_mumu_extras.screencap()) {
                auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
                if (duration < min_cost) {
                    fastest_method = AdbProperty::ScreencapMethod::MumuExtras;
                    m_inited = true;
                    min_cost = duration;
                }
                Log.info("MumuExtras cost", duration.count(), "ms");
                all_methods_cost.emplace_back(
                    AdbProperty::ScreencapMethod::MumuExtras,
                    std::to_string(duration.count()));
            }
            else {
                Log.info("MumuExtras is not supported");
                all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::MumuExtras, "???");
            }
        }
        if (m_ld_extras.inited()) {
            start_time = steady_clock::now();
            if (m_ld_extras.screencap()) {
                auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time);
                if (duration < min_cost) {
                    fastest_method = AdbProperty::ScreencapMethod::LDExtras;
                    m_inited = true;
                    min_cost = duration;
                }
                Log.info("LDExtras cost", duration.count(), "ms");
                all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::LDExtras, std::to_string(duration.count()));
            }
            else {
                Log.info("LDExtras is not supported");
                all_methods_cost.emplace_back(AdbProperty::ScreencapMethod::LDExtras, "???");
            }
        }
#endif

        m_adb.screencap_method = fastest_method;

        static const std::unordered_map<AdbProperty::ScreencapMethod, std::string> MethodName = {
            { AdbProperty::ScreencapMethod::UnknownYet, "UnknownYet" },
            { AdbProperty::ScreencapMethod::RawByNc, "RawByNc" },
            { AdbProperty::ScreencapMethod::RawWithGzip, "RawWithGzip" },
            { AdbProperty::ScreencapMethod::Encode, "Encode" },
#if ASST_WITH_EMULATOR_EXTRAS
            { AdbProperty::ScreencapMethod::MumuExtras, "MumuExtras" },
            { AdbProperty::ScreencapMethod::LDExtras, "LDExtras" },
#endif
        };
        Log.info("The fastest way is", MethodName.at(m_adb.screencap_method), ", cost:", min_cost.count(), "ms");
        if (m_adb.screencap_method != AdbProperty::ScreencapMethod::UnknownYet) {
            json::value info = json::object {
                { "uuid", m_uuid },
                { "what", "FastestWayToScreencap" },
                { "details",
                  json::object {
                      { "method", MethodName.at(m_adb.screencap_method) },
                      { "cost", min_cost.count() },
                  } },
            };
            json::array alt;
            for (auto& [method, cost] : all_methods_cost) {
                alt.push_back(json::object { { "method", MethodName.at(method) }, { "cost", cost } });
            }
            auto details_obj = info.at("details").as_object();
            details_obj["alternatives"] = std::move(alt);
            info.as_object()["details"] = std::move(details_obj);
            callback(AsstMsg::ConnectionInfo, info);
        }
        clear_lf_info();
        return m_adb.screencap_method != AdbProperty::ScreencapMethod::UnknownYet;
    }
    else {
        auto start_time = high_resolution_clock::now();
        ScreencapResult screencap_result = ScreencapResult::Failed;
        switch (m_adb.screencap_method) {
        case AdbProperty::ScreencapMethod::RawByNc:
            screencap_result = screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true);
            break;
        case AdbProperty::ScreencapMethod::RawWithGzip:
            screencap_result = screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect);
            break;
        case AdbProperty::ScreencapMethod::Encode:
            screencap_result = screencap(m_adb.screencap_encode, decode_encode, allow_reconnect);
            break;
#if ASST_WITH_EMULATOR_EXTRAS
        case AdbProperty::ScreencapMethod::MumuExtras: {
            auto img_opt = m_mumu_extras.screencap();
            screencap_result = img_opt.has_value() ? ScreencapResult::Success : ScreencapResult::Reprobe;

            if (screencap_result != ScreencapResult::Success && allow_reconnect) {
                m_mumu_extras.reload();
                img_opt = m_mumu_extras.screencap();
                screencap_result = img_opt.has_value() ? ScreencapResult::Success : ScreencapResult::Reprobe;
            }

            if (screencap_result == ScreencapResult::Success) {
                image_payload = img_opt.value();
            }
        } break;
        case AdbProperty::ScreencapMethod::LDExtras: {
            auto img_opt = m_ld_extras.screencap();
            screencap_result = img_opt.has_value() ? ScreencapResult::Success : ScreencapResult::Reprobe;

            if (screencap_result != ScreencapResult::Success && allow_reconnect) {
                m_ld_extras.reload();
                img_opt = m_ld_extras.screencap();
                screencap_result = img_opt.has_value() ? ScreencapResult::Success : ScreencapResult::Reprobe;
            }

            if (screencap_result == ScreencapResult::Success) {
                image_payload = img_opt.value();
            }
        } break;
#endif
        default:
            break;
        }
        const bool screencap_ret = screencap_result == ScreencapResult::Success;
        if (screencap_result == ScreencapResult::Reprobe) {
            m_adb.screencap_method = AdbProperty::ScreencapMethod::UnknownYet;
        }
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
        // 记录截图耗时，每10次截图回传一次最值+平均值
        m_screencap_cost.emplace_back(screencap_ret ? duration.count() : -1); // 记录截图耗时
        ++m_screencap_times;

        if (m_screencap_cost.size() > 30) {
            m_screencap_cost.pop_front();
        }
        if (m_screencap_times > 9) { // 每 10 次截图计算一次平均耗时
            m_screencap_times = 0;
            auto filtered_cost = m_screencap_cost | std::views::filter([](auto num) { return num > 0; });
            if (filtered_cost.empty()) {
                return screencap_ret;
            }
            // 过滤后的有效截图用时次数
            auto filtered_count = m_screencap_cost.size() - std::ranges::count(m_screencap_cost, -1);
            auto [screencap_cost_min, screencap_cost_max] = std::ranges::minmax(filtered_cost);
            json::value info = json::object {
                { "uuid", m_uuid },
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
            callback(AsstMsg::ConnectionInfo, info);
        }

        // 每 1 分钟检测一次模拟器帧率
        check_fps();

        const std::pair<int, int> current_screencap_size {
            (std::max)(image_payload.cols, image_payload.rows),
            (std::min)(image_payload.cols, image_payload.rows),
        };
        if (screencap_ret && current_screencap_size != m_last_screencap_size) {
            // 截图尺寸发生帧间变化：模拟器分辨率可能被外部修改（MumuExtras/LDExtras
            // 输出原生分辨率且无协议头校验点，Encode 路径同理，统一在此检测）。
            // 以图像自身历史尺寸为基准，连接初期 fallback 桌面等持续性差异不会误触发；
            // 首帧不触发。分辨率变化后触控倍率等整套映射全部过期，标记连接失效，
            // 通知上层走整体重连，本帧画面有效照常返回
            if (m_last_screencap_size.first != 0) {
                invalidate_connection("Screencap size changed", image_payload.cols, image_payload.rows);
            }
            m_last_screencap_size = current_screencap_size;
        }

        return screencap_ret;
    }
}

asst::AdbController::ScreencapResult asst::AdbController::screencap(
    const std::string& cmd,
    const DecodeFunc& decode_func,
    bool allow_reconnect,
    bool by_socket,
    int timeout)
{
    try {
        if ((!m_support_socket || !m_server_started) && by_socket) [[unlikely]] {
            return ScreencapResult::Failed;
        }
        auto ret = call_command(cmd, timeout, allow_reconnect, by_socket);

        if (!ret || ret.value().empty()) [[unlikely]] {
            Log.warn("data is empty!");
            return ScreencapResult::Failed;
        }
        auto& data = ret.value();

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
                return ScreencapResult::Reprobe;
            }

            Log.info("try to cvt lf");
            if (!convert_lf(data)) { // 没找到 "\r\n"，data 没有变化，不必重试
                Log.error("no `\\r\\n` found, skip retry decode");
                return ScreencapResult::Reprobe;
            }
            if (!decode_func(data)) {
                Log.error("convert lf and retry decode failed!");
                return ScreencapResult::Reprobe;
            }

            if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
                Log.info("screencap_end_of_line is CRLF");
            }
            else {
                Log.info("screencap_end_of_line is changed to CRLF");
            }
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;
        }
        return ScreencapResult::Success;
    }
    catch (const cv::Exception& e) {
        if (e.code == cv::Error::StsNoMem) {
            throw;
        }
        try {
            Log.error(
                "ADB screencap decode OpenCV exception",
                e.what(),
                "code",
                e.code,
                "file",
                e.file,
                "line",
                e.line);
        }
        catch (...) {
        }
        return ScreencapResult::Reprobe;
    }
}

bool asst::AdbController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    // 重连即全新探测，清除上一次连接期间因分辨率变化而置位的过期标记
    m_connection_expired = false;
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
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    std::string client_type = ctrler() ? ctrler()->get_client_type() : "";
    m_conn_ctx.reset(adb_path, address, client_type);

    auto adb_ret = Config.get_adb_cfg(config);
    if (!adb_ret) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigNotFound" },
        };
        callback(AsstMsg::ConnectionInfo, info);
#ifdef ASST_DEBUG
        return false;
#else
        Log.error("config ", config, "not found");
        adb_ret = Config.get_adb_cfg("General");
#endif
    }

    m_conn_ctx.adb_cfg = adb_ret.value();
    const auto& adb_cfg = m_conn_ctx.adb_cfg;

    if (need_exit()) {
        return false;
    }

    /* connect */
    {
        // 先用 devices 读取输出
        m_adb.devices = m_conn_ctx.replace_cmd(adb_cfg.devices);
        m_adb.address_regex = m_conn_ctx.replace_cmd(adb_cfg.address_regex);
        auto devices_ret = call_command(m_adb.devices, 60LL * 1000, false);
        bool need_connect = true;
        if (devices_ret) {
            const auto& devices_str = devices_ret.value();
            const boost::regex address_regex(m_adb.address_regex);
            for (boost::sregex_iterator iter(devices_str.begin(), devices_str.end(), address_regex), end; iter != end;
                 ++iter) {
                if (iter->size() > 1 && iter->str(1) == address) {
                    need_connect = false;
                    break;
                }
            }
        }

        // 设置配置 connect、release 命令，即使这里不连接，后续也会需要用到
        m_adb.connect = m_conn_ctx.replace_cmd(adb_cfg.connect);
        m_adb.release = m_conn_ctx.replace_cmd(adb_cfg.release);
        m_platform_io->set_adb_serial(address);
        if (need_connect) {
            // 如果不包含 `:` 且需要连接，connect 命令也不会成功
            if (address.find(':') == std::string::npos) {
                json::value info = get_info_json() | json::object {
                    { "what", "ConnectFailed" },
                    { "why", "Cannot connect: address appears to be serial number but device not found" },
                };
                callback(AsstMsg::ConnectionInfo, info);
                return false;
            }

            auto connect_ret = call_command(m_adb.connect, 60LL * 1000, false /* adb 连接时不允许重试 */);
            if (connect_ret) {
                auto& connect_str = connect_ret.value();
                // 检查连接字符串是否包含 "connected"
                if (connect_str.find("connected") == std::string::npos) {
                    json::value info = get_info_json() | json::object {
                        { "what", "ConnectFailed" },
                        { "why", "Connection command did not report \"connected\"" },
                    };
                    callback(AsstMsg::ConnectionInfo, info);
                    return false;
                }
            }
            else {
                json::value info = get_info_json() | json::object {
                    { "what", "ConnectFailed" },
                    { "why", "Connection command failed to exec" },
                };
                callback(AsstMsg::ConnectionInfo, info);
                return false;
            }
        }
    }

    if (need_exit()) {
        return false;
    }

    /* get uuid (imei) */
    {
        auto uuid_ret = call_command(m_conn_ctx.replace_cmd(adb_cfg.uuid), 20000, false /* adb 连接时不允许重试 */);
        if (!uuid_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Uuid command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        std::string raw_output = std::move(uuid_ret.value());
        std::string uuid_str = raw_output;
        std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });

        boost::regex hex_regex("^[0-9a-fA-F]{8,}$");
        if (uuid_str.empty() || !boost::regex_match(uuid_str, hex_regex)) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Uuid invalid or shell command not available" },
            };
            info["details"]["raw_output"] = std::move(raw_output);
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        m_uuid = std::move(uuid_str);

        json::value info = get_info_json() | json::object {
            { "what", "UuidGot" },
            { "why", "" },
        };
        info["details"]["uuid"] = m_uuid;
        callback(AsstMsg::ConnectionInfo, info);
    }
    /* get android version */
    {
        auto version_ret = call_command(m_conn_ctx.replace_cmd(adb_cfg.version));
        if (!version_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Android version command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        std::string raw_output = version_ret.value();
        convert_lf(raw_output);

        std::erase_if(raw_output, [](char c) { return !std::isalnum(static_cast<unsigned char>(c)); });
        bool all_digit = !raw_output.empty() && std::ranges::all_of(raw_output, [](char c) {
            return std::isdigit(static_cast<unsigned char>(c));
        });

        if (!all_digit) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Android version invalid or shell command not available" },
            };
            info["details"]["raw_output"] = std::move(version_ret.value());
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        m_version = std::stoul(raw_output);
    }

    if (need_exit()) {
        return false;
    }

    // 按需获取display ID 信息
    if (!adb_cfg.display_id.empty()) {
        // [PackageName] 不参与 replace_cmd 的统一替换（保留为运行时参数）。
        // 只有 connect 阶段命令确实依赖包名的配置，才需要在连接前调用
        // AsstSetInstanceOption(ClientType, ...)。当前内置配置仅 Androws / WSA 的
        // displayId 查询走这里，因此其他连接配置不应被迫传入 client type。
        auto display_id_cmd = utils::string_replace_all(
            m_conn_ctx.replace_cmd(adb_cfg.display_id),
            "[PackageName]",
            m_conn_ctx.package_name);
        auto display_id_ret = call_command(display_id_cmd);
        if (!display_id_ret) {
            return false;
        }

        auto& display_id_pipe_str = display_id_ret.value();
        convert_lf(display_id_pipe_str);
        auto last = display_id_pipe_str.rfind(':');
        if (last == std::string::npos) {
            return false;
        }

        m_conn_ctx.display_id = display_id_pipe_str.substr(last + 1);
        // 去掉换行
        m_conn_ctx.display_id.pop_back();
    }

    if (need_exit()) {
        return false;
    }

    // 按需获取 event ID 信息（用于 minitouch 的 /dev/input/eventN）
    if (!adb_cfg.event_id.empty()) {
        auto event_id_ret = call_command(m_conn_ctx.replace_cmd(adb_cfg.event_id));
        if (!event_id_ret) {
            Log.warn("Failed to get event_id, skip");
        }
        else {
            auto& event_id_pipe_str = event_id_ret.value();
            convert_lf(event_id_pipe_str);
            // 去掉空白字符
            std::erase_if(event_id_pipe_str, [](char c) { return !std::isdigit(c); });

            if (event_id_pipe_str.empty()) {
                Log.warn("event_id is empty, skip");
            }
            else {
                m_conn_ctx.event_id = event_id_pipe_str;
                Log.info("event_id:", m_conn_ctx.event_id);
            }
        }
    }

    if (need_exit()) {
        return false;
    }

    /* display */
    if (!reprobe_screen_size()) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "Display command failed to exec" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
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

    m_adb.click = m_conn_ctx.replace_cmd(adb_cfg.click);
    m_adb.input = m_conn_ctx.replace_cmd(adb_cfg.input);
    m_adb.swipe = m_conn_ctx.replace_cmd(adb_cfg.swipe);
    m_adb.press_esc = m_conn_ctx.replace_cmd(adb_cfg.press_esc);
    m_adb.screencap_raw_with_gzip = m_conn_ctx.replace_cmd(adb_cfg.screencap_raw_with_gzip);
    m_adb.screencap_encode = m_conn_ctx.replace_cmd(adb_cfg.screencap_encode);
    m_adb.start = m_conn_ctx.replace_cmd(adb_cfg.start);
    m_adb.stop = m_conn_ctx.replace_cmd(adb_cfg.stop);
    m_adb.back_to_home = m_conn_ctx.replace_cmd(adb_cfg.back_to_home);
    m_adb.fps = m_conn_ctx.replace_cmd(adb_cfg.fps);

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
        auto nc_address_ret = call_command(m_conn_ctx.replace_cmd(adb_cfg.nc_address));
        if (nc_address_ret && !m_server_started) {
            auto& nc_result_str = nc_address_ret.value();
            if (auto pos = nc_result_str.find(' '); pos != std::string::npos) {
                m_conn_ctx.nc_address = nc_result_str.substr(0, pos);
            }
        }

        auto socket_opt = init_socket(bind_address);
        if (socket_opt) {
            m_conn_ctx.nc_port = socket_opt.value();
            m_adb.screencap_raw_by_nc = m_conn_ctx.replace_cmd(adb_cfg.screencap_raw_by_nc);
            m_server_started = true;
        }
        else {
            m_server_started = false;
        }
    }

    if (need_exit()) {
        return false;
    }

    if (config == "MuMuEmulator12") {
        init_mumu_extras(adb_cfg, address);
    }
    else if (config == "LDPlayer") {
        init_ld_extras(adb_cfg, address);
    }
    if (need_exit()) {
        return false;
    }

    return true;
}

void asst::AdbController::set_kill_adb_on_exit(bool enable) noexcept
{
    m_kill_adb_on_exit = enable;
}

void asst::AdbController::clear_lf_info()
{
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::UnknownYet;
}

void asst::AdbController::check_fps()
{
    // 命令未配置或尚未连接，跳过
    if (m_adb.fps.empty()) {
        return;
    }

    // 上一次异步检测尚未完成，跳过（避免堆积）
    if (m_fps_future.valid() && m_fps_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }

    // 每 1 分钟检测一次
    constexpr auto FpsCheckInterval = std::chrono::minutes(1);
    auto now = std::chrono::steady_clock::now();
    if (m_last_fps_check_time.time_since_epoch().count() != 0 && now - m_last_fps_check_time < FpsCheckInterval) {
        return;
    }
    m_last_fps_check_time = now;

    // 异步执行，避免 adb 延迟阻塞截图返回
    // 值捕获易变数据（m_uuid / m_adb.fps），防止 clear_info() 清空后异步线程读到悬空数据；
    // call_command / callback 依赖的 mutex、platform_io、callback 等均为构造后不变，
    // 生命周期由 ~AdbController 和 clear_info() 中的 m_fps_future.wait() 保证安全
    m_fps_future = std::async(std::launch::async, [this, cmd = m_adb.fps, uuid = m_uuid]() {
        if (need_exit()) {
            return;
        }

        // 放宽到 5 秒：异步执行后不再阻塞截图，可给 adb 足够时间
        auto ret = call_command(cmd, 5000, false);
        if (!ret || ret.value().empty()) {
            Log.warn("fps command failed or empty");
            return;
        }

        // SurfaceFlinger --latency 第一行是每帧刷新周期（纳秒），例如 16666666 表示 60 FPS
        // 注意：这里检测的是模拟器/系统的设置刷新率，而非游戏实际运行帧率。
        auto output = std::move(ret.value());
        convert_lf(output);
        auto newline_pos = output.find('\n');
        std::string first_line = newline_pos == std::string::npos ? output : output.substr(0, newline_pos);

        // 去掉空白和非数字字符
        std::erase_if(first_line, [](char c) { return !std::isdigit(static_cast<unsigned char>(c)); });

        if (first_line.empty()) {
            Log.warn("fps output is empty after sanitize");
            return;
        }

        long long refresh_period_ns = 0;
        if (!utils::chars_to_number<long long, true>(first_line, refresh_period_ns)) {
            Log.warn("fps output parse failed:", first_line);
            return;
        }

        if (refresh_period_ns <= 0) {
            Log.warn("invalid refresh period:", refresh_period_ns);
            return;
        }

        // ns -> FPS
        double fps = 1000000000.0 / static_cast<double>(refresh_period_ns);
        int fps_int = static_cast<int>(std::round(fps));

        json::value info = json::object {
            { "uuid", uuid },
            { "what", "EmulatorFPS" },
            { "details",
              json::object {
                  { "fps", fps_int },
                  { "refresh_period_ns", refresh_period_ns },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    });
}

void asst::AdbController::back_to_home() noexcept
{
    call_command(m_adb.back_to_home);
    return;
}
