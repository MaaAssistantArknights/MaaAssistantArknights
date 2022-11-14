#pragma once

#ifdef _WIN32
#include "Utils/Platform/SafeWindows.h"
#include <mswsock.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "Utils/AsstConf.h"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>

#include "Utils/AsstMsg.h"
#include "Utils/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
    class Controller
    {
    public:
        Controller(AsstCallback callback, void* callback_arg);
        Controller(const Controller&) = delete;
        Controller(Controller&&) = delete;
        ~Controller();

        inline static void set_resource_path(const std::filesystem::path& path) { m_resource_path = path; }

        bool connect(const std::string& adb_path, const std::string& address, const std::string& config);
        bool inited() const noexcept;
        void set_exit_flag(bool* flag);
        void set_minitouch_enabled(bool enable) noexcept { m_minitouch_enabled = enable; }

        const std::string& get_uuid() const;
        cv::Mat get_image(bool raw = false);
        std::vector<uchar> get_encoded_image_cache() const;

        bool start_game(const std::string& client_type);
        bool stop_game();

        bool click(const Point& p);
        bool click(const Rect& rect);
        bool click_without_scale(const Point& p);
        bool click_without_scale(const Rect& rect);

        bool swipe(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                   double acceleration_coef = 0);
        bool swipe(const Rect& r1, const Rect& r2, int duration = 0, bool extra_swipe = false,
                   double acceleration_coef = 0);
        bool swipe_without_scale(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                                 double acceleration_coef = 0);
        bool swipe_without_scale(const Rect& r1, const Rect& r2, int duration = 0, bool extra_swipe = false,
                                 double acceleration_coef = 0);

        std::pair<int, int> get_scale_size() const noexcept;

        Controller& operator=(const Controller&) = delete;
        Controller& operator=(Controller&&) = delete;

    private:
        bool need_exit() const;
        std::optional<std::string> call_command(const std::string& cmd, int64_t timeout = 20000,
                                                bool allow_reconnect = true, bool recv_by_socket = false);
        bool release();
        void kill_adb_daemon();
        bool make_instance_inited(bool inited);

        void close_socket() noexcept;
        std::optional<unsigned short> init_socket(const std::string& local_address);

        using DecodeFunc = std::function<bool(const std::string&)>;
        bool screencap(bool allow_reconnect = false);
        bool screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect = false,
                       bool by_socket = false);
        void clear_lf_info();
        cv::Mat get_resized_image_cache() const;

        Point rand_point_in_rect(const Rect& rect);

        void random_delay() const;
        void clear_info() noexcept;
        void callback(AsstMsg msg, const json::value& details);

        bool call_and_hup_minitouch(const std::string& cmd);
        bool input_to_minitouch(const std::string& cmd);
        void release_minitouch(bool force = false);

        // 转换 data 中的 CRLF 为 LF：有些模拟器自带的 adb，exec-out 输出的 \n 会被替换成 \r\n，
        // 导致解码错误，所以这里转一下回来（点名批评 mumu 和雷电）
        static bool convert_lf(std::string& data);

        bool* m_exit_flag = nullptr;
        AsstCallback m_callback;
        void* m_callback_arg = nullptr;

        std::minstd_rand m_rand_engine;

        std::mutex m_callcmd_mutex;

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
            std::string click;
            std::string swipe;

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

        bool m_minitouch_enabled = false;  // 开关
        bool m_minitouch_avaiable = false; // 状态

#ifdef _WIN32
        HANDLE m_minitouch_child_read = nullptr;
        HANDLE m_minitouch_parent_write = nullptr;
        ASST_AUTO_DEDUCED_ZERO_INIT_START
        PROCESS_INFORMATION m_minitouch_process_info = { nullptr };
        ASST_AUTO_DEDUCED_ZERO_INIT_END
#else
        // TODO
#endif

        inline static std::filesystem::path m_resource_path;

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
        } m_minitouch_props;

        class MinitouchHelper
        {
        public:
            static constexpr int DefaultClickDelay = 50;
            static constexpr int DefaultSwipeDelay = 1;

            MinitouchHelper(const MinitouchProps& props, bool auto_sleep = true)
                : m_props(props), m_auto_sleep(auto_sleep)
            {}

            ~MinitouchHelper()
            {
                if (m_auto_sleep) {
                    sleep();
                }
            }

            [[nodiscard]] inline std::string reset() { return "r\n"; }
            [[nodiscard]] inline std::string commit() { return "c\n"; }
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
            [[nodiscard]] std::string down(int x, int y, int wait_ms = DefaultClickDelay, bool with_commit = true,
                                           int contact = 0)
            {
                // mac 编不过
                // std::string str = std::format("d {} {} {} {}\n", contact, x, y, pressure);

                char buff[64] = { 0 };
                sprintf(buff, "d %d %d %d %d\n", contact, scale_x(x), scale_y(y), m_props.max_pressure);
                std::string str = buff;

                if (with_commit) str += commit();
                if (wait_ms) str += wait(wait_ms);
                return str;
            }

            [[nodiscard]] std::string move(int x, int y, int wait_ms = DefaultSwipeDelay, bool with_commit = true,
                                           int contact = 0)
            {
                char buff[64] = { 0 };
                sprintf(buff, "m %d %d %d %d\n", contact, scale_x(x), scale_y(y), m_props.max_pressure);
                std::string str = buff;

                if (with_commit) str += commit();
                if (wait_ms) str += wait(wait_ms);
                return str;
            }

            [[nodiscard]] std::string up(int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
            {
                char buff[16] = { 0 };
                sprintf(buff, "u %d\n", contact);
                std::string str = buff;

                if (with_commit) str += commit();
                if (wait_ms) str += wait(wait_ms);
                return str;
            }

            [[nodiscard]] std::string wait(int ms)
            {
                m_wait_ms_count += ms;

                char buff[16] = { 0 };
                sprintf(buff, "w %d\n", ms);
                return buff;
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            void clear() noexcept { m_wait_ms_count = 0; }
            void sleep()
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(m_wait_ms_count * 1ms);
                m_wait_ms_count = 0;
            }

        private:
            int scale_x(int x) const noexcept { return static_cast<int>(x * m_props.x_scaling); }
            int scale_y(int y) const noexcept { return static_cast<int>(y * m_props.y_scaling); }

            const MinitouchProps& m_props;
            int m_wait_ms_count = 0;
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
