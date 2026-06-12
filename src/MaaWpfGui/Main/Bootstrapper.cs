// <copyright file="Bootstrapper.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Threading;
using GlobalHotKey;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Properties;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.Services.Web;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.Views.Dialogs;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;
using Serilog;
using Serilog.Core;
using Serilog.Events;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.Main;

/// <summary>
/// The bootstrapper.
/// </summary>
public class Bootstrapper : Bootstrapper<RootViewModel>
{
    private static ILogger _logger = Logger.None;

    private static Mutex _mutex;
    private static bool _hasMutex;
    private static EventWaitHandle _instanceActivationEvent;
    private static CancellationTokenSource _instanceActivationListenerCancellation;

    public static readonly string UiLogFile = Path.Combine(PathsHelper.DebugDir, "gui.log");
    public static readonly string UiLogBakFile = Path.Combine(PathsHelper.DebugDir, "gui.bak.log");
    public static readonly string CoreLogFile = Path.Combine(PathsHelper.DebugDir, "asst.log");
    public static readonly string CoreLogBakFile = Path.Combine(PathsHelper.DebugDir, "asst.bak.log");

    [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern IntPtr LoadLibrary(string dllName);

    [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
    private static extern bool FreeLibrary(IntPtr hModule);

    private static List<string> UnknownDllDetected()
    {
        try
        {
            // 属于 MAA 的 DLL 列表
            // 因为经常有人把 MAA 和别的东西解压到一起然后发生 DLL 劫持然后报错，遂检测
            var maaDlls = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "hostfxr.dll",
                "hostpolicy.dll",
                "libloader.dll",
                "DirectML.dll",
                "fastdeploy_ppocr.dll",
                "MaaCore.dll",
                "onnxruntime_maa.dll",
                "opencv_world4_maa.dll",
            };

            var currentDirectory = AppDomain.CurrentDomain.BaseDirectory;

            var dllFiles = Directory.GetFiles(currentDirectory, "*.dll");

            return [.. dllFiles
                .Select(Path.GetFileName)
                .Where(fileName => !maaDlls.Contains(fileName) && !fileName.Contains("maa", StringComparison.OrdinalIgnoreCase))];
        }
        catch (Exception)
        {
            return [];
        }
    }

    private static bool IsVCppInstalled()
    {
        IntPtr handle = IntPtr.Zero;
        try
        {
            handle = LoadLibrary("MaaCore.dll");

            // 如果句柄非空，说明 DLL 存在且可加载
            return handle != IntPtr.Zero;
        }
        catch (Exception)
        {
            return false;
        }
        finally
        {
            if (handle != IntPtr.Zero)
            {
                FreeLibrary(handle); // 释放 DLL 句柄
            }
        }
    }

    public static bool IsWritable(string path)
    {
        try
        {
            string testFile = Path.Combine(path, "write_test.tmp");
            File.WriteAllText(testFile, "test");
            File.Delete(testFile);
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static readonly Environment.SpecialFolder[] s_unsupportedInstallLocationSpecialFolders =
    {
        Environment.SpecialFolder.CommonApplicationData,
        Environment.SpecialFolder.ApplicationData,
        Environment.SpecialFolder.LocalApplicationData,
        Environment.SpecialFolder.CommonProgramFiles,
        Environment.SpecialFolder.CommonProgramFilesX86,
        Environment.SpecialFolder.ProgramFiles,
        Environment.SpecialFolder.ProgramFilesX86,
        Environment.SpecialFolder.UserProfile,
        Environment.SpecialFolder.Windows,
    };

    private static bool TryGetUnsupportedInstallLocation(out string matchedLocation)
    {
        matchedLocation = string.Empty;

        try
        {
            string currentPath = NormalizeDirectoryPath(AppDomain.CurrentDomain.BaseDirectory);
            if (IsDriveRootDirectory(currentPath))
            {
                matchedLocation = currentPath;
                return true;
            }

            if (TryGetTempInstallLocation(currentPath, out matchedLocation))
            {
                return true;
            }

            foreach (string unsupportedLocation in GetUnsupportedInstallLocationPaths())
            {
                if (string.Equals(currentPath, unsupportedLocation, StringComparison.OrdinalIgnoreCase))
                {
                    matchedLocation = unsupportedLocation;
                    return true;
                }
            }

            return false;
        }
        catch
        {
            return false;
        }
    }

    private static bool TryGetTempInstallLocation(string currentPath, out string matchedLocation)
    {
        matchedLocation = string.Empty;

        foreach (string tempPath in GetTempDirectoryPaths())
        {
            if (IsPathUnderDirectory(currentPath, tempPath))
            {
                matchedLocation = tempPath;
                return true;
            }
        }

        string currentDirectoryName = Path.GetFileName(currentPath);
        if (IsTempLikeDirectoryName(currentDirectoryName))
        {
            matchedLocation = currentPath;
            return true;
        }

        DirectoryInfo parent = Directory.GetParent(currentPath);
        if (parent != null && IsTempLikeDirectoryName(parent.Name))
        {
            matchedLocation = NormalizeDirectoryPath(parent.FullName);
            return true;
        }

        return false;
    }

    private static bool IsTempLikeDirectoryName(string directoryName)
    {
        return !string.IsNullOrWhiteSpace(directoryName)
            && (directoryName.StartsWith("temp", StringComparison.OrdinalIgnoreCase)
                || directoryName.StartsWith("tmp", StringComparison.OrdinalIgnoreCase));
    }

    private static HashSet<string> GetTempDirectoryPaths()
    {
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        AddCandidateDirectoryPath(paths, Path.GetTempPath());

        string[] envVars = ["TEMP", "TMP", "TMPDIR"];
        foreach (string envVar in envVars)
        {
            AddCandidateDirectoryPath(paths, Environment.GetEnvironmentVariable(envVar) ?? string.Empty);
        }

        return paths;
    }

    private static HashSet<string> GetUnsupportedInstallLocationPaths()
    {
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (Environment.SpecialFolder specialFolder in s_unsupportedInstallLocationSpecialFolders)
        {
            AddCandidateDirectoryPath(paths, Environment.GetFolderPath(specialFolder));
        }

        string commonDocumentsPath = Environment.GetFolderPath(Environment.SpecialFolder.CommonDocuments);
        string publicUserPath = Directory.GetParent(NormalizeDirectoryPath(commonDocumentsPath))?.FullName ?? string.Empty;
        AddCandidateDirectoryPath(paths, publicUserPath);

        string windowsPath = Environment.GetFolderPath(Environment.SpecialFolder.Windows);
        AddCandidateDirectoryPath(paths, Path.Combine(windowsPath, "System32", "Drivers", "DriverData"));

        return paths;
    }

    private static void AddCandidateDirectoryPath(HashSet<string> paths, string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        try
        {
            paths.Add(NormalizeDirectoryPath(path));
        }
        catch
        {
        }
    }

    private static string NormalizeDirectoryPath(string path)
    {
        return Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
    }

    private static bool IsDriveRootDirectory(string currentPath)
    {
        string rootPath = Path.GetPathRoot(currentPath)?.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return !string.IsNullOrEmpty(rootPath)
            && rootPath.Length == 2
            && rootPath[1] == ':'
            && string.Equals(currentPath, rootPath, StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsPathUnderDirectory(string currentPath, string parentPath)
    {
        return EnsureTrailingSeparator(currentPath)
            .StartsWith(EnsureTrailingSeparator(parentPath), StringComparison.OrdinalIgnoreCase);
    }

    private static string EnsureTrailingSeparator(string path)
    {
        return path.Length > 0 && (path[^1] == Path.DirectorySeparatorChar || path[^1] == Path.AltDirectorySeparatorChar)
            ? path
            : path + Path.DirectorySeparatorChar;
    }

    public static void ParseCrashLog()
    {
        var crashFile = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "crash.log");
        if (!File.Exists(crashFile))
        {
            return;
        }

        try
        {
            var localAppData = Environment.GetEnvironmentVariable("LocalAppData");
            if (localAppData is not null && Directory.Exists($"{localAppData}/CrashDumps"))
            {
                var crashDumpsSource = Path.Combine(localAppData, "CrashDumps");
                var dumpDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "debug", "dumps");
                if (Directory.Exists(dumpDir))
                {
                    Directory.Delete(dumpDir, true);
                }
                Directory.CreateDirectory(dumpDir);

                var time = File.GetLastWriteTime(crashFile);
                bool foundDump = false;
                foreach (var file in new DirectoryInfo(crashDumpsSource).EnumerateFiles("MAA.exe.*.dmp"))
                {
                    if (file.LastWriteTime >= time.AddMinutes(-10) && file.LastWriteTime <= time.AddMinutes(10))
                    {
                        _logger.Information("Found crash dump file: {CrashDumpFile}", file.FullName);
                        File.Copy(file.FullName, Path.Combine(dumpDir, file.Name), true);
                        foundDump = true;
                    }
                }
                if (foundDump)
                {
                    _logger.Information("Crash dumps are copied to {DumpDir}", dumpDir);
                }
            }
            else
            {
                _logger.Information("%LocalAppData%/CrashDumps not found");
            }

            string[] lines = File.ReadAllLines(crashFile, Encoding.UTF8);

            StringBuilder message = new StringBuilder();
            string currentReason = null;
            string currentDetail = null;

            foreach (var line in lines)
            {
                if (line.StartsWith("Reason: "))
                {
                    currentReason = line[7..].Trim();
                }
                else if (line.StartsWith("Detail: "))
                {
                    currentDetail = line[8..].Trim();
                }
                else if (line.StartsWith("==================="))
                {
                    if (!string.IsNullOrEmpty(currentReason))
                    {
                        message.AppendLine($"Reason: {currentReason}");
                        if (!string.IsNullOrEmpty(currentDetail))
                        {
                            message.AppendLine($"Detail: {currentDetail}");
                        }

                        message.AppendLine();
                    }

                    currentReason = null;
                    currentDetail = null;
                }
            }

            if (message.Length > 0)
            {
                message.AppendLine(LocalizationHelper.GetString("ErrorCrashMessageHeader"));
                message.AppendLine();
                message.AppendLine(LocalizationHelper.GetString("ErrorCrashMessageOpenLog"));
                message.AppendLine(LocalizationHelper.GetString("ErrorCrashMessageGenerateReport"));
                message.AppendLine();
                message.AppendLine(LocalizationHelper.GetString("ErrorCrashMessageHelpTip"));

                _logger.Warning(message.ToString());

                MessageBoxHelper.Show(
                    message.ToString(),
                    LocalizationHelper.GetString("ErrorCrashDialogTitle"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }

            try
            {
                File.Delete(crashFile);
            }
            catch
            {
                // ignored
            }
        }
        catch
        {
            // ignored
        }
    }

    /// <inheritdoc/>
    /// <remarks>初始化些啥自己加。</remarks>
    protected override void OnStart()
    {
        Directory.SetCurrentDirectory(AppContext.BaseDirectory);
        if (!Directory.Exists("debug"))
        {
            Directory.CreateDirectory("debug");
        }

        if (File.Exists(UiLogFile) && new FileInfo(UiLogFile).Length > 4 * 1024 * 1024)
        {
            if (File.Exists(UiLogBakFile))
            {
                File.Delete(UiLogBakFile);
            }

            File.Move(UiLogFile, UiLogBakFile);
        }

        // Bootstrap serilog
        var loggerConfiguration = new LoggerConfiguration()
            .WriteTo.Debug(outputTemplate: "[{Timestamp:HH:mm:ss}][{Level:u3}]{ClassName} <{ThreadId}> {Message:lj}{NewLine}{Exception}")
            .WriteTo.File(
                UiLogFile,
                outputTemplate: "[{Timestamp:yyyy-MM-dd HH:mm:ss.fff}][{Level:u3}]{ClassName} <{ThreadId}> {Message:lj}{NewLine}{Exception}")
            .Enrich.With<ClassNameEnricher>()
            .Enrich.FromLogContext()
            .Enrich.WithThreadId()
            .Enrich.WithThreadName();

        var uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";
        uiVersion = uiVersion == "0.0.1" ? "DEBUG_VERSION" : uiVersion;
        var builtDate = Assembly.GetExecutingAssembly().GetCustomAttribute<BuildDateTimeAttribute>()?.BuildTime.ToLocalTime() ?? DateTimeOffset.MinValue;
        var maaEnv = Environment.GetEnvironmentVariable("MAA_ENVIRONMENT") == "Debug"
            ? "Debug"
            : "Production";
        var args = Environment.GetCommandLineArgs();
        var withDebugFile = File.Exists("DEBUG") || File.Exists("DEBUG.txt");
        loggerConfiguration = (maaEnv == "Debug" || withDebugFile)
            ? loggerConfiguration.MinimumLevel.Verbose()
            : loggerConfiguration.MinimumLevel.Information();
        var workingDirectory = PathsHelper.BaseDir;
        var folderName = Path.GetFileName(workingDirectory.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
        var isBuildOutputFolder =
            string.Equals(folderName, "Release", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(folderName, "Debug", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(folderName, "RelWithDebInfo", StringComparison.OrdinalIgnoreCase);

        Log.Logger = loggerConfiguration.CreateLogger();
        _logger = Log.Logger.ForContext<Bootstrapper>();
        _logger.Information("===================================");
        _logger.Information("MaaAssistantArknights GUI started");
        _logger.Information("Version {UiVersion}", uiVersion);
        _logger.Information("Built at {BuiltDate:O}", builtDate);
        _logger.Information("Maa ENV: {MaaEnv}", maaEnv);
        _logger.Information("Command Line: {Join}", string.Join(' ', args));
        _logger.Information("User Dir {BaseDirectory}", workingDirectory);
        if (withDebugFile)
        {
            _logger.Information("Start with DEBUG file");
        }

        if (IsAdministratorWithUac())
        {
            _logger.Information("Run as Administrator");
        }

        if (WineRuntimeInformation.IsRunningUnderWine)
        {
            _logger.Information("Running under Wine {WineVersion} on {HostSystemName}", WineRuntimeInformation.WineVersion, WineRuntimeInformation.HostSystemName);
            RenderOptions.ProcessRenderMode = RenderMode.SoftwareOnly;
            _logger.Information("MaaWineBridge status: {WineBridgeAvailability}", MaaWineBridge.Availability);
            _logger.Information("MaaDesktopIntegration available: {Available}", MaaDesktopIntegration.Available);
        }

        _logger.Information("===================================");

        ConfigurationHelper.Load();
        LocalizationHelper.Load();
        if (PendingUpdateApplier.TryConsumeDelegatedUpdateSuccess())
        {
            _logger.Information("Delegated pending update completed successfully");
        }

        if (PendingUpdateApplier.TryConsumeDelegatedUpdateFailure(out string delegatedUpdateFailureReason))
        {
            _logger.Error("Delegated pending update failed. Reason: {Reason}", delegatedUpdateFailureReason);
            ShowPendingUpdateRecoveryDialog();
            Shutdown();
            return;
        }

        if (TryGetUnsupportedInstallLocation(out string unsupportedLocation))
        {
            string currentBaseDirectory = NormalizeDirectoryPath(AppDomain.CurrentDomain.BaseDirectory);
            _logger.Error(
                "Blocked startup from unsupported install location: currentPath={CurrentPath}, matchedLocation={MatchedLocation}",
                currentBaseDirectory,
                unsupportedLocation);
            MessageBoxHelper.Show(
                LocalizationHelper.GetStringFormat("UnsupportedInstallLocationError", currentBaseDirectory, unsupportedLocation),
                LocalizationHelper.GetString("Error"),
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            Shutdown();
            return;
        }

        if (PendingUpdateApplier.HasPendingUpdatePackage())
        {
            _logger.Information("Pending update package detected, applying before full startup");
            var pendingUpdateResult = PendingUpdateApplier.TryApplyPendingUpdatePackage();
            if (pendingUpdateResult.Delegated)
            {
                _logger.Information("Pending update package handed off to external updater, exiting current process");
                Shutdown();
                return;
            }

            if (pendingUpdateResult.Succeeded)
            {
                RestartAfterPendingUpdateEarly();
                return;
            }

            if (pendingUpdateResult.Status == PendingUpdateApplyResult.StatusKind.MissingUpdaterExecutable)
            {
                _logger.Error("Pending update package could not be delegated because MAA.Updater.exe is missing. Reason: {Reason}", pendingUpdateResult.FailureReason);
                ShowPendingUpdateMissingUpdaterDialog();
                Shutdown();
                return;
            }

            if (pendingUpdateResult.RequiresManualRecovery)
            {
                _logger.Error("Pending update package left the installation in an incomplete state. Reason: {Reason}", pendingUpdateResult.FailureReason);
                ShowPendingUpdateRecoveryDialog();
                Shutdown();
                return;
            }

            _logger.Warning("Pending update package could not be applied, continuing with normal startup");
        }

        ConfigConverter.ConvertConfig();
        ETagCache.Load();

        if (ConfigFactory.Root.GUI.IgnoreBadModulesAndUseSoftwareRendering)
        {
            RenderOptions.ProcessRenderMode = RenderMode.SoftwareOnly;
            _logger.Information("Using software rendering mode due to user preference (bad modules detected)");
        }

        // 检查 MaaCore.dll 是否存在
        if (!File.Exists("MaaCore.dll"))
        {
            throw new FileNotFoundException("MaaCore.dll not found!");
        }

        // 检查 resource 文件夹是否存在
        if (!Directory.Exists(PathsHelper.ResourceDir))
        {
            throw new DirectoryNotFoundException("resource folder not found!");
        }

        // Debug 模式下 DLL 是未打包的
        if (maaEnv != "Debug" && !isBuildOutputFolder)
        {
            var unknownDlls = UnknownDllDetected();
            if (unknownDlls.Count > 0)
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("UnknownDllDetected") + "\n" + string.Join("\n", unknownDlls),
                    "MAA",
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
                _logger.Fatal("Unknown DLL(s) detected: {UnknownDlls}", string.Join(", ", unknownDlls));
                Shutdown();
                return;
            }
        }

        if (!IsVCppInstalled())
        {
            var ret = MessageBoxHelper.Show(
                LocalizationHelper.GetString("VC++NotInstalled"),
                "MAA",
                MessageBoxButton.OKCancel,
                MessageBoxImage.Information,
                ok: LocalizationHelper.GetString("Confirm"),
                cancel: LocalizationHelper.GetString("Cancel"));
            if (ret == MessageBoxResult.OK)
            {
                var startInfo = new ProcessStartInfo {
                    FileName = "DependencySetup_依赖库安装.bat",
                    WorkingDirectory = AppDomain.CurrentDomain.BaseDirectory, // 设置工作目录
                    WindowStyle = ProcessWindowStyle.Normal, // 显示窗口让用户看到进度
                };

                Process.Start(startInfo);
            }

            Shutdown();
            return;
        }

        if (!HandleMultipleInstances())
        {
            Shutdown();
            return;
        }

        if (!IsWritable(PathsHelper.BaseDir))
        {
            Task.Run(() => MessageBoxHelper.Show(LocalizationHelper.GetString("SoftwareLocationWarning"), LocalizationHelper.GetString("Error"), MessageBoxButton.OK, MessageBoxImage.Error));
        }

        Task.Run(ParseCrashLog);

        base.OnStart();
        _hasMutex = true;

        const string ConfigFlag = "--config";
        const string AnotherFlag = "--another"; // 示例，之后如果有其他参数，可以继续添加

        var parsedArgs = ParseArgs(args, ConfigFlag, AnotherFlag);

        if (parsedArgs.TryGetValue(ConfigFlag, out string configArgs) && Config(configArgs))
        {
            // return;
        }
    }

    public class ClassNameEnricher : ILogEventEnricher
    {
        public void Enrich(LogEvent logEvent, ILogEventPropertyFactory propertyFactory)
        {
            if (!logEvent.Properties.TryGetValue("SourceContext", out var sourceContextValue))
            {
                return;
            }

            var sourceContext = sourceContextValue.ToString().Trim('"');
            var className = sourceContext.Split('.').Last();
            className = ("[" + className + "]").PadRight(24);
            logEvent.AddOrUpdateProperty(propertyFactory.CreateProperty("ClassName", className));
        }
    }

    protected override void OnLaunch()
    {
        BadModules.CheckAndWarnBadInjectedModules();
    }

    private static bool HandleMultipleInstances()
    {
        string activationEventName = "MAA_SHOW_" + InstanceKey;
        _mutex = new Mutex(true, MutexName, out var isOnlyInstance);

        try
        {
            if (isOnlyInstance || _mutex.WaitOne(500))
            {
                EnsureInstanceActivationEvent(activationEventName);
                return true;
            }

            if (SignalExistingInstance(activationEventName))
            {
                return false;
            }

            MessageBoxHelper.Show(LocalizationHelper.GetString("MultiInstanceUnderSamePath"), "MAA", MessageBoxButton.OK, MessageBoxImage.Warning);
            return false;
        }
        catch (AbandonedMutexException)
        {
            // 上一个程序没有正常释放互斥量
            // 即使捕获到这个异常，此时也已经获得了锁
            EnsureInstanceActivationEvent(activationEventName);
            return true;
        }
        catch (Exception e)
        {
            MessageBoxHelper.Show(LocalizationHelper.GetString("MultiInstanceUnderSamePath") + e.Message, "MAA", MessageBoxButton.OK, MessageBoxImage.Warning);
            return false;
        }
    }

    public static string InstanceKey
    {
        get
        {
            var normalizedBaseDir = Path.GetFullPath(PathsHelper.BaseDir)
                .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
                .ToUpperInvariant();
            return normalizedBaseDir.StableHash();
        }
    }

    public static string MutexName => "MAA_" + InstanceKey;

    private static void EnsureInstanceActivationEvent(string activationEventName)
    {
        _instanceActivationEvent ??= new EventWaitHandle(false, EventResetMode.AutoReset, activationEventName);
    }

    private static bool SignalExistingInstance(string activationEventName)
    {
        try
        {
            using var activationEvent = EventWaitHandle.OpenExisting(activationEventName);
            activationEvent.Set();
            _logger.Information("Secondary launch detected, activation signal sent to existing instance");
            return true;
        }
        catch (WaitHandleCannotBeOpenedException)
        {
            _logger.Warning("Secondary launch detected, but no activation listener was available");
            return false;
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Failed to signal the existing instance");
            return false;
        }
    }

    private static void StartInstanceActivationListener()
    {
        if (_instanceActivationEvent == null || _instanceActivationListenerCancellation != null)
        {
            return;
        }

        _instanceActivationListenerCancellation = new CancellationTokenSource();
        _ = Task.Run(() => ListenForInstanceActivation(_instanceActivationListenerCancellation.Token));
    }

    private static void ListenForInstanceActivation(CancellationToken cancellationToken)
    {
        if (_instanceActivationEvent == null)
        {
            return;
        }

        WaitHandle[] waitHandles = [_instanceActivationEvent, cancellationToken.WaitHandle];

        try
        {
            while (true)
            {
                int signaledIndex = WaitHandle.WaitAny(waitHandles);
                if (signaledIndex != 0)
                {
                    return;
                }

                Application.Current?.Dispatcher.BeginInvoke(new Action(ActivateMainWindow), DispatcherPriority.Normal);
            }
        }
        catch (ObjectDisposedException)
        {
            // ignored during shutdown
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Existing instance activation listener stopped unexpectedly");
        }
    }

    private static void ActivateMainWindow()
    {
        if (Application.Current == null || Application.Current.IsShuttingDown())
        {
            return;
        }

        Instances.MainWindowManager.Show();
        _logger.Information("Existing instance window activated by a secondary launch");
    }

    public static bool IsUserAdministrator() => new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);

    public static bool IsUacEnabled()
    {
        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System");
            if (key == null)
            {
                return true;
            }

            var value = key.GetValue("EnableLUA");
            if (value is int intValue)
            {
                return intValue != 0;
            }

            return true;
        }
        catch
        {
            return true;
        }
    }

    public static bool IsAdministratorWithUac() => IsUserAdministrator() && IsUacEnabled();

    /// <inheritdoc/>
    protected override void ConfigureIoC(IStyletIoCBuilder builder)
    {
        builder.Bind<TaskQueueViewModel>().ToSelf().InSingletonScope();
        builder.Bind<CopilotViewModel>().ToSelf().InSingletonScope();
        builder.Bind<ToolboxViewModel>().ToSelf().InSingletonScope();
        builder.Bind<SettingsViewModel>().ToSelf().InSingletonScope();

        builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
        builder.Bind<StageManager>().ToSelf().InSingletonScope();

        builder.Bind<HotKeyManager>().ToSelf().InSingletonScope();

        builder.Bind<IMaaHotKeyManager>().To<MaaHotKeyManager>().InSingletonScope();
        builder.Bind<IMaaHotKeyActionHandler>().To<MaaHotKeyActionHandler>().InSingletonScope();

        builder.Bind<RemoteControlService>().To<RemoteControlService>().InSingletonScope();

        builder.Bind<IMainWindowManager>().To<MainWindowManager>().InSingletonScope();

        builder.Bind<IHttpService>().To<HttpService>().InSingletonScope();
        builder.Bind<IMaaApiService>().To<MaaApiService>().InSingletonScope();

        builder.Bind<OverlayViewModel>().ToSelf().InSingletonScope();
    }

    protected override void Configure()
    {
        base.Configure();
        Instances.Instantiate(Container);
    }

    /// <inheritdoc/>
    protected override void DisplayRootView(object rootViewModel)
    {
        if (Application.Current.IsShuttingDown())
        {
            return;
        }

        bool wasFirstBoot = Instances.VersionUpdateDialogViewModel.IsFirstBootAfterUpdate;

        Instances.WindowManager.ShowWindow(rootViewModel);
        Instances.InstantiateOnRootViewDisplayed(Container);
        StartInstanceActivationListener();

        // 如果 IsFirstBootAfterUpdate 从 false 变为 true，说明这次启动只是解压更新包，不用执行后续逻辑
        if (!wasFirstBoot && Instances.VersionUpdateDialogViewModel.IsFirstBootAfterUpdate)
        {
            return;
        }

        AchievementTrackerHelper.Events.Startup();

        var buildTimeInterval = (DateTimeOffset.UtcNow - VersionUpdateSettingsUserControlModel.BuildDateTime).TotalDays;
        var resourceTimeInterval = (DateTimeOffset.UtcNow - SettingsViewModel.VersionUpdateSettings.ResourceDateTime).TotalDays;
        var maxTimeInterval = Math.Max(buildTimeInterval, resourceTimeInterval);
        if (maxTimeInterval > 90)
        {
            Instances.TaskQueueViewModel.LogItemViewModels.Add(new(LocalizationHelper.GetStringFormat("Achievement.Martian.ConditionsTip", (maxTimeInterval / 30.436875).ToString("F2")), UiLogColor.Error));
        }
    }

    /// <inheritdoc/>
    /// <remarks>退出时执行啥自己加。</remarks>
    protected override void OnExit(ExitEventArgs e)
    {
        // MessageBox.Show("O(∩_∩)O 拜拜");
        try
        {
            Instances.TaskQueueViewModel.ResetAllTemporaryVariable(false);
        }
        catch
        {
            // ignored
        }

        Release();

        _logger.Information("MaaAssistantArknights GUI exited");
        _logger.Information("{Message}", string.Empty);
        Log.CloseAndFlush();
        base.OnExit(e);

        if (!_isRestartingAfterUpdate)
        { // 如果是更新后重启，则不删除 .old
            try
            { // 退出时移除.old
                Directory.Delete(".old", true);
            }
            catch (Exception)
            { // ignored
            }

            foreach (var file in new DirectoryInfo(".").EnumerateFiles("*.old"))
            {
                try
                {
                    file.Delete();
                }
                catch (Exception)
                { // ignored
                }
            }

            // 清理残留的 OTA 更新包临时文件
            foreach (var file in new DirectoryInfo(".").EnumerateFiles("MAAComponent-OTA*.temp"))
            {
                try
                {
                    file.Delete();
                }
                catch (Exception)
                { // ignored
                }
            }
        }

        if (_restartStartInfo is not null)
        {
            Process.Start(_restartStartInfo);
            return;
        }

        if (!_isRestartingWithoutArgs)
        {
            return;
        }

        if (Environment.ProcessPath is null)
        {
            return;
        }

        ProcessStartInfo startInfo = new ProcessStartInfo { FileName = Environment.ProcessPath, };

        Process.Start(startInfo);
    }

    public static void Release()
    {
        _instanceActivationListenerCancellation?.Cancel();
        _instanceActivationListenerCancellation?.Dispose();
        _instanceActivationListenerCancellation = null;

        _instanceActivationEvent?.Dispose();
        _instanceActivationEvent = null;

        ETagCache.Save();
        Instances.SettingsViewModel.Sober();
        Instances.MaaHotKeyManager.Release();

        // 关闭程序时清理操作中心中的通知
        ToastNotification.Cleanup();

        ConfigurationHelper.Release();
        ConfigFactory.Release();

        // 释放互斥量
        if (!_hasMutex)
        {
            return;
        }

        _mutex?.ReleaseMutex();
        _mutex?.Dispose();
    }

    private static bool _isRestartingWithoutArgs;
    private static ProcessStartInfo _restartStartInfo;

    /// <summary>
    /// 在完整 GUI 尚未初始化前，应用待处理更新后立即重启。
    /// </summary>
    private static void RestartAfterPendingUpdateEarly()
    {
        _logger.Information("Pending update package applied, restarting application");
        if (Environment.ProcessPath is not null)
        {
            Process.Start(new ProcessStartInfo {
                FileName = Environment.ProcessPath,
                WorkingDirectory = AppDomain.CurrentDomain.BaseDirectory,
                UseShellExecute = true,
            });
        }

        Environment.Exit(0);
    }

    private static void ShowPendingUpdateRecoveryDialog()
    {
        MessageBoxHelper.Show(
            LocalizationHelper.GetString("UpdateApplyFailed"),
            LocalizationHelper.GetString("Error"),
            icon: MessageBoxImage.Error);
    }

    private static void ShowPendingUpdateMissingUpdaterDialog()
    {
        MessageBoxHelper.Show(
        LocalizationHelper.GetString("UpdateApplyMissingUpdater"),
        LocalizationHelper.GetString("Error"),
        icon: MessageBoxImage.Error);
    }

    /// <summary>
    /// 重启，不带参数
    /// </summary>
    /// <param name="caller">Caller Member Name</param>
    public static void ShutdownAndRestartWithoutArgs([CallerMemberName] string caller = "")
    {
        _isRestartingWithoutArgs = true;
        _logger.Information("Shutdown and restart without Args, call by `{Caller}`", caller);
        Execute.OnUIThread(Application.Current.Shutdown);
    }

    /// <summary>
    /// 重启，使用自定义启动参数
    /// </summary>
    /// <param name="startInfo">新进程的启动参数。</param>
    /// <param name="caller">Caller Member Name</param>
    public static void ShutdownAndRestartWith(ProcessStartInfo startInfo, [CallerMemberName] string caller = "")
    {
        _restartStartInfo = startInfo;
        _logger.Information("Shutdown and restart with custom StartInfo, call by `{Caller}`", caller);
        Execute.OnUIThread(Application.Current.Shutdown);
    }

    private static bool _isRestartingAfterUpdate;

    public static void RestartAfterUpdate([CallerMemberName] string caller = "")
    {
        _isRestartingAfterUpdate = true;
        ShutdownAndRestartWithoutArgs();
    }

    public static void Shutdown([CallerMemberName] string caller = "")
    {
        _logger.Information("Shutdown called by `{Caller}`", caller);
        Execute.OnUIThread(Application.Current.Shutdown);
    }

    /// <summary>
    /// 以管理员权限重启应用，UAC 弹窗在退出时触发。
    /// </summary>
    public static void RestartAsAdmin()
    {
        if (Environment.ProcessPath is null)
        {
            return;
        }

        ShutdownAndRestartWith(new ProcessStartInfo {
            FileName = Environment.ProcessPath,
            UseShellExecute = true,
            Verb = "runas",
        });
    }

    private static bool _isWaitingToRestart;

    public static async Task RestartAfterIdleAsync()
    {
        if (_isWaitingToRestart)
        {
            return;
        }

        _isWaitingToRestart = true;

        await RunningState.Instance.UntilIdleAsync(60000);
        ShutdownAndRestartWithoutArgs();
    }

    /// <inheritdoc/>
    protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
    {
        LogUnhandledException(e.Exception);
        ShowErrorDialog(e.Exception);
        e.Handled = true;
    }

    private static void LogUnhandledException(Exception exception)
    {
        if (_logger != Logger.None)
        {
            _logger.Fatal(exception, "Unhandled exception occurred");
        }
    }

    private static void ShowErrorDialog(Exception exception)
    {
        Application.Current.Dispatcher.Invoke(() => {
            // DragDrop.DoDragSourceMove 会导致崩溃，但不需要退出程序
            // 这是一坨屎，但是没办法，只能这样了
            var isDragDropException = exception is COMException && exception.ToString()!.Contains("DragDrop.DoDragSourceMove");

            var shouldExit = !isDragDropException;

            var errorView = new ErrorDialogView(exception, shouldExit);
            errorView.ShowDialog();
        });
    }

    private static Dictionary<string, string> ParseArgs(string[] args, params string[] flags)
    {
        var result = new Dictionary<string, string>();
        var flagSet = new HashSet<string>(flags);

        for (int i = 0; i < args.Length; ++i)
        {
            if (flagSet.Contains(args[i]) && i + 1 < args.Length)
            {
                result[args[i]] = args[i + 1];
                ++i;
            }
        }

        return result;
    }

    /// <summary>
    /// 检查配置并切换，如果成功切换则重启
    /// </summary>
    /// <param name="desiredConfig">配置名</param>
    /// <returns>切换并重启</returns>
    private static bool Config(string desiredConfig)
    {
        const string ConfigFile = @".\config\gui.json";
        if (!File.Exists(ConfigFile) || string.IsNullOrEmpty(desiredConfig))
        {
            return false;
        }

        try
        {
            if (UpdateConfiguration(desiredConfig))
            {
                ShutdownAndRestartWithoutArgs();
                return true;
            }
        }
        catch (Exception ex)
        {
            _logger.Error("Error updating configuration: {DesiredConfig}, ex: {ExMessage}", desiredConfig, ex.Message);
        }

        return false;
    }

    /// <summary>
    /// 切换配置
    /// </summary>
    /// <param name="desiredConfig">配置名</param>
    /// <returns>是否成功切换配置</returns>
    private static bool UpdateConfiguration(string desiredConfig)
    {
        // 配置名可能就包在引号中，需要转义符，如 \"a\"
        string currentConfig = ConfigurationHelper.GetCurrentConfiguration();
        return currentConfig != desiredConfig && ConfigurationHelper.SwitchConfiguration(desiredConfig) && ConfigFactory.SwitchConfig(desiredConfig);
    }
}
