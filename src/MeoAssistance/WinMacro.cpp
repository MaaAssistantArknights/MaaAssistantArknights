#include "WinMacro.h"

#include <stdint.h>
#include <WinUser.h>

#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

#include <opencv2/opencv.hpp>

#include "AsstDef.h"
#include "Logger.hpp"
#include "Configer.h"
#include "UserConfiger.h"

using namespace asst;

WinMacro::WinMacro()
	: m_rand_engine(std::chrono::system_clock::now().time_since_epoch().count())
{
	DebugTraceFunction;

	// 安全属性描述符
	m_pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_pipe_sec_attr.lpSecurityDescriptor = nullptr;
	m_pipe_sec_attr.bInheritHandle = TRUE;

	// 创建管道，本进程读-子进程写
	BOOL pipe_read_ret = ::CreatePipe(&m_pipe_read, &m_pipe_child_write, &m_pipe_sec_attr, PipeBuffSize);
	// 创建管道，本进程写-子进程读
	BOOL pipe_write_ret = ::CreatePipe(&m_pipe_write, &m_pipe_child_read, &m_pipe_sec_attr, PipeBuffSize);

	if (!pipe_read_ret || !pipe_write_ret) {
		// TODO 报错
	}

	m_child_startup_info.cb = sizeof(STARTUPINFO);
	m_child_startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	m_child_startup_info.wShowWindow = SW_HIDE;
	// 重定向子进程的读写
	m_child_startup_info.hStdInput = m_pipe_child_read;
	m_child_startup_info.hStdOutput = m_pipe_child_write;
	m_child_startup_info.hStdError = m_pipe_child_write;

	auto bind_pipe_working_proc = std::bind(&WinMacro::pipe_working_proc, this);
	m_cmd_thread = std::thread(bind_pipe_working_proc);
}

asst::WinMacro::~WinMacro()
{
	DebugTraceFunction;

	m_thread_exit = true;
	//m_thread_idle = true;
	m_cmd_condvar.notify_all();
	m_completed_id = UINT_MAX;	// make all WinMacor::wait to exit

	if (m_cmd_thread.joinable()) {
		m_cmd_thread.join();
	}

	::CloseHandle(m_pipe_read);
	::CloseHandle(m_pipe_write);
	::CloseHandle(m_pipe_child_read);
	::CloseHandle(m_pipe_child_write);
}

Rect asst::WinMacro::shaped_correct(const Rect& rect) const
{
	if (rect.width == 0 || rect.height == 0) {
		return rect;
	}
	// 明日方舟在异形屏上，有的地方是按比例缩放的，有的地方又是直接位移。没法整，这里简单粗暴一点截一个长条
	Rect dst = rect;
	if (m_scale_size.first != Configer::WindowWidthDefault) {	// 说明是宽屏
		dst.x = 0;
		dst.width = m_scale_size.first - 1;
	}
	else if (m_scale_size.second != Configer::WindowHeightDefault) { // 说明是偏方形屏
		dst.y = 0;
		dst.height = m_scale_size.second - 1;
	}
	return dst;
}

