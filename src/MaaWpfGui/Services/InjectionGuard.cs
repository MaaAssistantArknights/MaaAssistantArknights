// <copyright file="InjectionGuard.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Helper;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Services;

/// <summary>
/// 检测并阻止第三方 DLL 注入（如 Nahimic OSD 导致 MAA 渲染异常/闪退）。
/// <para>
/// 实时检测基于 ntdll 的 <c>LdrRegisterDllNotification</c>（未文档化 API），任何 DLL 加载进
/// 本进程时都会收到回调；命中规则后在后台线程执行 <see cref="FreeLibrary"/> 卸载并提示用户。
/// 同时保留启动全量扫描与周期轮询兜底（覆盖注册监听前已注入的模块，以及正则等复杂规则）。
/// </para>
/// <para>
/// 回调运行在 loader lock 之下，其中严禁任何可能分配内存或获取锁的操作
/// （日志、弹窗、字符串拼接等一律放后台线程处理），回调内部仅做零分配的
/// 关键字匹配，命中后通过事件唤醒后台线程。
/// </para>
/// </summary>
internal class InjectionGuard
{
    private static readonly ILogger _logger = Log.ForContext<InjectionGuard>();

    private const uint LdrDllNotificationReasonLoaded = 1;

    private const uint ListModulesAll = 0x03;

    private const int MaxModulePathChars = 1024;

    private const int PollingIntervalMs = 10_000;

    // 以下 API 为 ntdll 未文档化接口或不在 CsWin32 元数据中，手写 DllImport。
    [DllImport("ntdll.dll")]
    private static extern uint LdrRegisterDllNotification(uint flags, IntPtr notificationFunction, IntPtr context, out IntPtr cookie);

