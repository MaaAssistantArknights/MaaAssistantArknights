#pragma once

#ifdef _WIN32

#include "MaaUtils/SafeWindows.hpp"
#include <Utils/Logger.hpp>
#include <cmath>

namespace asst
{
class WindowGuard
{
public:
    WindowGuard(void* hwnd);

    ~WindowGuard();

private:
    RECT m_clientRect = { 0, 0, 0, 0 };
    HWND m_hwnd = nullptr;
    LONG_PTR m_style = 0;
    bool m_restore = false;
};
}

#endif // _WIN32

