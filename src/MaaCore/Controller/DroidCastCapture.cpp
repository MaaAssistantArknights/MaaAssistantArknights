#include "DroidCastCapture.h"

#include <thread>

#pragma warning(push, 0)
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#pragma warning(pop)

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;
using namespace std::chrono;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

bool DroidCastCapture::init(
    const std::string& adb_path,
    const std::string& serial,
    const std::filesystem::path& apk_local_path,
    const Config& cfg,
    PlatformIO* platform_io)
{
    LogTraceFunction;
    uninit();

    adb_path_      = adb_path;
    serial_        = serial;
    apk_local_path_ = apk_local_path;
    cfg_           = cfg;
    platform_io_   = platform_io;

    port_ = pick_free_port();
    if (port_ == 0) {
        Log.error("DroidCastCapture: failed to find a free port");
        return false;
    }
    Log.info("DroidCastCapture: using port", port_);

    if (!push_apk_if_needed() || !forward_port() || !start_server()) {
        uninit();
        return false;
    }

    if (!wait_for_server(5'000)) {
        Log.error("DroidCastCapture: server did not become ready in time");
        uninit();
        return false;
    }

    inited_ = true;
    return true;
}

void DroidCastCapture::uninit()
{
    if (server_handle_) {
        server_handle_.reset();
    }
    if (port_ != 0 && platform_io_) {
        // Remove the port forward; best-effort, ignore errors.
        std::string cmd = adb_path_ + " -s " + serial_
                          + " forward --remove tcp:" + std::to_string(port_);
        run_cmd(cmd, 5'000);
    }
    inited_   = false;
    port_     = 0;
}

std::optional<cv::Mat> DroidCastCapture::screencap()
{
    auto data_opt = http_get();
    if (!data_opt) {
        return std::nullopt;
    }
    const auto& data = *data_opt;
    cv::Mat img = cv::imdecode({ data.data(), int(data.size()) }, cv::IMREAD_COLOR);
    if (img.empty()) {
        Log.error("DroidCastCapture: cv::imdecode returned empty mat");
        return std::nullopt;
    }
    return img;
}

// ── private helpers ──────────────────────────────────────────────────────────

uint16_t DroidCastCapture::pick_free_port()
{
    try {
        asio::io_context ioc;
        tcp::acceptor acceptor(ioc);
        tcp::endpoint ep(asio::ip::address_v4::loopback(), 0);
        acceptor.open(ep.protocol());
        acceptor.set_option(asio::socket_base::reuse_address(true));
        acceptor.bind(ep);
        uint16_t port = acceptor.local_endpoint().port();
        acceptor.close();
        return port;
    }
    catch (const std::exception& e) {
        Log.error("DroidCastCapture::pick_free_port:", e.what());
        return 0;
    }
}

bool DroidCastCapture::run_cmd(const std::string& cmd, int64_t timeout_ms)
{
    std::string dummy_out;
    return run_cmd_out(cmd, dummy_out, timeout_ms);
}

bool DroidCastCapture::run_cmd_out(const std::string& cmd, std::string& out, int64_t timeout_ms)
{
    std::string sock_data;
    auto start = steady_clock::now();
    auto result = platform_io_->call_command(cmd, false, out, sock_data, timeout_ms, start);
    if (!result) {
        Log.warn("DroidCastCapture: command failed:", cmd);
        return false;
    }
    return result.value() == 0;
}

bool DroidCastCapture::push_apk_if_needed()
{
    // Check whether the APK already exists on the device.
    std::string ls_cmd = adb_path_ + " -s " + serial_
                         + " shell ls " + kDeviceApkPath;
    std::string ls_out;
    if (run_cmd_out(ls_cmd, ls_out, 5'000) && ls_out.find(kDeviceApkPath) != std::string::npos) {
        Log.info("DroidCastCapture: APK already on device, skipping push");
        return true;
    }

    std::string local = utils::path_to_utf8_string(apk_local_path_);
    std::string push_cmd = adb_path_ + " -s " + serial_
                           + " push \"" + local + "\" " + kDeviceApkPath;
    Log.info("DroidCastCapture: pushing APK:", push_cmd);
    return run_cmd(push_cmd, 60'000);
}

bool DroidCastCapture::forward_port()
{
    std::string cmd = adb_path_ + " -s " + serial_
                      + " forward tcp:" + std::to_string(port_)
                      + " tcp:" + std::to_string(port_);
    Log.info("DroidCastCapture: setting up port forward:", cmd);
    return run_cmd(cmd, 10'000);
}

bool DroidCastCapture::start_server()
{
    std::string cmd = adb_path_ + " -s " + serial_
                      + " shell CLASSPATH=" + kDeviceApkPath
                      + " exec app_process /system/bin " + kMainClass
                      + " --port=" + std::to_string(port_);
    Log.info("DroidCastCapture: starting server:", cmd);
    server_handle_ = platform_io_->interactive_shell(cmd);
    if (!server_handle_) {
        Log.error("DroidCastCapture: interactive_shell returned null");
        return false;
    }
    return true;
}

bool DroidCastCapture::wait_for_server(int timeout_ms)
{
    const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
    while (steady_clock::now() < deadline) {
        if (http_get().has_value()) {
            return true;
        }
        std::this_thread::sleep_for(milliseconds(500));
    }
    return false;
}

std::optional<std::vector<uint8_t>> DroidCastCapture::http_get()
{
    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);

        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port_));
        asio::connect(socket, endpoints);

        // Send HTTP GET request.
        std::string request = "GET /screenshot?format=" + cfg_.format
                              + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        asio::write(socket, asio::buffer(request));

        // Read until EOF.
        std::vector<uint8_t> buf;
        boost::system::error_code ec;
        while (true) {
            uint8_t tmp[4096];
            size_t n = socket.read_some(asio::buffer(tmp), ec);
            if (n > 0) {
                buf.insert(buf.end(), tmp, tmp + n);
            }
            if (ec == asio::error::eof || ec) {
                break;
            }
        }

        // Find end of HTTP headers (\r\n\r\n).
        const std::vector<uint8_t> sep = { '\r', '\n', '\r', '\n' };
        auto it = std::search(buf.begin(), buf.end(), sep.begin(), sep.end());
        if (it == buf.end()) {
            Log.warn("DroidCastCapture: no HTTP header separator found");
            return std::nullopt;
        }

        // Verify HTTP 200.
        const std::string headers(buf.begin(), it);
        if (headers.find("200") == std::string::npos) {
            Log.warn("DroidCastCapture: non-200 response:", headers.substr(0, 128));
            return std::nullopt;
        }

        auto body_start = it + 4; // skip \r\n\r\n
        if (body_start >= buf.end()) {
            Log.warn("DroidCastCapture: empty HTTP body");
            return std::nullopt;
        }

        return std::vector<uint8_t>(body_start, buf.end());
    }
    catch (const std::exception& e) {
        Log.warn("DroidCastCapture::http_get:", e.what());
        return std::nullopt;
    }
}
