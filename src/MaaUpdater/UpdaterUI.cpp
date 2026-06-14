#include "UpdaterUI.h"
#include "UpdaterLog.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr wchar_t PROGRESS_WINDOW_CLASS_NAME[] = L"MaaUpdaterProgressWindow";
static constexpr int PROGRESS_WINDOW_WIDTH = 540;
static constexpr int PROGRESS_WINDOW_HEIGHT = 190;
static constexpr int PROGRESS_STATUS_CONTROL_ID = 1001;
static constexpr int PROGRESS_DETAIL_CONTROL_ID = 1002;
static constexpr int PROGRESS_COUNT_CONTROL_ID = 1003;
static constexpr int PROGRESS_BAR_CONTROL_ID = 1004;
static constexpr DWORD LEGACY_DWMWA_USE_IMMERSIVE_DARK_MODE = 19;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

UpdateProgressUi g_progressUi;
ProgressUiTheme g_progressUiTheme;

// ---------------------------------------------------------------------------
// Theme detection
// ---------------------------------------------------------------------------

bool IsSystemDarkModeEnabled()
{
    DWORD value = 1;
    DWORD valueSize = sizeof(value);
    LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &valueSize);
    return status == ERROR_SUCCESS && value == 0;
}

void RefreshProgressUiTheme()
{
    g_progressUiTheme.isDarkMode = IsSystemDarkModeEnabled();

    if (g_progressUiTheme.isDarkMode) {
        g_progressUiTheme.backgroundColor = RGB(32, 32, 32);
        g_progressUiTheme.primaryTextColor = RGB(241, 241, 241);
        g_progressUiTheme.secondaryTextColor = RGB(200, 200, 200);
        g_progressUiTheme.progressTrackColor = RGB(58, 58, 58);
        g_progressUiTheme.progressBarColor = RGB(76, 194, 255);
    }
    else {
        g_progressUiTheme.backgroundColor = RGB(255, 255, 255);
        g_progressUiTheme.primaryTextColor = RGB(32, 32, 32);
        g_progressUiTheme.secondaryTextColor = RGB(96, 96, 96);
        g_progressUiTheme.progressTrackColor = RGB(232, 234, 237);
        g_progressUiTheme.progressBarColor = RGB(0, 120, 212);
    }

    if (g_progressUiTheme.backgroundBrush != nullptr) {
        DeleteObject(g_progressUiTheme.backgroundBrush);
        g_progressUiTheme.backgroundBrush = nullptr;
    }
    g_progressUiTheme.backgroundBrush = CreateSolidBrush(g_progressUiTheme.backgroundColor);

    if (!g_progressUi.enabled || g_progressUi.window == nullptr) {
        return;
    }

    BOOL useDarkMode = g_progressUiTheme.isDarkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(g_progressUi.window, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(g_progressUi.window, LEGACY_DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    if (g_progressUi.progressBar != nullptr) {
        SendMessageW(g_progressUi.progressBar, PBM_SETBKCOLOR, 0, g_progressUiTheme.progressTrackColor);
        SendMessageW(g_progressUi.progressBar, PBM_SETBARCOLOR, 0, g_progressUiTheme.progressBarColor);
    }

    RedrawWindow(g_progressUi.window, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------

static LRESULT CALLBACK UpdateProgressWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        RefreshProgressUiTheme();
        return 0;
    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        RECT clientRect {};
        GetClientRect(hwnd, &clientRect);
        FillRect(
            dc,
            &clientRect,
            g_progressUiTheme.backgroundBrush != nullptr ? g_progressUiTheme.backgroundBrush
                                                         : reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        COLORREF textColor = control == g_progressUi.detailLabel ? g_progressUiTheme.secondaryTextColor
                                                                 : g_progressUiTheme.primaryTextColor;
        SetTextColor(dc, textColor);
        SetBkColor(dc, g_progressUiTheme.backgroundColor);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(
            g_progressUiTheme.backgroundBrush != nullptr ? g_progressUiTheme.backgroundBrush
                                                         : reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    }
    case WM_CLOSE:
        return 0;
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

// ---------------------------------------------------------------------------
// Window class registration
// ---------------------------------------------------------------------------

static void ApplyDefaultWindowFont(HWND hwnd)
{
    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    if (font != nullptr) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

static bool EnsureProgressWindowClassRegistered()
{
    static bool isRegistered = false;
    if (isRegistered) {
        return true;
    }

    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = UpdateProgressWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = PROGRESS_WINDOW_CLASS_NAME;

    if (RegisterClassExW(&windowClass) == 0) {
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }

    isRegistered = true;
    return true;
}

// ---------------------------------------------------------------------------
// Progress UI lifecycle
// ---------------------------------------------------------------------------

bool ShouldShowProgressUi()
{
    return !g_writeConsoleLog && GetConsoleWindow() == nullptr;
}

void PumpProgressUiMessages()
{
    if (!g_progressUi.enabled || g_progressUi.window == nullptr) {
        return;
    }

    MSG message {};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

bool InitializeProgressUi()
{
    if (!ShouldShowProgressUi()) {
        return false;
    }

    if (!EnsureProgressWindowClassRegistered()) {
        return false;
    }

    RefreshProgressUiTheme();

    INITCOMMONCONTROLSEX controls {};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&controls);

    int originX = (GetSystemMetrics(SM_CXSCREEN) - PROGRESS_WINDOW_WIDTH) / 2;
    int originY = (GetSystemMetrics(SM_CYSCREEN) - PROGRESS_WINDOW_HEIGHT) / 2;

    HWND window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_APPWINDOW | WS_EX_DLGMODALFRAME,
        PROGRESS_WINDOW_CLASS_NAME,
        L"MAA 正在更新 | MAA Updating",
        WS_CAPTION,
        originX,
        originY,
        PROGRESS_WINDOW_WIDTH,
        PROGRESS_WINDOW_HEIGHT,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);
    if (window == nullptr) {
        return false;
    }

    HWND statusLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"正在准备更新... | Preparing update...",
        WS_CHILD | WS_VISIBLE,
        20,
        18,
        PROGRESS_WINDOW_WIDTH - 40,
        24,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(PROGRESS_STATUS_CONTROL_ID)),
        GetModuleHandleW(nullptr),
        nullptr);
    HWND detailLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"请稍候... | Please wait...",
        WS_CHILD | WS_VISIBLE,
        20,
        48,
        PROGRESS_WINDOW_WIDTH - 40,
        42,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(PROGRESS_DETAIL_CONTROL_ID)),
        GetModuleHandleW(nullptr),
        nullptr);
    HWND countLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"已处理文件 0/0 | Files processed 0/0",
        WS_CHILD | WS_VISIBLE,
        20,
        96,
        PROGRESS_WINDOW_WIDTH - 40,
        20,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(PROGRESS_COUNT_CONTROL_ID)),
        GetModuleHandleW(nullptr),
        nullptr);
    HWND progressBar = CreateWindowExW(
        0,
        PROGRESS_CLASSW,
        nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        20,
        124,
        PROGRESS_WINDOW_WIDTH - 40,
        20,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(PROGRESS_BAR_CONTROL_ID)),
        GetModuleHandleW(nullptr),
        nullptr);

    if (statusLabel == nullptr || detailLabel == nullptr || countLabel == nullptr || progressBar == nullptr) {
        DestroyWindow(window);
        return false;
    }

    ApplyDefaultWindowFont(statusLabel);
    ApplyDefaultWindowFont(detailLabel);
    ApplyDefaultWindowFont(countLabel);
    SendMessageW(progressBar, PBM_SETRANGE32, 0, 1);
    SendMessageW(progressBar, PBM_SETPOS, 0, 0);

    g_progressUi.enabled = true;
    g_progressUi.window = window;
    g_progressUi.statusLabel = statusLabel;
    g_progressUi.detailLabel = detailLabel;
    g_progressUi.countLabel = countLabel;
    g_progressUi.progressBar = progressBar;
    g_progressUi.processedFileCount = 0;
    g_progressUi.totalFileCount = 0;

    RefreshProgressUiTheme();
    RefreshProgressUiCountText();

    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);
    PumpProgressUiMessages();
    return true;
}

