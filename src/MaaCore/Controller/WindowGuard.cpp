#ifdef _WIN32

#include "WindowGuard.h"

namespace asst
{
WindowGuard::WindowGuard(void* hwnd) :
    m_restore(false),
    m_hwnd(reinterpret_cast<HWND>(hwnd)),
    m_clientRect { }
{
    if (m_hwnd == nullptr || !IsWindow(m_hwnd)) {
        Log.error(__FUNCTION__, "invalid hwnd");
        return;
    }
    m_style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
    if ((m_style & WS_POPUP) != 0) {
        Log.error(__FUNCTION__, "Does not support full screen!");
        return;
    }
    ShowWindow(m_hwnd, SW_RESTORE);
    UpdateWindow(m_hwnd);
    if (!GetClientRect(m_hwnd, &m_clientRect)) {
        Log.error(__FUNCTION__, "GetClientRect failed!");
        return;
    }
    Log.info(
        __FUNCTION__,
        "clientRect, left: ",
        m_clientRect.left,
        ", right: ",
        m_clientRect.right,
        ", top: ",
        m_clientRect.top,
        ", bottom: ",
        m_clientRect.bottom);
    POINT centerPoint;
    centerPoint.x = (m_clientRect.left + m_clientRect.right) / 2;
    centerPoint.y = (m_clientRect.top + m_clientRect.bottom) / 2;
    if (static_cast<double>(m_clientRect.right - m_clientRect.left) /
                static_cast<double>(m_clientRect.bottom - m_clientRect.top) !=
            0 &&
        std::fabs(
            static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault) -
            static_cast<double>(m_clientRect.right - m_clientRect.left) /
                static_cast<double>(m_clientRect.bottom - m_clientRect.top)) > 1e-7) {
        Log.info(__FUNCTION__, "The resolution is not 16:9, try adjusting the window size");
        m_restore = true;
        LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
        BOOL hasMenu = (GetMenu(m_hwnd) != NULL);
        RECT rect = { 0, 0, WindowWidthDefault, WindowHeightDefault };
        AdjustWindowRectEx(&rect, (DWORD)m_style, hasMenu, (DWORD)exStyle);
        Log.info(
            __FUNCTION__,
            "AdjustWindow, left: ",
            rect.left,
            ", right: ",
            rect.right,
            ", top: ",
            rect.top,
            ", bottom: ",
            rect.bottom);
        SetWindowPos(
            m_hwnd,
            HWND_NOTOPMOST,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_SHOWWINDOW | SWP_FRAMECHANGED);

        if (GetClientRect(m_hwnd, &rect)) {
            centerPoint.x = (rect.left + rect.right) / 2;
            centerPoint.y = (rect.top + rect.bottom) / 2;
        }
    }
    // 将光标移动到窗口中央防止因UI偏斜导致后续找图失败
    if (ClientToScreen(m_hwnd, &centerPoint)) {
        SetCursorPos(centerPoint.x, centerPoint.y);
    }
}

WindowGuard::~WindowGuard()
{
    Log.info(__FUNCTION__, "WindowGuard destroy");
    if (m_restore) {
        Log.info(__FUNCTION__, "restore window");
        ShowWindow(m_hwnd, SW_RESTORE);
        UpdateWindow(m_hwnd);
        LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
        BOOL hasMenu = (GetMenu(m_hwnd) != NULL);
        RECT rect = { 0, 0, m_clientRect.right - m_clientRect.left, m_clientRect.bottom - m_clientRect.top };
        AdjustWindowRectEx(&rect, (DWORD)m_style, hasMenu, (DWORD)exStyle);
        SetWindowPos(
            m_hwnd,
            HWND_NOTOPMOST,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }
}
}

#endif // _WIN32
