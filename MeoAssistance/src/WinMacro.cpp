#include "WinMacro.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

#include <stdint.h>
#include <WinUser.h>

#include "AsstDef.h"
#include "Logger.hpp"

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
		m_x_offset = m_emulator_info.x_offset;
		m_y_offset = m_emulator_info.y_offset;
		handle_vec = m_emulator_info.control;
		break;
	default:
		DebugTraceError("Handle type error!", m_handle_type);
		return false;
	}

	m_handle = NULL;
	for (auto&& handle_info : handle_vec)
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
		int ret = system(connect_cmd.c_str());
		DebugTrace("Call", connect_cmd, "—— ret", ret);

		m_click_cmd = adb_dir + m_emulator_info.adb.click;
	}
	DebugTrace("Handle:", m_handle, "Name:", m_emulator_info.name, "Type:", m_handle_type);

	if (m_handle != NULL) {
		return true;
	}
	else {
		return false;
	}
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

	return ::ShowWindow(m_handle, SW_RESTORE);
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
		HWND hWnd = GetDesktopWindow();
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

	if (m_is_adb) {
		int x = (p.x + m_x_offset);
		int y = (p.y + m_y_offset);
		std::string cur_cmd = StringReplaceAll(m_click_cmd, "[x]", std::to_string(x));
		cur_cmd = StringReplaceAll(cur_cmd, "[y]", std::to_string(y));
		int ret = system(cur_cmd.c_str());
		DebugTrace("Call", cur_cmd, "—— ret", ret);
		return !ret;
	}
	else {
		int x = (p.x + m_x_offset) / getScreenScale();
		int y = (p.y + m_y_offset) / getScreenScale();
		DebugTrace("Click, raw:", p.x, p.y, "corr:", x, y);

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

	return click({ x, y });
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
	#ifdef _DEBUG
		std::string filename = GetCurrentDir() + "\\print.bmp";
		cv::imwrite(filename, dst_mat);
	#endif
	*/

	return dst_mat;
}