void DestroyProgressUi()
{
    if (g_progressUi.window != nullptr) {
        DestroyWindow(g_progressUi.window);
    }

    g_progressUi = {};

    if (g_progressUiTheme.backgroundBrush != nullptr) {
        DeleteObject(g_progressUiTheme.backgroundBrush);
        g_progressUiTheme.backgroundBrush = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Progress state helpers
// ---------------------------------------------------------------------------

std::wstring BuildProgressCountText(int processedFileCount, int totalFileCount)
{
    return L"已处理文件 " + std::to_wstring(processedFileCount) + L"/" + std::to_wstring(totalFileCount) +
           L" | Files processed " + std::to_wstring(processedFileCount) + L"/" + std::to_wstring(totalFileCount);
}

void RefreshProgressUiCountText()
{
    if (!g_progressUi.enabled || g_progressUi.countLabel == nullptr) {
        return;
    }

    std::wstring countText = BuildProgressCountText(g_progressUi.processedFileCount, g_progressUi.totalFileCount);
    SetWindowTextW(g_progressUi.countLabel, countText.c_str());
}

void SetProgressUiTotalFileCount(int totalFileCount)
{
    if (!g_progressUi.enabled || g_progressUi.progressBar == nullptr) {
        return;
    }

    g_progressUi.totalFileCount = totalFileCount > 0 ? totalFileCount : 0;
    if (g_progressUi.processedFileCount > g_progressUi.totalFileCount) {
        g_progressUi.processedFileCount = g_progressUi.totalFileCount;
    }

    int progressBarMaximum = g_progressUi.totalFileCount > 0 ? g_progressUi.totalFileCount : 1;
    SendMessageW(g_progressUi.progressBar, PBM_SETRANGE32, 0, progressBarMaximum);
    SendMessageW(g_progressUi.progressBar, PBM_SETPOS, g_progressUi.processedFileCount, 0);
    RefreshProgressUiCountText();
    PumpProgressUiMessages();
}

void SetProgressUiStatus(const std::wstring& status, const std::wstring& detail)
{
    if (!g_progressUi.enabled) {
        return;
    }

    if (g_progressUi.statusLabel != nullptr) {
        SetWindowTextW(g_progressUi.statusLabel, status.c_str());
    }
    if (g_progressUi.detailLabel != nullptr) {
        SetWindowTextW(g_progressUi.detailLabel, detail.c_str());
    }

    RefreshProgressUiCountText();
    PumpProgressUiMessages();
}

void AdvanceProgressUi(const std::wstring& status, const std::wstring& detail)
{
    if (!g_progressUi.enabled) {
        return;
    }

    if (g_progressUi.processedFileCount < g_progressUi.totalFileCount) {
        ++g_progressUi.processedFileCount;
    }

    SetProgressUiStatus(status, detail);
    if (g_progressUi.progressBar != nullptr) {
        SendMessageW(g_progressUi.progressBar, PBM_SETPOS, g_progressUi.processedFileCount, 0);
    }
    PumpProgressUiMessages();
}

void CompleteProgressUi(const std::wstring& status, const std::wstring& detail)
{
    if (!g_progressUi.enabled) {
        return;
    }

    g_progressUi.processedFileCount = g_progressUi.totalFileCount;
    SetProgressUiStatus(status, detail);
    if (g_progressUi.progressBar != nullptr) {
        int progressBarPosition = g_progressUi.totalFileCount > 0 ? g_progressUi.processedFileCount : 1;
        SendMessageW(g_progressUi.progressBar, PBM_SETPOS, progressBarPosition, 0);
    }
    PumpProgressUiMessages();
}

void ShowProgressUiFailure(const std::wstring& failureReason)
{
    if (!g_progressUi.enabled) {
        return;
    }

    SetProgressUiStatus(L"更新失败 | Update failed", failureReason);
    MessageBoxW(g_progressUi.window, failureReason.c_str(), L"MAA 更新失败 | MAA Update Failed", MB_OK | MB_ICONERROR);
}
