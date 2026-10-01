#pragma once

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

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

    bool inited() const noexcept { return prepared_.load(); }

    bool warmup();
    std::optional<cv::Mat> screencap();

private:
    static std::uint16_t pick_free_port();

    bool run_cmd(const std::string& cmd, int64_t timeout_ms = 30'000);
    bool run_cmd_out(const std::string& cmd, std::string& out, int64_t timeout_ms = 30'000);

    bool push_apk();
    bool ensure_session_locked();
    bool accept_hello();
    bool ensure_capture_started_locked();
    bool ensure_runtime_power_locked();
    bool send_power_control_locked(
        const std::string& reason,
        std::optional<bool> keep_screen_on,
        std::optional<bool> power_on_if_screen_off);
    std::optional<arps::ArpsPowerState> wait_power_state_locked(const std::string& request_id, int timeout_ms);
    void close_session_locked(const std::string& reason);
    bool reverse_port();
    bool start_client();
    void start_idle_release_thread();
    void stop_idle_release_thread();
    void idle_release_proc();
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
    std::atomic_bool prepared_ = false;
    bool session_active_ = false;
    bool capture_started_ = false;
    bool idle_power_released_ = false;

    std::mutex screencap_mutex_;
    std::condition_variable idle_release_cv_;
    std::thread idle_release_thread_;
    std::chrono::steady_clock::time_point last_screencap_time_;
    bool idle_release_thread_stop_ = false;
    std::uint64_t power_request_id_ = 0;

    static constexpr int kAcceptTimeoutMs = 10'000;
    static constexpr int kReadTimeoutMs = 5'000;
    static constexpr int kIdleReleaseScreenMs = 5'000;
    static constexpr const char* kListenHost = "127.0.0.1";
    static constexpr const char* kDeviceApkPath = "/data/local/tmp/arps-device.apk";
    static constexpr const char* kMainClass = "com.visotc.ARPS.Main";
};

} // namespace asst
