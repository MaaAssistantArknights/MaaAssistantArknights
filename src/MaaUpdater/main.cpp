// MAA.Updater.exe
// Applies a pending MAA update package after the main MAA process exits.
// Invoked by MaaWpfGui's PendingUpdateApplier; do NOT run manually.
//
// Usage:
//   MAA.Updater.exe <ParentProcessId> <RootDir> <ExtractDir> <BackupDir>
//                   <PackagePath> <SuccessStatusFile> <FailureStatusFile>
//                   <RelaunchExecutablePath> <PlanFile>
//                   [--mutex-name <name>] [--show-console]
//
// Plan file format (UTF-8 JSON):
//   { "packageType": "full|ota", "removeList": ["rel/path", ...], "moveList": ["rel/path", ...] }

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>

#include <cstdarg>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct PendingUpdatePlan;
struct UpdateProgressUi;

static bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8);
static HANDLE GetConsoleStreamHandle(FILE* stream);
static bool IsSystemDarkModeEnabled();
static bool ShouldShowProgressUi();
static bool InitializeProgressUi();
static void DestroyProgressUi();
static void PumpProgressUiMessages();
static void RefreshProgressUiTheme();
static std::wstring BuildProgressCountText(int processedFileCount, int totalFileCount);
static void RefreshProgressUiCountText();
static void SetProgressUiTotalFileCount(int totalFileCount);
static void SetProgressUiStatus(const std::wstring& status, const std::wstring& detail);
static void AdvanceProgressUi(const std::wstring& status, const std::wstring& detail);
static void CompleteProgressUi(const std::wstring& status, const std::wstring& detail);
static void ShowProgressUiFailure(const std::wstring& failureReason);
static void WriteLog(const wchar_t* message);
static void WriteLogF(const wchar_t* fmt, ...);
static void WriteLogEntries(const std::wstring& title, const std::vector<std::wstring>& entries);
static void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline);
static void RotateLogIfNeeded();
static bool HasArgument(int argc, wchar_t* argv[], const wchar_t* argument);

static std::wstring EnsureTrailingSeparator(const std::wstring& path);
static std::wstring NormalizeRelativePath(const std::wstring& relativePath);
static bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right);
static bool IsDriveRootDirectory(const std::wstring& path);
static bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath);
static bool TryResolvePathUnderRoot(
    const std::wstring& rootPath,
    const std::wstring& relativePath,
    std::wstring& out);

static bool PathExistsW(const std::wstring& path);
static bool IsDirectory(const std::wstring& path);
static bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir);
static bool MovePathToRecycleBin(const std::wstring& path);
static bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath);
static void EnsureParentDirectory(const std::wstring& path);
static std::wstring CreateArchivedPath(const std::wstring& base);
static bool MovePathEntry(const std::wstring& src, const std::wstring& dst);
static void PrepareBackupDestination(const std::wstring& backupPath);
static bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup);
static bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath);
static bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath);

static bool WriteUtf8File(const std::wstring& path, const char* content);
static bool WriteUtf8File(const std::wstring& path, const std::string& content);

static std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode);
static bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason);
static std::wstring Utf8ToWide(const std::string& s);
static std::string ParseJsonString(const std::string& json, size_t& pos);
static size_t SkipJsonWhitespace(const std::string& json, size_t pos);
static size_t SkipJsonValue(const std::string& json, size_t pos);
static size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key);
static std::vector<std::wstring> ParseJsonStringArray(const std::string& json, const char* key);
static std::wstring ParseJsonStringProperty(const std::string& json, const char* key);
static bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson = nullptr);
static void PrintPlanEntries(const std::wstring& title, const std::vector<std::wstring>& entries);
static int RunPlanParserTest(const std::wstring& initialPlanFile);
static HANDLE AcquireUpdateMutex(const std::wstring& mutexName);
static void ReleaseUpdateMutex(HANDLE hMutex);

// Retry and force-delete helpers
static bool RetryFileOp(const std::function<bool()>& op, int maxAttempts = 5, DWORD initialDelayMs = 200);
static std::wstring RenameLockedFile(const std::wstring& path);
static bool ForceDeleteFile(const std::wstring& path);
static bool ForceRemoveDirectoryRecursive(const std::wstring& dir);
static bool InstallFileAtomic(const std::wstring& sourcePath, const std::wstring& targetPath);
static void CleanupPendingDeleteFiles(const std::wstring& rootDir);

struct UpdateProgressUi
{
    bool enabled = false;
    HWND window = nullptr;
    HWND statusLabel = nullptr;
    HWND detailLabel = nullptr;
    HWND countLabel = nullptr;
    HWND progressBar = nullptr;
    int processedFileCount = 0;
    int totalFileCount = 0;
};

struct ProgressUiTheme
{
    bool isDarkMode = false;
    COLORREF backgroundColor = RGB(255, 255, 255);
    COLORREF primaryTextColor = RGB(32, 32, 32);
    COLORREF secondaryTextColor = RGB(96, 96, 96);
    COLORREF progressTrackColor = RGB(232, 234, 237);
    COLORREF progressBarColor = RGB(0, 120, 212);
    HBRUSH backgroundBrush = nullptr;
};

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

static std::wstring g_logFile;
static bool g_writeConsoleLog = false;
static UpdateProgressUi g_progressUi;
static ProgressUiTheme g_progressUiTheme;

static constexpr wchar_t PROGRESS_WINDOW_CLASS_NAME[] = L"MaaUpdaterProgressWindow";
static constexpr int PROGRESS_WINDOW_WIDTH = 540;
static constexpr int PROGRESS_WINDOW_HEIGHT = 190;
static constexpr int PROGRESS_STATUS_CONTROL_ID = 1001;
static constexpr int PROGRESS_DETAIL_CONTROL_ID = 1002;
static constexpr int PROGRESS_COUNT_CONTROL_ID = 1003;
static constexpr int PROGRESS_BAR_CONTROL_ID = 1004;
static constexpr DWORD LEGACY_DWMWA_USE_IMMERSIVE_DARK_MODE = 19;
static constexpr wchar_t MUTEX_NAME_ARG[] = L"--mutex-name";
static constexpr DWORD UPDATE_MUTEX_TIMEOUT_MS = 3000;
#define PENDING_DELETE_SUFFIX L".pendingdelete"
static constexpr int FILE_OP_MAX_RETRIES = 5;
static constexpr DWORD FILE_OP_INITIAL_DELAY_MS = 200;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8)
{
    int utf8Len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (utf8Len <= 0) {
        utf8.clear();
        return false;
    }

    utf8.assign(static_cast<size_t>(utf8Len), '\0');
    return WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Len,
        nullptr,
        nullptr) == utf8Len;
}

static HANDLE GetConsoleStreamHandle(FILE* stream)
{
    DWORD stdHandle = stream == stderr
        ? STD_ERROR_HANDLE
        : stream == stdout
            ? STD_OUTPUT_HANDLE
            : static_cast<DWORD>(-1);
    if (stdHandle == static_cast<DWORD>(-1)) {
        return nullptr;
    }

    HANDLE handle = GetStdHandle(stdHandle);
    return handle == INVALID_HANDLE_VALUE ? nullptr : handle;
}

