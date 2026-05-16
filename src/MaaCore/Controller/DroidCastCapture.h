#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "MaaUtils/NoWarningCV.hpp"
#include "Platform/PlatformIO.h"

namespace asst
{

class DroidCastCapture
{
public:
    struct Config
    {
        std::string format = "jpeg"; // "jpeg" | "png" | "webp"
    };

    DroidCastCapture() = default;
    ~DroidCastCapture() { uninit(); }

    DroidCastCapture(const DroidCastCapture&) = delete;
    DroidCastCapture& operator=(const DroidCastCapture&) = delete;

    bool init(
        const std::string& adb_path,
        const std::string& serial,
        const std::filesystem::path& apk_local_path,
        const Config& cfg,
        PlatformIO* platform_io);

    void uninit();

    bool inited() const noexcept { return inited_; }

    std::optional<cv::Mat> screencap();

private:
    // Find a free port on the host by binding to :0 and releasing immediately.
    // Returns 0 on failure.
    static uint16_t pick_free_port();

    bool run_cmd(const std::string& cmd, int64_t timeout_ms = 30'000);
    bool run_cmd_out(const std::string& cmd, std::string& out, int64_t timeout_ms = 30'000);

    bool push_apk_if_needed();
    bool forward_port();
    bool start_server();
    bool wait_for_server(int timeout_ms = 5'000);

    // Raw HTTP GET /screenshot?format=... → binary body bytes.
    std::optional<std::vector<uint8_t>> http_get();

    std::string adb_path_;
    std::string serial_;
    std::filesystem::path apk_local_path_;
    Config cfg_;
    PlatformIO* platform_io_ = nullptr;

    uint16_t port_ = 0;
    std::shared_ptr<IOHandler> server_handle_;
    bool inited_ = false;

    static constexpr const char* kDeviceApkPath = "/data/local/tmp/maa_DroidCast-1.2.1.apk";
    static constexpr const char* kMainClass = "com.rayworks.droidcast.Main";
};

} // namespace asst
