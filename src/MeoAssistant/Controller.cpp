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

#include "AsstDef.h"
#include "Logger.hpp"
#include "Resource.h"
#include "UserConfiger.h"

asst::Controller::Controller()
    : m_rand_engine(std::chrono::system_clock::now().time_since_epoch().count())
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

    auto bind_pipe_working_proc = std::bind(&Controller::pipe_working_proc, this);
    m_cmd_thread = std::thread(bind_pipe_working_proc);
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

#ifndef _WIN32
    if (m_child) {
#else
    if (true) {
#endif
        call_command(m_emulator_info.adb.release);
    }

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

bool asst::Controller::connect_adb(const std::string & address)
{
    LogTraceScope("connect_adb " + address);

    std::string connect_cmd = utils::string_replace_all(
        utils::string_replace_all(m_emulator_info.adb.connect, "[Adb]", m_emulator_info.adb.path),
        "[Address]", address);
    auto&& [connect_ret, connect_result] = call_command(connect_cmd);
    // 端口即使错误，命令仍然会返回0，TODO 对connect_result进行判断
    if (!connect_ret) {
        return false;
    }

    std::string display_cmd = utils::string_replace_all(
        utils::string_replace_all(m_emulator_info.adb.display, "[Adb]", m_emulator_info.adb.path),
        "[Address]", address);
    auto&& [display_ret, display_result] = call_command(display_cmd);
    if (!display_ret) {
        return false;
    }

    std::string display_pipe_str(
        std::make_move_iterator(display_result.begin()),
        std::make_move_iterator(display_result.end()));
    int size_value1 = 0;
    int size_value2 = 0;
#ifdef _MSC_VER
    sscanf_s(display_pipe_str.c_str(), m_emulator_info.adb.display_format.c_str(), &size_value1, &size_value2);
#else
    sscanf(display_pipe_str.c_str(), m_emulator_info.adb.display_format.c_str(), &size_value1, &size_value2);
#endif
    // 为了防止抓取句柄的时候手机是竖屏的（还没进游戏），这里取大的值为宽，小的为高
    // 总不能有人竖屏玩明日方舟吧（？
    m_emulator_info.adb.display_width = (std::max)(size_value1, size_value2);
    m_emulator_info.adb.display_height = (std::min)(size_value1, size_value2);

    constexpr double DefaultRatio =
        static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
    double cur_ratio = static_cast<double>(m_emulator_info.adb.display_width) / static_cast<double>(m_emulator_info.adb.display_height);

    if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9，按照高度计算缩放
        || std::fabs(cur_ratio - DefaultRatio) < DoubleDiff) {
        int scale_width = cur_ratio * WindowHeightDefault;
        m_scale_size = std::make_pair(scale_width, WindowHeightDefault);
        m_control_scale = static_cast<double>(m_emulator_info.adb.display_height) / static_cast<double>(WindowHeightDefault);
    }
    else { // 否则可能是偏正方形的屏幕，按宽度计算
        int scale_height = WindowWidthDefault / cur_ratio;
        m_scale_size = std::make_pair(WindowWidthDefault, scale_height);
        m_control_scale = static_cast<double>(m_emulator_info.adb.display_width) / static_cast<double>(WindowWidthDefault);
    }

    m_emulator_info.adb.click = utils::string_replace_all(utils::string_replace_all(m_emulator_info.adb.click, "[Adb]", m_emulator_info.adb.path), "[Address]", address);
    m_emulator_info.adb.swipe = utils::string_replace_all(utils::string_replace_all(m_emulator_info.adb.swipe, "[Adb]", m_emulator_info.adb.path), "[Address]", address);
    m_emulator_info.adb.screencap = utils::string_replace_all(utils::string_replace_all(m_emulator_info.adb.screencap, "[Adb]", m_emulator_info.adb.path), "[Address]", address);
    m_emulator_info.adb.release = utils::string_replace_all(m_emulator_info.adb.release, "[Adb]", m_emulator_info.adb.path);

    return true;
}