    [DllImport("ntdll.dll")]
    private static extern uint LdrUnregisterDllNotification(IntPtr cookie);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("psapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool K32EnumProcessModulesEx(IntPtr hProcess, IntPtr[] modules, uint cb, out uint cbNeeded, uint dwFilterFlag);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetModuleFileNameW(IntPtr hModule, char[] buffer, int size);

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetCurrentProcess();

    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct LdrUnicodeString
    {
        public ushort Length;

        public ushort MaximumLength;

        public char* Buffer;
    }

    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct LdrDllNotificationData
    {
        public uint Flags;

        public LdrUnicodeString* FullDllName;

        public LdrUnicodeString* BaseDllName;

        public IntPtr DllBase;

        public uint SizeOfImage;
    }

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void LdrDllNotificationCallback(uint reason, IntPtr notificationData, IntPtr context);

    private static readonly LdrDllNotificationCallback _notificationCallback = OnLdrDllNotification;

    private static IntPtr _notificationCookie;

    private static AutoResetEvent? _wakeEvent;

    private static ManualResetEvent? _shutdownEvent;

    private static Timer? _pollingTimer;

    private static IntPtr _pendingUnloadBase;

    private static readonly HashSet<string> _handledDlls = new(StringComparer.OrdinalIgnoreCase);

    private static readonly object _handledDllsLock = new();

    // 规则内置默认名单（可扩展），对应现有 BadModules 检测的已知注入 DLL。
    private static readonly InjectionRule[] _rules =
    [
        new() {
            Name = "Nahimic OSD",
            DllNameExact = ["NahimicOSD.dll"],
            DllNameKeywords = ["nahimic", "audiodevprops"],
            PathKeywords = ["nahimic", "a-volute"],
        },
        new() {
            Name = "MSI OSD",
            DllNameExact = ["GTII-OSD64.dll", "GTIII-OSD64.dll"],
            DllNameKeywords = ["gtii-osd", "gtiii-osd"],
        },
    ];

    // 预编译用于 loader lock 回调的零分配匹配列表（小写化后的文件名）。
    private static string[] _callbackDllNameExact = [];

    private static string[] _callbackDllNameKeywords = [];

    /// <summary>
    /// 启动监听：注册 DLL 加载通知、启动后台处理线程与轮询兜底。
    /// 应在日志与本地化初始化完成后、窗口创建前调用。
    /// </summary>
    internal static void Start()
    {
        try
        {
            // 预热回调（触发静态构造与首次 JIT 编译，避免首次真实回调在 loader lock 下编译）
            _notificationCallback(0, IntPtr.Zero, IntPtr.Zero);

            PrecompileCallbackMatchers();

            // 预热回调内部匹配路径的 JIT 编译，避免其在 loader lock 下首次被编译
            unsafe
            {
                char* warmup = stackalloc char[4];
                MatchesCallbackRules(warmup, 0);
            }

            _wakeEvent = new AutoResetEvent(false);
            _shutdownEvent = new ManualResetEvent(false);

            IntPtr callbackPtr = Marshal.GetFunctionPointerForDelegate(_notificationCallback);
            uint status = LdrRegisterDllNotification(0, callbackPtr, IntPtr.Zero, out IntPtr cookie);
            if (status != 0)
            {
                _logger.Error("LdrRegisterDllNotification failed with status {Status}", status);
            }
            else
            {
                _notificationCookie = cookie;
                _logger.Information("InjectionGuard: DLL load notification registered");
            }

            _ = Task.Run(BackgroundLoop);

            // 启动时同步扫描一次已加载模块（覆盖注册监听前已注入的模块）。
            // 同步执行可确保 OnLaunch 阶段的 BadModules 检查不再检测到已被拦截的 DLL，
            // 避免重复弹窗（"检测到注入" + "建议软件渲染"）。
            PollingTick();

            // 随后周期兜底
            _pollingTimer = new Timer(static _ => PollingTick(), null, TimeSpan.FromMilliseconds(PollingIntervalMs), TimeSpan.FromMilliseconds(PollingIntervalMs));
        }
        catch (Exception e)
        {
            _logger.Error(e, "InjectionGuard failed to start");
        }
    }

    /// <summary>
    /// 停止监听并释放资源，应在应用退出时调用。
    /// </summary>
    internal static void Stop()
    {
        try
        {
            _pollingTimer?.Dispose();
            _pollingTimer = null;

            if (_notificationCookie != IntPtr.Zero)
            {
                LdrUnregisterDllNotification(_notificationCookie);
                _notificationCookie = IntPtr.Zero;
                _logger.Information("InjectionGuard: DLL load notification unregistered");
            }

            _shutdownEvent?.Set();
            _wakeEvent?.Set();
        }
        catch (Exception e)
        {
            _logger.Error(e, "InjectionGuard failed to stop");
        }
    }

    private static void PrecompileCallbackMatchers()
    {
        _callbackDllNameExact = [.. _rules
            .SelectMany(r => r.DllNameExact)
            .Where(static n => !string.IsNullOrWhiteSpace(n))
            .Select(static n => n.ToLowerInvariant())];

        _callbackDllNameKeywords = [.. _rules
            .SelectMany(r => r.DllNameKeywords)
            .Where(static n => !string.IsNullOrWhiteSpace(n))
            .Select(static n => n.ToLowerInvariant())];
    }

    /// <summary>
    /// ntdll 加载通知回调。运行在 loader lock 下：不得分配、不得取锁、不得调用任何
    /// Win32 加载/卸载 API，仅做零分配匹配，命中后唤醒后台线程处理。
    /// </summary>
    private static void OnLdrDllNotification(uint reason, IntPtr notificationData, IntPtr context)
    {
        try
        {
            if (reason != LdrDllNotificationReasonLoaded || notificationData == IntPtr.Zero)
            {
                return;
            }

            unsafe
            {
                var data = (LdrDllNotificationData*)notificationData;
                if (data->BaseDllName is null)
                {
                    return;
                }

                char* baseName = stackalloc char[MaxModulePathChars];
                int baseLen = CopyUnicodeString(data->BaseDllName, baseName, MaxModulePathChars);
                if (baseLen <= 0)
                {
                    return;
                }

                for (int i = 0; i < baseLen; ++i)
                {
                    baseName[i] = ToLowerInvariant(baseName[i]);
                }

                if (!MatchesCallbackRules(baseName, baseLen))
                {
                    return;
                }

                // 命中规则：记录模块基址并唤醒后台线程（Interlocked/SetEvent 均无锁、零分配）
                Interlocked.Exchange(ref _pendingUnloadBase, data->DllBase);
                _wakeEvent?.Set();
            }
        }
        catch
        {
            // 回调内禁止任何异常逃逸（会终止进程）
        }
    }

    private static unsafe int CopyUnicodeString(LdrUnicodeString* source, char* destination, int maxChars)
    {
        if (source is null || source->Buffer is null || source->Length == 0)
        {
            return 0;
        }

        int count = Math.Min(source->Length / 2, maxChars - 1);
        Buffer.MemoryCopy(source->Buffer, destination, (long)maxChars * 2, (long)count * 2);
        destination[count] = '\0';
        return count;
    }

    private static unsafe bool MatchesCallbackRules(char* baseNameLower, int length)
    {
        foreach (string exact in _callbackDllNameExact)
        {
            if (length == exact.Length && EqualsAt(baseNameLower, exact))
            {
                return true;
            }
        }

        foreach (string keyword in _callbackDllNameKeywords)
        {
            if (ContainsOrdinalIgnoreCase(baseNameLower, length, keyword))
            {
                return true;
            }
        }

        return false;
    }

    private static unsafe bool EqualsAt(char* baseNameLower, string lowerTarget)
    {
        for (int i = 0; i < lowerTarget.Length; ++i)
        {
            if (baseNameLower[i] != lowerTarget[i])
            {
                return false;
            }
        }

        return true;
    }

    private static unsafe bool ContainsOrdinalIgnoreCase(char* haystackLower, int haystackLen, string needleLower)
    {
        if (needleLower.Length == 0 || needleLower.Length > haystackLen)
        {
            return false;
        }

        int needleLen = needleLower.Length;
        for (int i = 0; i <= haystackLen - needleLen; ++i)
        {
            bool matched = true;
            for (int j = 0; j < needleLen; ++j)
            {
                if (haystackLower[i + j] != needleLower[j])
                {
                    matched = false;
                    break;
                }
            }

            if (matched)
            {
                return true;
            }
        }

        return false;
    }

    private static char ToLowerInvariant(char c)
    {
        return c is >= 'A' and <= 'Z' ? (char)(c + 32) : c;
    }

    private static void BackgroundLoop()
    {
        try
        {
            WaitHandle[] waitHandles = [_wakeEvent!, _shutdownEvent!];
            while (true)
            {
                int signaledIndex = WaitHandle.WaitAny(waitHandles);
                if (signaledIndex == 1)
                {
                    return;
                }

                IntPtr moduleBase = Interlocked.Exchange(ref _pendingUnloadBase, IntPtr.Zero);
                if (moduleBase == IntPtr.Zero)
                {
                    continue;
                }

                string? path = GetModulePath(moduleBase);
                HandleDetected(moduleBase, path ?? string.Empty, "LdrRegisterDllNotification");
            }
        }
        catch (Exception e)
        {
            _logger.Error(e, "InjectionGuard background loop stopped unexpectedly");
        }
    }

    private static void PollingTick()
    {
        try
        {
            // 枚举当前进程全部已加载模块，兜底覆盖监听注册前已注入的模块与正则规则
            foreach ((IntPtr moduleBase, string path) in EnumerateLoadedModules())
            {
                string fileName = System.IO.Path.GetFileName(path);
                if (!MatchesFullRules(fileName, path))
                {
                    continue;
                }

                HandleDetected(moduleBase, path, "Polling");
            }
        }
        catch (Exception e)
        {
            _logger.Error(e, "InjectionGuard polling failed");
        }
    }

    private static IEnumerable<(IntPtr Base, string Path)> EnumerateLoadedModules()
    {
        var result = new List<(IntPtr, string)>();

        IntPtr hProcess = GetCurrentProcess();
        IntPtr[] modules = new IntPtr[512];
        while (true)
        {
            if (!K32EnumProcessModulesEx(hProcess, modules, (uint)(modules.Length * IntPtr.Size), out uint cbNeeded, ListModulesAll))
            {
                _logger.Warning(
                    "InjectionGuard failed to enumerate modules, GetLastError={ErrorCode}",
                    Marshal.GetLastWin32Error());
                break;
            }

            if (cbNeeded <= modules.Length * IntPtr.Size)
            {
                int count = (int)(cbNeeded / IntPtr.Size);
                for (int i = 0; i < count; ++i)
                {
                    result.Add((modules[i], GetModulePath(modules[i]) ?? string.Empty));
                }

                break;
            }

            modules = new IntPtr[(int)(cbNeeded / IntPtr.Size) + 16];
        }

        return result;
    }

    private static string? GetModulePath(IntPtr moduleBase)
    {
        char[] buffer = new char[MaxModulePathChars];
        int len = GetModuleFileNameW(moduleBase, buffer, buffer.Length);
        return len > 0 ? new string(buffer, 0, len) : null;
    }

    private static bool MatchesFullRules(string fileName, string path)
    {
        string fileNameLower = fileName.ToLowerInvariant();
        string pathLower = path.ToLowerInvariant();

        foreach (InjectionRule rule in _rules)
        {
            if (rule.DllNameExact.Any(n => string.Equals(fileNameLower, n, StringComparison.OrdinalIgnoreCase)))
            {
                return true;
            }

            if (rule.DllNameKeywords.Any(k => fileNameLower.Contains(k, StringComparison.OrdinalIgnoreCase)))
            {
                return true;
            }

            if (rule.PathKeywords.Any(k => pathLower.Contains(k, StringComparison.OrdinalIgnoreCase)))
            {
                return true;
            }

            if (rule.PathRegex.Any(r => Regex.IsMatch(path, r, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant)))
            {
                return true;
            }
        }

        return false;
    }

    private static void HandleDetected(IntPtr moduleBase, string path, string source)
    {
        // 仅日志与弹窗按路径去重；卸载始终执行（幂等），
        // 因为被卸载的 DLL 可能再次被注入（如 Nahimic 周期性重新注入）
        bool isFirstHit = false;
        lock (_handledDllsLock)
        {
            isFirstHit = _handledDlls.Add(path);
        }

        string moduleName = string.IsNullOrEmpty(path) ? $"0x{moduleBase.ToInt64():X}" : System.IO.Path.GetFileName(path);
        if (moduleBase != IntPtr.Zero)
        {
            // 卸载目标 DLL。若它正在被其他线程执行代码，可能失败，仅记录。
            if (!FreeLibrary(moduleBase))
            {
                _logger.Error(
                    "InjectionGuard failed to unload injected DLL {ModuleName}, GetLastError={ErrorCode}",
                    moduleName,
                    Marshal.GetLastWin32Error());
            }
            else
            {
                _logger.Information("InjectionGuard unloaded injected DLL {ModuleName}", moduleName);
            }
        }

        if (!isFirstHit)
        {
            return;
        }

        _logger.Warning("InjectionGuard [{Source}] detected injected DLL: {Path}", source, path);

        // 首次命中时弹窗提示（弹到 UI 线程执行）
        try
        {
            Application.Current?.Dispatcher.BeginInvoke(() => ShowWarning(path));
        }
        catch (Exception e)
        {
            _logger.Warning(e, "InjectionGuard failed to post warning dialog");
        }
    }

    private static void ShowWarning(string path)
    {
        string message = LocalizationHelper.GetStringFormat("InjectionGuard.Warning.Text", path);
        try
        {
            MessageBoxHelper.Show(
                message,
                LocalizationHelper.GetString("InjectionGuard.Warning.Heading"),
                MessageBoxButton.OK,
                MessageBoxImage.Warning);
        }
        catch (Exception e)
        {
            // 注入环境下 WPF 弹窗可能崩溃，退回原生 Win32 MessageBox
            _logger.Warning(e, "InjectionGuard warning dialog failed, falling back to native MessageBox");
            try
            {
                _ = PInvoke.MessageBox(
                    HWND.Null,
                    message + "\n\n" + LocalizationHelper.GetString("BadModules.Warning.Fallback"),
                    LocalizationHelper.GetString("InjectionGuard.Warning.Heading"),
                    MESSAGEBOX_STYLE.MB_ICONERROR | MESSAGEBOX_STYLE.MB_OK);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "InjectionGuard native MessageBox also failed");
            }
        }
    }

    /// <summary>
    /// 一条注入检测规则：命中任一条件即视为命中。
    /// 可通过 <see cref="DllNameKeywords"/> / <see cref="PathKeywords"/> 按关键字扩展
    /// （例如新增某个 OSD 软件时，将其 DLL 名关键字加入即可）。
    /// </summary>
    private sealed class InjectionRule
    {
        /// <summary>
        /// Gets or sets the rule name (for logging).
        /// </summary>
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the exact DLL file names to match, e.g. "NahimicOSD.dll".
        /// </summary>
        public string[] DllNameExact { get; set; } = [];

        /// <summary>
        /// Gets or sets the DLL file name keywords to match (case-insensitive substring).
        /// </summary>
        public string[] DllNameKeywords { get; set; } = [];

        /// <summary>
        /// Gets or sets the full path keywords to match (case-insensitive substring).
        /// </summary>
        public string[] PathKeywords { get; set; } = [];

        /// <summary>
        /// Gets or sets the full path regular expressions to match.
        /// </summary>
        public string[] PathRegex { get; set; } = [];
    }
}
