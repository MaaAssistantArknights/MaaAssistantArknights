#include "WinMacro.h"

#include <vector>
#include <utility>
#include <ctime>
#include <algorithm>

#include <stdint.h>
#include <WinUser.h>

#include "Configer.h"
#include "AsstDef.h"

using namespace asst;

WinMacro::WinMacro(const std::string& simulator_name, HandleType type)
	: m_simulator_name(simulator_name),
	m_handle_type(type),
	m_rand_engine(time(NULL))
{
	findHandle();
}

bool WinMacro::captured() const noexcept
{
	return m_handle != NULL;
}

bool WinMacro::findHandle()
{
	std::vector<HandleInfo> handle_vec;
	auto&& info = Configer::m_handles[m_simulator_name];
	switch (m_handle_type) {
	case HandleType::Window:
		m_width = info.width;
		m_height = info.height;
		handle_vec = info.window;
		break;
	case HandleType::View:
		handle_vec = info.view;
		break;
	case HandleType::Control:
		m_xOffset = info.xOffset;
		m_yOffset = info.yOffset;
		handle_vec = info.control;
		break;
	default:
		DebugTraceError("Handle type error!", static_cast<int>(m_handle_type));
		return false;
	}

	m_handle = NULL;
	for (auto&& handle_info : handle_vec)
	{
		wchar_t* class_wbuff = NULL;
		if (!handle_info.className.empty()) {
			size_t class_len = (handle_info.className.size() + 1) * 2;
			class_wbuff = new wchar_t[class_len];
			::MultiByteToWideChar(CP_UTF8, 0, handle_info.className.c_str(), -1, class_wbuff, class_len);
		}
		wchar_t* window_wbuff = NULL;
		if (!handle_info.windowName.empty()) {
			size_t window_len = (handle_info.windowName.size() + 1) * 2;
			window_wbuff = new wchar_t[window_len];
			memset(window_wbuff, 0, window_len);
			::MultiByteToWideChar(CP_UTF8, 0, handle_info.windowName.c_str(), -1, window_wbuff, window_len);
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

	DebugTrace("Handle:", m_handle, "Name:", m_simulator_name, "Type:", static_cast<int>(m_handle_type));

	if (m_handle != NULL) {
		return true;
	}
	else {
		return false;
	}
}

bool WinMacro::resizeWindow(int width, int height)
{
	if (m_handle_type != HandleType::Window) {
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
	if (m_handle_type != HandleType::Window) {
		return false;
	}

	return ::ShowWindow(m_handle, SW_SHOW);
}

bool WinMacro::hideWindow()
{
	if (m_handle_type != HandleType::Window) {
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
	if (m_handle_type != HandleType::Control) {
		return false;
	}

	int x = (p.x + m_xOffset) / getScreenScale();
	int y = (p.y + m_yOffset) / getScreenScale();

	DebugTrace("click, raw:", p.x, p.y, "cor:", x, y);

	LPARAM lparam = MAKELPARAM(x, y);

	::SendMessage(m_handle, WM_LBUTTONDOWN, MK_LBUTTON, lparam);
	::SendMessage(m_handle, WM_LBUTTONUP, 0, lparam);

	return true;
}

bool WinMacro::clickRange(const Rect& rect)
{
	if (m_handle_type != HandleType::Control) {
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
	if (m_handle_type != HandleType::View) {
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