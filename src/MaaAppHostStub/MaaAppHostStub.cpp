// MAA.exe 的原生替代入口，打包时用它覆盖 .NET SDK 生成的 apphost。
//
// MAA.exe 本体是自包含发布生成的 apphost 启动器：若用户把 MAA.exe 单独拖出文件夹，
// 它会在托管代码运行前就退出，报错只写 stderr（双击时无人接收），
// GUI 里 MaaCore.dll / resource 的检查（ErrorSolutionMoveMaaExeOutOfFolder）执行不到。
// 这里在进入 .NET 之前检查安装是否完整：缺失时（或处于临时目录，如直接双击压缩包内
// 文件）弹提示；完整则加载同目录 hostfxr.dll，经 hostfxr_main_startupinfo 转发启动，
// 与 SDK apphost 的调用方式一致，托管侧 Main/GetCommandLineArgs 不会多收参数。

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>

namespace
{

// 标题取 ErrorCongratulations（与 GUI 喜报窗口同名），
// 正文取 src/MaaWpfGui/Res/Localizations/*.xaml 的 ErrorSolutionMoveMaaExeOutOfFolder，
// 临时目录场景的正文为 stub 自带文案（对应 GUI 侧 UnsupportedInstallLocationError 的场景）。
// 以上文案与 xaml 逐字同步，由 tools/CheckStubLocalization.py 在 CI 校验。
struct FatalMessage
{
    const wchar_t *title;
    const wchar_t *text;
};

FatalMessage GetFatalMessage(const bool running_from_temp)
{
    const LANGID lang = GetUserDefaultUILanguage();

    if (PRIMARYLANGID(lang) == LANG_CHINESE)
    {
        switch (SUBLANGID(lang))
        {
        case SUBLANG_CHINESE_TRADITIONAL:
        case SUBLANG_CHINESE_HONGKONG:
        case SUBLANG_CHINESE_MACAU:
            return {
                L"喜報",
                running_from_temp
                    ? L"偵測到 MAA.exe 正在臨時目錄中執行（可能直接雙擊了壓縮檔內的檔案）。"
                      L"請先將整個壓縮檔解壓縮到獨立資料夾（例如 D:\\MAA\\），再執行其中的 MAA.exe。"
                    : L"請勿直接移動 MAA.exe 檔案。若需變更軟體位置，請移動整個資料夾；或右鍵點擊 MAA.exe "
                      L"建立捷徑，然後將捷徑移動到其他位置。",
            };
        default:
            return {
                L"喜报",
                running_from_temp
                    ? L"检测到 MAA.exe 正在临时目录中运行（可能直接双击了压缩包内的文件）。"
                      L"请先将整个压缩包解压到独立文件夹（例如 D:\\MAA\\），再运行其中的 MAA.exe。"
                    : L"请勿直接移动 MAA.exe 文件。若需更改软件位置，请移动整个文件夹；或右键 MAA.exe 创建快捷方式，"
                      L"然后将快捷方式移动到其他位置。",
            };
        }
    }

    switch (PRIMARYLANGID(lang))
    {
    case LANG_JAPANESE:
        return {
            L"おめでとうございます",
            running_from_temp
                ? L"MAA.exe が一時フォルダーで実行されています（アーカイブ内のファイルを直接ダブルクリックした可能性が"
                  L"あります）。アーカイブ全体を独立したフォルダー（例: D:\\MAA\\）に展開してから、その中の "
                  L"MAA.exe を実行してください。"
                : L"MAA.exe ファイルを単独で移動しないでください。ソフトウェアの場所を変更する必要がある場合は、"
                  L"フォルダー全体を移動するか、このフォルダーに MAA.exe のショートカットを作成してから、"
                  L"ショートカットを別の場所にドラッグしてください。",
        };
    case LANG_KOREAN:
        return {
            L"축하합니다",
            running_from_temp
                ? L"MAA.exe가 임시 폴더에서 실행 중입니다(압축 파일 내부의 파일을 직접 실행했을 수 있습니다). "
                  L"전체 압축 파일을 독립된 폴더(예: D:\\MAA\\)에 먼저 풀고 그 안의 MAA.exe를 실행하세요."
                : L"MAA.exe 파일을 단독으로 옮기지 마세요. 소프트웨어 위치를 변경해야 하는 경우, 전체 폴더를 "
                  L"이동시키거나 MAA.exe의 바로 가기를 만든 다음 바로 가기를 옮기세요.",
        };
    default:
        return {
            L"Congratulations",
            running_from_temp
                ? L"MAA.exe is running from a temporary directory (a file inside the archive may have been launched "
                  L"directly). Extract the entire archive to a standalone folder (e.g. D:\\MAA\\) first, then run "
                  L"MAA.exe from there."
                : L"Do not move the MAA.exe file alone. If you need to change the software location, please move the "
                  L"entire folder, or create a shortcut for MAA.exe in this folder and then drag the shortcut to "
                  L"another location.",
        };
    }
}

// 尽力而为的日志：追加到 debug\apphost-stub.log（UTF-8），任何一步失败都直接放弃
void AppendDebugLog(const std::wstring &dir, const std::wstring &detail)
{
    if (dir.empty())
    {
        return;
    }

    SYSTEMTIME time = {};
    GetLocalTime(&time);
    wchar_t line_w[1024] = {};
    // _TRUNCATE：detail 超长时截断而不是触发 invalid parameter handler 终止进程；
    // 截断时返回 -1，缓冲区已保证 null 结尾，改取实际长度以保留截断后的内容
    int len = _snwprintf_s(
        line_w,
        _countof(line_w),
        _TRUNCATE,
        L"[%04u-%02u-%02u %02u:%02u:%02u] %s\r\n",
        static_cast<unsigned>(time.wYear),
        static_cast<unsigned>(time.wMonth),
        static_cast<unsigned>(time.wDay),
        static_cast<unsigned>(time.wHour),
        static_cast<unsigned>(time.wMinute),
        static_cast<unsigned>(time.wSecond),
        detail.c_str());
    if (len < 0)
    {
        len = static_cast<int>(wcslen(line_w));
    }
    if (len <= 0)
    {
        return;
    }

    char line_u8[3072] = {};
    const int u8_len = WideCharToMultiByte(CP_UTF8, 0, line_w, len, line_u8, sizeof(line_u8), nullptr, nullptr);
    if (u8_len <= 0)
    {
        return;
    }

    const std::wstring debug_dir = dir + L"\\debug";
    CreateDirectoryW(debug_dir.c_str(), nullptr);
    HANDLE file = CreateFileW(
        (debug_dir + L"\\apphost-stub.log").c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }
    DWORD written = 0;
    WriteFile(file, line_u8, static_cast<DWORD>(u8_len), &written, nullptr);
    CloseHandle(file);
}

HHOOK g_message_box_hook = nullptr;
HICON g_dialog_small_icon = nullptr;
HICON g_dialog_big_icon = nullptr;

HICON LoadLogoIcon(const int size)
{
    return reinterpret_cast<HICON>(LoadImageW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(1),
        IMAGE_ICON,
        size,
        size,
        LR_DEFAULTCOLOR));
}