static bool IsSystemDarkModeEnabled()
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

static void RefreshProgressUiTheme()
{
    g_progressUiTheme.isDarkMode = IsSystemDarkModeEnabled();

    if (g_progressUiTheme.isDarkMode) {
        g_progressUiTheme.backgroundColor = RGB(32, 32, 32);
        g_progressUiTheme.primaryTextColor = RGB(241, 241, 241);
        g_progressUiTheme.secondaryTextColor = RGB(200, 200, 200);
        g_progressUiTheme.progressTrackColor = RGB(58, 58, 58);
        g_progressUiTheme.progressBarColor = RGB(76, 194, 255);
    } else {
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
    DwmSetWindowAttribute(
        g_progressUi.window,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &useDarkMode,
        sizeof(useDarkMode));
    DwmSetWindowAttribute(
        g_progressUi.window,
        LEGACY_DWMWA_USE_IMMERSIVE_DARK_MODE,
        &useDarkMode,
        sizeof(useDarkMode));

    if (g_progressUi.progressBar != nullptr) {
        SendMessageW(g_progressUi.progressBar, PBM_SETBKCOLOR, 0, g_progressUiTheme.progressTrackColor);
        SendMessageW(g_progressUi.progressBar, PBM_SETBARCOLOR, 0, g_progressUiTheme.progressBarColor);
    }

    RedrawWindow(
        g_progressUi.window,
        nullptr,
        nullptr,
        RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

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
            g_progressUiTheme.backgroundBrush != nullptr
                ? g_progressUiTheme.backgroundBrush
                : reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        COLORREF textColor = control == g_progressUi.detailLabel
            ? g_progressUiTheme.secondaryTextColor
            : g_progressUiTheme.primaryTextColor;
        SetTextColor(dc, textColor);
        SetBkColor(dc, g_progressUiTheme.backgroundColor);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(
            g_progressUiTheme.backgroundBrush != nullptr
                ? g_progressUiTheme.backgroundBrush
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

static bool ShouldShowProgressUi()
{
    return !g_writeConsoleLog && GetConsoleWindow() == nullptr;
}

static void PumpProgressUiMessages()
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

static bool InitializeProgressUi()
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

static void DestroyProgressUi()
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

static std::wstring BuildProgressCountText(int processedFileCount, int totalFileCount)
{
        return L"已处理文件 " + std::to_wstring(processedFileCount) + L"/" + std::to_wstring(totalFileCount) +
            L" | Files processed " + std::to_wstring(processedFileCount) + L"/" + std::to_wstring(totalFileCount);
}

static void RefreshProgressUiCountText()
{
    if (!g_progressUi.enabled || g_progressUi.countLabel == nullptr) {
        return;
    }

    std::wstring countText = BuildProgressCountText(g_progressUi.processedFileCount, g_progressUi.totalFileCount);
    SetWindowTextW(g_progressUi.countLabel, countText.c_str());
}

static void SetProgressUiTotalFileCount(int totalFileCount)
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

static void SetProgressUiStatus(const std::wstring& status, const std::wstring& detail)
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

static void AdvanceProgressUi(const std::wstring& status, const std::wstring& detail)
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

static void CompleteProgressUi(const std::wstring& status, const std::wstring& detail)
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

static void ShowProgressUiFailure(const std::wstring& failureReason)
{
    if (!g_progressUi.enabled) {
        return;
    }

    SetProgressUiStatus(L"更新失败 | Update failed", failureReason);
    MessageBoxW(
        g_progressUi.window,
        failureReason.c_str(),
        L"MAA 更新失败 | MAA Update Failed",
        MB_OK | MB_ICONERROR);
}

static void WriteLog(const wchar_t* message)
{
    SYSTEMTIME st {};
    GetLocalTime(&st);

    wchar_t buf[64];
    _snwprintf_s(buf, _countof(buf), _TRUNCATE,
                 L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                 st.wYear, st.wMonth, st.wDay,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    std::wstring timestampedLine = buf;
    timestampedLine += message;

    if (g_writeConsoleLog) {
        WriteConsoleText(stdout, timestampedLine, true);
    }

    if (g_logFile.empty()) return;

    std::wstring line = timestampedLine;
    line += L"\r\n";

    // Ensure parent directory exists
    size_t sep = g_logFile.rfind(L'\\');
    if (sep != std::wstring::npos) {
        std::wstring dir = g_logFile.substr(0, sep);
        CreateDirectoryW(dir.c_str(), nullptr);
    }

    HANDLE hFile = CreateFileW(
        g_logFile.c_str(),
        FILE_APPEND_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Write UTF-8
    std::string utf8;
    if (TryConvertWideToUtf8(line, utf8)) {
        DWORD written = 0;
        WriteFile(hFile, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    CloseHandle(hFile);
}

static void WriteLogF(const wchar_t* fmt, ...)
{
    wchar_t buf[2048];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    WriteLog(buf);
}

static void WriteLogEntries(const std::wstring& title, const std::vector<std::wstring>& entries)
{
    WriteLogF(L"%s (%zu)", title.c_str(), entries.size());
    for (const std::wstring& entry : entries) {
        std::wstring line = L"  - " + entry;
        WriteLog(line.c_str());
    }
}

static void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline)
{
    std::wstring outputText = text;
    if (appendNewline) {
        outputText += L"\n";
    }

    HANDLE consoleHandle = GetConsoleStreamHandle(stream);
    DWORD consoleMode = 0;
    if (consoleHandle != nullptr && GetConsoleMode(consoleHandle, &consoleMode)) {
        DWORD written = 0;
        WriteConsoleW(
            consoleHandle,
            outputText.c_str(),
            static_cast<DWORD>(outputText.size()),
            &written,
            nullptr);
        return;
    }

    std::string utf8;
    if (TryConvertWideToUtf8(outputText, utf8)) {
        fwrite(utf8.data(), 1, utf8.size(), stream);
    }
    fflush(stream);
}

static constexpr LONGLONG MAX_UPDATER_LOG_SIZE = 4LL * 1024 * 1024;

static void RotateLogIfNeeded()
{
    if (g_logFile.empty() || !PathExistsW(g_logFile)) {
        return;
    }

    HANDLE hFile = CreateFileW(
        g_logFile.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER size {};
    bool shouldRotate = GetFileSizeEx(hFile, &size) && size.QuadPart > MAX_UPDATER_LOG_SIZE;
    CloseHandle(hFile);

    if (!shouldRotate) {
        return;
    }

    std::wstring bakFile = g_logFile + L".bak";
    DeleteFileW(bakFile.c_str());
    MoveFileExW(g_logFile.c_str(), bakFile.c_str(), MOVEFILE_REPLACE_EXISTING);
}

static bool HasArgument(int argc, wchar_t* argv[], const wchar_t* argument)
{
    for (int index = 1; index < argc; ++index) {
        if (_wcsicmp(argv[index], argument) == 0) {
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Path utilities
// ---------------------------------------------------------------------------

static std::wstring EnsureTrailingSeparator(const std::wstring& path)
{
    if (!path.empty() && path.back() != L'\\')
        return path + L'\\';
    return path;
}

static std::wstring NormalizeRelativePath(const std::wstring& relativePath)
{
    std::wstring normalized = relativePath;
    for (wchar_t& c : normalized) {
        if (c == L'/') {
            c = L'\\';
        }
    }

    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L' ' || normalized.back() == L'\t')) {
        normalized.pop_back();
    }

    return normalized;
}

static bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right)
{
    return _wcsicmp(left.c_str(), right) == 0;
}

static bool IsDriveRootDirectory(const std::wstring& path)
{
    if (path.empty()) {
        return false;
    }

    wchar_t full[MAX_PATH * 4];
    DWORD len = GetFullPathNameW(path.c_str(), _countof(full), full, nullptr);
    if (len == 0 || len >= _countof(full)) {
        return false;
    }

    std::wstring normalized = full;
    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }

    if (normalized.size() < 2) {
        return false;
    }

    wchar_t drive = normalized[0];
    bool hasDriveLetter = (drive >= L'A' && drive <= L'Z') || (drive >= L'a' && drive <= L'z');
    return hasDriveLetter && normalized.size() == 2 && normalized[1] == L':';
}

static bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath)
{
    std::wstring normalized = NormalizeRelativePath(relativePath);
    return EqualsIgnoreCase(normalized, L"resource") || EqualsIgnoreCase(normalized, L"externals");
}

// Resolves `relativePath` under `rootPath`, writes result to `out`.
// Returns false if the path is rooted, empty, or escapes the root.
static bool TryResolvePathUnderRoot(
    const std::wstring& rootPath,
    const std::wstring& relativePath,
    std::wstring& out)
{
    out.clear();

    // Reject empty or whitespace
    if (relativePath.empty()) return false;
    bool allSpace = true;
    for (wchar_t c : relativePath)
        if (c != L' ' && c != L'\t') { allSpace = false; break; }
    if (allSpace) return false;

    // Normalise slashes
    std::wstring rel = relativePath;
    for (wchar_t& c : rel)
        if (c == L'/') c = L'\\';

    // Reject absolute paths
    if (rel.size() >= 2 && rel[1] == L':') return false;
    if (!rel.empty() && rel[0] == L'\\') return false;

    // Build candidate and canonicalise
    std::wstring combined = rootPath + L'\\' + rel;
    wchar_t full[MAX_PATH * 4];
    DWORD len = GetFullPathNameW(combined.c_str(), _countof(full), full, nullptr);
    if (len == 0 || len >= _countof(full)) return false;

    std::wstring candidate = full;

    // Root must be canonical too
    DWORD rootLen = GetFullPathNameW(rootPath.c_str(), _countof(full), full, nullptr);
    if (rootLen == 0 || rootLen >= _countof(full)) return false;
    std::wstring normalRoot = EnsureTrailingSeparator(std::wstring(full));

    // Case-insensitive prefix check
    if (candidate.size() < normalRoot.size()) return false;
    if (_wcsnicmp(candidate.c_str(), normalRoot.c_str(), normalRoot.size()) != 0) return false;

    out = candidate;
    return true;
}

// ---------------------------------------------------------------------------
// File / directory helpers
// ---------------------------------------------------------------------------

static bool PathExistsW(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool IsDirectory(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir)
{
    DWORD attr = GetFileAttributesW(sourceDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    CreateDirectoryW(destinationDir.c_str(), nullptr);

    std::wstring pattern = sourceDir + L"\\*";
    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return true;
    }

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring sourceChild = sourceDir + L"\\" + fd.cFileName;
        std::wstring destinationChild = destinationDir + L"\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CopyDirectoryRecursive(sourceChild, destinationChild)) {
                FindClose(hFind);
                return false;
            }
            continue;
        }

        EnsureParentDirectory(destinationChild);
        if (!CopyFileW(sourceChild.c_str(), destinationChild.c_str(), FALSE)) {
            FindClose(hFind);
            return false;
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return true;
}

static bool MovePathToRecycleBin(const std::wstring& path)
{
    std::wstring doubleNullPath = path;
    doubleNullPath.push_back(L'\0');

    SHFILEOPSTRUCTW op {};
    op.wFunc = FO_DELETE;
    op.pFrom = doubleNullPath.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&op);
    return result == 0 && !op.fAnyOperationsAborted;
}

static bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath)
{
    DWORD attr = GetFileAttributesW(sourcePath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return CopyDirectoryRecursive(sourcePath, destinationPath);
    }

    EnsureParentDirectory(destinationPath);
    return CopyFileW(sourcePath.c_str(), destinationPath.c_str(), FALSE) != FALSE;
}

static void EnsureParentDirectory(const std::wstring& path)
{
    size_t sep = path.rfind(L'\\');
    if (sep == std::wstring::npos) return;
    std::wstring parent = path.substr(0, sep);
    if (parent.empty()) return;

    // Recursively create
    EnsureParentDirectory(parent);
    CreateDirectoryW(parent.c_str(), nullptr);
}

static std::wstring CreateArchivedPath(const std::wstring& base)
{
    SYSTEMTIME st {};
    GetLocalTime(&st);
    wchar_t ts[32];
    _snwprintf_s(ts, _countof(ts), _TRUNCATE,
                 L".%04d%02d%02d%02d%02d%02d.",
                 st.wYear, st.wMonth, st.wDay,
                 st.wHour, st.wMinute, st.wSecond);

    int index = 0;
    while (true) {
        wchar_t idx[16];
        _itow_s(index, idx, _countof(idx), 10);
        std::wstring candidate = base + ts + idx;
        if (!PathExistsW(candidate)) return candidate;
        index++;
    }
}

// Move a file or directory entry (handles cross-volume by CopyFile+Delete for files).
// Uses retry and rename fallback for locked files.
static bool MovePathEntry(const std::wstring& src, const std::wstring& dst)
{
    EnsureParentDirectory(dst);

    DWORD attr = GetFileAttributesW(src.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        auto moveOp = [&]() -> bool {
            return MoveFileExW(src.c_str(), dst.c_str(), 0) != FALSE;
        };
        return RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);
    }

    // For files: try atomic move first; fall back to copy+delete
    {
        auto moveOp = [&]() -> bool {
            return MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
        };

        if (RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
            return true;
        }
    }

    // Move failed (likely the source file is locked). Try copy+force-delete.
    WriteLogF(L"MovePathEntry: move failed, falling back to copy+delete for: %ls", src.c_str());
    {
        auto copyOp = [&]() -> bool {
            return CopyFileW(src.c_str(), dst.c_str(), FALSE) != FALSE;
        };

        if (RetryFileOp(copyOp, 3, 500)) {
            // Copy succeeded; force-delete the source
            ForceDeleteFile(src);
            return true;
        }
    }

    // Copy also failed. As a last resort, try to rename the source out of the way
    // and then do a fresh copy to the destination.
    WriteLogF(L"MovePathEntry: copy+delete failed, trying rename-and-copy for: %ls", src.c_str());
    {
        std::wstring renamed = RenameLockedFile(src);
        if (!renamed.empty()) {
            // The original path is now free; copy the renamed file to destination
            auto copyOp = [&]() -> bool {
                return CopyFileW(renamed.c_str(), dst.c_str(), FALSE) != FALSE;
            };

            if (RetryFileOp(copyOp, 3, 500)) {
                WriteLogF(L"MovePathEntry: rename-and-copy succeeded: %ls", src.c_str());
                return true;
            }
        }
    }

    WriteLogF(L"MovePathEntry: all strategies failed for src=%ls, dst=%ls", src.c_str(), dst.c_str());
    return false;
}

static void PrepareBackupDestination(const std::wstring& backupPath)
{
    EnsureParentDirectory(backupPath);
    if (!PathExistsW(backupPath)) return;

    std::wstring archived = CreateArchivedPath(backupPath);
    MovePathEntry(backupPath, archived);
}

static bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup)
{
    PrepareBackupDestination(backup);

    // First try: normal move with retry
    if (MovePathEntry(src, backup)) {
        return true;
    }

    // Second try: copy to backup, then force-delete the source
    WriteLogF(L"MoveExistingPathToBackup: move failed, trying copy+delete for: %ls", src.c_str());
    {
        auto copyOp = [&]() -> bool {
            return CopyPathEntry(src, backup) != FALSE;
        };

        if (RetryFileOp(copyOp, 3, 500)) {
            // Copy succeeded; now remove the source
            ForceDeleteFile(src);
            return true;
        }
    }

    WriteLogF(L"MoveExistingPathToBackup: all strategies failed for: %ls", src.c_str());
    return false;
}

static bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    {
        auto copyOp = [&]() -> bool {
            return CopyDirectoryRecursive(sourcePath, backupPath) != FALSE;
        };
        if (!RetryFileOp(copyOp, 3, 500)) {
            return false;
        }
    }

    // Try recycle bin first, fall back to force-delete
    if (MovePathToRecycleBin(sourcePath)) {
        return true;
    }

    return ForceRemoveDirectoryRecursive(sourcePath);
}

static bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    {
        auto copyOp = [&]() -> bool {
            return CopyPathEntry(sourcePath, backupPath) != FALSE;
        };
        if (!RetryFileOp(copyOp, 3, 500)) {
            return false;
        }
    }

    // Try recycle bin first, fall back to force-delete
    if (MovePathToRecycleBin(sourcePath)) {
        return true;
    }

    return ForceDeleteFile(sourcePath);
}

