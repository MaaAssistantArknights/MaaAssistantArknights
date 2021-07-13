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
	json::array handle_arr;
	json::value simulator_json = Configer::handleObj[m_simulator_name];
	switch (m_handle_type) {
	case HandleType::Window:
		m_width = simulator_json["Width"].as_integer();
		m_height = simulator_json["Height"].as_integer();
		handle_arr = simulator_json["Window"].as_array();
		break;
	case HandleType::View:
		handle_arr = simulator_json["View"].as_array();
		break;
	case HandleType::Control:
		m_xOffset = simulator_json["xOffset"].as_integer();
		m_yOffset = simulator_json["yOffset"].as_integer();
		handle_arr = simulator_json["Control"].as_array();
		break;
	default:
		DebugTraceError("Handle type error!: %d", m_handle_type);
		return false;
	}

	m_handle = NULL;
	for (auto&& obj : handle_arr)
	{
		std::string class_str = obj["class"].as_string();
		size_t class_len = (class_str.size() + 1) * 2;
		wchar_t* class_wbuff = new wchar_t[class_len];
		::MultiByteToWideChar(CP_UTF8, 0, obj["class"].as_string().c_str(), -1, class_wbuff, class_len);

		std::string window_str = obj["window"].as_string();
		size_t window_len = (window_str.size() + 1) * 2;
		wchar_t* window_wbuff = new wchar_t[window_len];
		memset(window_wbuff, 0, window_len);
		::MultiByteToWideChar(CP_UTF8, 0, obj["window"].as_string().c_str(), -1, window_wbuff, window_len);

		m_handle = ::FindWindowExW(m_handle, NULL, class_wbuff, window_wbuff);

		delete[] class_wbuff;
		delete[] window_wbuff;
	}

	DebugTrace("Handle: 0x%x, Name: %s, Type: %d", m_handle, m_simulator_name.c_str(), m_handle_type);

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

	return ::MoveWindow(m_handle, 0, 0, width / getScreenScale(), height / getScreenScale(), true);
}

bool WinMacro::resizeWindow()
{
	return resizeWindow(m_width, m_height);
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

	DebugTrace("click, raw: %d, %d, cor: %d, %d", p.x, p.y, x, y);

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

#ifdef _DEBUG
	std::string filename = Configer::getCurDir() + "\\test.bmp";
	cv::imwrite(filename, dst_mat);
#endif

	return dst_mat;
}