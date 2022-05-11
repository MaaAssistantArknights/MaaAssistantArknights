#include "Controller.h"

#ifdef _WIN32
#include <WinUser.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif

#include <stdint.h>
#include <algorithm>
#include <chrono>
#include <regex>
#include <utility>
#include <vector>

#include <opencv2/opencv.hpp>

#pragma warning( push )
#pragma warning( disable : 4068)
#include <zlib/decompress.hpp>
#pragma warning( pop )

#include "AsstTypes.h"
#include "Logger.hpp"
#include "Resource.h"

asst::Controller::Controller(AsstCallback callback, void* callback_arg)
    : m_callback(callback),
    m_callback_arg(callback_arg),
    m_rand_engine(static_cast<unsigned int>(time(nullptr)))
{
    LogTraceFunction;

#ifdef _WIN32
    // 安全属性描述符
    m_pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    m_pipe_sec_attr.lpSecurityDescriptor = nullptr;
    m_pipe_sec_attr.bInheritHandle = TRUE;

    // 创建管道，本进程读-子进程写
    BOOL pipe_read_ret = ::CreatePipe(&m_pipe_read, &m_pipe_child_write, &m_pipe_sec_attr, PipeBuffSize);
    // 创建管道，本进程写-子进程读
    BOOL pipe_write_ret = ::CreatePipe(&m_pipe_write, &m_pipe_child_read, &m_pipe_sec_attr, PipeBuffSize);

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
#else
    int pipe_in_ret = pipe(m_pipe_in);
    int pipe_out_ret = pipe(m_pipe_out);
    fcntl(m_pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);

    if (pipe_in_ret < 0 || pipe_out_ret < 0) {
        throw "controller pipe created failed";
    }

#endif
    m_pipe_buffer = std::make_unique<uchar[]>(PipeBuffSize);
    if (!m_pipe_buffer) {
        throw "controller pipe buffer allocated failed";
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

    disconnect();

#ifdef _WIN32
    ::CloseHandle(m_pipe_read);
    ::CloseHandle(m_pipe_write);
    ::CloseHandle(m_pipe_child_read);
    ::CloseHandle(m_pipe_child_write);
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

std::optional<std::vector<unsigned char>> asst::Controller::call_command(const std::string& cmd, int64_t timeout)
{
    LogTraceFunction;

    std::vector<uchar> pipe_data;

    static std::mutex pipe_mutex;
    std::unique_lock<std::mutex> pipe_lock(pipe_mutex);

    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&]() -> bool {
        return timeout &&
            timeout < std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time)
            .count();
    };

#ifdef _WIN32
    PROCESS_INFORMATION process_info = { 0 }; // 进程信息结构体
    BOOL create_ret = ::CreateProcessA(nullptr, const_cast<LPSTR>(cmd.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &m_child_startup_info, &process_info);
    if (!create_ret) {
        return std::nullopt;
    }
    DWORD peek_num = 0;
    DWORD read_num = 0;
    do {
        //DWORD write_num = 0;
        //WriteFile(parent_write, cmd.c_str(), cmd.size(), &write_num, nullptr);
        while (::PeekNamedPipe(m_pipe_read, nullptr, 0, nullptr, &peek_num, nullptr) && peek_num > 0) {
            if (::ReadFile(m_pipe_read, m_pipe_buffer.get(), PipeBuffSize, &read_num, nullptr)) {
                pipe_data.insert(pipe_data.end(), m_pipe_buffer.get(), m_pipe_buffer.get() + read_num);
            }
        }
    } while (::WaitForSingleObject(process_info.hProcess, 0) == WAIT_TIMEOUT && !check_timeout());

    DWORD exit_ret = 255;
    ::GetExitCodeProcess(process_info.hProcess, &exit_ret);
    ::CloseHandle(process_info.hProcess);
    ::CloseHandle(process_info.hThread);

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
    else {
        return std::nullopt;
    }
}

void asst::Controller::convert_lf(std::vector<unsigned char>& data)
{
    LogTraceFunction;

    if (data.empty() || data.size() < 2) {
        return;
    }
    using Iter = std::vector<unsigned char>::iterator;
    auto pred = [](const Iter& cur) -> bool {
        return *cur == '\r' && *(cur + 1) == '\n';
    };
    // find the first of "\r\n"
    Iter first_iter = data.end();
    for (Iter iter = data.begin(); iter != data.end() - 1; ++iter) {
        if (pred(iter)) {
            first_iter = iter;
            break;
        }
    }
    if (first_iter == data.end()) {
        return;
    }
    // move forward all non-crlf elements
    Iter end_r1_iter = data.end() - 1;
    Iter next_iter = first_iter;
    while (++first_iter != end_r1_iter) {
        if (!pred(first_iter)) {
            *next_iter = std::move(*first_iter);
            ++next_iter;
        }
    }
    *next_iter = std::move(*end_r1_iter);
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
        int x_rand = std::poisson_distribution<int>(rect.width / 2)(m_rand_engine);

        x = x_rand + rect.x;
    }

    if (rect.height == 0) {
        y = rect.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(rect.height / 2)(m_rand_engine);
        y = y_rand + rect.y;
    }

    return Point(x, y);
}

