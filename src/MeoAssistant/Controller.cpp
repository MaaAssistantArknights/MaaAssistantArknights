#include "AsstConf.h"
#include "Controller.h"

#ifdef _WIN32
#include <WinUser.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif

#include <cstdint>
#include <algorithm>
#include <chrono>
#include <regex>
#include <utility>
#include <vector>
#include <memory>

#include "NoWarningCV.h"

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4068 )
#endif
#include <zlib/decompress.hpp>
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "AsstTypes.h"
#include "Logger.hpp"
#include "Resource.h"

#ifdef _WIN32
// for Windows socket
class WsaHelper
{
public:
    ~WsaHelper()
    {
        WSACleanup();
    }
    static WsaHelper& get_instance()
    {
        static WsaHelper instance;
        return instance;
    }
    bool operator()() const noexcept { return m_supports; }
private:
    WsaHelper()
    {
        m_supports =
            WSAStartup(MAKEWORD(2, 2), &m_wsa_data) == 0 &&
            m_wsa_data.wVersion == MAKEWORD(2, 2);
    }
    WSADATA m_wsa_data = { 0 };
    bool m_supports = false;
};
#endif

asst::Controller::Controller(AsstCallback callback, void* callback_arg)
    : m_callback(std::move(callback)),
    m_callback_arg(callback_arg),
    m_rand_engine(std::random_device{}())
{
    LogTraceFunction;

#ifdef _WIN32
    // 安全属性描述符
    m_pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    m_pipe_sec_attr.lpSecurityDescriptor = nullptr;
    m_pipe_sec_attr.bInheritHandle = TRUE;

    // 创建管道，本进程读-子进程写
    BOOL pipe_read_ret = CreatePipe(&m_pipe_read, &m_pipe_child_write, &m_pipe_sec_attr, PipeBuffSize);
    // 创建管道，本进程写-子进程读
    BOOL pipe_write_ret = CreatePipe(&m_pipe_write, &m_pipe_child_read, &m_pipe_sec_attr, PipeBuffSize);

    if (!pipe_read_ret || !pipe_write_ret) {
        throw "controller pipe created failed";
    }

    m_child_startup_info.cb = sizeof(STARTUPINFO);
    m_child_startup_info.dwFlags = STARTF_USESTDHANDLES;
    m_child_startup_info.wShowWindow = SW_HIDE;
    // 重定向子进程的读写
    m_child_startup_info.hStdInput = m_pipe_child_read;
    m_child_startup_info.hStdOutput = m_pipe_child_write;
    m_child_startup_info.hStdError = m_pipe_child_write;

    m_support_socket = false;
    do {
        if (!WsaHelper::get_instance()()) {
            Log.error("WSA not supports");
            break;
        }
        m_support_socket = true;
    } while (false);

#else
    int pipe_in_ret = pipe(m_pipe_in);
    int pipe_out_ret = pipe(m_pipe_out);
    fcntl(m_pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);

    if (pipe_in_ret < 0 || pipe_out_ret < 0) {
        throw "controller pipe created failed";
    }
    m_support_socket = false;
#endif

    m_pipe_buffer = std::make_unique<uchar[]>(PipeBuffSize);
    if (!m_pipe_buffer) {
        throw "controller pipe buffer allocated failed";
    }
    if (m_support_socket) {
        m_socket_buffer = std::make_unique<char[]>(SocketBuffSize);
        if (!m_socket_buffer) {
            throw "controller socket buffer allocated failed";
        }
    }

    m_cmd_thread = std::thread(&Controller::pipe_working_proc, this);
}

asst::Controller::~Controller()
{
    LogTraceFunction;

    m_thread_exit = true;
    //m_thread_idle = true;
    m_cmd_condvar.notify_all();
    m_completed_id = UINT_MAX; // make all WinMacor::wait to exit

    if (m_cmd_thread.joinable()) {
        m_cmd_thread.join();
    }

    if (--m_instance_count) {
        release();
    }

#ifdef _WIN32
    CloseHandle(m_pipe_read);
    CloseHandle(m_pipe_write);
    CloseHandle(m_pipe_child_read);
    CloseHandle(m_pipe_child_write);

#else
    close(m_pipe_in[PIPE_READ]);
    close(m_pipe_in[PIPE_WRITE]);
    close(m_pipe_out[PIPE_READ]);
    close(m_pipe_out[PIPE_WRITE]);
#endif
}

