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

WinMacro::WinMacro(SimulatorType type) 
    : m_simulator_type(type),
    m_rand_engine(time(NULL))
{
    srand(time(NULL));
}

bool WinMacro::findHandle()
{
    switch (m_simulator_type) {
    case SimulatorType::BlueStacks: {
        m_view_handle = ::FindWindow(L"BS2CHINAUI", L"BlueStacks App Player");
        HWND temp_handle = ::FindWindowEx(m_view_handle, NULL, L"BS2CHINAUI", L"HOSTWND");
        m_control_handle = ::FindWindowEx(temp_handle, NULL, L"WindowsForms10.Window.8.app.0.34f5582_r6_ad1", L"BlueStacks Android PluginAndroid");
        //  ::FindWindowEx(m_view_handle, NULL, L"BlueStacksApp", L"_ctl.Window");

#ifdef _DEBUG
        std::cout << "view handle: " << m_view_handle << std::endl;
        std::cout << "control handle: " << m_control_handle << std::endl;
#endif

        if (m_view_handle != NULL && m_control_handle != NULL) {
            MoveWindow(m_view_handle, 0, 0, 1200, 720, true);
            return true;
        }
        else {
            return false;
        }
    } break;
    default:
        std::cerr << "simulator type error! " << static_cast<int>(m_simulator_type) << std::endl;
        break;
    }
}

void WinMacro::click(Point p)
{
#ifdef _DEBUG
    std::cout << "click: " << p.x << ", " << p.y << std::endl;
#endif

	LPARAM lparam = MAKELPARAM(p.x, p.y);

	::SendMessage(m_control_handle, WM_LBUTTONDOWN, MK_LBUTTON, lparam);
	::SendMessage(m_control_handle, WM_LBUTTONUP, 0, lparam);
}

void WinMacro::clickRange(PointRange pr)
{
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

    click({ x, y });
}