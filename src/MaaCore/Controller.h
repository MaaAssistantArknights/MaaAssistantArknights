#pragma once

#ifdef _WIN32
#include "Utils/Platform/SafeWindows.h"
#include <mswsock.h>
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <memory>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/SingletonHolder.hpp"
#include "adb-lite/client.hpp"

namespace asst
{
    class Assistant;

    class Controller : private InstHelper
    {
    public:
        Controller(const AsstCallback& callback, Assistant* inst);
        Controller(const Controller&) = delete;
        Controller(Controller&&) = delete;
        ~Controller();

        bool connect(const std::string& adb_path, const std::string& address, const std::string& config);
        bool inited() const noexcept;
        void set_minitouch_enabled(bool enable, bool maa_touch = false) noexcept;
        void set_swipe_with_pause(bool enable) noexcept;
        void set_adb_lite_enabled(bool enable) noexcept;

        const std::string& get_uuid() const;
        cv::Mat get_image(bool raw = false);
        cv::Mat get_image_cache() const;
        bool screencap(bool allow_reconnect = false);

        bool start_game(const std::string& client_type);
        bool stop_game();

        bool click(const Point& p);
        bool click(const Rect& rect);
        bool click_without_scale(const Point& p);
        bool click_without_scale(const Rect& rect);

        bool swipe(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false, double slope_in = 1,
                   double slope_out = 1, bool with_pause = false);
        bool swipe(const Rect& r1, const Rect& r2, int duration = 0, bool extra_swipe = false, double slope_in = 1,
                   double slope_out = 1, bool with_pause = false);
        bool swipe_without_scale(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                                 double slope_in = 1, double slope_out = 1, bool with_pause = false);
        bool swipe_without_scale(const Rect& r1, const Rect& r2, int duration = 0, bool extra_swipe = false,
                                 double slope_in = 1, double slope_out = 1, bool with_pause = false);

        bool press_esc();
        bool support_swipe_with_pause() const noexcept;

        std::pair<int, int> get_scale_size() const noexcept;

        Controller& operator=(const Controller&) = delete;
        Controller& operator=(Controller&&) = delete;

    private:
        std::optional<std::string> call_command(const std::string& cmd, int64_t timeout = 20000,
                                                bool allow_reconnect = true, bool recv_by_socket = false);

#ifdef _WIN32
        std::optional<int> call_command_win32(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                              std::string& sock_data, const int64_t timeout,
                                              const std::chrono::steady_clock::time_point start_time);
#else
        std::optional<int> call_command_posix(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                              std::string& sock_data, const int64_t timeout,
                                              const std::chrono::steady_clock::time_point start_time);
#endif

        std::optional<int> call_command_tcpip(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                              std::string& sock_data, const int64_t timeout,
                                              const std::chrono::steady_clock::time_point start_time);

        void release();
        void kill_adb_daemon();
        void make_instance_inited(bool inited);

        void close_socket() noexcept;
        std::optional<unsigned short> init_socket(const std::string& local_address);

        using DecodeFunc = std::function<bool(const std::string&)>;
        bool screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect = false,
                       bool by_socket = false);
        void clear_lf_info();
        cv::Mat get_resized_image_cache() const;

        Point rand_point_in_rect(const Rect& rect);

        void random_delay() const;
        void clear_info() noexcept;
        void callback(AsstMsg msg, const json::value& details);

        bool call_and_hup_minitouch();
#ifdef _WIN32
        bool call_and_hup_minitouch_win32(const std::string& cmd, const auto& check_timeout, std::string& pipe_str);
#else
        bool call_and_hup_minitouch_posix(const std::string& cmd, const auto& check_timeout, std::string& pipe_str);
#endif
        bool call_and_hup_minitouch_tcpip(const std::string& cmd, const int timeout, std::string& pipe_str);

        bool input_to_minitouch(const std::string& cmd);
        bool input_to_minitouch_adb(const std::string& cmd);
        void release_minitouch(bool force = false);

        // 转换 data 中的 CRLF 为 LF：有些模拟器自带的 adb，exec-out 输出的 \n 会被替换成 \r\n，
        // 导致解码错误，所以这里转一下回来（点名批评 mumu 和雷电）
        static bool convert_lf(std::string& data);