//asst::Rect asst::Controller::shaped_correct(const Rect & rect) const
//{
//    if (rect.empty()
//        || m_scale_size.first == 0
//        || m_scale_size.second == 0) {
//        return rect;
//    }
//    // 明日方舟在异形屏上，有的地方是按比例缩放的，有的地方又是直接位移。没法整，这里简单粗暴一点截一个长条
//    Rect dst = rect;
//    if (m_scale_size.first != WindowWidthDefault) {                 // 说明是宽屏
//        if (rect.width <= WindowWidthDefault / 2) {
//            if (rect.x + rect.width <= WindowWidthDefault / 2) {     // 整个矩形都在左半边
//                dst.x = 0;
//                dst.width = m_scale_size.first / 2;
//            }
//            else if (rect.x >= WindowWidthDefault / 2) {            // 整个矩形都在右半边
//                dst.x = m_scale_size.first / 2;
//                dst.width = m_scale_size.first / 2;
//            }
//            else {                                                  // 整个矩形横跨了中线
//                dst.x = 0;
//                dst.width = m_scale_size.first;
//            }
//        }
//        else {
//            dst.x = 0;
//            dst.width = m_scale_size.first;
//        }
//    }
//    else if (m_scale_size.second != WindowHeightDefault) {          // 说明是偏方形屏
//        if (rect.height <= WindowHeightDefault / 2) {
//            if (rect.y + rect.height <= WindowHeightDefault / 2) {   // 整个矩形都在上半边
//                dst.y = 0;
//                dst.height = m_scale_size.second / 2;
//            }
//            else if (rect.y >= WindowHeightDefault / 2) {           // 整个矩形都在下半边
//                dst.y = m_scale_size.second / 2;
//                dst.height = m_scale_size.second / 2;                // 整个矩形横跨了中线
//            }
//            else {
//                dst.y = 0;
//                dst.height = m_scale_size.second;
//            }
//        }
//
//        else {
//            dst.y = 0;
//            dst.height = m_scale_size.second;
//        }
//    }
//    return dst;
//}

std::pair<int, int> asst::Controller::get_scale_size() const noexcept
{
    return m_scale_size;
}

void asst::Controller::pipe_working_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        std::unique_lock<std::mutex> cmd_queue_lock(m_cmd_queue_mutex);

        if (!m_cmd_queue.empty()) { // 队列中有任务就执行任务
            std::string cmd = m_cmd_queue.front();
            m_cmd_queue.pop();
            cmd_queue_lock.unlock();
            // todo 判断命令是否执行成功
            call_command(cmd);
            ++m_completed_id;
        }
        //else if (!m_thread_idle) {	// 队列中没有任务，又不是闲置的时候，就去截图
        //	cmd_queue_lock.unlock();
        //	auto start_time = std::chrono::steady_clock::now();
        //	screencap();
        //	cmd_queue_lock.lock();
        //	if (!m_cmd_queue.empty()) {
        //		continue;
        //	}
        //	m_cmd_condvar.wait_until(
        //		cmd_queue_lock,
        //		start_time + std::chrono::milliseconds(1000));	// todo 时间写到配置文件里
        //}
        else {
            //m_cmd_condvar.wait(cmd_queue_lock, [&]() -> bool {return !m_thread_idle; });
            m_cmd_condvar.wait(cmd_queue_lock);
        }
    }
}

std::optional<std::vector<uchar>> asst::Controller::call_command(const std::string& cmd, int64_t timeout, bool recv_by_socket)
{
    using namespace std::chrono_literals;
    LogTraceScope(std::string(__FUNCTION__) + " | `" + cmd + "`");

    std::vector<uchar> pipe_data;

    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&]() -> bool {
        return timeout &&
            timeout < std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time)
            .count();
    };