// ---------------------------------------------------------------------------
// Write a small UTF-8 text file
// ---------------------------------------------------------------------------

static bool WriteUtf8File(const std::wstring& path, const char* content)
{
    return WriteUtf8File(path, std::string(content));
}

static bool WriteUtf8File(const std::wstring& path, const std::string& content)
{
    EnsureParentDirectory(path);
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    DWORD len = static_cast<DWORD>(content.size());
    bool ok = WriteFile(hFile, content.data(), len, &written, nullptr) != FALSE;
    CloseHandle(hFile);
    return ok && written == len;
}

// ---------------------------------------------------------------------------
// Minimal JSON parser for the plan file
// { "packageType": "full|ota", "removeList": [ "...", ... ], "moveList": [ "...", ... ] }
// ---------------------------------------------------------------------------

static std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode)
{
    return std::wstring(action) + L": " + path + L" (error=" + std::to_wstring(errorCode) + L")";
}

static bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason)
{
    content.clear();
    failureReason.clear();

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        failureReason = BuildFileIoFailureReason(L"Failed to open file", path, GetLastError());
        return false;
    }

    std::string buf;
    char chunk[4096];
    while (true) {
        DWORD chunkRead = 0;
        if (!ReadFile(hFile, chunk, static_cast<DWORD>(sizeof(chunk)), &chunkRead, nullptr)) {
            failureReason = BuildFileIoFailureReason(L"Failed to read file", path, GetLastError());
            CloseHandle(hFile);
            return false;
        }

        if (chunkRead == 0) {
            break;
        }

        buf.append(chunk, chunkRead);
    }

    CloseHandle(hFile);
    content.swap(buf);
    return true;
}