asst::Rect asst::Controller::shaped_correct(const Rect & rect) const
{
    if (rect.empty()) {
        return rect;
    }
    // 明日方舟在异形屏上，有的地方是按比例缩放的，有的地方又是直接位移。没法整，这里简单粗暴一点截一个长条
    Rect dst = rect;
    if (m_scale_size.first != WindowWidthDefault) {                 // 说明是宽屏
        if (rect.width <= WindowWidthDefault / 2) {
            if (rect.x + rect.width <= WindowWidthDefault / 2) {     // 整个矩形都在左半边
                dst.x = 0;
                dst.width = m_scale_size.first / 2;
            }
            else if (rect.x >= WindowWidthDefault / 2) {            // 整个矩形都在右半边
                dst.x = m_scale_size.first / 2;
                dst.width = m_scale_size.first / 2;
            }
            else {                                                  // 整个矩形横跨了中线
                dst.x = 0;
                dst.width = m_scale_size.first;
            }
        }
        else {
            dst.x = 0;
            dst.width = m_scale_size.first;
        }
    }
    else if (m_scale_size.second != WindowHeightDefault) {          // 说明是偏方形屏
        if (rect.height <= WindowHeightDefault / 2) {
            if (rect.y + rect.height <= WindowHeightDefault / 2) {   // 整个矩形都在上半边
                dst.y = 0;
                dst.height = m_scale_size.second / 2;
            }
            else if (rect.y >= WindowHeightDefault / 2) {           // 整个矩形都在下半边
                dst.y = m_scale_size.second / 2;
                dst.height = m_scale_size.second / 2;                // 整个矩形横跨了中线
            }
            else {
                dst.y = 0;
                dst.height = m_scale_size.second;
            }
        }

        else {
            dst.y = 0;
            dst.height = m_scale_size.second;
        }
    }
    return dst;
}

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

void asst::Controller::set_dirname(std::string dirname) noexcept
{
    m_dirname = std::move(dirname);
}