#ifdef _WIN32

    ASST_AUTO_DEDUCED_ZERO_INIT_START
    PROCESS_INFORMATION process_info = { nullptr }; // 进程信息结构体
    ASST_AUTO_DEDUCED_ZERO_INIT_END

    BOOL create_ret = CreateProcessA(nullptr, const_cast<LPSTR>(cmd.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &m_child_startup_info, &process_info);
    if (!create_ret) {
        Log.error("Call `", cmd, "` create process failed, ret", create_ret);
        return std::nullopt;
    }
    if (!recv_by_socket) {
        DWORD peek_num = 0;
        DWORD read_num = 0;
        std::unique_lock<std::mutex> pipe_lock(m_pipe_mutex);
        do {
            //DWORD write_num = 0;
            //WriteFile(parent_write, cmd.c_str(), cmd.size(), &write_num, nullptr);
            while (PeekNamedPipe(m_pipe_read, nullptr, 0, nullptr, &peek_num, nullptr) && peek_num > 0) {
                if (ReadFile(m_pipe_read, m_pipe_buffer.get(), PipeBuffSize, &read_num, nullptr)) {
                    pipe_data.insert(pipe_data.end(), m_pipe_buffer.get(), m_pipe_buffer.get() + read_num);
                }
            }
        } while (WaitForSingleObject(process_info.hProcess, 0) == WAIT_TIMEOUT && !check_timeout());
    }
    else {
        std::unique_lock<std::mutex> pipe_lock(m_pipe_mutex);
        fd_set fdset = { 0 };
        FD_SET(m_server_sock, &fdset);
        constexpr int TimeoutMilliseconds = 5000;
        timeval select_timeout = { TimeoutMilliseconds / 1000, (TimeoutMilliseconds % 1000) * 1000 };
        select(static_cast<int>(m_server_sock) + 1, &fdset, nullptr, nullptr, &select_timeout);
        if (FD_ISSET(m_server_sock, &fdset)) {
            SOCKET client_sock = ::accept(m_server_sock, nullptr, nullptr);
            setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&TimeoutMilliseconds), sizeof(int));
            int recv_size = 0;
            do {
                recv_size = ::recv(client_sock, (char*)m_socket_buffer.get(), SocketBuffSize, NULL);
                if (recv_size < 0) {
                    Log.error("recv error", recv_size);
                    break;
                }
                pipe_data.insert(pipe_data.end(), m_socket_buffer.get(), m_socket_buffer.get() + recv_size);
            } while (recv_size > 0 && !check_timeout());
            ::closesocket(client_sock);
        }
        WaitForSingleObject(process_info.hProcess, TimeoutMilliseconds);
    }
    DWORD exit_ret = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_ret);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

#else
    int exit_ret = 0;
    m_child = fork();
    if (m_child == 0) {
        // child process
        LogTraceScope("Child process: " + cmd);
        std::cout << ("Child process: " + cmd) << std::endl;

        dup2(m_pipe_in[PIPE_READ], STDIN_FILENO);
        dup2(m_pipe_out[PIPE_WRITE], STDOUT_FILENO);
        dup2(m_pipe_out[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        // close(m_pipe_in[PIPE_READ]);
        // close(m_pipe_in[PIPE_WRITE]);
        // close(m_pipe_out[PIPE_READ]);
        // close(m_pipe_out[PIPE_WRITE]);

        exit_ret = execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(exit_ret);
    }
    else if (m_child > 0) {
        // parent process
        // LogTraceScope("Parent process: " + cmd);
        std::unique_lock<std::mutex> pipe_lock(m_pipe_mutex);
        do {
            ssize_t read_num = read(m_pipe_out[PIPE_READ], m_pipe_buffer.get(), PipeBuffSize);

            while (read_num > 0) {
                pipe_data.insert(pipe_data.end(), m_pipe_buffer.get(), m_pipe_buffer.get() + read_num);
                read_num = read(m_pipe_out[PIPE_READ], m_pipe_buffer.get(), PipeBuffSize);
            };
        } while (::waitpid(m_child, &exit_ret, WNOHANG) == 0 && !check_timeout());
    }
    else {
        // failed to create child process
        return std::nullopt;
    }
#endif

    Log.trace("Call `", cmd, "` ret", exit_ret, ", output size:", pipe_data.size());
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        Log.trace("output:", std::string(pipe_data.cbegin(), pipe_data.cend()));
    }

    if (!exit_ret) {
        return pipe_data;
    }
    else if (m_inited) {
        // 这里用 m_inited 限制了仅递归一层，修改需要注意下
        m_inited = false;

        // 之前可以运行，突然运行不了了，这种情况多半是 adb 炸了。所以重新连接一下
        json::value reconnect_info = json::object{
            { "uuid", m_uuid},
            { "what", "Reconnecting" },
            { "why", "" },
            { "details", json::object {
                { "reconnect", m_adb.connect },
                { "cmd", cmd }
        }} };
        constexpr static int ReconnectTimes = 20;
        for (int i = 0; i < ReconnectTimes; ++i) {
            reconnect_info["details"]["times"] = i;
            m_callback(AsstMsg::ConnectionInfo, reconnect_info, m_callback_arg);

            std::this_thread::sleep_for(10s);
            auto reconnect_ret = call_command(m_adb.connect, 60LL * 1000);
            bool is_reconnect_success = false;
            if (reconnect_ret) {
                auto& reconnect_val = reconnect_ret.value();
                std::string reconnect_str(
                    std::make_move_iterator(reconnect_val.begin()),
                    std::make_move_iterator(reconnect_val.end()));
                is_reconnect_success = reconnect_str.find("error") == std::string::npos;
            }
            if (is_reconnect_success) {
                auto recall_ret = call_command(cmd, timeout, recv_by_socket);
                if (recall_ret) {
                    // 重连并成功执行了
                    m_inited = true;
                    reconnect_info["what"] = "Reconnected";
                    m_callback(AsstMsg::ConnectionInfo, reconnect_info, m_callback_arg);

                    return recall_ret;
                }
            }
        }
        json::value info = json::object{
                { "uuid", m_uuid},
                { "what", "Disconnect" },
                { "why", "Reconnect failed" },
                { "details", json::object {
                    { "cmd", m_adb.connect }
                }} };
        m_inited = false;
        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
    }

    return std::nullopt;
}

