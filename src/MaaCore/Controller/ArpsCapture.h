#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "arps/receiver.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Platform/PlatformIO.h"

namespace asst
{

class ArpsCapture
{
public:
    struct Config
    {
        std::string compression = "lz4_block";
        int max_fps = 30;
        std::string capture_mode = "auto";
        bool power_on_if_screen_off = true;
        bool turn_screen_off = false;
        bool keep_screen_on = true;
        std::string exit_power_mode = "restore_previous";
        std::uint32_t display_id = 0;
        std::uint32_t max_packet_len = arps::kDefaultMaxPacketLen;
    };

    ArpsCapture() = default;
    ~ArpsCapture() { uninit(); }

    ArpsCapture(const ArpsCapture&) = delete;
    ArpsCapture& operator=(const ArpsCapture&) = delete;

    bool init(
        const std::string& adb_path,
        const std::string& serial,
        const std::filesystem::path& apk_local_path,
        const Config& cfg,
        PlatformIO* platform_io);

    void uninit();

    bool inited() const noexcept { return inited_.load(); }

    std::optional<cv::Mat> screencap();

private:
    static std::uint16_t pick_free_port();

    bool run_cmd(const std::string& cmd, int64_t timeout_ms = 30'000);
    bool run_cmd_out(const std::string& cmd, std::string& out, int64_t timeout_ms = 30'000);

    bool push_apk();
    bool reverse_port();
    bool start_client();
    bool accept_and_start();
    std::optional<cv::Mat> read_frame();
    std::optional<cv::Mat> frame_to_bgr(const arps::ArpsFrame& frame);

    std::string adb_path_;
    std::string serial_;
    std::filesystem::path apk_local_path_;
    Config cfg_;
    PlatformIO* platform_io_ = nullptr;

    std::uint16_t port_ = 0;
    std::unique_ptr<arps::ArpsReceiver> receiver_;
    std::shared_ptr<IOHandler> client_handle_;
    std::atomic_bool inited_ = false;

    std::mutex screencap_mutex_;

    static constexpr int kAcceptTimeoutMs = 10'000;
    static constexpr int kReadTimeoutMs = 5'000;
    static constexpr const char* kListenHost = "127.0.0.1";
    static constexpr const char* kDeviceApkPath = "/data/local/tmp/arps-device.apk";
    static constexpr const char* kMainClass = "com.visotc.ARPS.Main";
};

} // namespace asst