void asst::Controller::random_delay() const
{
    auto& opt = Resrc.cfg().get_options();
    if (opt.control_delay_upper != 0) {
        LogTraceFunction;
        static std::default_random_engine rand_engine(static_cast<unsigned int>(time(nullptr)));
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
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_control_scale = 1.0;
    m_scale_size = decltype(m_scale_size)();
}

int asst::Controller::push_cmd(const std::string& cmd)
{
    random_delay();

    std::unique_lock<std::mutex> lock(m_cmd_queue_mutex);
    m_cmd_queue.emplace(cmd);
    m_cmd_condvar.notify_one();
    return ++m_push_id;
}

void asst::Controller::wait(unsigned id) const noexcept
{
    while (id > m_completed_id) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::this_thread::yield();
    }
}

bool asst::Controller::screencap()
{
    LogTraceFunction;

    DecodeFunc decode_raw_with_gzip = [&](const std::vector<uchar>& data) -> bool {
        auto unzip_data = gzip::decompress(data.data(), data.size());
        Log.trace("unzip data size:", unzip_data.size());
        if (unzip_data.empty()) {
            return false;
        }
        size_t std_size = m_width * m_height * 4;
        if (unzip_data.size() < std_size) {
            return false;
        }
        size_t header_size = unzip_data.size() - std_size;
        Log.trace("header size:", header_size);

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

    DecodeFunc decode_encode = [&](const std::vector<uchar>& data) -> bool {
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
        if (screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip)) {
            m_adb.screencap_method = AdbProperty::ScreencapMethod::RawWithGzip;
            return true;
        }
        else if (screencap(m_adb.screencap_encode, decode_encode)) {
            m_adb.screencap_method = AdbProperty::ScreencapMethod::Encode;
            return true;
        }
        else {
            return false;
        }
    }
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

bool asst::Controller::screencap(const std::string& cmd, DecodeFunc decode_func)
{
    auto ret = call_command(cmd);

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
                return true;
            }
            else {
                Log.error("convert lf and retry decode falied!");
            }
        }
        return false;
    }
}