void asst::Controller::convert_lf(std::vector<uchar>& data)
{
    LogTraceFunction;

    if (data.empty() || data.size() < 2) {
        return;
    }
    auto pred = [](const std::vector<uchar>::iterator& cur) -> bool {
        return *cur == '\r' && *(cur + 1) == '\n';
    };
    // find the first of "\r\n"
    auto first_iter = data.end();
    for (auto iter = data.begin(); iter != data.end() - 1; ++iter) {
        if (pred(iter)) {
            first_iter = iter;
            break;
        }
    }
    if (first_iter == data.end()) {
        return;
    }
    // move forward all non-crlf elements
    auto end_r1_iter = data.end() - 1;
    auto next_iter = first_iter;
    while (++first_iter != end_r1_iter) {
        if (!pred(first_iter)) {
            *next_iter = *first_iter;
            ++next_iter;
        }
    }
    *next_iter = *end_r1_iter;
    ++next_iter;
    data.erase(next_iter, data.end());
}

asst::Point asst::Controller::rand_point_in_rect(const Rect& rect)
{
    int x = 0, y = 0;
    if (rect.width == 0) {
        x = rect.x;
    }
    else {
        int x_rand = std::poisson_distribution<int>(rect.width / 2.)(m_rand_engine);

        x = x_rand + rect.x;
    }

    if (rect.height == 0) {
        y = rect.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(rect.height / 2.)(m_rand_engine);
        y = y_rand + rect.y;
    }

    return { x, y };
}

void asst::Controller::random_delay() const
{
    auto& opt = Resrc.cfg().get_options();
    if (opt.control_delay_upper != 0) {
        LogTraceFunction;
        static std::default_random_engine rand_engine(std::random_device{}());
        static std::uniform_int_distribution<unsigned> rand_uni(
            opt.control_delay_lower,
            opt.control_delay_upper);

        unsigned rand_delay = rand_uni(rand_engine);

        Log.trace("random_delay |", rand_delay, "ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(rand_delay));
    }
}

void asst::Controller::clear_info() noexcept
{
    m_inited = false;
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_control_scale = 1.0;
    m_scale_size = { WindowWidthDefault, WindowHeightDefault };
#ifdef _WIN32
    if (m_server_sock) {
        ::closesocket(m_server_sock);
        m_server_sock = 0U;
    }
    m_server_started = false;
#endif
    --m_instance_count;
}

int asst::Controller::push_cmd(const std::string& cmd)
{
    random_delay();

    std::unique_lock<std::mutex> lock(m_cmd_queue_mutex);
    m_cmd_queue.emplace(cmd);
    m_cmd_condvar.notify_one();
    return static_cast<int>(++m_push_id);
}

std::optional<unsigned short> asst::Controller::try_to_init_socket(const std::string& local_address, unsigned short try_port, unsigned short try_times)
{
    LogTraceFunction;

#ifdef _WIN32
    m_server_sock = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server_sock == INVALID_SOCKET) {
        return std::nullopt;
    }
    m_server_addr.sin_family = PF_INET;
    m_server_addr.sin_addr.s_addr = inet_addr(local_address.c_str());
#else
    // Linux, todo
#endif

    bool server_start = false;
    u_short port_result = 0;

    u_short max_port = try_port + try_times;
    for (u_short port = try_port; port < max_port; ++port) {
        Log.trace("try to bind port", port);

#ifdef _WIN32
        m_server_addr.sin_port = htons(port);
        int bind_ret = ::bind(m_server_sock, reinterpret_cast<SOCKADDR*>(&m_server_addr), sizeof(SOCKADDR));
        int listen_ret = ::listen(m_server_sock, 3);
        server_start = bind_ret == 0 && listen_ret == 0;
#else
        // todo
#endif
        if (server_start) {
            port_result = port;
            break;
        }
    }

    if (!server_start) {
        Log.info("not supports netcat");
        return std::nullopt;
    }

    Log.info("server_start", local_address, port_result);
    return port_result;
}

void asst::Controller::wait(unsigned id) const noexcept
{
    using namespace std::chrono_literals;
    static constexpr auto delay = 10ms;
    while (id > m_completed_id) {
        std::this_thread::sleep_for(delay);
    }
}

