#include "WinMacro.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

#include <stdint.h>
#include <WinUser.h>

#include <opencv2/opencv.hpp>

#include "AsstDef.h"
#include "Logger.hpp"
#include "Configer.h"

using namespace asst;

WinMacro::WinMacro(const EmulatorInfo& info, HandleType type)
	: m_emulator_info(info),
	m_handle_type(type),
	m_rand_engine(std::chrono::system_clock::now().time_since_epoch().count())
{
	findHandle();
}

bool WinMacro::captured() const noexcept
{
	return m_handle != NULL && ::IsWindow(m_handle);
}

bool WinMacro::findHandle()
{
	std::vector<HandleInfo> handle_vec;
	switch (m_handle_type) {
	case HandleType::Window:
		m_width = m_emulator_info.width;
		m_height = m_emulator_info.height;
		handle_vec = m_emulator_info.window;
		break;
	case HandleType::View:
		handle_vec = m_emulator_info.view;
		break;
	case HandleType::Control:
		m_is_adb = m_emulator_info.is_adb;
		//m_x_offset = m_emulator_info.x_offset;
		//m_y_offset = m_emulator_info.y_offset;
		handle_vec = m_emulator_info.control;
		break;
	default:
		DebugTraceError("Handle type error!", m_handle_type);
		return false;
	}

	m_handle = NULL;
	for (const HandleInfo& handle_info : handle_vec)
	{
		wchar_t* class_wbuff = NULL;
		if (!handle_info.class_name.empty()) {
			size_t class_len = (handle_info.class_name.size() + 1) * 2;
			class_wbuff = new wchar_t[class_len];
			::MultiByteToWideChar(CP_UTF8, 0, handle_info.class_name.c_str(), -1, class_wbuff, class_len);
		}
		wchar_t* window_wbuff = NULL;
		if (!handle_info.window_name.empty()) {
			size_t window_len = (handle_info.window_name.size() + 1) * 2;
			window_wbuff = new wchar_t[window_len];
			memset(window_wbuff, 0, window_len);
			::MultiByteToWideChar(CP_UTF8, 0, handle_info.window_name.c_str(), -1, window_wbuff, window_len);
		}

		m_handle = ::FindWindowExW(m_handle, NULL, class_wbuff, window_wbuff);

		if (class_wbuff != NULL) {
			delete[] class_wbuff;
			class_wbuff = NULL;
		}
		if (window_wbuff != NULL) {
			delete[] window_wbuff;
			window_wbuff = NULL;
		}
	}
	if (m_is_adb && m_handle != NULL) {
		DWORD pid = 0;
		::GetWindowThreadProcessId(m_handle, &pid);
		HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		LPSTR path_buff = new CHAR[MAX_PATH];
		memset(path_buff, 0, MAX_PATH);
		DWORD buff_size = MAX_PATH;
		QueryFullProcessImageNameA(handle, 0, path_buff, &buff_size);
		std::string adb_dir = path_buff;
		if (path_buff != NULL) {
			delete[] path_buff;
			path_buff = NULL;
		}
		size_t pos = adb_dir.find_last_of('\\') + 1;
		adb_dir = adb_dir.substr(0, pos);
		adb_dir = '"' + StringReplaceAll(m_emulator_info.adb.path, "[EmulatorPath]", adb_dir) + '"';
		std::string connect_cmd = adb_dir + m_emulator_info.adb.connect;

		if (!callCmd(connect_cmd)) {
			DebugTraceError("Connect Adb Error", connect_cmd);
			return false;
		}
		auto&& display_ret = callCmd(adb_dir + m_emulator_info.adb.display);
		if (display_ret) {
			std::string pipe_str = display_ret.value();
			sscanf_s(pipe_str.c_str(), m_emulator_info.adb.display_regex.c_str(),
				&m_emulator_info.adb.display_width, &m_emulator_info.adb.display_height);
		}
		else {
			DebugTraceError("Get Display Error");
			return false;
		}

		m_click_cmd = adb_dir + m_emulator_info.adb.click;
		m_swipe_cmd = adb_dir + m_emulator_info.adb.swipe;
	}
	DebugTrace("Handle:", m_handle, "Name:", m_emulator_info.name, "Type:", m_handle_type);

	if (m_handle != NULL) {
		return true;
	}
	else {
		return false;
	}
}

