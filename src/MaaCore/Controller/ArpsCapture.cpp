#include "ArpsCapture.h"

#include <algorithm>
#include <chrono>
#include <utility>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <boost/asio/ip/tcp.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;
using namespace std::chrono;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace
{
bool has_protocol_version(const arps::ArpsReadResult& result)
{
    return result.protocol_major != 0 || result.protocol_minor != 0;
}

bool is_protocol_supported(const arps::ArpsReadResult& result, const char* packet_name)
{
    if (result.protocol_major == arps::kProtocolMajor && result.protocol_minor >= arps::kProtocolMinor) {
        return true;
    }

    Log.error(
        "ArpsCapture: unsupported ARPS protocol in",
        packet_name,
        "remote",
        result.protocol_major,
        ".",
        result.protocol_minor,
        "required",
        arps::kProtocolMajor,
        ".",
        arps::kProtocolMinor,
        "or newer");
    return false;
}
}

bool ArpsCapture::init(
    const std::string& adb_path,
    const std::string& serial,
    const std::filesystem::path& apk_local_path,
    const Config& cfg,
    PlatformIO* platform_io)
{
    LogTraceFunction;
    uninit();

    if (!platform_io) {
        Log.error("ArpsCapture: platform_io is null");
        return false;
    }
    if (!std::filesystem::exists(apk_local_path)) {
        Log.error("ArpsCapture: device APK not found", utils::path_to_utf8_string(apk_local_path));
        return false;
    }

    adb_path_ = adb_path;
    serial_ = serial;
    apk_local_path_ = apk_local_path;
    cfg_ = cfg;
    platform_io_ = platform_io;

    if (!push_apk()) {
        adb_path_.clear();
        serial_.clear();
        apk_local_path_.clear();
        platform_io_ = nullptr;
        return false;
    }

    prepared_ = true;
    start_idle_release_thread();
    return true;
}

void ArpsCapture::uninit()
{
    stop_idle_release_thread();
    prepared_ = false;

    std::lock_guard lock(screencap_mutex_);
    close_session_locked("host_uninit");

    adb_path_.clear();
    serial_.clear();
    apk_local_path_.clear();
    platform_io_ = nullptr;
}

bool ArpsCapture::ensure_session_locked()
{
    if (!prepared_.load()) {
        return false;
    }
    if (session_active_) {
        return true;
    }

    for (int retry = 0; retry < 2; ++retry) {
        if (retry > 0 && !push_apk()) {
            return false;
        }

        port_ = pick_free_port();
        if (port_ == 0) {
            Log.error("ArpsCapture: failed to find a free port");
            return false;
        }
        Log.info("ArpsCapture: using port", port_);

        receiver_ = std::make_unique<arps::ArpsReceiver>();
        std::string error;
        if (!receiver_->Listen(kListenHost, port_, &error)) {
            Log.error("ArpsCapture: listen failed:", error);
            close_session_locked("listen_failed");
        }
        else if (reverse_port() && start_client() && accept_hello()) {
            last_screencap_time_ = steady_clock::now();
            session_active_ = true;
            capture_started_ = false;
            idle_power_released_ = false;
            idle_release_cv_.notify_all();
            return true;
        }
        else {
            close_session_locked("start_failed");
        }
    }

    return false;
}