bool asst::Controller::screencap()
{
    LogTraceFunction;

    //if (true) {
    //    m_inited = true;
    //    std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
    //    m_cache_image = cv::imread("err/1.png");
    //    return true;
    //}

    DecodeFunc decode_raw = [&](std::vector<uchar>& data) -> bool {
        if (data.empty()) {
            return false;
        }
        size_t std_size = 4ULL * m_width * m_height;
        if (data.size() < std_size) {
            return false;
        }
        size_t header_size = data.size() - std_size;
        bool is_all_zero = std::all_of(data.data() + header_size, data.data() + std_size,
            [](uchar uch) -> bool {
                return uch == 0;
            });
        if (is_all_zero) {
            return false;
        }
        cv::Mat temp(m_height, m_width, CV_8UC4, data.data() + header_size);
        if (temp.empty()) {
            return false;
        }
        cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    DecodeFunc decode_raw_with_gzip = [&](std::vector<uchar>& data) -> bool {
        auto unzip_data = gzip::decompress(data.data(), data.size());
        if (unzip_data.empty()) {
            return false;
        }
        size_t std_size = 4ULL * m_width * m_height;
        if (unzip_data.size() < std_size) {
            return false;
        }
        size_t header_size = unzip_data.size() - std_size;
        bool is_all_zero = std::all_of(unzip_data.data() + header_size, unzip_data.data() + std_size,
            [](uchar uch) -> bool {
                return uch == 0;
            });
        if (is_all_zero) {
            return false;
        }
        cv::Mat temp(m_height, m_width, CV_8UC4, unzip_data.data() + header_size);
        if (temp.empty()) {
            return false;
        }
        cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    DecodeFunc decode_encode = [&](std::vector<uchar>& data) -> bool {
        cv::Mat temp = cv::imdecode(data, cv::IMREAD_COLOR);
        if (temp.empty()) {
            return false;
        }
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    switch (m_adb.screencap_method) {
    case AdbProperty::ScreencapMethod::UnknownYet:
    {
        Log.info("Try to find the fastest way to screencap");
        m_inited = false;
        auto min_cost = std::chrono::nanoseconds(LLONG_MAX);

        auto start_time = std::chrono::high_resolution_clock::now();
        if (m_support_socket && m_server_started &&
            screencap(m_adb.screencap_raw_by_nc, decode_raw, true)) {
            auto duration = std::chrono::high_resolution_clock::now() - start_time;
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawByNc;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("RawByNc cost", duration.count(), "ns");
        }
        else {
            Log.info("RawByNc is not supported");
        }
        clear_lf_info();

        start_time = std::chrono::high_resolution_clock::now();
        if (screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip)) {
            auto duration = std::chrono::high_resolution_clock::now() - start_time;
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawWithGzip;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("RawWithGzip cost", duration.count(), "ns");
        }
        else {
            Log.info("RawWithGzip is not supported");
        }
        clear_lf_info();

        start_time = std::chrono::high_resolution_clock::now();
        if (screencap(m_adb.screencap_encode, decode_encode)) {
            auto duration = std::chrono::high_resolution_clock::now() - start_time;
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::Encode;
                m_inited = true;
                min_cost = duration;
            }
            Log.info("Encode cost", duration.count(), "ns");
        }
        else {
            Log.info("Encode is not supported");
        }
        Log.info("The fastest way is", static_cast<int>(m_adb.screencap_method), ", cost:", min_cost.count(), "ns");
        clear_lf_info();
        return m_inited;
    }
    break;
    case AdbProperty::ScreencapMethod::RawByNc:
    {
        return screencap(m_adb.screencap_raw_by_nc, decode_raw, true);
    }
    break;
    case AdbProperty::ScreencapMethod::RawWithGzip:
    {
        return screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip);
    }
    break;
    case AdbProperty::ScreencapMethod::Encode:
    {
        return screencap(m_adb.screencap_encode, decode_encode);
    }
    break;
    }

    return false;
}

bool asst::Controller::screencap(const std::string& cmd, const DecodeFunc& decode_func, bool by_socket)
{
    if ((!m_support_socket || !m_server_started) && by_socket) {
        return false;
    }

    auto ret = call_command(cmd, 20000, by_socket);

    if (!ret || ret.value().empty()) {
        Log.error("data is empty!");
        return false;
    }
    auto data = std::move(ret).value();

    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::CRLF) {
        convert_lf(data);
    }

    if (decode_func(data)) {
        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
            Log.info("screencap_end_of_line is LF");
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
        return true;
    }
    else {
        Log.info("data is not empty, but image is empty");

        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
            Log.info("try to cvt lf");
            convert_lf(data);

            if (decode_func(data)) {
                m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;
                Log.info("screencap_end_of_line is CRLF");
                return true;
            }
            else {
                Log.error("convert lf and retry decode failed!");
            }
        }
        return false;
    }
}

