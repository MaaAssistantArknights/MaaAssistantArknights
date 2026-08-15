// MAA.exe 的原生替代入口，打包时用它覆盖 .NET SDK 生成的 apphost。
//
// MAA.exe 本体是自包含发布生成的 apphost 启动器：若用户把 MAA.exe 单独拖出文件夹，
// 它会在托管代码运行前就退出，报错只写 stderr（双击时无人接收），
// GUI 里 MaaCore.dll / resource 的检查（ErrorSolutionMoveMaaExeOutOfFolder）执行不到。
// 这里在进入 .NET 之前检查旁边的安装是否完整（MAA.dll、MaaCore.dll、resource、externals），
// 缺失时弹出同样的提示文案；完整则加载同目录 hostfxr.dll，按 `dotnet MAA.dll` 相同的方式转发启动。

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
// 临时目录场景的正文为 stub 自带文案（对应 GUI 侧 UnsupportedInstallLocationError 的场景）
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

HHOOK g_message_box_hook = nullptr;

// MessageBox 弹出的对话框默认没有标题栏图标，在首次激活时补上资源里的 newlogo（ID 1）
LRESULT CALLBACK FatalMessageBoxCbtProc(const int code, const WPARAM wParam, const LPARAM lParam)
{
    if (code == HCBT_ACTIVATE && g_message_box_hook != nullptr)
    {
        const HWND dialog = reinterpret_cast<HWND>(wParam);
        const int small = GetSystemMetrics(SM_CXSMICON);
        const int big = GetSystemMetrics(SM_CXICON);
        const auto load = [](const int icon_size) {
            return reinterpret_cast<LPARAM>(LoadImageW(
                GetModuleHandleW(nullptr),
                MAKEINTRESOURCEW(1),
                IMAGE_ICON,
                icon_size,
                icon_size,
                LR_DEFAULTCOLOR | LR_SHARED));
        };
        SendMessageW(dialog, WM_SETICON, ICON_SMALL, load(small));
        SendMessageW(dialog, WM_SETICON, ICON_BIG, load(big));

        UnhookWindowsHookEx(g_message_box_hook);
        g_message_box_hook = nullptr;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void ShowFatalError(const bool running_from_temp)
{
    const FatalMessage msg = GetFatalMessage(running_from_temp);

    g_message_box_hook = SetWindowsHookExW(WH_CBT, FatalMessageBoxCbtProc, nullptr, GetCurrentThreadId());
    MessageBoxW(nullptr, msg.text, msg.title, MB_OK | MB_ICONERROR);
    if (g_message_box_hook != nullptr)
    {
        UnhookWindowsHookEx(g_message_box_hook);
        g_message_box_hook = nullptr;
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    std::wstring self_path(0x8000, L'\0');
    const DWORD path_len = GetModuleFileNameW(nullptr, self_path.data(), static_cast<DWORD>(self_path.size()));
    if (path_len == 0 || path_len >= self_path.size())
    {
        ShowFatalError(false);
        return 1;
    }
    self_path.resize(path_len);

    const auto last_sep = self_path.find_last_of(L"\\/");
    if (last_sep == std::wstring::npos)
    {
        ShowFatalError(false);
        return 1;
    }
    const std::wstring dir = self_path.substr(0, last_sep);
    const std::wstring app_dll = dir + L"\\MAA.dll";
    const std::wstring hostfxr_dll = dir + L"\\hostfxr.dll";

    // 临时目录（典型：直接双击压缩包内的 MAA.exe）需要专门的文案，先于哨兵检查
    if (IsRunningFromTempDirectory(dir))
    {
        ShowFatalError(true);
        return 1;
    }

    // 哨兵与 GUI 内 Bootstrapper 的检查同源（MaaCore.dll、resource），
    // 外加 MAA.dll 和 externals（NetBeauty 打包后的 .NET 运行时目录）：
    // 除移动整个文件夹外，任何部分移动都在这里拦下并弹提示。
    // 注意开发构建目录没有 externals，stub 只用于打包产物。
    const wchar_t *required_paths[] = {
        L"\\MAA.dll",
        L"\\MaaCore.dll",
        L"\\resource",
        L"\\externals",
    };
    for (const wchar_t *name : required_paths)
    {
        if (GetFileAttributesW((dir + name).c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            ShowFatalError(false);
            return 1;
        }
    }

    const HMODULE fxr = LoadLibraryW(hostfxr_dll.c_str());
    if (fxr == nullptr)
    {
        // MAA.dll 在而 hostfxr.dll 不在，同样只可能是文件被单独挪走，提示相同文案
        ShowFatalError(false);
        return 1;
    }

    using hostfxr_main_fn = int (*)(const int argc, const wchar_t *const argv[]);
    const auto hostfxr_main = reinterpret_cast<hostfxr_main_fn>(GetProcAddress(fxr, "hostfxr_main"));
    if (hostfxr_main == nullptr)
    {
        FreeLibrary(fxr);
        ShowFatalError(false);
        return 1;
    }

    int argc = 0;
    LPWSTR *const raw_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (raw_argv == nullptr)
    {
        FreeLibrary(fxr);
        ShowFatalError(false);
        return 1;
    }

    std::vector<const wchar_t *> forward_argv;
    forward_argv.reserve(static_cast<size_t>(argc) + 1);
    forward_argv.push_back(self_path.c_str());
    forward_argv.push_back(app_dll.c_str());
    for (int i = 1; i < argc; ++i)
    {
        forward_argv.push_back(raw_argv[i]);
    }

    const int exit_code = hostfxr_main(static_cast<int>(forward_argv.size()), forward_argv.data());

    LocalFree(raw_argv);
    return exit_code;
}