void ArpsCapture::close_session_locked(const std::string& reason)
{
    if (session_active_ || receiver_ || client_handle_ || port_ != 0) {
        Log.info("ArpsCapture: closing session:", reason);
    }
    if (receiver_) {
        if (receiver_->client_socket() != arps::kInvalidArpsSocket) {
            std::string error;
            if (!receiver_->SendStop(reason, &error)) {
                Log.warn("ArpsCapture: send STOP failed:", error);
            }
        }
        receiver_->Close();
        receiver_.reset();
    }
    if (client_handle_) {
        client_handle_.reset();
    }
    if (port_ != 0 && platform_io_) {
        std::string cmd = adb_path_ + " -s " + serial_
                          + " reverse --remove tcp:" + std::to_string(port_);
        run_cmd(cmd, 5'000);
    }

    port_ = 0;
    session_active_ = false;
    capture_started_ = false;
    idle_power_released_ = false;
}

void ArpsCapture::start_idle_release_thread()
{
    std::lock_guard lock(screencap_mutex_);
    idle_release_thread_stop_ = false;
    if (!idle_release_thread_.joinable()) {
        idle_release_thread_ = std::thread(&ArpsCapture::idle_release_proc, this);
    }
}

void ArpsCapture::stop_idle_release_thread()
{
    {
        std::lock_guard lock(screencap_mutex_);
        idle_release_thread_stop_ = true;
    }
    idle_release_cv_.notify_all();

    if (idle_release_thread_.joinable()) {
        idle_release_thread_.join();
    }
}

void ArpsCapture::idle_release_proc()
{
    std::unique_lock lock(screencap_mutex_);
    while (!idle_release_thread_stop_) {
        if (!prepared_.load() || !session_active_ || !capture_started_ || idle_power_released_ || !cfg_.keep_screen_on) {
            idle_release_cv_.wait(lock, [&] {
                return idle_release_thread_stop_
                       || (prepared_.load() && session_active_ && capture_started_ && !idle_power_released_
                           && cfg_.keep_screen_on);
            });
            continue;
        }

        const auto idle_time = steady_clock::now() - last_screencap_time_;
        if (idle_time < milliseconds(kIdleReleaseScreenMs)) {
            idle_release_cv_.wait_for(lock, milliseconds(kIdleReleaseScreenMs) - idle_time);
            continue;
        }

        if (send_power_control_locked("screencap_idle", false, std::nullopt)) {
            idle_power_released_ = true;
            Log.info("ArpsCapture: idle power released");
        }
        else {
            close_session_locked("idle_power_release_failed");
        }
    }
}

bool ArpsCapture::warmup()
{
    std::lock_guard lock(screencap_mutex_);
    if (!ensure_session_locked()) {
        return false;
    }
    if (!ensure_capture_started_locked()) {
        close_session_locked("warmup_failed");
        return false;
    }
    if (!ensure_runtime_power_locked()) {
        close_session_locked("warmup_power_failed");
        return false;
    }
    return true;
}

std::optional<cv::Mat> ArpsCapture::screencap()
{
    std::lock_guard lock(screencap_mutex_);

    for (int retry = 0; retry < 2; ++retry) {
        if (!ensure_session_locked() || !receiver_) {
            return std::nullopt;
        }
        if (!ensure_capture_started_locked()) {
            close_session_locked("start_failed");
            continue;
        }
        if (!ensure_runtime_power_locked()) {
            close_session_locked("power_resume_failed");
            continue;
        }

        last_screencap_time_ = steady_clock::now();
        idle_release_cv_.notify_all();

        std::string error;
        if (!receiver_->RequestFrame(&error)) {
            Log.warn("ArpsCapture: request frame failed:", error);
            close_session_locked("request_frame_failed");
            continue;
        }

        auto frame = read_frame();
        if (!frame) {
            Log.warn("ArpsCapture: requested frame failed");
            close_session_locked("read_frame_failed");
            continue;
        }

        last_screencap_time_ = steady_clock::now();
        idle_release_cv_.notify_all();
        return frame;
    }

    return std::nullopt;
}

std::uint16_t ArpsCapture::pick_free_port()
{
    try {
        asio::io_context ioc;
        tcp::acceptor acceptor(ioc);
        tcp::endpoint ep(asio::ip::address_v4::loopback(), 0);
        acceptor.open(ep.protocol());
        acceptor.set_option(asio::socket_base::reuse_address(true));
        acceptor.bind(ep);
        std::uint16_t port = acceptor.local_endpoint().port();
        acceptor.close();
        return port;
    }
    catch (const std::exception& e) {
        Log.error("ArpsCapture::pick_free_port:", e.what());
        return 0;
    }
}

bool ArpsCapture::run_cmd(const std::string& cmd, int64_t timeout_ms)
{
    std::string dummy_out;
    return run_cmd_out(cmd, dummy_out, timeout_ms);
}

bool ArpsCapture::run_cmd_out(const std::string& cmd, std::string& out, int64_t timeout_ms)
{
    if (!platform_io_) {
        return false;
    }

    std::string sock_data;
    auto start = steady_clock::now();
    auto result = platform_io_->call_command(cmd, false, out, sock_data, timeout_ms, start);
    if (!result) {
        Log.warn("ArpsCapture: command failed:", cmd);
        return false;
    }
    return result.value() == 0;
}

bool ArpsCapture::push_apk()
{
    std::string local = utils::path_to_utf8_string(apk_local_path_);
    std::string cmd = adb_path_ + " -s " + serial_
                      + " push \"" + local + "\" " + kDeviceApkPath;
    Log.info("ArpsCapture: pushing APK:", cmd);
    return run_cmd(cmd, 60'000);
}

bool ArpsCapture::reverse_port()
{
    std::string cmd = adb_path_ + " -s " + serial_
                      + " reverse tcp:" + std::to_string(port_)
                      + " tcp:" + std::to_string(port_);
    Log.info("ArpsCapture: setting up reverse:", cmd);
    return run_cmd(cmd, 10'000);
}

bool ArpsCapture::start_client()
{
    std::string cmd = adb_path_ + " -s " + serial_
                      + " shell \"CLASSPATH=" + kDeviceApkPath
                      + " app_process / " + kMainClass
                      + " --connect-host=127.0.0.1"
                      + " --connect-port=" + std::to_string(port_)
                      + " & ARPS_PID=$!; cat>/dev/null; kill $ARPS_PID 2>/dev/null\"";
    Log.info("ArpsCapture: starting client:", cmd);
    client_handle_ = platform_io_->interactive_shell(cmd);
    if (!client_handle_) {
        Log.error("ArpsCapture: interactive_shell returned null");
        return false;
    }
    return true;
}

bool ArpsCapture::accept_hello()
{
    if (!receiver_) {
        return false;
    }

    std::string error;
    if (!receiver_->AcceptOnce(kAcceptTimeoutMs, &error)) {
        Log.error("ArpsCapture: accept failed:", error);
        return false;
    }

    auto hello = receiver_->ReadNext(kReadTimeoutMs);
    if (has_protocol_version(hello) && !is_protocol_supported(hello, "HELLO")) {
        return false;
    }
    if (hello.status != arps::ArpsReadStatus::Hello) {
        Log.error(
            "ArpsCapture: expected HELLO, status:",
            static_cast<int>(hello.status),
            "message:",
            hello.message,
            "protocol:",
            hello.protocol_major,
            ".",
            hello.protocol_minor);
        return false;
    }
    if (!is_protocol_supported(hello, "HELLO")) {
        return false;
    }
    Log.info(
        "ArpsCapture: HELLO protocol:",
        hello.protocol_major,
        ".",
        hello.protocol_minor,
        "json:",
        hello.json);
    return true;
}

bool ArpsCapture::ensure_capture_started_locked()
{
    if (capture_started_) {
        return true;
    }
    if (!receiver_ || !session_active_) {
        return false;
    }

    arps::ArpsStartOptions options;
    options.display_id = cfg_.display_id;
    options.compression = cfg_.compression;
    options.max_fps = static_cast<std::uint32_t>(std::clamp(cfg_.max_fps, 0, 240));
    options.max_packet_len = cfg_.max_packet_len;
    options.power_on_if_screen_off = cfg_.power_on_if_screen_off;
    options.turn_screen_off = cfg_.turn_screen_off;
    options.keep_screen_on = cfg_.keep_screen_on;
    options.capture_mode = cfg_.capture_mode;
    options.exit_power_mode = cfg_.exit_power_mode;
    options.stream_mode = "pull";

    std::string error;
    if (!receiver_->SendStart(options, &error)) {
        Log.error("ArpsCapture: send START failed:", error);
        return false;
    }
    Log.info("ArpsCapture: START:", options.ToJson());

    auto ready = receiver_->ReadNext(kReadTimeoutMs);
    if (has_protocol_version(ready) && !is_protocol_supported(ready, "READY")) {
        return false;
    }
    if (ready.status != arps::ArpsReadStatus::Ready) {
        Log.error(
            "ArpsCapture: expected READY, status:",
            static_cast<int>(ready.status),
            "message:",
            ready.message,
            "protocol:",
            ready.protocol_major,
            ".",
            ready.protocol_minor,
            "json:",
            ready.json);
        return false;
    }
    if (!is_protocol_supported(ready, "READY")) {
        return false;
    }
    Log.info(
        "ArpsCapture: READY protocol:",
        ready.protocol_major,
        ".",
        ready.protocol_minor,
        "json:",
        ready.json);
    capture_started_ = true;
    last_screencap_time_ = steady_clock::now();
    idle_release_cv_.notify_all();
    return true;
}

bool ArpsCapture::ensure_runtime_power_locked()
{
    if (!idle_power_released_) {
        return true;
    }

    std::optional<bool> keep_screen_on;
    std::optional<bool> power_on_if_screen_off;
    if (cfg_.keep_screen_on) {
        keep_screen_on = true;
    }
    if (cfg_.power_on_if_screen_off) {
        power_on_if_screen_off = true;
    }
    if (!keep_screen_on && !power_on_if_screen_off) {
        idle_power_released_ = false;
        return true;
    }

    if (!send_power_control_locked("screencap_resume", keep_screen_on, power_on_if_screen_off)) {
        return false;
    }
    idle_power_released_ = false;
    return true;
}

bool ArpsCapture::send_power_control_locked(
    const std::string& reason,
    std::optional<bool> keep_screen_on,
    std::optional<bool> power_on_if_screen_off)
{
    if (!receiver_ || !session_active_) {
        return false;
    }

    arps::ArpsPowerControlOptions options;
    options.display_id = cfg_.display_id;
    options.request_id = "maa-" + reason + "-" + std::to_string(++power_request_id_);
    options.reason = reason;
    options.keep_screen_on = keep_screen_on;
    options.power_on_if_screen_off = power_on_if_screen_off;

    std::string error;
    if (!receiver_->SendPowerControl(options, &error)) {
        Log.warn("ArpsCapture: send POWER_CONTROL failed:", error);
        return false;
    }
    Log.info("ArpsCapture: POWER_CONTROL:", options.ToJson());

    auto state = wait_power_state_locked(options.request_id, kReadTimeoutMs);
    if (!state) {
        return false;
    }
    Log.info(
        "ArpsCapture: POWER_STATE request_id:",
        state->request_id,
        "ok:",
        state->ok,
        "screen_on:",
        state->screen_on,
        "wake_lock:",
        state->wake_lock_held_by_arps,
        "display_power:",
        state->display_power_override);
    if (!state->ok) {
        Log.warn("ArpsCapture: POWER_CONTROL rejected:", state->error);
        return false;
    }
    return true;
}

std::optional<arps::ArpsPowerState> ArpsCapture::wait_power_state_locked(const std::string& request_id, int timeout_ms)
{
    if (!receiver_) {
        return std::nullopt;
    }

    const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
    while (prepared_.load() && session_active_) {
        const auto remaining = duration_cast<milliseconds>(deadline - steady_clock::now()).count();
        if (remaining <= 0) {
            Log.warn("ArpsCapture: wait POWER_STATE timeout:", request_id);
            return std::nullopt;
        }

        auto result = receiver_->ReadNext(static_cast<int>(remaining));
        if (has_protocol_version(result) && !is_protocol_supported(result, "POWER_STATE")) {
            return std::nullopt;
        }
        switch (result.status) {
        case arps::ArpsReadStatus::PowerState:
            if (result.power_state.request_id == request_id) {
                return result.power_state;
            }
            Log.info(
                "ArpsCapture: ignoring POWER_STATE for request_id:",
                result.power_state.request_id,
                "while waiting:",
                request_id);
            break;
        case arps::ArpsReadStatus::Frame:
            Log.warn("ArpsCapture: dropping unexpected FRAME while waiting POWER_STATE");
            break;
        case arps::ArpsReadStatus::Hello:
            Log.info("ArpsCapture: received unexpected HELLO while waiting POWER_STATE");
            break;
        case arps::ArpsReadStatus::Ready:
            Log.info("ArpsCapture: received unexpected READY while waiting POWER_STATE:", result.json);
            break;
        case arps::ArpsReadStatus::Timeout:
            Log.warn("ArpsCapture: wait POWER_STATE timeout:", request_id);
            return std::nullopt;
        case arps::ArpsReadStatus::Error:
            Log.warn("ArpsCapture: device error while waiting POWER_STATE:", result.json);
            return std::nullopt;
        case arps::ArpsReadStatus::Stop:
            Log.warn("ArpsCapture: device stopped while waiting POWER_STATE:", result.json);
            return std::nullopt;
        case arps::ArpsReadStatus::Closed:
            Log.warn("ArpsCapture: socket closed while waiting POWER_STATE:", result.message);
            return std::nullopt;
        case arps::ArpsReadStatus::ProtocolError:
            Log.warn("ArpsCapture: protocol error while waiting POWER_STATE:", result.message);
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<cv::Mat> ArpsCapture::read_frame()
{
    if (!receiver_) {
        return std::nullopt;
    }

    while (prepared_.load() && session_active_) {
        auto result = receiver_->ReadNext(kReadTimeoutMs);
        switch (result.status) {
        case arps::ArpsReadStatus::Frame: {
            auto image = frame_to_bgr(result.frame);
            if (!image) {
                return std::nullopt;
            }
            return image;
        }
        case arps::ArpsReadStatus::Hello:
            Log.info("ArpsCapture: received unexpected HELLO packet while reading frame");
            break;
        case arps::ArpsReadStatus::Ready:
            Log.info("ArpsCapture: received unexpected READY packet while reading frame:", result.json);
            break;
        case arps::ArpsReadStatus::PowerState:
            Log.info(
                "ArpsCapture: received POWER_STATE while reading frame:",
                result.power_state.request_id,
                "ok:",
                result.power_state.ok);
            break;
        case arps::ArpsReadStatus::Timeout:
            Log.warn("ArpsCapture: read frame timeout");
            return std::nullopt;
        case arps::ArpsReadStatus::Error:
            Log.warn("ArpsCapture: device error:", result.json);
            return std::nullopt;
        case arps::ArpsReadStatus::Stop:
            Log.warn("ArpsCapture: device stopped:", result.json);
            return std::nullopt;
        case arps::ArpsReadStatus::Closed:
            Log.warn("ArpsCapture: socket closed:", result.message);
            return std::nullopt;
        case arps::ArpsReadStatus::ProtocolError:
            Log.warn("ArpsCapture: protocol error:", result.message);
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<cv::Mat> ArpsCapture::frame_to_bgr(const arps::ArpsFrame& frame)
{
    const auto width = static_cast<int>(frame.meta.width);
    const auto height = static_cast<int>(frame.meta.height);
    const auto row_bytes = static_cast<size_t>(frame.meta.row_bytes);
    const size_t min_row_bytes = static_cast<size_t>(width) * 4;

    if (!frame.argb8888 || width <= 0 || height <= 0 || row_bytes < min_row_bytes) {
        Log.error("ArpsCapture: invalid frame metadata");
        return std::nullopt;
    }
    if (frame.argb8888_len < row_bytes * static_cast<size_t>(height)) {
        Log.error("ArpsCapture: frame buffer too small");
        return std::nullopt;
    }

    cv::Mat rgba(height, width, CV_8UC4, const_cast<std::uint8_t*>(frame.argb8888), row_bytes);
    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
    Log.trace(
        "ArpsCapture: frame",
        frame.meta.frame_no,
        width,
        "x",
        height,
        "row_bytes",
        frame.meta.row_bytes,
        "read",
        frame.packet_read_ms,
        "ms decode",
        frame.decode_ms,
        "ms");
    return bgr;
}