void asst::Controller::clear_lf_info()
{
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::UnknownYet;
}

cv::Mat asst::Controller::get_resized_image() const
{
    const static cv::Size d_size(m_scale_size.first, m_scale_size.second);

    std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    if (m_cache_image.empty()) {
        Log.error("image is empty");
        return { d_size, CV_8UC3 };
    }
    cv::Mat resized_mat;
    cv::resize(m_cache_image, resized_mat, d_size, 0.0, 0.0, cv::INTER_AREA);
    return resized_mat;
}

std::optional<int> asst::Controller::start_game(const std::string& client_type, bool block)
{
    if (client_type.empty()) {
        return std::nullopt;
    }
    if (auto intent_name = Resrc.cfg().get_intent_name(client_type)) {
        std::string cur_cmd = utils::string_replace_all(m_adb.start, "[Intent]", intent_name.value());
        int id = push_cmd(cur_cmd);
        if (block) {
            wait(id);
        }
        return id;
    }
    return std::nullopt;
}

std::optional<int> asst::Controller::stop_game(bool block)
{
    std::string cur_cmd = m_adb.stop;
    int id = push_cmd(cur_cmd);
    if (block) {
        wait(id);
    }
    return id;
}

int asst::Controller::click(const Point& p, bool block)
{
    int x = static_cast<int>(p.x * m_control_scale);
    int y = static_cast<int>(p.y * m_control_scale);
    //log.trace("Click, raw:", p.x, p.y, "corr:", x, y);

    return click_without_scale(Point(x, y), block);
}

int asst::Controller::click(const Rect& rect, bool block)
{
    return click(rand_point_in_rect(rect), block);
}

int asst::Controller::click_without_scale(const Point& p, bool block)
{
    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }
    std::string cur_cmd = utils::string_replace_all_batch(m_adb.click, {
        { "[x]", std::to_string(p.x) },
        { "[y]", std::to_string(p.y) }
    });
    int id = push_cmd(cur_cmd);
    if (block) {
        wait(id);
    }
    return id;
}

int asst::Controller::click_without_scale(const Rect& rect, bool block)
{
    return click_without_scale(rand_point_in_rect(rect), block);
}

int asst::Controller::swipe(const Point& p1, const Point& p2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    int x1 = static_cast<int>(p1.x * m_control_scale);
    int y1 = static_cast<int>(p1.y * m_control_scale);
    int x2 = static_cast<int>(p2.x * m_control_scale);
    int y2 = static_cast<int>(p2.y * m_control_scale);
    //log.trace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

    return swipe_without_scale(Point(x1, y1), Point(x2, y2), duration, block, extra_delay, extra_swipe);
}

int asst::Controller::swipe(const Rect& r1, const Rect& r2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    return swipe(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block, extra_delay, extra_swipe);
}

int asst::Controller::swipe_without_scale(const Point& p1, const Point& p2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    int x1 = p1.x;
    int y1 = p1.y;
    int x2 = p2.x;
    int y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    std::string cur_cmd = utils::string_replace_all_batch(m_adb.swipe, {
        { "[x1]", std::to_string(x1) },
        { "[y1]", std::to_string(y1) },
        { "[x2]", std::to_string(x2) },
        { "[y2]", std::to_string(y2) },
        { "[duration]", duration <= 0 ? "" : std::to_string(duration) }
    });

    int id = 0;
    // 额外的滑动：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
    const auto& opt = Resrc.cfg().get_options();
    if (extra_swipe && opt.adb_extra_swipe_duration > 0) {
        std::string extra_cmd = utils::string_replace_all_batch(m_adb.swipe, {
            { "[x1]", std::to_string(x2)},
            { "[y1]", std::to_string(y2) },
            { "[x2]", std::to_string(x2) },
            { "[y2]", std::to_string(y2 - opt.adb_extra_swipe_dist /* * m_control_scale*/) },
            { "[duration]", std::to_string(opt.adb_extra_swipe_duration) },
        });
        push_cmd(cur_cmd);
        id = push_cmd(extra_cmd);
    }
    else {
        id = push_cmd(cur_cmd);
    }

    if (block) {
        wait(id);
        std::this_thread::sleep_for(std::chrono::milliseconds(extra_delay));
    }
    return id;
}

int asst::Controller::swipe_without_scale(const Rect& r1, const Rect& r2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    return swipe_without_scale(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block, extra_delay, extra_swipe);
}

