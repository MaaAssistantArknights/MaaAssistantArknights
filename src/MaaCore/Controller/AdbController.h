#pragma once

#include "ControllerAPI.h"

#include <random>
#include <deque>

#include "Platform/PlatformFactory.h"

#include "Common/AsstMsg.h"
#include "InstHelper.h"

namespace asst
{
    class AdbController : public ControllerAPI, protected InstHelper
    {
    public:
        AdbController(const AsstCallback& callback, Assistant* inst, PlatformType type);
        AdbController(const AdbController&) = delete;
        AdbController(AdbController&&) = delete;
        virtual ~AdbController();

        virtual bool connect(const std::string& adb_path, const std::string& address,
                             const std::string& config) override;

        virtual void set_kill_adb_on_exit(bool enable) noexcept override;

        virtual bool inited() const noexcept override;

        virtual const std::string& get_uuid() const override;

        virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

        virtual bool start_game(const std::string& client_type) override;
        virtual bool stop_game() override;

        virtual bool click(const Point& p) override;

        virtual bool swipe(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                           double slope_in = 1, double slope_out = 1, bool with_pause = false) override;

        virtual bool inject_input_event([[maybe_unused]] const InputEvent& event) override { return false; }

        virtual bool press_esc() override;
        virtual ControlFeat::Feat support_features() const noexcept override { return ControlFeat::NONE; }

        virtual std::pair<int, int> get_screen_res() const noexcept override;

        AdbController& operator=(const AdbController&) = delete;
        AdbController& operator=(AdbController&&) = delete;

        virtual void back_to_home() noexcept override;

    protected:
        std::optional<std::string> call_command(const std::string& cmd, int64_t timeout = 20000,
                                                bool allow_reconnect = true, bool recv_by_socket = false);

        virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket);

        void release();

        void close_socket() noexcept;
        std::optional<unsigned short> init_socket(const std::string& local_address);

        using DecodeFunc = std::function<bool(const std::string&)>;
        bool screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect = false,
                       bool by_socket = false, int max_timeout = 20000);
        void clear_lf_info();

        virtual void clear_info() noexcept;
        void callback(AsstMsg msg, const json::value& details);

        // 转换 data 中的 CRLF 为 LF：有些模拟器自带的 adb，exec-out 输出的 \n 会被替换成 \r\n，
        // 导致解码错误，所以这里转一下回来（点名批评 mumu 和雷电）
        static bool convert_lf(std::string& data);

        AsstCallback m_callback;

        std::minstd_rand m_rand_engine;

        std::mutex m_callcmd_mutex;

        std::shared_ptr<asst::PlatformIO> m_platform_io = nullptr;

        struct AdbProperty
        {
            /* command */
            std::string connect;
            std::string call_minitouch;
            std::string call_maatouch;
            std::string click;
            std::string swipe;
            std::string press_esc;

            std::string screencap_raw_by_nc;
            std::string screencap_raw_with_gzip;
            std::string screencap_encode;
            std::string release;

            std::string start;
            std::string stop;

            std::string back_to_home;

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
                Encode
            } screencap_method = ScreencapMethod::UnknownYet;
        } m_adb;

        std::string m_uuid;
        std::pair<int, int> m_screen_size = { 0, 0 };
        int m_width = 0;
        int m_height = 0;
        bool m_support_socket = false;
        bool m_server_started = false;
        bool m_inited = false;
        bool m_kill_adb_on_exit = false;
        long long m_last_command_duration = 0;      // 上次命令执行用时
        std::deque<long long> m_screencap_duration; // 截图用时
        int m_screencap_time = 0;                   // 截图次数
    };
} // namespace asst
