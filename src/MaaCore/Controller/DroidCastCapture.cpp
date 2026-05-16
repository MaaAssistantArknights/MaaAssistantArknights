#include "DroidCastCapture.h"

#include <thread>

#pragma warning(push, 0)
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/steady_timer.hpp>
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

    if (!push_apk() || !forward_port() || !start_server()) {
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
    std::lock_guard lock(screencap_mutex_);
    if (server_handle_) {
        server_handle_.reset();
    }
    if (port_ != 0 && platform_io_) {
        // Remove the port forward; best-effort, ignore errors.
        std::string cmd = adb_path_ + " -s " + serial_
                          + " forward --remove tcp:" + std::to_string(port_);
        run_cmd(cmd, 5'000);
    }
    inited_ = false;
    port_   = 0;
}

std::optional<cv::Mat> DroidCastCapture::screencap()
{
    std::lock_guard lock(screencap_mutex_);

    if (!inited_) {
        return std::nullopt;
    }

    auto result_opt = http_get();
    if (!result_opt) {
        return std::nullopt;
    }

    // Response is raw ARGB_8888 (4 bytes/pixel).
    // X-Screenshot-Width = cols, X-Screenshot-Height = rows (standard image convention).
    // cv::Mat takes (rows, cols) = (H, W).
    const int W        = result_opt->width;
    const int H        = result_opt->height;
    const int bpp      = result_opt->bytes_per_pixel;
    const int expected = W * H * bpp;
    Log.info("DroidCastCapture: body size", result_opt->body.size(),
             "expected", expected, "(", W, "x", H, "x", bpp, ")");
    if (expected <= 0 || int(result_opt->body.size()) != expected) {
        Log.error("DroidCastCapture: body size mismatch, cannot decode");
        return std::nullopt;
    }
    cv::Mat mat(H, W, CV_8UC4, result_opt->body.data());
    cv::Mat img;
    cv::cvtColor(mat, img, cv::COLOR_RGBA2BGR);
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

bool DroidCastCapture::push_apk()
{
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
    // app_process runs in background; the shell blocks on stdin (cat).
    // When the stdin pipe closes (uninit or process exit), cat returns and kill cleans up.
    std::string cmd = adb_path_ + " -s " + serial_
                      + " shell \"CLASSPATH=" + kDeviceApkPath
                      + " app_process / " + kMainClass
                      + " --port=" + std::to_string(port_)
                      + " & APID=$!; cat>/dev/null; kill $APID 2>/dev/null\"";
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
        if (http_get(/*silent=*/true).has_value()) {
            return true;
        }
        std::this_thread::sleep_for(milliseconds(500));
    }
    return false;
}

std::optional<DroidCastCapture::ScreencapResult> DroidCastCapture::http_get(bool silent)
{
    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);

        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port_));

        // 5-second connect timeout: when DroidCast's event loop is stuck its TCP
        // backlog fills up, SYN packets get dropped, and synchronous connect()
        // would block for the OS retransmit timeout (~75 s).  Use async connect
        // + a steady_timer so we bail out after 5 s instead.
        {
            boost::system::error_code connect_ec = asio::error::would_block;
            asio::steady_timer timer(ioc);
            timer.expires_after(std::chrono::seconds(5));
            timer.async_wait([&](const boost::system::error_code& ec) {
                if (!ec) socket.cancel();
            });
            asio::async_connect(socket, endpoints,
                                [&](const boost::system::error_code& ec, const tcp::endpoint&) {
                                    connect_ec = ec;
                                    timer.cancel();
                                });
            ioc.run();
            if (connect_ec) {
                if (!silent) Log.warn("DroidCastCapture::http_get: connect:", connect_ec.message());
                return std::nullopt;
            }
        }

        // Cap read wait to 5 s so a stuck DroidCast event loop (e.g. PixelCopy
        // waiting for a frame during a screen transition) doesn't block forever.
#ifdef _WIN32
        DWORD rcvtimo = 5'000;
        setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&rcvtimo), sizeof(rcvtimo));
#else
        struct timeval rcvtimo { 5, 0 };
        setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, &rcvtimo, sizeof(rcvtimo));
#endif

        // Send HTTP GET request.
        std::string query = "/screenshot";
        if (cfg_.width > 0 && cfg_.height > 0) {
            query += "?width=" + std::to_string(cfg_.width)
                     + "&height=" + std::to_string(cfg_.height)
                     + "&format=rgb8888";
        }
        std::string request = "GET " + query
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
            if (!silent) Log.warn("DroidCastCapture: no HTTP header separator found");
            return std::nullopt;
        }

        // Verify HTTP 200.
        const std::string headers(buf.begin(), it);
        if (!silent) Log.info("DroidCastCapture: response headers:", headers.substr(0, 256));
        if (headers.find("200") == std::string::npos) {
            if (!silent) Log.warn("DroidCastCapture: non-200 response:", headers.substr(0, 128));
            return std::nullopt;
        }

        // Parse metadata headers set by DroidCast_raw.
        auto parse_header_int = [&](const std::string& key) -> int {
            auto pos = headers.find(key + ": ");
            if (pos == std::string::npos) return 0;
            try { return std::stoi(headers.substr(pos + key.size() + 2)); }
            catch (...) { return 0; }
        };
        ScreencapResult result;
        result.width           = parse_header_int("X-Screenshot-Width");
        result.height          = parse_header_int("X-Screenshot-Height");
        result.bytes_per_pixel = parse_header_int("X-Screenshot-Bytes-Per-Pixel");

        auto body_start = it + 4; // skip \r\n\r\n
        if (body_start >= buf.end()) {
            if (!silent) Log.warn("DroidCastCapture: empty HTTP body");
            return std::nullopt;
        }

        std::vector<uint8_t> body(body_start, buf.end());

        // AndroidAsync may use chunked transfer encoding for binary responses.
        // Decode chunks: each chunk is "{hex_size}\r\n{data}\r\n", ending with "0\r\n\r\n".
        const bool chunked =
            headers.find("Transfer-Encoding: chunked") != std::string::npos ||
            headers.find("transfer-encoding: chunked") != std::string::npos;
        if (chunked) {
            std::vector<uint8_t> decoded;
            size_t pos = 0;
            while (pos < body.size()) {
                size_t eol = pos;
                while (eol + 1 < body.size() &&
                       !(body[eol] == '\r' && body[eol + 1] == '\n'))
                    ++eol;
                if (eol + 1 >= body.size()) break;
                std::string hex_str(body.begin() + pos, body.begin() + eol);
                auto semi = hex_str.find(';');
                if (semi != std::string::npos) hex_str = hex_str.substr(0, semi);
                size_t chunk_size = 0;
                try {
                    chunk_size = std::stoul(hex_str, nullptr, 16);
                }
                catch (...) {
                    break;
                }
                if (chunk_size == 0) break;
                pos = eol + 2;
                if (pos + chunk_size > body.size()) break;
                decoded.insert(decoded.end(), body.begin() + pos,
                               body.begin() + pos + chunk_size);
                pos += chunk_size + 2;
            }
            Log.info("DroidCastCapture: chunked decode:", body.size(), "->", decoded.size(), "bytes");
            result.body = std::move(decoded);
            return result;
        }

        result.body = std::move(body);
        return result;
    }
    catch (const std::exception& e) {
        if (!silent) Log.warn("DroidCastCapture::http_get:", e.what());
        return std::nullopt;
    }
}