bool asst::Controller::try_capture(const EmulatorInfo & info, bool without_handle)
{
    LogTraceScope("try_capture | " + info.name);

    const HandleInfo& handle_info = info.handle;

    std::string adb_path;
    if (!without_handle) { // 使用模拟器自带的adb
#ifdef _WIN32   // Only support Windows
        // 转成宽字符的
        wchar_t* class_wbuff = nullptr;
        if (!handle_info.class_name.empty()) {
            size_t class_len = (handle_info.class_name.size() + 1) * 2;
            class_wbuff = new wchar_t[class_len];
            ::MultiByteToWideChar(CP_UTF8, 0, handle_info.class_name.c_str(), -1, class_wbuff, class_len);
        }
        wchar_t* window_wbuff = nullptr;
        if (!handle_info.window_name.empty()) {
            size_t window_len = (handle_info.window_name.size() + 1) * 2;
            window_wbuff = new wchar_t[window_len];
            memset(window_wbuff, 0, window_len);
            ::MultiByteToWideChar(CP_UTF8, 0, handle_info.window_name.c_str(), -1, window_wbuff, window_len);
        }
        // 查找窗口句柄
        HWND window_handle = nullptr;
        window_handle = ::FindWindowExW(window_handle, nullptr, class_wbuff, window_wbuff);

        if (class_wbuff != nullptr) {
            delete[] class_wbuff;
            class_wbuff = nullptr;
        }
        if (window_wbuff != nullptr) {
            delete[] window_wbuff;
            window_wbuff = nullptr;
        }
        if (window_handle == nullptr) {
            return false;
        }

        std::string emulator_path;
        if (info.path.empty()) {
            // 获取模拟器窗口句柄对应的进程句柄
            DWORD process_id = 0;
            ::GetWindowThreadProcessId(window_handle, &process_id);
            HANDLE process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

            // 获取模拟器程序所在路径
            LPSTR path_buff = new CHAR[MAX_PATH];
            memset(path_buff, 0, MAX_PATH);
            DWORD path_size = MAX_PATH;
            QueryFullProcessImageNameA(process_handle, 0, path_buff, &path_size);
            emulator_path = std::string(path_buff);
            if (path_buff != nullptr) {
                delete[] path_buff;
                path_buff = nullptr;
            }
            if (emulator_path.empty()) {
                return false;
            }
            Resrc.cfg().set_emulator_path(info.name, emulator_path);
            Resrc.user().set_emulator_path(info.name, emulator_path);
        }
        else {
            emulator_path = info.path;
        }

        // 到这一步说明句柄和权限没问题了，接下来就是adb的事情了
        m_emulator_info = info;
        Log.trace("Handle:", window_handle, "Name:", m_emulator_info.name);

        adb_path = emulator_path.substr(0, emulator_path.find_last_of('\\') + 1);
        adb_path = '"' + utils::string_replace_all(m_emulator_info.adb.path, "[EmulatorPath]", adb_path) + '"';
        adb_path = utils::string_replace_all(adb_path, "[ExecDir]", m_dirname);
#else
        Log.error("Capture handle is only supported on Windows!");
        return false;
#endif
    }
    else { // 使用辅助自带的标准adb
        m_emulator_info = info;
#ifdef _WIN32
        adb_path = '"' + utils::string_replace_all(m_emulator_info.adb.path, "[ExecDir]", m_dirname) + '"';
#else
        adb_path = m_emulator_info.adb.path;
#endif
    }

    m_emulator_info.adb.path = std::move(adb_path);

    // 优先使用addresses里指定的地址
    for (const std::string& address : info.adb.addresses) {
        if (connect_adb(address)) {
            return true;
        }
    }

    // 若指定地址都没连上，再尝试用devices查找地址
    std::string devices_cmd = utils::string_replace_all(m_emulator_info.adb.devices, "[Adb]", m_emulator_info.adb.path);
    auto&& [devices_ret, devices_result] = call_command(devices_cmd);
    if (!devices_ret) {
        return false;
    }
    std::string devices_pipe_str(
        std::make_move_iterator(devices_result.begin()),
        std::make_move_iterator(devices_result.end()));
    auto lines = utils::string_split(devices_pipe_str, "\r\n");

    std::string address;
    const std::regex address_regex(m_emulator_info.adb.address_regex);
    for (const std::string& line : lines) {
        std::smatch smatch;
        if (std::regex_match(line, smatch, address_regex)) {
            address = smatch[1];
        }
    }
    Log.trace("device address", address);
    if (address.empty()) {
        return false;
    }

    return connect_adb(address);
}

//void asst::Controller::set_idle(bool flag)
//{
//	LogTraceFunction;
//
//	m_thread_idle = flag;
//	if (!flag) {
//		// 开始前，立即截一张图，保证第一张图片非空
//		//screencap();
//
//		m_cmd_condvar.notify_one();
//	}
//}