// MessageBox 弹出的对话框默认没有标题栏图标，在首次激活时补上资源里的 newlogo（ID 1）。
// 图标句柄由 ShowFatalError 持有并在弹窗结束后销毁（未用 LR_SHARED，尺寸为自定义值）。
LRESULT CALLBACK FatalMessageBoxCbtProc(const int code, const WPARAM wParam, const LPARAM lParam)
{
    if (code == HCBT_ACTIVATE && g_message_box_hook != nullptr)
    {
        const HWND dialog = reinterpret_cast<HWND>(wParam);
        if (g_dialog_small_icon != nullptr)
        {
            SendMessageW(dialog, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_dialog_small_icon));
        }
        if (g_dialog_big_icon != nullptr)
        {
            SendMessageW(dialog, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_dialog_big_icon));
        }

        UnhookWindowsHookEx(g_message_box_hook);
        g_message_box_hook = nullptr;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void ShowFatalError(
    const std::wstring &dir,
    const bool running_from_temp,
    const std::wstring &detail,
    const bool write_debug_log)
{
    if (write_debug_log)
    {
        AppendDebugLog(dir, detail);
    }

    const FatalMessage msg = GetFatalMessage(running_from_temp);

    g_dialog_small_icon = LoadLogoIcon(GetSystemMetrics(SM_CXSMICON));
    g_dialog_big_icon = LoadLogoIcon(GetSystemMetrics(SM_CXICON));
    g_message_box_hook = SetWindowsHookExW(WH_CBT, FatalMessageBoxCbtProc, nullptr, GetCurrentThreadId());
    MessageBoxW(nullptr, msg.text, msg.title, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    if (g_message_box_hook != nullptr)
    {
        UnhookWindowsHookEx(g_message_box_hook);
        g_message_box_hook = nullptr;
    }
    if (g_dialog_small_icon != nullptr)
    {
        DestroyIcon(g_dialog_small_icon);
        g_dialog_small_icon = nullptr;
    }
    if (g_dialog_big_icon != nullptr)
    {
        DestroyIcon(g_dialog_big_icon);
        g_dialog_big_icon = nullptr;
    }
}

// 与 GUI 侧 Bootstrapper.TryGetTempInstallLocation 的判定保持一致：
// 位于 TEMP/TMP/TMPDIR 目录之下，或所在目录（含父目录）名以 temp/tmp 开头
bool IsPathUnderDirectory(const std::wstring &path, const std::wstring &directory)
{
    if (directory.empty() || path.size() <= directory.size())
    {
        return false;
    }
    std::wstring base = directory;
    while (!base.empty() && (base.back() == L'\\' || base.back() == L'/'))
    {
        base.pop_back();
    }
    if (path.size() <= base.size() || _wcsnicmp(path.c_str(), base.c_str(), base.size()) != 0)
    {
        return false;
    }
    const wchar_t next = path[base.size()];
    return next == L'\\' || next == L'/';
}

bool HasTempLikeName(const std::wstring &directory)
{
    const auto last_sep = directory.find_last_of(L"\\/");
    const std::wstring name = last_sep == std::wstring::npos ? directory : directory.substr(last_sep + 1);
    return _wcsnicmp(name.c_str(), L"temp", 4) == 0 || _wcsnicmp(name.c_str(), L"tmp", 3) == 0;
}

bool IsRunningFromTempDirectory(const std::wstring &dir)
{
    std::vector<std::wstring> temp_roots;

    wchar_t temp_path[0x800];
    const DWORD temp_len = GetTempPathW(0x800, temp_path);
    if (temp_len != 0 && temp_len < 0x800)
    {
        temp_roots.emplace_back(temp_path, temp_len);
    }

    for (const wchar_t *variable : { L"TEMP", L"TMP", L"TMPDIR" })
    {
        const DWORD needed = GetEnvironmentVariableW(variable, nullptr, 0);
        if (needed == 0)
        {
            continue;
        }
        std::wstring value(needed, L'\0');
        const DWORD written = GetEnvironmentVariableW(variable, value.data(), needed);
        if (written != 0)
        {
            value.resize(written);
            temp_roots.push_back(std::move(value));
        }
    }

    for (const std::wstring &root : temp_roots)
    {
        if (IsPathUnderDirectory(dir, root))
        {
            return true;
        }
    }

    if (HasTempLikeName(dir))
    {
        return true;
    }
    const auto last_sep = dir.find_last_of(L"\\/");
    return last_sep != std::wstring::npos && HasTempLikeName(dir.substr(0, last_sep));
}

} // namespace

