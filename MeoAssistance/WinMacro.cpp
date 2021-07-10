#include "WinMacro.h"

#include <vector>
#include <utility>
#include <ctime>

#include <stdint.h>
#include <WinUser.h>

#ifdef _DEBUG
#include <iostream>
#endif

using namespace MeoAssistance;

WinMacro::WinMacro(HandleType type)
    : m_handle_type(type),
    m_rand_engine(time(NULL))
{
}

bool WinMacro::findHandle()
{
    switch (m_handle_type) {
    case HandleType::BlueStacksControl: {
        HWND temp_handle = ::FindWindow(L"BS2CHINAUI", L"BlueStacks App Player");
        temp_handle = ::FindWindowEx(temp_handle, NULL, L"BS2CHINAUI", L"HOSTWND");
        m_handle = ::FindWindowEx(temp_handle, NULL, L"WindowsForms10.Window.8.app.0.34f5582_r6_ad1", L"BlueStacks Android PluginAndroid");
    } break;
    case HandleType::BlueStacksView:
    case HandleType::BlueStacksWindow:
        m_handle = ::FindWindow(L"BS2CHINAUI", L"BlueStacks App Player");
        break;
    default:
        std::cerr << "handle type error! " << static_cast<int>(m_handle_type) << std::endl;
        break;
    }

#ifdef _DEBUG
    std::cout << "handle: " << m_handle << std::endl;
#endif

    if (m_handle != NULL) {
        return true;
    }
    else {
        return false;
    }
}

bool WinMacro::resizeWindow(int width, int height)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Window))) {
        return false;
    }

    return ::MoveWindow(m_handle, 0, 0, width, height, true);
}

bool WinMacro::click(Point p)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Control))) {
        return false;
    }

#ifdef _DEBUG
    std::cout << "click: " << p.x << ", " << p.y << std::endl;
#endif

	LPARAM lparam = MAKELPARAM(p.x, p.y);

	::SendMessage(m_handle, WM_LBUTTONDOWN, MK_LBUTTON, lparam);
	::SendMessage(m_handle, WM_LBUTTONUP, 0, lparam);

    return true;
}

bool WinMacro::clickRange(PointRange pr)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Control))) {
        return false;
    }

    int x = 0, y = 0;
    if (pr.right == 0) {
        x = pr.left;
    }
    else {
        std::poisson_distribution<int> x_rand(pr.right - pr.left);
        x = x_rand(m_rand_engine) + pr.left;

    }
    if (pr.bottom == 0) {
        y = pr.top;
    }
    else {
        std::poisson_distribution<int> y_rand(pr.bottom - pr.top);
        int y = y_rand(m_rand_engine) + pr.right;
    }

    return click({ x, y });
}