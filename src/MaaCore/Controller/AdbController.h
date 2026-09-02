#pragma once

#include "ControllerAPI.h"

#include <chrono>
#include <deque>
#include <future>
#include <optional>
#include <random>

#include "Platform/PlatformFactory.h"

#include "Common/AsstMsg.h"
#include "Config/GeneralConfig.h"
#include "InstHelper.h"
#include "LDExtras.h"
#include "MumuExtras.h"
#include "Utils/StringMisc.hpp"

namespace asst
{

// 连接过程中的临时状态，基类 connect() 填充后子类直接复用
struct AdbConnectionContext
{
    // 连接基本信息（reset 时设置）
    std::string adb_path;
    std::string address;
    std::string package_name;

    // 缓存的 ADB 配置（基类 connect 获取后存入）
    AdbCfg adb_cfg;

    // 连接过程中获取/更新的动态状态
    std::string display_id;
    std::string screencap_display_id;
    std::string event_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    void reset(const std::string& adb, const std::string& addr, const std::string& client_type)
    {
        adb_path = adb;
        address = addr;
        package_name = client_type.empty() ? "" : Config.get_package_name(client_type).value_or("");
        adb_cfg = {};
        display_id.clear();
        screencap_display_id.clear();
        event_id.clear();
        nc_address = "10.0.2.2";
        nc_port = 0;
    }

    std::string replace_cmd(const std::string& cfg_cmd) const
    {
        const std::string screencap_display_id_arg =
            screencap_display_id.empty() ? std::string() : "-d " + screencap_display_id;
        return utils::string_replace_all(
            cfg_cmd,
            {
                { "[Adb]", adb_path },
                { "[AdbSerial]", address },
                { "[DisplayId]", display_id },
                { "[ScreencapDisplayId]", screencap_display_id },
                { "[ScreencapDisplayIdArg]", screencap_display_id_arg },
                { "[EventId]", event_id },
                { "[NcPort]", std::to_string(nc_port) },
                { "[NcAddress]", nc_address },
            });
    }
};

class AdbController : public ControllerAPI, protected InstHelper
{
public:
    AdbController(const AsstCallback& callback, Assistant* inst, PlatformType type);
    AdbController(const AdbController&) = delete;
    AdbController(AdbController&&) = delete;
    virtual ~AdbController();

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    virtual void set_kill_adb_on_exit(bool enable) noexcept override;

    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override;

    virtual size_t get_version() const noexcept override;

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;

    virtual bool input(const std::string& text) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event([[maybe_unused]] const InputEvent& event) override { return false; }

    virtual bool press_esc() override;

    virtual ControlFeat::Feat support_features() const noexcept override { return ControlFeat::NONE; }

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    AdbController& operator=(const AdbController&) = delete;
    AdbController& operator=(AdbController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    // 重新执行 display 命令探测分辨率并刷新 m_width/m_height/m_screen_size；
    // 连接时首次探测使用
    bool reprobe_screen_size();

    // 检测到分辨率被外部修改时调用：标记连接失效并通知上层，
    // 任务失败后由上层整体重连（重连时重新探测并重建全部输入映射）
    void invalidate_connection(std::string_view reason, int width, int height);

    // 显示方向旋转（截图宽高互换）时回调，方向相关的输入子类重写以重建映射
    virtual void on_display_rotated() {}

    std::optional<std::string> call_command(
        const std::string& cmd,
        int64_t timeout = 20000,
        bool allow_reconnect = true,
        bool recv_by_socket = false);

    virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket);

    void release();

    void close_socket() noexcept;
    std::optional<unsigned short> init_socket(const std::string& local_address);

    enum class ScreencapResult
    {
        Failed,
        Success,
        Reprobe,
    };

    using DecodeFunc = std::function<bool(const std::string&)>;
    ScreencapResult screencap(
        const std::string& cmd,
        const DecodeFunc& decode_func,
        bool allow_reconnect = false,
        bool by_socket = false,
        int max_timeout = 20000);
    void clear_lf_info();

    virtual void clear_info() noexcept;
    void callback(AsstMsg msg, const json::value& details);
    static std::optional<int> get_mumu_index(const std::string& address);
    void init_mumu_extras(const AdbCfg& adb_cfg, const std::string& address);
    void set_mumu_package(const std::string& client_type);
    void init_ld_extras(const AdbCfg& adb_cfg, const std::string& address);
    static std::optional<int> get_ld_index(const std::string& address);

    // 转换 data 中的 CRLF 为 LF：有些模拟器自带的 adb，exec-out 输出的 \n 会被替换成 \r\n，
    // 导致解码错误，所以这里转一下回来（点名批评 mumu 和雷电）
    static bool convert_lf(std::string& data);

    // 每 1 分钟检测一次模拟器帧率，回调给 UI 用于低帧率提示
    void check_fps();

    AdbConnectionContext m_conn_ctx;

    AsstCallback m_callback;

    std::mutex m_callcmd_mutex;

    std::shared_ptr<asst::PlatformIO> m_platform_io = nullptr;

    struct AdbProperty
    {
        /* command */
        std::string devices;
        std::string address_regex;
        std::string connect;
        std::string call_minitouch;
        std::string call_maatouch;
        std::string click;
        std::string input;
        std::string swipe;
        std::string press_esc;

        std::string screencap_raw_by_nc;
        std::string screencap_raw_with_gzip;
        std::string screencap_encode;
        std::string release;

        std::string start;
        std::string stop;

        std::string version;

        std::string back_to_home;

        std::string fps; // 获取模拟器刷新率的命令

        /* properties */
        enum class ScreencapEndOfLine
        {
            UnknownYet,
            CRLF,
            LF,
            CR
        } screencap_end_of_line = ScreencapEndOfLine::UnknownYet;

        enum class ScreencapMethod
        {
            UnknownYet,
            // Default,
            RawByNc,
            RawWithGzip,
            Encode,
#if ASST_WITH_EMULATOR_EXTRAS
            MumuExtras,
            LDExtras,
#endif
        } screencap_method = ScreencapMethod::UnknownYet;
    } m_adb;

    std::string m_uuid;
    size_t m_pipe_data_size = 0;
    size_t m_version = 0;
    std::pair<int, int> m_screen_size = { 0, 0 };
    int m_width = 0;
    int m_height = 0;
    // 最近一次成功截图的实际尺寸，用于帧间对比发现分辨率被外部修改
    std::pair<int, int> m_last_screencap_size = { 0, 0 };
    // 分辨率被外部修改后置位：此后截图一律快速失败，任务随即停止，等待上层整体重连
    bool m_connection_expired = false;
    bool m_support_socket = false;
    bool m_server_started = false;
    bool m_inited = false;
    bool m_kill_adb_on_exit = false;
    long long m_last_command_duration = 0;                       // 上次命令执行用时
    std::deque<long long> m_screencap_cost;                      // 截图用时
    int m_screencap_times = 0;                                   // 截图次数
    std::chrono::steady_clock::time_point m_last_fps_check_time; // 上次帧率检测时间
    std::future<void> m_fps_future;                              // 异步帧率检测任务

#if ASST_WITH_EMULATOR_EXTRAS
    MumuExtras m_mumu_extras;
    LDExtras m_ld_extras;
#endif
};
} // namespace asst