// Convert a UTF-8 JSON string value to std::wstring.
// Handles basic \uXXXX, \n, \r, \t, \\, \/
static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 1) return {};
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

// Parse a JSON string literal starting at pos (after the opening quote).
// Advances pos past the closing quote. Returns the raw UTF-8 content.
static std::string ParseJsonString(const std::string& json, size_t& pos)
{
    std::string result;
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '"') break;
        if (c == '\\' && pos < json.size()) {
            char esc = json[pos++];
            switch (esc) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case '/':  result += '/';  break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case 'u': {
                if (pos + 4 <= json.size()) {
                    char hex[5] = {};
                    memcpy(hex, json.c_str() + pos, 4);
                    pos += 4;
                    unsigned cp = static_cast<unsigned>(strtoul(hex, nullptr, 16));
                    // Encode code point as UTF-8
                    if (cp < 0x80) {
                        result += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        result += static_cast<char>(0xC0 | (cp >> 6));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (cp >> 12));
                        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                }
                break;
            }
            default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    return result;
}

static size_t SkipJsonWhitespace(const std::string& json, size_t pos)
{
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\r' || json[pos] == '\n'))
    {
        ++pos;
    }
    return pos;
}

// Skips one JSON value starting at `pos` and returns the position right after it.
static size_t SkipJsonValue(const std::string& json, size_t pos)
{
    pos = SkipJsonWhitespace(json, pos);
    if (pos >= json.size()) return pos;

    char c = json[pos];
    if (c == '"') {
        ++pos;
        ParseJsonString(json, pos);
        return pos;
    }

    if (c == '{' || c == '[') {
        char open = c;
        char close = (c == '{') ? '}' : ']';
        int depth = 0;
        bool inString = false;
        bool escaped = false;

        for (; pos < json.size(); ++pos) {
            char ch = json[pos];
            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (ch == '\\') {
                    escaped = true;
                } else if (ch == '"') {
                    inString = false;
                }
                continue;
            }

            if (ch == '"') {
                inString = true;
                continue;
            }

            if (ch == open) {
                ++depth;
            } else if (ch == close) {
                --depth;
                if (depth == 0) {
                    ++pos;
                    break;
                }
            }
        }

        return pos;
    }

    // number / true / false / null
    while (pos < json.size()) {
        char ch = json[pos];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        ++pos;
    }
    return pos;
}

// Finds `"key": <value>` in the top-level JSON object and returns the start of `<value>`.
static size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key)
{
    size_t pos = SkipJsonWhitespace(json, 0);
    if (pos >= json.size() || json[pos] != '{') return std::string::npos;
    ++pos;

    while (pos < json.size()) {
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (json[pos] == ',') {
            ++pos;
            continue;
        }

        if (json[pos] == '}') {
            return std::string::npos;
        }

        if (json[pos] != '"') {
            ++pos;
            continue;
        }

        ++pos;
        std::string currentKey = ParseJsonString(json, pos);

        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            return std::string::npos;
        }

        ++pos;
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (currentKey == key) {
            return pos;
        }

        pos = SkipJsonValue(json, pos);
    }

    return std::string::npos;
}