std::pair<bool, std::vector<unsigned char>> asst::Controller::call_command(const std::string & cmd)
{
    LogTraceFunction;

    static std::mutex pipe_mutex;
    std::unique_lock<std::mutex> pipe_lock(pipe_mutex);

    std::vector<uchar> pipe_data;

#ifdef _WIN32
    PROCESS_INFORMATION process_info = { 0 }; // 进程信息结构体
    ::CreateProcessA(NULL, const_cast<LPSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &m_child_startup_info, &process_info);

    do {
        //DWORD write_num = 0;
        //WriteFile(parent_write, cmd.c_str(), cmd.size(), &write_num, NULL);

        DWORD read_num = 0;
        DWORD std_num = 0;
        while (::PeekNamedPipe(m_pipe_read, NULL, 0, NULL, &read_num, NULL) && read_num > 0) {
            uchar* pipe_buffer = new uchar[read_num];
            BOOL read_ret = ::ReadFile(m_pipe_read, pipe_buffer, read_num, &std_num, NULL);
            if (read_ret) {
                pipe_data.insert(pipe_data.end(), pipe_buffer, pipe_buffer + std_num);
            }
            if (pipe_buffer != nullptr) {
                delete[] pipe_buffer;
                pipe_buffer = nullptr;
            }
        }
    } while (::WaitForSingleObject(process_info.hProcess, 0) == WAIT_TIMEOUT);

    DWORD exit_ret = -1;
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

        exit_ret = execlp("sh", "sh", "-c", cmd.c_str(), NULL);
        exit(exit_ret);
    }
    else if (m_child > 0) {
        // parent process
        // LogTraceScope("Parent process: " + cmd);
        constexpr int BuffSize = 4096;
        std::unique_ptr<uchar> pipe_buffer(new uchar[BuffSize + 1]);
        do {
            memset(pipe_buffer.get(), 0, BuffSize);
            ssize_t read_num = read(m_pipe_out[PIPE_READ], pipe_buffer.get(), BuffSize);

            while (read_num > 0) {
                pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                read_num = read(m_pipe_out[PIPE_READ], pipe_buffer.get(), BuffSize);
            };
        } while (::waitpid(m_child, NULL, WNOHANG) == 0);
    }
    else {
        // failed to create child process
    }
#endif

    Log.trace("Call `", cmd, "` ret", exit_ret, ", output size:", pipe_data.size());
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        Log.trace("output:", std::string(pipe_data.cbegin(), pipe_data.cend()));
    }

    return make_pair(!exit_ret, std::move(pipe_data));
}

void asst::Controller::convert_lf(std::vector<unsigned char>&data)
{
    LogTraceFunction;

    if (data.empty() || data.size() <= 1) {
        return;
    }
    using Iter = std::vector<unsigned char>::iterator;
    auto pred = [](const Iter& cur) -> bool {
        return *cur == '\r' && *(cur + 1) == '\n';
    };
    // find the first of "\r\n"
    Iter first_iter;
    for (Iter iter = data.begin(); iter != data.end() - 1; ++iter) {
        if (pred(iter)) {
            first_iter = iter;
            break;
        }
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

asst::Point asst::Controller::rand_point_in_rect(const Rect & rect)
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
        static std::default_random_engine rand_engine(
            std::chrono::steady_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<unsigned> rand_uni(
            opt.control_delay_lower,
            opt.control_delay_upper);

        unsigned rand_delay = rand_uni(rand_engine);

        Log.trace("random_delay |", rand_delay, "ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(rand_delay));
    }
}

int asst::Controller::push_cmd(const std::string & cmd)
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
        std::this_thread::yield();
    }
}

bool asst::Controller::screencap()
{
    LogTraceFunction;

#if 1
    auto&& [ret, data] = call_command(m_emulator_info.adb.screencap);
    if (ret && !data.empty()) {
        if (m_image_convert_lf) {
            convert_lf(data);
        }
        m_cache_image = cv::imdecode(data, cv::IMREAD_COLOR);
        if (m_cache_image.empty()) {
            Log.info("Data is not empty, but image is empty, try to convert lf");
            convert_lf(data);
            m_cache_image = cv::imdecode(data, cv::IMREAD_COLOR);
            if (m_cache_image.empty()) {
                m_image_convert_lf = false;
                Log.error("convert lf and retry decode falied!");
                return false;
            }
            m_image_convert_lf = true;
            return true;
        }
        return true;
    }
    else {
        Log.error("Data is empty!");
        return false;
    }
#else
    m_cache_image = cv::imread("/home/mreo/test.png");
    return true;
#endif

    //cv::Mat temp_image = cv::imdecode(data, cv::IMREAD_COLOR);
    ////std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
    //m_cache_image = std::move(temp_image);
}