std::optional<std::string> asst::WinMacro::callCmd(const std::string& cmd, bool use_pipe)
{
	// 初始化管道
	constexpr int PipeBuffSize = 1024;
	HANDLE pipe_read = NULL;
	HANDLE pipe_write = NULL;
	SECURITY_ATTRIBUTES sa_out_pipe;
	::ZeroMemory(&sa_out_pipe, sizeof(sa_out_pipe));
	sa_out_pipe.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa_out_pipe.lpSecurityDescriptor = NULL;
	sa_out_pipe.bInheritHandle = TRUE;
	BOOL pipe_ret = FALSE;
	if (use_pipe) {
		pipe_ret = ::CreatePipe(&pipe_read, &pipe_write, &sa_out_pipe, PipeBuffSize);
		DebugTrace("Create Pipe ret", pipe_ret);
	}

	// 准备启动ADB进程
	STARTUPINFOA startup_info;
	PROCESS_INFORMATION process_info;
	ZeroMemory(&startup_info, sizeof(startup_info));
	ZeroMemory(&process_info, sizeof(process_info));
	startup_info.cb = sizeof(STARTUPINFO);
	startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startup_info.hStdOutput = pipe_write;
	startup_info.hStdError = pipe_write;
	startup_info.wShowWindow = SW_HIDE;

	DWORD ret = -1;
	if (!::CreateProcessA(NULL, const_cast<LPSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_info)) {
		DebugTraceError("Create process error");
		return std::nullopt;
	}
	::WaitForSingleObject(process_info.hProcess, 30000);

	std::string pipe_str;
	if (use_pipe && pipe_ret) {
		DWORD read_num = 0;
		DWORD std_num = 0;
		if (::PeekNamedPipe(pipe_read, NULL, 0, NULL, &read_num, NULL) && read_num > 0) {
			char pipe_buffer[PipeBuffSize] = { 0 };
			::ReadFile(pipe_read, pipe_buffer, read_num, &std_num, NULL);
			pipe_str = std::string(pipe_buffer, std_num);
		}
	}

	::GetExitCodeProcess(process_info.hProcess, &ret);

	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);
	DebugTrace("Call", cmd, "ret", ret);
	if (use_pipe) {
		DebugTrace("Pipe:", pipe_str);
	}

	if (pipe_read != NULL) {
		::CloseHandle(pipe_read);
	}
	if (pipe_write != NULL) {
		::CloseHandle(pipe_write);
	}

	return pipe_str;
}

Point asst::WinMacro::randPointInRect(const Rect& rect)
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

bool WinMacro::resizeWindow(int width, int height)
{
	if (m_handle_type != HandleType::Window || !::IsWindow(m_handle)) {
		return false;
	}

	RECT rect;
	bool ret = ::GetWindowRect(m_handle, &rect);

	return ret && ::MoveWindow(m_handle, rect.left, rect.top, width / getScreenScale(), height / getScreenScale(), true);
}

bool WinMacro::resizeWindow()
{
	return resizeWindow(m_width, m_height);
}

bool WinMacro::showWindow()
{
	if (m_handle_type != HandleType::Window || !::IsWindow(m_handle)) {
		return false;
	}

	bool ret = ::ShowWindow(m_handle, SW_RESTORE);
	return ret;
}

bool WinMacro::hideWindow()
{
	if (m_handle_type != HandleType::Window || !::IsWindow(m_handle)) {
		return false;
	}

	return ::ShowWindow(m_handle, SW_HIDE);
}

double WinMacro::getScreenScale()
{
	static double scale = 0;
	if (scale == 0) {
		// 获取窗口当前显示的监视器
		// 使用桌面的句柄.
		HWND hWnd = ::GetDesktopWindow();
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		// 获取监视器逻辑宽度与高度
		MONITORINFOEX miex;
		miex.cbSize = sizeof(miex);
		GetMonitorInfo(hMonitor, &miex);
		int cxLogical = (miex.rcMonitor.right - miex.rcMonitor.left);
		int cyLogical = (miex.rcMonitor.bottom - miex.rcMonitor.top);

		// 获取监视器物理宽度与高度
		DEVMODE dm;
		dm.dmSize = sizeof(dm);
		dm.dmDriverExtra = 0;
		EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
		int cxPhysical = dm.dmPelsWidth;
		int cyPhysical = dm.dmPelsHeight;

		// 考虑状态栏大小，逻辑尺寸会比实际小
		double horzScale = ((double)cxPhysical / (double)cxLogical);
		double vertScale = ((double)cyPhysical / (double)cyLogical);

		// 考虑状态栏大小，选择里面大的那个
		scale = std::max(horzScale, vertScale);
	}
	return scale;
}