cv::Mat asst::Controller::get_resized_image() const
{
    const static cv::Size dsize(m_scale_size.first, m_scale_size.second);

    std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    if (m_cache_image.empty()) {
        Log.error("image is empty");
        return cv::Mat(dsize, CV_8UC3);
    }
    cv::Mat resized_mat;
    cv::resize(m_cache_image, resized_mat, dsize);
    return resized_mat;
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
    std::string cur_cmd = utils::string_replace_all(m_adb.click, "[x]", std::to_string(p.x));
    cur_cmd = utils::string_replace_all(cur_cmd, "[y]", std::to_string(p.y));
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
    if (p1.x < 0 || p1.x >= m_width || p1.y < 0 || p1.y >= m_height || p2.x < 0 || p2.x >= m_width || p2.y < 0 || p2.y >= m_height) {
        Log.error("swipe point out of range");
    }
    std::string cur_cmd = utils::string_replace_all(m_adb.swipe, "[x1]", std::to_string(p1.x));
    cur_cmd = utils::string_replace_all(cur_cmd, "[y1]", std::to_string(p1.y));
    cur_cmd = utils::string_replace_all(cur_cmd, "[x2]", std::to_string(p2.x));
    cur_cmd = utils::string_replace_all(cur_cmd, "[y2]", std::to_string(p2.y));
    if (duration <= 0) {
        cur_cmd = utils::string_replace_all(cur_cmd, "[duration]", "");
    }
    else {
        cur_cmd = utils::string_replace_all(cur_cmd, "[duration]", std::to_string(duration));
    }

    int id = 0;

    int extra_swipe_dist = Resrc.cfg().get_options().adb_extra_swipe_dist /* * m_control_scale*/;
    int extra_swipe_duration = Resrc.cfg().get_options().adb_extra_swipe_duration;

    // 额外的滑动：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
    if (extra_swipe && extra_swipe_duration >= 0) {
        std::string extra_cmd = utils::string_replace_all(m_adb.swipe, "[x1]", std::to_string(p2.x));
        extra_cmd = utils::string_replace_all(extra_cmd, "[y1]", std::to_string(p2.y));
        int end_x = 0, end_y = 0;
        if (p2.x != p1.x) {
            double k = (double)(p2.y - p1.y) / (p2.x - p1.x);
            double temp = extra_swipe_dist / std::sqrt(1 + k * k);
            end_x = p2.x + static_cast<int>((p2.x > p1.x ? -1.0 : 1.0) * temp);
            end_y = p2.y + static_cast<int>((p2.y > p1.y ? -1.0 : 1.0) * std::fabs(k) * temp);
        }
        else {
            end_x = p2.x;
            end_y = p2.y + (p2.y > p1.y ? -1 : 1) * extra_swipe_dist;
        }
        extra_cmd = utils::string_replace_all(extra_cmd, "[x2]", std::to_string(end_x));
        extra_cmd = utils::string_replace_all(extra_cmd, "[y2]", std::to_string(end_y));
        extra_cmd = utils::string_replace_all(extra_cmd, "[duration]", std::to_string(extra_swipe_duration));

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

    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        const std::unordered_map<std::string, std::string> replacements = {
            { "[Adb]", adb_path },
            { "[Address]", address },
            { "[DisplayId]", display_id }
        };
        std::string formatted = cfg_cmd;
        for (const auto& [key, value] : replacements) {
            formatted = utils::string_replace_all(formatted, key, value);
        }
        return formatted;
    };

    /* connect */
    {
        auto connect_ret = call_command(cmd_replace(adb_cfg.connect), 60 * 1000);
        // 端口即使错误，命令仍然会返回0，TODO 对connect_result进行判断
        if (!connect_ret) {
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
        uuid_str.erase(std::remove(uuid_str.begin(), uuid_str.end(), ' '), uuid_str.end());
        m_uuid = std::move(uuid_str);

        json::value info = get_info_json() |
            json::object{
                { "what", "UuidGetted" },
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
            { "what", "ResolutionGetted" },
            { "why", "" }
        };

        info["details"] |= json::object{
                { "width", m_width },
                { "height", m_height }
        };

        m_callback(AsstMsg::ConnectionInfo, info, m_callback_arg);

        if (m_width < WindowWidthDefault || m_height < WindowHeightDefault) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Low screen resolution";
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

    return true;
}

bool asst::Controller::disconnect()
{
#ifndef _WIN32
    if (m_child)
#else
    if (true)
#endif
    {
        return call_command(m_adb.release).has_value();
    }
}

const std::string& asst::Controller::get_uuid() const
{
    return m_uuid;
}

cv::Mat asst::Controller::get_image(bool raw)
{
    // 有些模拟器adb偶尔会莫名其妙截图失败，多试几次
    for (int i = 0; i != 20; ++i) {
        if (screencap()) {
            break;
        }
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