// Find a JSON array by key name and return its string elements.
static std::vector<std::wstring> ParseJsonStringArray(
    const std::string& json,
    const char* key)
{
    std::vector<std::wstring> result;

    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return result;

    if (pos >= json.size() || json[pos] != '[') return result;
    pos++; // consume '['

    // Parse array elements
    while (pos < json.size()) {
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                      json[pos] == '\r' || json[pos] == '\n' ||
                                      json[pos] == ','))
            pos++;

        if (pos >= json.size()) break;
        if (json[pos] == ']') break;
        if (json[pos] == '"') {
            pos++; // consume opening quote
            std::string val = ParseJsonString(json, pos);
            result.push_back(Utf8ToWide(val));
        } else {
            pos++;
        }
    }

    return result;
}

static std::wstring ParseJsonStringProperty(const std::string& json, const char* key)
{
    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return {};

    if (pos >= json.size() || json[pos] != '"') return {};

    pos++;
    return Utf8ToWide(ParseJsonString(json, pos));
}

struct PendingUpdatePlan
{
    std::wstring packageType;
    std::vector<std::wstring> removeList;
    std::vector<std::wstring> moveList;
};

static bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson)
{
    outPlan = {};
    failureReason.clear();

    if (!PathExistsW(planFile)) {
        failureReason = L"Plan file not found: " + planFile;
        return false;
    }

    std::string planJson;
    if (!TryReadUtf8File(planFile, planJson, failureReason)) {
        return false;
    }

    if (rawJson != nullptr) {
        *rawJson = planJson;
    }

    if (planJson.empty()) {
        failureReason = L"Plan file is empty: " + planFile;
        return false;
    }

    outPlan.packageType = ParseJsonStringProperty(planJson, "packageType");
    outPlan.removeList = ParseJsonStringArray(planJson, "removeList");
    outPlan.moveList = ParseJsonStringArray(planJson, "moveList");
    return true;
}

static void PrintPlanEntries(const std::wstring& title, const std::vector<std::wstring>& entries)
{
    WriteConsoleText(stdout, title + L" (" + std::to_wstring(entries.size()) + L")", true);
    for (const std::wstring& entry : entries) {
        WriteConsoleText(stdout, L"  - " + entry, true);
    }
}

static int RunPlanParserTest(const std::wstring& initialPlanFile)
{
    std::wstring planFile = initialPlanFile;
    if (planFile.empty()) {
        WriteConsoleText(stdout, L"请输入要解析的 plan 文件路径 | Enter the plan file path to parse: ", false);
        std::getline(std::wcin, planFile);
    }

    if (planFile.empty()) {
        WriteConsoleText(stderr, L"未提供 plan 文件路径。 | No plan file path was provided.", true);
        return 1;
    }

    PendingUpdatePlan plan;
    std::wstring failureReason;
    std::string rawJson;
    if (!LoadPendingUpdatePlan(planFile, plan, failureReason, &rawJson)) {
        WriteConsoleText(stderr, failureReason, true);
        return 2;
    }

    WriteConsoleText(stdout, L"文件读取成功: " + planFile + L" | Plan file loaded successfully: " + planFile, true);
    WriteConsoleText(stdout, L"原始字节数: " + std::to_wstring(rawJson.size()) + L" | Raw byte count: " + std::to_wstring(rawJson.size()), true);
    WriteConsoleText(stdout, L"包类型: " + (plan.packageType.empty() ? std::wstring(L"<空>") : plan.packageType) +
        L" | Package type: " + (plan.packageType.empty() ? std::wstring(L"<empty>") : plan.packageType), true);
    PrintPlanEntries(L"待删除文件列表 | Files to remove", plan.removeList);
    PrintPlanEntries(L"待安装文件列表 | Files to install", plan.moveList);
    return 0;
}

// ---------------------------------------------------------------------------
// Update mutex helpers
// ---------------------------------------------------------------------------

// Acquires a named system mutex to prevent new MAA instances from starting
// while the update is in progress. Returns nullptr if the mutex cannot be
// acquired (e.g. another MAA instance holds the lock).
static HANDLE AcquireUpdateMutex(const std::wstring& mutexName)
{
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());
    if (hMutex == nullptr) {
        WriteLog((L"CreateMutexW failed, error=" + std::to_wstring(GetLastError())).c_str());
        return nullptr;
    }

    DWORD waitResult = WaitForSingleObject(hMutex, UPDATE_MUTEX_TIMEOUT_MS);

    if (waitResult == WAIT_OBJECT_0) {
        WriteLog((L"Mutex acquired: " + mutexName).c_str());
        return hMutex;
    }

    if (waitResult == WAIT_ABANDONED) {
        // Previous MAA instance terminated abnormally; we now own the mutex.
        WriteLog((L"Mutex acquired after WAIT_ABANDONED (previous instance crashed): " + mutexName).c_str());
        return hMutex;
    }

    if (waitResult == WAIT_TIMEOUT) {
        WriteLog((L"Mutex acquisition timed out after " + std::to_wstring(UPDATE_MUTEX_TIMEOUT_MS) + L"ms: " + mutexName).c_str());
    } else {
        WriteLog((L"WaitForSingleObject failed, error=" + std::to_wstring(GetLastError())).c_str());
    }

    CloseHandle(hMutex);
    return nullptr;
}

static void ReleaseUpdateMutex(HANDLE hMutex)
{
    if (hMutex == nullptr || hMutex == INVALID_HANDLE_VALUE) {
        return;
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}

// ---------------------------------------------------------------------------
// Retry and force-delete helpers
// ---------------------------------------------------------------------------

// Retries a file operation with exponential backoff on lock-related errors.
// Returns true if the operation eventually succeeded.
// Does NOT retry on non-lock errors (ERROR_FILE_NOT_FOUND, etc.).
static bool RetryFileOp(const std::function<bool()>& op, int maxAttempts, DWORD initialDelayMs)
{
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (op()) {
            return true;
        }

        DWORD error = GetLastError();
        if (error != ERROR_SHARING_VIOLATION && error != ERROR_LOCK_VIOLATION &&
            error != ERROR_ACCESS_DENIED) {
            // Not a transient locking error — no point retrying.
            return false;
        }

        if (attempt < maxAttempts) {
            DWORD delay = initialDelayMs * static_cast<DWORD>(1 << (attempt - 1));
            WriteLogF(L"RetryFileOp: attempt %d/%d failed (error=%u), retrying in %u ms",
                      attempt, maxAttempts, error, delay);
            Sleep(delay);
        }
    }

    return false;
}