        // adb 的 shell 请求无法识别用双引号包裹的字符串，所以这里去掉双引号
        static bool remove_quotes(std::string& data);

        AsstCallback m_callback;

        std::minstd_rand m_rand_engine;

        std::mutex m_callcmd_mutex;

        // adb-lite properties
        bool m_use_adb_lite = true;
        std::shared_ptr<adb::client> m_adb_client = nullptr;
        std::shared_ptr<adb::io_handle> m_minitouch_handle = nullptr;

#ifdef _WIN32

        ASST_AUTO_DEDUCED_ZERO_INIT_START
        WSADATA m_wsa_data {};
        SOCKET m_server_sock = INVALID_SOCKET;
        sockaddr_in m_server_sock_addr {};
        LPFN_ACCEPTEX m_server_accept_ex = nullptr;
        ASST_AUTO_DEDUCED_ZERO_INIT_END

#else
        int m_server_sock = -1;
        sockaddr_in m_server_sock_addr {};
        static constexpr int PIPE_READ = 0;
        static constexpr int PIPE_WRITE = 1;
        int m_pipe_in[2] = { 0 };
        int m_pipe_out[2] = { 0 };
        int m_child = 0;
#endif

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

        bool m_swipe_with_pause_enabled = false;

        bool m_minitouch_enabled = true; // 开关
        bool m_use_maa_touch = false;
        bool m_minitouch_available = false; // 状态

#ifdef _WIN32
        HANDLE m_minitouch_parent_write = INVALID_HANDLE_VALUE;
        ASST_AUTO_DEDUCED_ZERO_INIT_START
        PROCESS_INFORMATION m_minitouch_process_info = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
        ASST_AUTO_DEDUCED_ZERO_INIT_END
#else
        ::pid_t m_minitouch_process = -1;
        int m_write_to_minitouch_fd = -1;
        // TODO
#endif

        std::string m_uuid;
        inline static std::string m_adb_release; // 开了 adb daemon，但是没连上模拟器的时候，
                                                 // m_adb 并不会存下 release 的命令，但最后仍然需要一次释放。
        std::pair<int, int> m_scale_size = { WindowWidthDefault, WindowHeightDefault };
        double m_control_scale = 1.0;
        int m_width = 0;
        int m_height = 0;
        bool m_support_socket = false;
        bool m_server_started = false;
        bool m_inited = false;
        size_t m_screencap_data_general_size = 0;

        inline static int m_instance_count = 0;

        mutable std::shared_mutex m_image_mutex;
        cv::Mat m_cache_image;

    private:
        struct MinitouchProps
        {
            int max_contacts = 0;
            int max_x = 0;
            int max_y = 0;
            int max_pressure = 0;

            double x_scaling = 0;
            double y_scaling = 0;
            int orientation = 0;
        } m_minitouch_props;

        class Minitoucher
        {
        public:
            using InputFunc = std::function<bool(const std::string&)>;

        public:
            static constexpr int DefaultClickDelay = 50;
            static constexpr int DefaultSwipeDelay = 2;
            static constexpr int ExtraDelay = 0;

            Minitoucher(InputFunc func, const MinitouchProps& props, bool auto_sleep = true)
                : m_input_func(func), m_props(props), m_auto_sleep(auto_sleep)
            {}

            ~Minitoucher()
            {
                if (m_auto_sleep) {
                    sleep();
                }
            }

