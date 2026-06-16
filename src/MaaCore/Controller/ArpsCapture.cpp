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

bool ArpsCapture::init(
    const std::string& adb_path,
    const std::string& serial,
    const std::filesystem::path& apk_local_path,
    const Config& cfg,
    PlatformIO* platform_io)
{
    LogTraceFunction;
    uninit();

    adb_path_ = adb_path;
    serial_ = serial;
    apk_local_path_ = apk_local_path;
    cfg_ = cfg;
    platform_io_ = platform_io;

    if (!platform_io_) {
        Log.error("ArpsCapture: platform_io is null");
        return false;
    }
    if (!std::filesystem::exists(apk_local_path_)) {
        Log.error("ArpsCapture: device APK not found", utils::path_to_utf8_string(apk_local_path_));
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
        uninit();
        return false;
    }

    if (!push_apk() || !reverse_port() || !start_client() || !accept_and_start()) {
        uninit();
        return false;
    }

    inited_ = true;
    return true;
}

void ArpsCapture::uninit()
{
    inited_ = false;

    std::lock_guard lock(screencap_mutex_);

    if (receiver_) {
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
}

std::optional<cv::Mat> ArpsCapture::screencap()
{
    std::lock_guard lock(screencap_mutex_);
    if (!inited_.load()) {
        return std::nullopt;
    }
    if (!receiver_) {
        return std::nullopt;
    }

    std::string error;
    if (!receiver_->RequestFrame(&error)) {
        Log.warn("ArpsCapture: request frame failed:", error);
        return std::nullopt;
    }

    auto frame = read_frame();
    if (!frame) {
        Log.warn("ArpsCapture: requested frame failed");
        return std::nullopt;
    }
    return frame;
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

bool ArpsCapture::accept_and_start()
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
    if (hello.status != arps::ArpsReadStatus::Hello) {
        Log.error("ArpsCapture: expected HELLO, status:", static_cast<int>(hello.status), "message:", hello.message);
        return false;
    }
    Log.info("ArpsCapture: HELLO:", hello.json);

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

    if (!receiver_->SendStart(options, &error)) {
        Log.error("ArpsCapture: send START failed:", error);
        return false;
    }
    Log.info("ArpsCapture: START:", options.ToJson());

    auto ready = receiver_->ReadNext(kReadTimeoutMs);
    if (ready.status != arps::ArpsReadStatus::Ready) {
        Log.error(
            "ArpsCapture: expected READY, status:",
            static_cast<int>(ready.status),
            "message:",
            ready.message,
            "json:",
            ready.json);
        return false;
    }
    Log.info("ArpsCapture: READY:", ready.json);
    return true;
}

std::optional<cv::Mat> ArpsCapture::read_frame()
{
    if (!receiver_) {
        return std::nullopt;
    }

    while (inited_.load()) {
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
