#pragma once

#include "AsstConf.h"

#include <optional>
#include <random>
#include <string>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "NoWarningCVMat.h"

#include "AsstTypes.h"
#include "AsstMsg.h"

namespace asst
{
    class Controller
    {
    public:
        Controller(AsstCallback callback, void* callback_arg);
        Controller(const Controller&) = delete;
        Controller(Controller&&) = delete;
        ~Controller();

        bool connect(const std::string& adb_path, const std::string& address, const std::string& config);
        bool release();
        bool inited() const noexcept;

        const std::string& get_uuid() const;
        cv::Mat get_image(bool raw = false);
        std::vector<uchar> get_image_encode() const;

        /* 开启游戏、点击和滑动都是异步执行，返回该任务的id */

        std::optional<int> start_game(const std::string& client_type, bool block = true);
        std::optional<int> stop_game(bool block = true);

        int click(const Point& p, bool block = true);
        int click(const Rect& rect, bool block = true);
        int click_without_scale(const Point& p, bool block = true);
        int click_without_scale(const Rect& rect, bool block = true);

        constexpr static int SwipeExtraDelayDefault = 1000;
        int swipe(const Point& p1, const Point& p2, int duration = 0, bool block = true, int extra_delay = SwipeExtraDelayDefault, bool extra_swipe = false);
        int swipe(const Rect& r1, const Rect& r2, int duration = 0, bool block = true, int extra_delay = SwipeExtraDelayDefault, bool extra_swipe = false);
        int swipe_without_scale(const Point& p1, const Point& p2, int duration = 0, bool block = true, int extra_delay = SwipeExtraDelayDefault, bool extra_swipe = false);
        int swipe_without_scale(const Rect& r1, const Rect& r2, int duration = 0, bool block = true, int extra_delay = SwipeExtraDelayDefault, bool extra_swipe = false);

        void wait(unsigned id) const noexcept;

        // 异形屏矫正
        //Rect shaped_correct(const Rect& rect) const;
        std::pair<int, int> get_scale_size() const noexcept;

        Controller& operator=(const Controller&) = delete;
        Controller& operator=(Controller&&) = delete;

    private:
        void pipe_working_proc();
        std::optional<std::vector<uchar>> call_command(const std::string& cmd, int64_t timeout = 20000, bool recv_by_socket = false);
        int push_cmd(const std::string& cmd);

        std::optional<unsigned short> try_to_init_socket(const std::string& local_address, unsigned short try_port, unsigned short try_times = 10U);

        using DecodeFunc = std::function<bool(std::vector<uchar>&)>;
        bool screencap();
        bool screencap(const std::string& cmd, const DecodeFunc& decode_func, bool by_nc = false);
        void clear_lf_info();
        cv::Mat get_resized_image() const;

        Point rand_point_in_rect(const Rect& rect);

        void random_delay() const;
        void clear_info() noexcept;

        // 转换data中所有的crlf为lf：有些模拟器自带的adb，exec-out输出的\n，会被替换成\r\n，导致解码错误，所以这里转一下回来（点名批评mumu）
        static void convert_lf(std::vector<uchar>& data);

        AsstCallback m_callback;
        void* m_callback_arg = nullptr;

        std::minstd_rand m_rand_engine;

        constexpr static int PipeBuffSize = 4 * 1024 * 1024; // 管道缓冲区大小
        constexpr static int SocketBuffSize = 4 * 1024 * 1024;   // socket 缓冲区大小
        std::unique_ptr<uchar[]> m_pipe_buffer = nullptr;
        std::mutex m_pipe_mutex;
        std::unique_ptr<char[]> m_socket_buffer = nullptr;
        std::mutex m_socket_mutex;
#ifdef _WIN32
        HANDLE m_pipe_read = nullptr;                // 读管道句柄
        HANDLE m_pipe_write = nullptr;               // 写管道句柄
        HANDLE m_pipe_child_read = nullptr;          // 子进程的读管道句柄
        HANDLE m_pipe_child_write = nullptr;         // 子进程的写管道句柄

        ASST_AUTO_DEDUCED_ZERO_INIT_START
        SECURITY_ATTRIBUTES m_pipe_sec_attr = { 0 }; // 管道安全描述符
        STARTUPINFOA m_child_startup_info = { 0 };   // 子进程启动信息

        WSADATA m_wsa_data = { 0 };
        SOCKET m_server_sock = 0ULL;
        sockaddr_in m_server_addr = { 0 };
        ASST_AUTO_DEDUCED_ZERO_INIT_END

#else
        constexpr static int PIPE_READ = 0;
        constexpr static int PIPE_WRITE = 1;
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
                //Default,
                RawByNc,
                RawWithGzip,
                Encode
            } screencap_method = ScreencapMethod::UnknownYet;
        } m_adb;

        std::string m_uuid;
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

        bool m_thread_exit = false;
        //bool m_thread_idle = true;
        std::mutex m_cmd_queue_mutex;
        std::condition_variable m_cmd_condvar;
        std::queue<std::string> m_cmd_queue;
        std::atomic<unsigned> m_completed_id = 0;
        unsigned m_push_id = 0; // push_id的自增总是伴随着queue的push，肯定是要上锁的，所以没必要原子
        std::thread m_cmd_thread;
    };
}