// 参数批注与 winbase.h 的 wWinMain 声明保持一致，避免 C28251（/analyze 下批注不一致）
int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    std::wstring self_path(0x8000, L'\0');
    const DWORD path_len = GetModuleFileNameW(nullptr, self_path.data(), static_cast<DWORD>(self_path.size()));
    if (path_len == 0 || path_len >= self_path.size())
    {
        ShowFatalError(L"", false, L"GetModuleFileNameW failed", true);
        return 1;
    }
    self_path.resize(path_len);

    const auto last_sep = self_path.find_last_of(L"\\/");
    if (last_sep == std::wstring::npos)
    {
        ShowFatalError(L"", false, L"unexpected module path: " + self_path, true);
        return 1;
    }
    const std::wstring dir = self_path.substr(0, last_sep);
    const std::wstring app_dll = dir + L"\\MAA.dll";
    const std::wstring hostfxr_dll = dir + L"\\hostfxr.dll";

    // 完整性哨兵与 GUI 内 Bootstrapper 的检查同源（MaaCore.dll、resource），
    // 外加 MAA.dll 和 externals（NetBeauty 打包后的 .NET 运行时目录）：
    // 除移动整个文件夹外，任何部分移动都在这里拦下并弹提示；目录类哨兵须校验
    // FILE_ATTRIBUTE_DIRECTORY，防止同名文件骗过检查。
    // 注意开发构建目录没有 externals，stub 只用于打包产物。
    const wchar_t *required_files[] = {
        L"\\MAA.dll",
        L"\\MaaCore.dll",
    };
    const wchar_t *required_dirs[] = {
        L"\\resource",
        L"\\externals",
    };
    // 文件/目录哨兵对称校验：文件不得是目录，目录不得是文件，防止同名条目骗过检查
    const auto sentinel_ok = [](const std::wstring &path, const bool require_directory) {
        const DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        const bool is_directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        return require_directory ? is_directory : !is_directory;
    };
    const wchar_t *missing = nullptr;
    int present_count = 0;
    // 两个循环都无条件跑完：present_count 统计全部哨兵，missing 记录第一个缺失项
    for (const wchar_t *name : required_files)
    {
        if (sentinel_ok(dir + name, false))
        {
            ++present_count;
        }
        else if (missing == nullptr)
        {
            missing = name;
        }
    }
    for (const wchar_t *name : required_dirs)
    {
        if (sentinel_ok(dir + name, true))
        {
            ++present_count;
        }
        else if (missing == nullptr)
        {
            missing = name;
        }
    }
    if (missing != nullptr)
    {
        // 文件缺失时才判断是否处于临时目录（如直接双击压缩包内的文件），以选择对应文案；
        // 安装完整时的位置检查交给 GUI（UnsupportedInstallLocationError 的提示更详细）。
        // 仅当目录看起来像安装目录（至少命中一个哨兵）时才落日志，
        // 避免在桌面等位置凭空创建 debug 目录
        ShowFatalError(
            dir,
            IsRunningFromTempDirectory(dir),
            std::wstring(L"missing ") + missing,
            present_count > 0);
        return 1;
    }

    const HMODULE fxr = LoadLibraryW(hostfxr_dll.c_str());
    if (fxr == nullptr)
    {
        // MAA.dll 在而 hostfxr.dll 不在，同样只可能是文件被单独挪走，提示相同文案
        ShowFatalError(dir, false, L"LoadLibraryW(hostfxr.dll) failed, GetLastError=" + std::to_wstring(GetLastError()), true);
        return 1;
    }

    // 签名与 src/native/corehost/hostfxr.h 的 hostfxr_main_startupinfo_fn 一致：
    // (argc, argv, host_path, dotnet_root, app_path)，五个平铺参数
    using hostfxr_main_fn = int (*)(const int argc, const wchar_t *const argv[]);
    using hostfxr_main_startupinfo_fn =
        int (*)(const int argc, const wchar_t *const argv[], const wchar_t *host_path, const wchar_t *dotnet_root, const wchar_t *app_path);
    const auto hostfxr_main_startupinfo =
        reinterpret_cast<hostfxr_main_startupinfo_fn>(GetProcAddress(fxr, "hostfxr_main_startupinfo"));
    const auto hostfxr_main = reinterpret_cast<hostfxr_main_fn>(GetProcAddress(fxr, "hostfxr_main"));
    if (hostfxr_main_startupinfo == nullptr && hostfxr_main == nullptr)
    {
        FreeLibrary(fxr);
        ShowFatalError(dir, false, L"GetProcAddress(hostfxr_main_startupinfo/hostfxr_main) failed", true);
        return 1;
    }

    int argc = 0;
    LPWSTR *const raw_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (raw_argv == nullptr)
    {
        FreeLibrary(fxr);
        ShowFatalError(dir, false, L"CommandLineToArgvW failed, GetLastError=" + std::to_wstring(GetLastError()), true);
        return 1;
    }

    // 原样转发 [exe, 用户参数...]：hostfxr 在 apphost 模式下会把 argv[1..] 作为托管参数，
    // 额外插入 MAA.dll 会污染 Main(string[] args) / Environment.GetCommandLineArgs()
    std::vector<const wchar_t *> forward_argv;
    forward_argv.reserve(static_cast<size_t>(argc));
    forward_argv.push_back(self_path.c_str());
    for (int i = 1; i < argc; ++i)
    {
        forward_argv.push_back(raw_argv[i]);
    }

    // 首选 hostfxr_main_startupinfo：显式给出三个路径，不依赖 argv[0] 的名字推导
    const int exit_code = hostfxr_main_startupinfo != nullptr
                              ? hostfxr_main_startupinfo(
                                  static_cast<int>(forward_argv.size()), forward_argv.data(), self_path.c_str(), dir.c_str(), app_dll.c_str())
                              : hostfxr_main(static_cast<int>(forward_argv.size()), forward_argv.data());

    LocalFree(raw_argv);
    return exit_code;
}