bool asst::Controller::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    clear_info();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    auto get_info_json = [&]() -> json::value {
        return json::object{
            { "uuid", m_uuid},
            { "details", json::object {
                { "adb", adb_path },
                { "address", address },
                { "config", config }
            }}
        };
    };

    auto adb_ret = Resrc.cfg().get_adb_cfg(config);
    if (!adb_ret) {
        json::value info = get_info_json() |
            json::object{
                { "what", "ConnectFailed" },
                { "why", "ConfigNotFound" } };
        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
        return false;
    }

    const auto adb_cfg = std::move(adb_ret.value());
    std::string display_id;
    std::string nc_address = "10.0.2.2";
    u_short nc_port = 0;

    // 里面的值每次执行命令后可能更新，所以要用 lambda 拿最新的
    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all_batch(cfg_cmd, {
            { "[Adb]", adb_path },
            { "[AdbSerial]", address },
            { "[DisplayId]", display_id },
            { "[NcPort]", std::to_string(nc_port) },
            { "[NcAddress]", nc_address },
        });
    };

    /* connect */
    {
        m_adb.connect = cmd_replace(adb_cfg.connect);
        auto connect_ret = call_command(m_adb.connect, 60LL * 1000);
        // 端口即使错误，命令仍然会返回0，TODO 对connect_result进行判断
        bool is_connect_success = false;
        if (connect_ret) {
            auto& connect_val = connect_ret.value();
            std::string connect_str(
                std::make_move_iterator(connect_val.begin()),
                std::make_move_iterator(connect_val.end()));
            is_connect_success = connect_str.find("error") == std::string::npos;
        }
        if (!is_connect_success) {
            json::value info = get_info_json() |
                json::object{
                    { "what", "ConnectFailed" },
                    { "why", "Connection command failed to exec" }
            };
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }
    }

    /* get uuid (imei) */
    {
        auto uuid_ret = call_command(cmd_replace(adb_cfg.uuid));
        if (!uuid_ret) {
            json::value info = get_info_json() |
                json::object{
                    { "what", "ConnectFailed" },
                    { "why", "Uuid command failed to exec" }
            };
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }

        auto& uuid_result = uuid_ret.value();
        std::string uuid_str(
            std::make_move_iterator(uuid_result.begin()),
            std::make_move_iterator(uuid_result.end()));
        std::erase(uuid_str, ' ');
        m_uuid = std::move(uuid_str);

        json::value info = get_info_json() |
            json::object{
                { "what", "UuidGot" },
                { "why", "" }
        };
        info["details"]["uuid"] = m_uuid;
        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
    }

    // 按需获取display ID 信息
    if (!adb_cfg.display_id.empty()) {
        auto display_id_ret = call_command(cmd_replace(adb_cfg.display_id));
        if (!display_id_ret) {
            return false;
        }

        auto& display_id_result = display_id_ret.value();
        convert_lf(display_id_result);
        std::string display_id_pipe_str(
            std::make_move_iterator(display_id_result.begin()),
            std::make_move_iterator(display_id_result.end()));
        auto last = display_id_pipe_str.rfind(':');
        if (last == std::string::npos) {
            return false;
        }

        display_id = display_id_pipe_str.substr(last + 1);
        // 去掉换行
        display_id.pop_back();
    }

    /* display */
    {
        auto display_ret = call_command(cmd_replace(adb_cfg.display));
        if (!display_ret) {
            json::value info = get_info_json() |
                json::object{
                    { "what", "ConnectFailed" },
                    { "why", "Display command failed to exec" }
            };
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }

        auto& display_result = display_ret.value();
        std::string display_pipe_str(
            std::make_move_iterator(display_result.begin()),
            std::make_move_iterator(display_result.end()));
        int size_value1 = 0;
        int size_value2 = 0;
#ifdef _MSC_VER
        sscanf_s(display_pipe_str.c_str(), adb_cfg.display_format.c_str(), &size_value1, &size_value2);
#else
        sscanf(display_pipe_str.c_str(), adb_cfg.display_format.c_str(), &size_value1, &size_value2);
#endif
        // 为了防止抓取句柄的时候手机是竖屏的（还没进游戏），这里取大的值为宽，小的为高
        // 总不能有人竖屏玩明日方舟吧（？
        m_width = (std::max)(size_value1, size_value2);
        m_height = (std::min)(size_value1, size_value2);

        json::value info = get_info_json() |
            json::object{
            { "what", "ResolutionGot" },
            { "why", "" }
        };

        info["details"] |= json::object{
                { "width", m_width },
                { "height", m_height }
        };

        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);

        if (m_width == 0 || m_height == 0) {
            info["what"] = "ResolutionError";
            info["why"] = "Get resolution failed";
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }
        else if (m_width < WindowWidthDefault || m_height < WindowHeightDefault) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Low screen resolution";
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }
        else if (std::fabs(static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault)
            - static_cast<double>(m_width) / static_cast<double>(m_height)) > 1e-7) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Not 16:9";
            m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
            return false;
        }
    }

    /* calc ratio */
    {
        constexpr double DefaultRatio =
            static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
        double cur_ratio = static_cast<double>(m_width) / static_cast<double>(m_height);

        if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9，按照高度计算缩放
            || std::fabs(cur_ratio - DefaultRatio) < DoubleDiff) {
            int scale_width = static_cast<int>(cur_ratio * WindowHeightDefault);
            m_scale_size = std::make_pair(scale_width, WindowHeightDefault);
            m_control_scale = static_cast<double>(m_height) / static_cast<double>(WindowHeightDefault);
        }
        else { // 否则可能是偏正方形的屏幕，按宽度计算
            int scale_height = static_cast<int>(WindowWidthDefault / cur_ratio);
            m_scale_size = std::make_pair(WindowWidthDefault, scale_height);
            m_control_scale = static_cast<double>(m_width) / static_cast<double>(WindowWidthDefault);
        }
    }

    {
        json::value info = get_info_json() |
            json::object{
                { "what", "Connected" },
                { "why", "" }
        };
        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);
    }

    m_adb.click = cmd_replace(adb_cfg.click);
    m_adb.swipe = cmd_replace(adb_cfg.swipe);
    m_adb.screencap_raw_with_gzip = cmd_replace(adb_cfg.screencap_raw_with_gzip);
    m_adb.screencap_encode = cmd_replace(adb_cfg.screencap_encode);
    m_adb.release = cmd_replace(adb_cfg.release);
    m_adb.start = cmd_replace(adb_cfg.start);
    m_adb.stop = cmd_replace(adb_cfg.stop);

    if (m_support_socket && !m_server_started) {
        std::string bind_address;
        if (size_t pos = address.rfind(':');
            pos != std::string::npos) {
            bind_address = address.substr(0, pos);
        }
        else {
            bind_address = "127.0.0.1";
        }

        // Todo: detect remote address, and check remote port
        // reference from https://github.com/ArknightsAutoHelper/ArknightsAutoHelper/blob/master/automator/connector/ADBConnector.py#L436
        auto nc_address_ret = call_command(cmd_replace(adb_cfg.nc_address));
        if (nc_address_ret) {
            auto& nc_result = nc_address_ret.value();
            std::string nc_result_str(
                std::make_move_iterator(nc_result.begin()),
                std::make_move_iterator(nc_result.end()));
            if (auto pos = nc_result_str.find(' ');
                pos != std::string::npos) {
                nc_address = nc_result_str.substr(0, pos);
            }
        }

        auto socket_opt = try_to_init_socket(bind_address, adb_cfg.nc_port);
        if (socket_opt) {
            nc_port = socket_opt.value();
            m_adb.screencap_raw_by_nc = cmd_replace(adb_cfg.screencap_raw_by_nc);
            m_server_started = true;
        }
        else {
            m_server_started = false;
        }
    }

    // try to find the fastest way
    screencap();

    ++m_instance_count;
    return true;
}