// Renames a locked file to a unique name with .pendingdelete suffix.
// This works on NTFS even when the file is open with FILE_SHARE_DELETE.
// Returns the new path on success, or an empty string on failure.
static std::wstring RenameLockedFile(const std::wstring& path)
{
    // Generate a unique suffix using tick count + a counter
    static volatile LONG s_renameCounter = 0;
    DWORD tick = GetTickCount();
    LONG counter = InterlockedIncrement(&s_renameCounter);
    wchar_t suffix[64];
    _snwprintf_s(suffix, _countof(suffix), _TRUNCATE,
                 L".%08x%04x" PENDING_DELETE_SUFFIX, tick, static_cast<WORD>(counter & 0xFFFF));

    std::wstring newPath = path + suffix;

    // Try the rename; if it fails (e.g. path already exists), append more entropy
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (MoveFileExW(path.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE) {
            WriteLogF(L"Renamed locked file: %ls -> %ls", path.c_str(), newPath.c_str());
            return newPath;
        }

        // With MOVEFILE_REPLACE_EXISTING the only likely reason is sharing-violation
        // on the source. Give it a brief retry.
        Sleep(100);
        std::wstring retryPath = newPath + L"." + std::to_wstring(attempt);
        newPath = retryPath;
    }

    WriteLogF(L"Failed to rename locked file after 10 attempts: %ls", path.c_str());
    return {};
}