int asst::Controller::click(const Point & p, bool block)
{
    int x = p.x * m_control_scale;
    int y = p.y * m_control_scale;
    //log.trace("Click, raw:", p.x, p.y, "corr:", x, y);

    return click_without_scale(Point(x, y), block);
}

int asst::Controller::click(const Rect & rect, bool block)
{
    return click(rand_point_in_rect(rect), block);
}

int asst::Controller::click_without_scale(const Point & p, bool block)
{
    if (p.x < 0 || p.x >= m_emulator_info.adb.display_width || p.y < 0 || p.y >= m_emulator_info.adb.display_height) {
        Log.error("click point out of range");
    }
    std::string cur_cmd = utils::string_replace_all(m_emulator_info.adb.click, "[x]", std::to_string(p.x));
    cur_cmd = utils::string_replace_all(cur_cmd, "[y]", std::to_string(p.y));
    int id = push_cmd(cur_cmd);
    if (block) {
        wait(id);
    }
    return id;
}

int asst::Controller::click_without_scale(const Rect & rect, bool block)
{
    return click_without_scale(rand_point_in_rect(rect), block);
}

int asst::Controller::swipe(const Point & p1, const Point & p2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    int x1 = p1.x * m_control_scale;
    int y1 = p1.y * m_control_scale;
    int x2 = p2.x * m_control_scale;
    int y2 = p2.y * m_control_scale;
    //log.trace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

    return swipe_without_scale(Point(x1, y1), Point(x2, y2), duration, block, extra_delay, extra_swipe);
}

int asst::Controller::swipe(const Rect & r1, const Rect & r2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    return swipe(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block, extra_delay, extra_swipe);
}

int asst::Controller::swipe_without_scale(const Point & p1, const Point & p2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    if (p1.x < 0 || p1.x >= m_emulator_info.adb.display_width || p1.y < 0 || p1.y >= m_emulator_info.adb.display_height || p2.x < 0 || p2.x >= m_emulator_info.adb.display_width || p2.y < 0 || p2.y >= m_emulator_info.adb.display_height) {
        Log.error("swipe point out of range");
    }
    std::string cur_cmd = utils::string_replace_all(m_emulator_info.adb.swipe, "[x1]", std::to_string(p1.x));
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
        std::string extra_cmd = utils::string_replace_all(m_emulator_info.adb.swipe, "[x1]", std::to_string(p2.x));
        extra_cmd = utils::string_replace_all(extra_cmd, "[y1]", std::to_string(p2.y));
        int end_x = 0, end_y = 0;
        if (p2.x != p1.x) {
            double k = (double)(p2.y - p1.y) / (p2.x - p1.x);
            double temp = extra_swipe_dist / std::sqrt(1 + k * k);
            end_x = p2.x + (p2.x > p1.x ? -1 : 1) * temp;
            end_y = p2.y + (p2.y > p1.y ? -1 : 1) * std::fabs(k) * temp;
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

int asst::Controller::swipe_without_scale(const Rect & r1, const Rect & r2, int duration, bool block, int extra_delay, bool extra_swipe)
{
    return swipe_without_scale(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block, extra_delay, extra_swipe);
}

cv::Mat asst::Controller::get_image(bool raw)
{
    // 有些模拟器adb偶尔会莫名其妙截图失败，多试几次
    for (int i = 0; i != 20; ++i) {
        if (screencap()) {
            break;
        }
    }
    //std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    if (raw) {
        return m_cache_image;
    }
    else {
        if (m_cache_image.empty()) {
            return m_cache_image;
        }
        cv::Mat resized_mat;
        const static cv::Size dsize(m_scale_size.first, m_scale_size.second);
        cv::resize(m_cache_image, resized_mat, dsize);
        return resized_mat;
    }
}