bool asst::Controller::release()
{
#ifdef _WIN32
    if (m_server_sock) {
        m_server_started = false;
        ::closesocket(m_server_sock);
        m_server_sock = 0U;
    }
#endif

#ifndef _WIN32
    if (m_child)
#endif
    {
        return call_command(m_adb.release).has_value();
    }
}

bool asst::Controller::inited() const noexcept
{
    return m_inited;
}

const std::string& asst::Controller::get_uuid() const
{
    return m_uuid;
}

cv::Mat asst::Controller::get_image(bool raw)
{
    if (m_scale_size.first == 0 || m_scale_size.second == 0) {
        Log.error("Unknown image size");
        return {};
    }

    // 有些模拟器adb偶尔会莫名其妙截图失败，多试几次
    constexpr static int MaxTryCount = 20;
    bool success = false;
    for (int i = 0; i < MaxTryCount && m_inited; ++i) {
        if (screencap()) {
            success = true;
            break;
        }
    }
    if (!success) {
        const static cv::Size d_size(m_scale_size.first, m_scale_size.second);
        m_cache_image = cv::Mat(d_size, CV_8UC3);
    }

    if (raw) {
        std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
        cv::Mat copy = m_cache_image.clone();
        return copy;
    }

    return get_resized_image();
}

std::vector<uchar> asst::Controller::get_image_encode() const
{
    cv::Mat img = get_resized_image();
    std::vector<uchar> buf;
    cv::imencode(".png", img, buf);

    return buf;
}