// Attempts to delete a file through multiple escalation layers:
//   1. Normal DeleteFileW + retry
//   2. Rename the file out of the way, then delete
//   3. Schedule for deletion on next reboot (MOVEFILE_DELAY_UNTIL_REBOOT)
// Returns true if the file is no longer present at the original path.
static bool ForceDeleteFile(const std::wstring& path)
{
    // --- Layer 1: Normal delete with retry ---
    auto deleteOp = [&]() -> bool {
        return DeleteFileW(path.c_str()) != FALSE;
    };

    if (RetryFileOp(deleteOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
        WriteLogF(L"Deleted file: %ls", path.c_str());
        return true;
    }

    if (!PathExistsW(path)) {
        return true; // already gone
    }

    // --- Layer 2: Rename then delete ---
    std::wstring renamed = RenameLockedFile(path);
    if (!renamed.empty()) {
        // The rename succeeded; now try to delete the renamed file
        RetryFileOp([&]() -> bool { return DeleteFileW(renamed.c_str()) != FALSE; },
                    3, 500);
        if (PathExistsW(renamed)) {
            WriteLogF(L"Renamed file could not be deleted immediately (will remain as .pendingdelete): %ls",
                      renamed.c_str());
        }
        // Even if we couldn't delete the renamed file, the original path is now free.
        WriteLogF(L"File vacated via rename: %ls -> %ls", path.c_str(), renamed.c_str());
        return true;
    }

    // --- Layer 3: Schedule for deletion on next reboot ---
    if (MoveFileExW(path.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT) != FALSE) {
        WriteLogF(L"Scheduled file for deletion on next reboot: %ls", path.c_str());
        return true;
    }

    WriteLogF(L"All deletion strategies failed for: %ls (error=%u)", path.c_str(), GetLastError());
    return false;
}

// Recursively removes a directory, using ForceDeleteFile for each file.
static bool ForceRemoveDirectoryRecursive(const std::wstring& dir)
{
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
                continue;
            std::wstring child = dir + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ForceRemoveDirectoryRecursive(child);
            } else {
                ForceDeleteFile(child);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    return RemoveDirectoryW(dir.c_str()) != FALSE || !PathExistsW(dir);
}

// Atomically installs a file from sourcePath to targetPath.
// Uses ReplaceFileW which handles locked target files better than
// a separate copy-then-delete cycle.
// If ReplaceFileW fails, falls back to copy-then-move with retry.
static bool InstallFileAtomic(const std::wstring& sourcePath,
                               const std::wstring& targetPath)
{
    EnsureParentDirectory(targetPath);

    // Generate a temporary path alongside the target
    DWORD tick = GetTickCount();
    std::wstring tempPath = targetPath + L"." + std::to_wstring(tick) + L".tmpinstall";

    // Copy source to the temp location first
    {
        DWORD attr = GetFileAttributesW(sourcePath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            WriteLogF(L"InstallFileAtomic: source not found: %ls", sourcePath.c_str());
            return false;
        }

        auto copyOp = [&]() -> bool {
            return CopyFileW(sourcePath.c_str(), tempPath.c_str(), FALSE) != FALSE;
        };

        if (!RetryFileOp(copyOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
            WriteLogF(L"InstallFileAtomic: failed to copy to temp: %ls (error=%u)",
                      tempPath.c_str(), GetLastError());
            // Clean up the temp file if it exists
            DeleteFileW(tempPath.c_str());
            return false;
        }
    }

    // --- Try ReplaceFileW (atomic swap) ---
    {
        auto replaceOp = [&]() -> bool {
            // ReplaceFileW(replaced, replacement, backup, flags, exclude, reserved)
            return ReplaceFileW(targetPath.c_str(),
                                tempPath.c_str(),
                                nullptr, // no additional backup needed
                                REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_WRITE_THROUGH,
                                nullptr, nullptr) != FALSE;
        };

        if (RetryFileOp(replaceOp, 3, 500)) {
            WriteLogF(L"InstallFileAtomic: ReplaceFileW succeeded: %ls", targetPath.c_str());
            return true;
        }
    }

    // --- ReplaceFileW failed; fall back to copy-then-rename ---
    WriteLogF(L"InstallFileAtomic: ReplaceFileW failed, falling back to MoveFileEx for: %ls",
              targetPath.c_str());

    // If the target exists and is locked, try to rename it out of the way
    if (PathExistsW(targetPath)) {
        std::wstring renamed = RenameLockedFile(targetPath);
        if (!renamed.empty()) {
            // Target path is now free; move temp into place
            if (MoveFileExW(tempPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE) {
                WriteLogF(L"InstallFileAtomic: installed after renaming locked target: %ls", targetPath.c_str());
                return true;
            }
        }
    }

    // Last resort: direct move with retry
    auto moveOp = [&]() -> bool {
        return MoveFileExW(tempPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
    };

    if (RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
        WriteLogF(L"InstallFileAtomic: move succeeded: %ls", targetPath.c_str());
        return true;
    }

    // Clean up temp file
    DeleteFileW(tempPath.c_str());
    WriteLogF(L"InstallFileAtomic: all strategies failed for: %ls", targetPath.c_str());
    return false;
}

// Scans rootDir and deletes any remaining .pendingdelete files left
// from a previous interrupted update.
static void CleanupPendingDeleteFiles(const std::wstring& rootDir)
{
    std::wstring pattern = rootDir + L"\\*" + PENDING_DELETE_SUFFIX;
    int cleanedCount = 0;

    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        std::wstring filePath = rootDir + L"\\" + fd.cFileName;
        auto deleteOp = [&]() -> bool {
            return DeleteFileW(filePath.c_str()) != FALSE;
        };

        if (RetryFileOp(deleteOp, 3, 500)) {
            ++cleanedCount;
        } else {
            WriteLogF(L"Could not clean up pending-delete file: %ls", filePath.c_str());
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    if (cleanedCount > 0) {
        WriteLogF(L"Cleaned up %d pending-delete file(s).", cleanedCount);
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int wmain(int argc, wchar_t* argv[])
{
    constexpr int REQUIRED_ARGS = 9; // excluding argv[0]

    if (argc >= 2 && wcscmp(argv[1], L"--test-plan") == 0) {
        return RunPlanParserTest(argc >= 3 ? argv[2] : L"");
    }

    if (argc - 1 < REQUIRED_ARGS) {
        MessageBoxW(
            nullptr,
            L"MAA.Updater.exe 是 MAA 内部使用的更新程序，不应被手动启动。\n\n"
            L"请直接运行 MAA.exe。\n\n"
            L"MAA.Updater.exe is an updater used internally by MAA and should not be manually started.\n\n"
            L"Please run MAA.exe directly.",
            L"MAA 更新程序 | MAA Updater",
            MB_OK | MB_ICONINFORMATION);
        return 1;
    }

    DWORD     parentPid              = static_cast<DWORD>(_wtoi(argv[1]));
    std::wstring rootDir             = argv[2];
    std::wstring extractDir          = argv[3];
    std::wstring backupDir           = argv[4];
    std::wstring packagePath         = argv[5];
    std::wstring successStatusFile   = argv[6];
    std::wstring failureStatusFile   = argv[7];
    std::wstring relaunchExecutable  = argv[8];
    std::wstring planFile            = argv[9];
    g_writeConsoleLog = HasArgument(argc, argv, L"--show-console");

    std::wstring mutexName;
    for (int i = 1; i < argc - 1; ++i) {
        if (_wcsicmp(argv[i], MUTEX_NAME_ARG) == 0) {
            mutexName = argv[i + 1];
            break;
        }
    }

    g_logFile = rootDir + L"\\debug\\pending-update-applier.log";
    RotateLogIfNeeded();
    InitializeProgressUi();
    SetProgressUiStatus(
        L"正在准备更新... | Preparing update...",
        L"等待 MAA 主程序退出 | Waiting for the main MAA process to exit");

    WriteLog(L"MAA.Updater started (C++ external updater).");
    WriteLog((std::wstring(L"Console output: ") + (g_writeConsoleLog ? L"enabled" : L"disabled")).c_str());
    WriteLog((L"Parent PID: " + std::to_wstring(parentPid) + L", root dir: " + rootDir).c_str());
    WriteLog((L"Plan file: " + planFile + L", extract dir: " + extractDir).c_str());

    bool shouldRelaunch = false;
    bool success = false;
    std::wstring failureReason;
    HANDLE hUpdateMutex = nullptr;

    // ------------------------------------------------------------------
    // Wait for parent process to exit
    // ------------------------------------------------------------------
    HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (hParent != nullptr) {
        WriteLog((L"Waiting for parent process to exit, PID=" + std::to_wstring(parentPid)).c_str());
        while (WaitForSingleObject(hParent, 100) == WAIT_TIMEOUT) {
            PumpProgressUiMessages();
        }
        CloseHandle(hParent);
        WriteLog(L"Parent process exited.");
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"已确认主程序退出，开始读取更新计划 | Parent process exited, reading update plan");
    } else {
        WriteLog((L"Could not open the parent process, it may have already exited, PID=" + std::to_wstring(parentPid) + L". Continuing.").c_str());
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"主程序已退出，开始读取更新计划 | Main process already exited, reading update plan");
    }

    // ------------------------------------------------------------------
    // Acquire update mutex to prevent new MAA instances from starting
    // ------------------------------------------------------------------
    if (!mutexName.empty()) {
        hUpdateMutex = AcquireUpdateMutex(mutexName);
        if (hUpdateMutex == nullptr) {
            failureReason =
                L"检测到另一个 MAA 实例正在运行，无法执行更新。请关闭所有 MAA 窗口后重试。\n\n"
                L"Another MAA instance is running. Please close all MAA windows and try again.";
            WriteLog(failureReason.c_str());
            SetProgressUiStatus(
                L"无法继续更新 | Update blocked",
                L"检测到另一个 MAA 实例正在运行 | Another MAA instance is running");
        } else {
            WriteLog(L"Update mutex acquired, preventing new MAA instances from starting.");
        }

        // Clean up any .pendingdelete files left from a previous interrupted update
        CleanupPendingDeleteFiles(rootDir);
    } else {
        WriteLog(L"No mutex name provided, update mutex will not be used.");
    }

    if (IsDriveRootDirectory(rootDir)) {
        failureReason =
            L"检测到 MAA 安装在盘符根目录，已阻止更新继续执行。请先将 MAA 移动到独立文件夹后再重试。\n\n"
            L"Detected MAA installed directly in a drive root. Update execution was blocked. Please move MAA into a dedicated folder and try again.";
        WriteLog(failureReason.c_str());
        SetProgressUiStatus(
            L"无法继续更新 | Update blocked",
            L"检测到盘符根目录安装 | Drive-root install detected");
    }

    // ------------------------------------------------------------------
    // Read plan
    // ------------------------------------------------------------------
    do {
        if (!failureReason.empty()) {
            break;
        }

        PendingUpdatePlan plan;
        if (!LoadPendingUpdatePlan(planFile, plan, failureReason)) {
            WriteLog(failureReason.c_str());
            break;
        }

        bool isFullPackage = EqualsIgnoreCase(plan.packageType, L"full");
        const std::vector<std::wstring>& removeList = plan.removeList;
        const std::vector<std::wstring>& moveList = plan.moveList;

        SetProgressUiTotalFileCount(static_cast<int>(removeList.size() + moveList.size()));
        SetProgressUiStatus(
            L"正在分析更新内容... | Analyzing update contents...",
            L"更新计划读取完成 | Update plan loaded");

        WriteLog((L"Plan loaded, package type: " + plan.packageType + L", remove entries: " + std::to_wstring(removeList.size()) + L", install entries: " + std::to_wstring(moveList.size())).c_str());
        WriteLogEntries(L"Files to remove", removeList);
        WriteLogEntries(L"Files to install", moveList);

        CreateDirectoryW(backupDir.c_str(), nullptr);

        // ---- Remove entries ----
        for (const std::wstring& rel : removeList) {
            std::wstring targetPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath)) {
                failureReason = L"Illegal path in removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }
            if (!PathExistsW(targetPath)) {
                AdvanceProgressUi(
                    L"正在清理旧文件... | Cleaning old files...",
                    rel);
                continue;
            }

            if (!isFullPackage && IsDirectory(targetPath)) {
                WriteLog((L"Skipping directory removal for a non-full package, entry: " + rel + L", target: " + targetPath).c_str());
                AdvanceProgressUi(
                    L"正在清理旧文件... | Cleaning old files...",
                    rel);
                continue;
            }

            std::wstring backupPath;
            if (!TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                failureReason = L"Illegal backup path for removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            WriteLog((L"Removing and backing up: " + targetPath + L" -> " + backupPath).c_str());
            bool backupOk = isFullPackage
                ? RecycleAndBackupPath(targetPath, backupPath)
                : MoveExistingPathToBackup(targetPath, backupPath);
            if (!backupOk) {
                failureReason = L"Failed to move to backup: " + targetPath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            AdvanceProgressUi(
                L"正在清理旧文件... | Cleaning old files...",
                rel);
        }

        // ---- Move/install entries ----
        for (const std::wstring& rel : moveList) {
            std::wstring sourcePath, targetPath, backupPath;

            if (!TryResolvePathUnderRoot(extractDir, rel, sourcePath) ||
                !TryResolvePathUnderRoot(rootDir,    rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir,  rel, backupPath))
            {
                failureReason = L"Illegal path in moveList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            if (PathExistsW(targetPath)) {
                if (!isFullPackage && IsDirectory(targetPath)) {
                    failureReason = L"Non-full package cannot replace directory: " + targetPath;
                    WriteLog(failureReason.c_str());
                    goto apply_failed;
                }

                WriteLog((L"Backing up existing entry: " + targetPath).c_str());
                bool backupOk = IsRecycleAndReplaceDirectory(rel)
                    ? RecycleAndBackupDirectory(targetPath, backupPath)
                    : MoveExistingPathToBackup(targetPath, backupPath);
                if (!backupOk) {
                    failureReason = L"Failed to back up existing entry: " + targetPath;
                    WriteLog(failureReason.c_str());
                    goto apply_failed;
                }
            }

            WriteLog((L"Installing new file: " + sourcePath + L" -> " + targetPath).c_str());

            bool installOk = false;
            DWORD sourceAttr = GetFileAttributesW(sourcePath.c_str());
            bool isSourceFile = (sourceAttr != INVALID_FILE_ATTRIBUTES) &&
                                !(sourceAttr & FILE_ATTRIBUTE_DIRECTORY);

            if (isSourceFile) {
                // Use atomic file replacement for individual files
                installOk = InstallFileAtomic(sourcePath, targetPath);
            } else {
                // Directories: use the original move logic
                auto moveOp = [&]() -> bool {
                    return MoveFileExW(sourcePath.c_str(), targetPath.c_str(),
                                       MOVEFILE_REPLACE_EXISTING) != FALSE;
                };
                installOk = RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);
            }

            if (!installOk) {
                failureReason = L"Failed to move file into place: " + sourcePath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            AdvanceProgressUi(
                L"正在安装新文件... | Installing new files...",
                rel);
        }

        // ---- Cleanup package ----
        if (PathExistsW(packagePath)) {
            DeleteFileW(packagePath.c_str());
            WriteLog((L"Deleted update package: " + packagePath).c_str());
        }

        if (PathExistsW(failureStatusFile))
            DeleteFileW(failureStatusFile.c_str());

        SetProgressUiStatus(
            L"正在完成更新... | Finalizing update...",
            L"正在写入更新结果 | Writing update result");
        if (WriteUtf8File(successStatusFile, "succeeded")) {
            WriteLog((L"Wrote success status file: " + successStatusFile).c_str());
            success = true;
            shouldRelaunch = true;
        } else {
            failureReason = L"Failed to write success status file: " + successStatusFile;
            WriteLog(failureReason.c_str());
        }

        break; // normal exit from do-while

    apply_failed:
        success = false;

        // Attempt rollback: restore files that were already backed up
        WriteLog(L"Update failed, attempting rollback from backup directory.");
        for (const std::wstring& rel : removeList) {
            std::wstring targetPath, backupPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                continue;
            }
            if (PathExistsW(backupPath) && !PathExistsW(targetPath)) {
                WriteLog((L"Rollback: restoring " + backupPath + L" -> " + targetPath).c_str());
                MovePathEntry(backupPath, targetPath);
            }
        }
        for (const std::wstring& rel : moveList) {
            std::wstring targetPath, backupPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                continue;
            }
            if (PathExistsW(backupPath) && !PathExistsW(targetPath)) {
                WriteLog((L"Rollback: restoring " + backupPath + L" -> " + targetPath).c_str());
                MovePathEntry(backupPath, targetPath);
            }
        }
    } while (false);

    // ------------------------------------------------------------------
    // On failure: write failure status
    // ------------------------------------------------------------------
    if (!success && !failureReason.empty()) {
        // Convert wstring reason to UTF-8 for file
        std::string utf8Reason;
        if (TryConvertWideToUtf8(failureReason, utf8Reason)) {
            WriteUtf8File(failureStatusFile, utf8Reason);
        }
        if (PathExistsW(successStatusFile))
            DeleteFileW(successStatusFile.c_str());
        WriteLog((L"Update failed: " + failureReason).c_str());
        ShowProgressUiFailure(failureReason);
    }

    // ------------------------------------------------------------------
    // Cleanup extract dir and plan file
    // ------------------------------------------------------------------
    if (PathExistsW(extractDir)) {
        WriteLog((L"Cleaning extract directory: " + extractDir).c_str());
        ForceRemoveDirectoryRecursive(extractDir);
    }

    if (PathExistsW(planFile))
        DeleteFileW(planFile.c_str());

    // ------------------------------------------------------------------
    // Release update mutex
    // ------------------------------------------------------------------
    if (hUpdateMutex != nullptr) {
        ReleaseUpdateMutex(hUpdateMutex);
        hUpdateMutex = nullptr;
        WriteLog(L"Update mutex released.");
    }

    // ------------------------------------------------------------------
    // Relaunch MAA
    // ------------------------------------------------------------------
    if (shouldRelaunch && PathExistsW(relaunchExecutable)) {
        CompleteProgressUi(
            L"更新完成 | Update completed",
            L"正在重新启动 MAA... | Relaunching MAA...");
        WriteLog((L"Relaunching MAA: " + relaunchExecutable).c_str());

        // Find working directory (parent of the executable)
        std::wstring workDir = relaunchExecutable;
        size_t sep = workDir.rfind(L'\\');
        if (sep != std::wstring::npos) workDir = workDir.substr(0, sep);

        STARTUPINFOW si {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi {};
        std::wstring cmdLine = L"\"" + relaunchExecutable + L"\"";
        if (CreateProcessW(
                relaunchExecutable.c_str(),
                cmdLine.data(),
                nullptr, nullptr, FALSE, 0, nullptr,
                workDir.c_str(),
                &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            WriteLog(L"Relaunch succeeded.");
        } else {
            WriteLog((L"Relaunch failed, error=" + std::to_wstring(GetLastError())).c_str());
            ShowProgressUiFailure(
                L"更新已完成，但重新启动 MAA 失败，请手动启动 MAA。\n"
                L"Update finished, but failed to relaunch MAA. Please start MAA manually.");
        }
    } else if (success) {
        CompleteProgressUi(
            L"更新完成 | Update completed",
            L"更新已完成，请手动启动 MAA。 | Update completed. Please start MAA manually.");
    }

    WriteLog(L"MAA.Updater exiting.");
    DestroyProgressUi();
    return success ? 0 : 2;
}