            bool reset() { return m_input_func(reset_cmd()); }
            bool commit() { return m_input_func(commit_cmd()); }
            bool down(int x, int y, int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
            {
                return m_input_func(down_cmd(x, y, wait_ms, with_commit, contact));
            }
            bool move(int x, int y, int wait_ms = DefaultSwipeDelay, bool with_commit = true, int contact = 0)
            {
                return m_input_func(move_cmd(x, y, wait_ms, with_commit, contact));
            }
            bool up(int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
            {
                return m_input_func(up_cmd(wait_ms, with_commit, contact));
            }
            bool key_down(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
            {
                return m_input_func(key_down_cmd(key_code, wait_ms, with_commit));
            }
            bool key_up(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
            {
                return m_input_func(key_up_cmd(key_code, wait_ms, with_commit));
            }
            bool wait(int ms) { return m_input_func(wait_cmd(ms)); }
            void clear() noexcept { m_wait_ms_count = 0; }

        private:
            [[nodiscard]] std::string reset_cmd() const noexcept { return "r\n"; }
            [[nodiscard]] std::string commit_cmd() const noexcept { return "c\n"; }
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
            [[nodiscard]] std::string down_cmd(int x, int y, int wait_ms = DefaultClickDelay, bool with_commit = true,
                                               int contact = 0)
            {
                // mac 编不过
                // std::string str = std::format("d {} {} {} {}\n", contact, x, y, pressure);

                char buff[64] = { 0 };
                auto [c_x, c_y] = scale(x, y);
                sprintf(buff, "d %d %d %d %d\n", contact, c_x, c_y, m_props.max_pressure);
                std::string str = buff;

                if (with_commit) str += commit_cmd();
                if (wait_ms) str += wait_cmd(wait_ms);
                return str;
            }

            [[nodiscard]] std::string move_cmd(int x, int y, int wait_ms = DefaultSwipeDelay, bool with_commit = true,
                                               int contact = 0)
            {
                char buff[64] = { 0 };
                auto [c_x, c_y] = scale(x, y);
                sprintf(buff, "m %d %d %d %d\n", contact, c_x, c_y, m_props.max_pressure);
                std::string str = buff;

                if (with_commit) str += commit_cmd();
                if (wait_ms) str += wait_cmd(wait_ms);
                return str;
            }

            [[nodiscard]] std::string up_cmd(int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
            {
                char buff[16] = { 0 };
                sprintf(buff, "u %d\n", contact);
                std::string str = buff;

                if (with_commit) str += commit_cmd();
                if (wait_ms) str += wait_cmd(wait_ms);
                return str;
            }

            [[nodiscard]] std::string key_down_cmd(int key_code, int wait_ms = DefaultClickDelay,
                                                   bool with_commit = true)
            {
                char buff[64] = { 0 };
                sprintf(buff, "k %d d\n", key_code);
                std::string str = buff;

                if (with_commit) str += commit_cmd();
                if (wait_ms) str += wait_cmd(wait_ms);
                return str;
            }

            [[nodiscard]] std::string key_up_cmd(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
            {
                char buff[64] = { 0 };
                sprintf(buff, "k %d u\n", key_code);
                std::string str = buff;

                if (with_commit) str += commit_cmd();
                if (wait_ms) str += wait_cmd(wait_ms);
                return str;
            }

            [[nodiscard]] std::string wait_cmd(int ms)
            {
                m_wait_ms_count += ms;
                char buff[16] = { 0 };
                sprintf(buff, "w %d\n", ms);
                return buff;
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            void sleep()
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(m_wait_ms_count * 1ms);
                m_wait_ms_count = 0;
            }

        private:
            Point scale(int x, int y) const noexcept
            {
                switch (m_props.orientation) {
                case 0:
                default:
                    return {
                        static_cast<int>(x * m_props.x_scaling),
                        static_cast<int>(y * m_props.y_scaling),
                    };
                case 1:
                    return {
                        m_props.max_y - static_cast<int>(y * m_props.y_scaling),
                        static_cast<int>(x * m_props.x_scaling),
                    };
                case 2:
                    return {
                        m_props.max_x - static_cast<int>(x * m_props.x_scaling),
                        m_props.max_y - static_cast<int>(y * m_props.y_scaling),
                    };
                case 3:
                    return {
                        static_cast<int>(y * m_props.y_scaling),
                        m_props.max_x - static_cast<int>(x * m_props.x_scaling),
                    };
                }
            }

            const std::function<bool(const std::string&)> m_input_func = nullptr;
            const MinitouchProps& m_props;
            int m_wait_ms_count = ExtraDelay;
            bool m_auto_sleep = false;
        };

    private:
#ifdef _WIN32
        // for Windows socket
        class WsaHelper : public SingletonHolder<WsaHelper>
        {
            friend class SingletonHolder<WsaHelper>;

        public:
            virtual ~WsaHelper() override { WSACleanup(); }
            bool operator()() const noexcept { return m_supports; }

        private:
            WsaHelper()
            {
                m_supports = WSAStartup(MAKEWORD(2, 2), &m_wsa_data) == 0 && m_wsa_data.wVersion == MAKEWORD(2, 2);
            }
            WSADATA m_wsa_data = { 0 };
            bool m_supports = false;
        };
#endif
    };
} // namespace asst