bool WinMacro::click(const Point& p)
{
	if (m_handle_type != HandleType::Control || !::IsWindow(m_handle)) {
		return false;
	}

	int x = p.x * m_control_scale;
	int y = p.y * m_control_scale;
	DebugTrace("Click, raw:", p.x, p.y, "corr:", x, y);

	if (m_is_adb) {
		std::string cur_cmd = StringReplaceAll(m_click_cmd, "[x]", std::to_string(x));
		cur_cmd = StringReplaceAll(cur_cmd, "[y]", std::to_string(y));
		return callCmd(cur_cmd, false).has_value();
	}
	else {
		LPARAM lparam = MAKELPARAM(x, y);
		::SendMessage(m_handle, WM_LBUTTONDOWN, MK_LBUTTON, lparam);
		::SendMessage(m_handle, WM_LBUTTONUP, 0, lparam);
		return true;
	}
}

bool WinMacro::click(const Rect& rect)
{
	if (m_handle_type != HandleType::Control || !::IsWindow(m_handle)) {
		return false;
	}

	return click(randPointInRect(rect));
}

bool asst::WinMacro::swipe(const Point& p1, const Point& p2)
{
	if (m_handle_type != HandleType::Control || !::IsWindow(m_handle)) {
		return false;
	}

	int x1 = p1.x * m_control_scale;
	int y1 = p1.y * m_control_scale;
	int x2 = p2.x * m_control_scale;
	int y2 = p2.y * m_control_scale;
	DebugTrace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

	if (m_is_adb) {
		std::string cur_cmd = StringReplaceAll(m_swipe_cmd, "[x1]", std::to_string(x1));
		cur_cmd = StringReplaceAll(cur_cmd, "[y1]", std::to_string(y1));
		cur_cmd = StringReplaceAll(cur_cmd, "[x2]", std::to_string(x2));
		cur_cmd = StringReplaceAll(cur_cmd, "[y2]", std::to_string(y2));
		return callCmd(cur_cmd, false).has_value();
		return true;
	}
	else {	// TODO
		return false;
	}
}

bool asst::WinMacro::swipe(const Rect& r1, const Rect& r2)
{
	if (m_handle_type != HandleType::Control || !::IsWindow(m_handle)) {
		return false;
	}

	return swipe(randPointInRect(r1), randPointInRect(r2));
}

void asst::WinMacro::setControlScale(double scale)
{
	if (m_is_adb) {
		m_control_scale = scale * scale * m_emulator_info.adb.display_width / Configer::DefaultWindowWidth;
	}
	else {
		m_control_scale = scale / getScreenScale();
	}
}

Rect WinMacro::getWindowRect()
{
	if (!::IsWindow(m_handle)) {
		return Rect();
	}
	RECT rect;
	bool ret = ::GetWindowRect(m_handle, &rect);
	if (!ret) {
		return Rect();
	}
	return Rect{ rect.left, rect.top,
		static_cast<int>((rect.right - rect.left) * getScreenScale()),
		static_cast<int>((rect.bottom - rect.top) * getScreenScale()) };
}

cv::Mat WinMacro::getImage(const Rect& rect)
{
	if (m_handle_type != HandleType::View || !::IsWindow(m_handle)) {
		return cv::Mat();
	}

	HDC pDC;// 源DC
	pDC = ::GetDC(m_handle);//获取屏幕DC(0为全屏，句柄则为窗口)
	int BitPerPixel = ::GetDeviceCaps(pDC, BITSPIXEL);//获得颜色模式
	HDC memDC;//内存DC
	memDC = ::CreateCompatibleDC(pDC);
	HBITMAP memBitmap, oldmemBitmap;//建立和屏幕兼容的bitmap
	memBitmap = ::CreateCompatibleBitmap(pDC, rect.width, rect.height);
	oldmemBitmap = (HBITMAP)::SelectObject(memDC, memBitmap);//将memBitmap选入内存DC
	::PrintWindow(m_handle, memDC, PW_CLIENTONLY);

	BITMAP bmp;
	GetObject(memBitmap, sizeof(BITMAP), &bmp);
	int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8;
	cv::Mat dst_mat;
	dst_mat.create(cv::Size(bmp.bmWidth, bmp.bmHeight), CV_MAKETYPE(CV_8U, nChannels));
	GetBitmapBits(memBitmap, bmp.bmHeight * bmp.bmWidth * nChannels, dst_mat.data);

	DeleteObject(memBitmap);
	DeleteDC(memDC);
	ReleaseDC(m_handle, pDC);

	/*
	#ifdef LOG_TRACE
		std::string filename = GetCurrentDir() + "\\print.bmp";
		cv::imwrite(filename, dst_mat);
	#endif
	*/

	return dst_mat;
}