void asst::WinMacro::pipe_working_proc()
{
	DebugTraceFunction;

	while (!m_thread_exit) {
		std::unique_lock<std::mutex> cmd_queue_lock(m_cmd_queue_mutex);

		if (!m_cmd_queue.empty()) {	// 队列中有任务就执行任务
			std::string cmd = m_cmd_queue.front();
			m_cmd_queue.pop();
			cmd_queue_lock.unlock();
			// todo 判断命令是否执行成功
			call_command(cmd);
			++m_completed_id;
		}
		//else if (!m_thread_idle) {	// 队列中没有任务，又不是闲置的时候，就去截图
		//	cmd_queue_lock.unlock();
		//	auto start_time = std::chrono::system_clock::now();
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

bool WinMacro::try_capture(const EmulatorInfo& info, bool without_handle)
{
	DebugTraceFunction;

	const HandleInfo& handle_info = info.handle;
	std::string adb_dir;

	if (!without_handle) {	// 使用模拟器自带的adb
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
			UserConfiger::get_instance().set_emulator_path(info.name, emulator_path);
		}
		else {
			emulator_path = info.path;
		}

		// 到这一步说明句柄和权限没问题了，接下来就是adb的事情了
		m_emulator_info = info;
		m_handle = window_handle;
		DebugTrace("Handle:", m_handle, "Name:", m_emulator_info.name);

		adb_dir = emulator_path.substr(0, emulator_path.find_last_of('\\') + 1);
		adb_dir = '"' + StringReplaceAll(m_emulator_info.adb.path, "[EmulatorPath]", adb_dir) + '"';
	}
	else {	// 使用辅助自带的标准adb
		m_emulator_info = info;
		adb_dir = '"' + StringReplaceAll(m_emulator_info.adb.path, "[ExecDir]", GetCurrentDir()) + '"';
	}

	// TODO，检查连接是否成功
	std::string connect_cmd = StringReplaceAll(m_emulator_info.adb.connect, "[Adb]", adb_dir);
	call_command(connect_cmd);

	std::string display_cmd = StringReplaceAll(m_emulator_info.adb.display, "[Adb]", adb_dir);
	auto display_result = call_command(display_cmd);
	std::string display_pipe_str(
		std::make_move_iterator(display_result.begin()),
		std::make_move_iterator(display_result.end()));
	int size_value1 = 0;
	int size_value2 = 0;
	sscanf_s(display_pipe_str.c_str(), m_emulator_info.adb.display_regex.c_str(), &size_value1, &size_value2);
	// 为了防止抓取句柄的时候手机是竖屏的（还没进游戏），这里取大的值为宽，小的为高
	// 总不能有人竖屏玩明日方舟吧（？
	m_emulator_info.adb.display_width = (std::max)(size_value1, size_value2);
	m_emulator_info.adb.display_height = (std::min)(size_value1, size_value2);

	constexpr double DefaultRatio =
		static_cast<double>(Configer::WindowWidthDefault) / static_cast<double>(Configer::WindowHeightDefault);
	double cur_ratio = static_cast<double>(m_emulator_info.adb.display_width) / static_cast<double>(m_emulator_info.adb.display_height);

	if (cur_ratio >= DefaultRatio	// 说明是宽屏或默认16:9，按照高度计算缩放
		|| std::fabs(cur_ratio - DefaultRatio) < DoubleDiff)
	{
		int scale_width = cur_ratio * Configer::WindowHeightDefault;
		m_scale_size = std::make_pair(scale_width, Configer::WindowHeightDefault);
		m_control_scale = static_cast<double>(m_emulator_info.adb.display_height) / static_cast<double>(Configer::WindowHeightDefault);
	}
	else
	{	// 否则可能是偏正方形的屏幕，按宽度计算
		int scale_height = Configer::WindowWidthDefault / cur_ratio;
		m_scale_size = std::make_pair(Configer::WindowWidthDefault, scale_height);
		m_control_scale = static_cast<double>(m_emulator_info.adb.display_width) / static_cast<double>(Configer::WindowWidthDefault);
	}

	m_emulator_info.adb.click = StringReplaceAll(m_emulator_info.adb.click, "[Adb]", adb_dir);
	m_emulator_info.adb.swipe = StringReplaceAll(m_emulator_info.adb.swipe, "[Adb]", adb_dir);
	m_emulator_info.adb.screencap = StringReplaceAll(m_emulator_info.adb.screencap, "[Adb]", adb_dir);

	return true;
}

//void asst::WinMacro::set_idle(bool flag)
//{
//	DebugTraceFunction;
//
//	m_thread_idle = flag;
//	if (!flag) {
//		// 开始前，立即截一张图，保证第一张图片非空
//		//screencap();
//
//		m_cmd_condvar.notify_one();
//	}
//}

std::vector<unsigned char> WinMacro::call_command(const std::string& cmd)
{
	DebugTraceFunction;

	static std::mutex pipe_mutex;
	std::unique_lock<std::mutex> pipe_lock(pipe_mutex);

	PROCESS_INFORMATION process_info = { 0 };	// 进程信息结构体
	::CreateProcessA(NULL, const_cast<LPSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &m_child_startup_info, &process_info);

	std::vector<uchar> pipe_data;
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
	DebugTrace("Call", cmd, "ret", exit_ret, ", output size:", pipe_data.size());
	if (!pipe_data.empty() && pipe_data.size() < 4096) {
		DebugTrace("output:", std::string(pipe_data.cbegin(), pipe_data.cend()));
	}

	return pipe_data;
}

Point asst::WinMacro::rand_point_in_rect(const Rect& rect)
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

int asst::WinMacro::push_cmd(const std::string& cmd)
{
	std::unique_lock<std::mutex> lock(m_cmd_queue_mutex);
	m_cmd_queue.emplace(cmd);
	m_cmd_condvar.notify_one();
	return ++m_push_id;
}

void asst::WinMacro::wait(unsigned id) const noexcept
{
	while (id > m_completed_id) {
		std::this_thread::yield();
	}
}

void asst::WinMacro::screencap()
{
	DebugTraceFunction;

	auto data = call_command(m_emulator_info.adb.screencap);
	m_cache_image = cv::imdecode(data, cv::IMREAD_COLOR);

	//cv::Mat temp_image = cv::imdecode(data, cv::IMREAD_COLOR);
	////std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
	//m_cache_image = std::move(temp_image);
}

int WinMacro::click(const Point& p, bool block)
{
	int x = p.x * m_control_scale;
	int y = p.y * m_control_scale;
	//DebugTrace("Click, raw:", p.x, p.y, "corr:", x, y);

	return click_without_scale(Point(x, y), block);
}

int WinMacro::click(const Rect& rect, bool block)
{
	return click(rand_point_in_rect(rect), block);
}

int asst::WinMacro::click_without_scale(const Point& p, bool block)
{
	std::string cur_cmd = StringReplaceAll(m_emulator_info.adb.click, "[x]", std::to_string(p.x));
	cur_cmd = StringReplaceAll(cur_cmd, "[y]", std::to_string(p.y));
	int id = push_cmd(cur_cmd);
	if (block) {
		wait(id);
	}
	return id;
}

int asst::WinMacro::click_without_scale(const Rect& rect, bool block)
{
	return click_without_scale(rand_point_in_rect(rect), block);
}

int asst::WinMacro::swipe(const Point& p1, const Point& p2, int duration, bool block)
{
	int x1 = p1.x * m_control_scale;
	int y1 = p1.y * m_control_scale;
	int x2 = p2.x * m_control_scale;
	int y2 = p2.y * m_control_scale;
	//DebugTrace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

	return swipe_without_scale(Point(x1, y1), Point(x2, y2), duration, block);
}

int asst::WinMacro::swipe(const Rect& r1, const Rect& r2, int duration, bool block)
{
	return swipe(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block);
}

int asst::WinMacro::swipe_without_scale(const Point& p1, const Point& p2, int duration, bool block)
{
	std::string cur_cmd = StringReplaceAll(m_emulator_info.adb.swipe, "[x1]", std::to_string(p1.x));
	cur_cmd = StringReplaceAll(cur_cmd, "[y1]", std::to_string(p1.y));
	cur_cmd = StringReplaceAll(cur_cmd, "[x2]", std::to_string(p2.x));
	cur_cmd = StringReplaceAll(cur_cmd, "[y2]", std::to_string(p2.y));
	if (duration <= 0) {
		cur_cmd = StringReplaceAll(cur_cmd, "[duration]", "");
	}
	else {
		cur_cmd = StringReplaceAll(cur_cmd, "[duration]", std::to_string(duration));
	}

	int id = push_cmd(cur_cmd);
	if (block) {
		wait(id);
	}
	return id;
}

int asst::WinMacro::swipe_without_scale(const Rect& r1, const Rect& r2, int duration, bool block)
{
	return swipe_without_scale(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, block);
}

cv::Mat asst::WinMacro::get_image(bool raw)
{
	screencap();
	//std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
	if (raw) {
		return m_cache_image;
	}
	else {
		if (m_cache_image.empty()) {
			return m_cache_image;
		}
		const static cv::Size size(m_scale_size.first, m_scale_size.second);
		cv::Mat resize_mat;
		cv::resize(m_cache_image, resize_mat, size, cv::INPAINT_NS);
		return resize_mat;
	}
}