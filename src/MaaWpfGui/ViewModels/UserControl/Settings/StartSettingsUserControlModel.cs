// <copyright file="StartSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using HandyControl.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.EmulatorConnectionExtra;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Microsoft.Win32;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 启动设置
/// </summary>
public class StartSettingsUserControlModel : PropertyChangedBase
{
    static StartSettingsUserControlModel()
    {
        Instance = new();
    }

    public static StartSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<StartSettingsUserControlModel>();

    private static RunningState _runningState => RunningState.Instance;

    private static ConnectSettingsUserControlModel ConnectSettings => SettingsViewModel.ConnectSettings;

    private static VersionUpdateSettingsUserControlModel VersionUpdateSettings => SettingsViewModel.VersionUpdateSettings;

    private bool _startSelf = AutoStart.CheckStart();

    /// <summary>
    /// Gets or sets a value indicating whether to start itself.
    /// </summary>
    public bool StartSelf
    {
        get => _startSelf;
        set {
            if (!AutoStart.SetStart(value, out var error))
            {
                _logger.Error("Failed to set startup: {Error}", error);
                MessageBoxHelper.Show(error, LocalizationHelper.GetString("Warning"), icon: MessageBoxImage.Warning);
                return;
            }

            SetAndNotify(ref _startSelf, value);
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.StartupBoot);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to run directly.
    /// </summary>
    public bool RunDirectly
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.RunDirectly = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.RunDirectly;

    /// <summary>
    /// Gets or sets a value indicating whether to skip startup auto-run when restarting immediately for an update
    /// (auto-install or choosing restart now). Choosing “later” then starting MAA manually is a normal launch.
    /// </summary>
    public bool SkipStartupAutoRunAfterUpdate
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.SkipStartupAutoRunAfterUpdate = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.SkipStartupAutoRunAfterUpdate;

    /// <summary>
    /// Gets or sets a value indicating whether to minimize directly.
    /// </summary>
    public bool MinimizeDirectly
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.MinimizeOnStartup = value;
        }
    } = ConfigFactory.Root.Gui.MinimizeOnStartup;

    /// <summary>
    /// Gets or sets a value indicating whether to start emulator.
    /// </summary>
    public bool OpenEmulatorAfterLaunch
    {
        get; set {
            if (string.IsNullOrEmpty(SettingsViewModel.StartSettings.EmulatorPath))
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString(ConnectSettings.IsPCConnectConfig ? "PcClientPathNotFound" : "RetryOnDisconnectedEmulatorPathEmptyError"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                value = false;
            }

            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.StartEmulator = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.StartEmulator;

    private string _emulatorPath = ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorPath;

    /// <summary>
    /// Gets or sets the executable path for the current emulator or PC client target.
    /// </summary>
    public string EmulatorPath
    {
        get => ConnectSettings.ExtraConfig is Win32Extra win32Extra
            ? win32Extra.GamePath
            : _emulatorPath;
        set {
            value = value.Trim();

            // 这里不用 SetAndNotify 判断
            if (value == EmulatorPath)
            {
                return;
            }

            if (Path.GetFileName(value).Contains("maa", StringComparison.OrdinalIgnoreCase))
            {
                int count = 3;
                while (count-- > 0)
                {
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("EmulatorPathSelectionErrorPrompt"),
                        LocalizationHelper.GetString("Tip"),
                        MessageBoxButton.OKCancel,
                        MessageBoxImage.Warning,
                        ok: LocalizationHelper.GetString("EmulatorPathSelectionErrorImSure") + $"({count + 1})",
                        cancel: LocalizationHelper.GetString("EmulatorPathSelectionErrorSelectAgain"));
                    if (result == MessageBoxResult.Cancel)
                    {
                        return;
                    }
                }
            }

            if (string.IsNullOrEmpty(value))
            {
                if (ConnectSettings.IsPCConnectConfig)
                {
                    if (OpenEmulatorAfterLaunch)
                    {
                        OpenEmulatorAfterLaunch = false;
                        Growl.Warning(LocalizationHelper.GetString("PcClientPathNotFound"));
                    }
                }
                else if (ConnectSettings.RetryOnDisconnected || OpenEmulatorAfterLaunch)
                {
                    ConnectSettings.RetryOnDisconnected = false;
                    OpenEmulatorAfterLaunch = false;
                    Growl.Warning(LocalizationHelper.GetString("EmulatorPathEmptyWarning"));
                }
            }
            else if (!File.Exists(value))
            {
                Growl.Warning(LocalizationHelper.GetString(ConnectSettings.IsPCConnectConfig ? "PcClientPathNotFound" : "EmulatorPathNotExist"));
            }

            if (ConnectSettings.ExtraConfig is Win32Extra win32Extra)
            {
                win32Extra.GamePath = value;
                NotifyOfPropertyChange(nameof(EmulatorPath));
            }
            else
            {
                SetAndNotify(ref _emulatorPath, value);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorPath = value;
            }
        }
    }

    /// <summary>
    /// Refreshes the launch target displayed by the shared startup settings.
    /// </summary>
    internal void RefreshLaunchTarget()
    {
        NotifyOfPropertyChange(nameof(EmulatorPath));
    }

    /// <summary>
    /// Gets or sets the command to append after the emulator command.
    /// </summary>
    public string EmulatorAddCommand
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorAddCommand = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorAddCommand;

    /// <summary>
    /// Gets or sets the seconds to wait for the emulator.
    /// </summary>
    public int EmulatorWaitSeconds
    {
        get;
        set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorWaitSeconds = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorWaitSeconds;

    private (string FileName, string Arguments) ResolveShortcut(string path, bool useAdditionalCommand)
    {
        string fileName = string.Empty;
        string arguments = string.Empty;

        if (Path.GetExtension(path).Equals(".lnk", StringComparison.CurrentCultureIgnoreCase))
        {
            var link = (IShellLink)new ShellLink();
            var file = (IPersistFile)link;
            file.Load(path, 0); // STGM_READ
            link.Resolve(IntPtr.Zero, 1); // SLR_NO_UI
            var buf = new char[32768];
            unsafe
            {
                fixed (char* ptr = buf)
                {
                    link.GetPath(ptr, 260, IntPtr.Zero, 0); // MAX_PATH
                    var len = Array.IndexOf(buf, '\0');
                    if (len != -1)
                    {
                        fileName = new string(buf, 0, len);
                    }

                    link.GetArguments(ptr, 32768);
                    len = Array.IndexOf(buf, '\0');
                    if (len != -1)
                    {
                        arguments = new string(buf, 0, len);
                    }
                }
            }
        }
        else
        {
            fileName = path;
            arguments = useAdditionalCommand ? EmulatorAddCommand : string.Empty;
        }

        return (fileName, arguments);
    }

    private void WaitForEmulatorStart(int delay)
    {
        bool idle = _runningState.GetIdle();
        _runningState.SetIdle(false);
        var isPcClient = ConnectSettings.IsPCConnectConfig;
        var waitLocalizationKey = isPcClient ? "WaitForPcClient" : "WaitForEmulator";
        var launchTarget = isPcClient ? "PC client" : "emulator";

        for (var i = 0; i < delay; ++i)
        {
            if (_runningState.GetStopping())
            {
                _logger.Information("Stop waiting for the {LaunchTarget} to start", launchTarget);
                return;
            }

            if (i % 10 == 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString(waitLocalizationKey) + ": " + (delay - i) + "s");
                _logger.Information("Waiting for the {LaunchTarget} to start: {RemainingSeconds}s", launchTarget, delay - i);
            }

            Thread.Sleep(1000);
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("WaitForEmulatorFinish"));
        _logger.Information("The wait is over");

        _runningState.SetIdle(idle);
    }

    /// <summary>
    /// 尝试启动模拟器
    /// </summary>
    /// <param name="openWithMaaLaunch">启动 MAA 后自动开启模拟器</param>
    /// <param name="test">测试启动模拟器，即使配置中未设置自动启动，不读取等待时间</param>
    /// <param name="waitForStart">是否按启动设置等待目标启动。</param>
    /// <returns>是否成功发起启动。</returns>
    public bool TryToStartEmulator(bool openWithMaaLaunch = false, bool test = false, bool waitForStart = true)
    {
        if (!test && !OpenEmulatorAfterLaunch && openWithMaaLaunch)
        {
            return false;
        }

        var launchPath = ResolveLaunchPath();
        if (launchPath.Length == 0 || !File.Exists(launchPath))
        {
            return false;
        }

        int delay = test ? 0 : ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorWaitSeconds;
        var isPcClient = ConnectSettings.IsPCConnectConfig;
        var launchTarget = isPcClient ? "PC client" : "emulator";
        try
        {
            var (fileName, arguments) = ResolveShortcut(launchPath, useAdditionalCommand: !isPcClient);
            var startInfo = new ProcessStartInfo(fileName, arguments) {
                UseShellExecute = false,
            };
            if (isPcClient)
            {
                startInfo.WorkingDirectory = Path.GetDirectoryName(fileName) ?? string.Empty;
            }

            using Process process = new Process {
                StartInfo = startInfo,
            };

            _logger.Information("Try to start {LaunchTarget}:\nfileName: {FileName}\narguments: {Arguments}", launchTarget, fileName, arguments);
            process.Start();
        }
        catch (Exception)
        {
            _logger.Information("Start {LaunchTarget} error, try to start using the default: \n" +
                "EmulatorPath: " + launchPath + "\n" +
                "EmulatorAddCommand: " + EmulatorAddCommand, launchTarget);
            try
            {
                if (EmulatorAddCommand.Length != 0)
                {
                    Process.Start(launchPath);
                }
                else
                {
                    Process.Start(launchPath, EmulatorAddCommand);
                }
            }
            catch (Exception e)
            {
                if (e is Win32Exception { NativeErrorCode: 740 })
                {
                    var errorMessage = isPcClient
                        ? LocalizationHelper.GetStringFormat("PcClientStartFailed", e.Message)
                        : LocalizationHelper.GetString("EmulatorStartFailed");
                    Instances.TaskQueueViewModel.AddLog(errorMessage, UiLogColor.Warning);
                    _logger.Warning("Insufficient permissions to start the {LaunchTarget}:\nEmulatorPath: {EmulatorPath}\n", launchTarget, launchPath);
                }
                else
                {
                    _logger.Warning("{LaunchTarget} start failed with error: {ErrorMessage}", launchTarget, e.Message);
                }

                return false;
            }
        }

        if (waitForStart)
        {
            WaitForEmulatorStart(delay);
        }

        return true;
    }

    private string ResolveLaunchPath()
    {
        var launchPath = EmulatorPath;
        if (!ConnectSettings.IsPCConnectConfig || File.Exists(launchPath) || ConnectSettings.ExtraConfig is not Win32Extra win32Extra)
        {
            return launchPath;
        }

        var detectedPath = win32Extra.DetectGameExecutablePath();
        if (string.IsNullOrEmpty(detectedPath))
        {
            return launchPath;
        }

        win32Extra.GamePath = detectedPath;
        Execute.OnUIThread(() => NotifyOfPropertyChange(nameof(EmulatorPath)));
        return detectedPath;
    }

    /// <summary>
    /// Restarts the ADB (Android Debug Bridge).
    /// </summary>
    public void RestartAdb()
    {
        if (!ConnectSettings.AllowAdbRestart)
        {
            return;
        }

        string adbPath = ConnectSettings.AdbPath;

        if (string.IsNullOrEmpty(adbPath))
        {
            return;
        }

        ProcessStartInfo processStartInfo = new ProcessStartInfo {
            FileName = "cmd.exe",
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            CreateNoWindow = true,
            UseShellExecute = false,
        };

        using Process process = new Process {
            StartInfo = processStartInfo,
        };

        process.Start();
        process.StandardInput.WriteLine($"\"{adbPath}\" kill-server");
        process.StandardInput.WriteLine($"\"{adbPath}\" start-server");
        process.StandardInput.WriteLine("exit");
        process.WaitForExit();
    }

    /// <summary>
    /// Reconnect by ADB (Android Debug Bridge).
    /// </summary>
    public void ReconnectByAdb()
    {
        string adbPath = ConnectSettings.AdbPath;
        string address = ConnectSettings.ConnectAddress;

        if (string.IsNullOrEmpty(adbPath))
        {
            return;
        }

        ProcessStartInfo processStartInfo = new ProcessStartInfo {
            FileName = "cmd.exe",
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            CreateNoWindow = true,
            UseShellExecute = false,
        };

        using Process process = new Process { StartInfo = processStartInfo, };

        process.Start();
        process.StandardInput.WriteLine($"\"{adbPath}\" disconnect {address}");
        process.StandardInput.WriteLine("exit");
        process.WaitForExit();
    }

    /// <summary>
    /// Kill and restart the ADB (Android Debug Bridge) process.
    /// </summary>
    public void HardRestartAdb()
    {
        if (!ConnectSettings.AllowAdbHardRestart)
        {
            return;
        }

        string adbPath = ConnectSettings.AdbPath;

        if (string.IsNullOrEmpty(adbPath))
        {
            return;
        }

        try
        {
            // This allows for SQL injection, but since it is not on a real database nothing horrible would happen.
            // The following query string does what I want, but WMI does not accept it.
            // var wmiQueryString = string.Format("SELECT ProcessId, CommandLine FROM Win32_Process WHERE ExecutablePath='{0}'", adbPath);
            const string WmiQueryString = "SELECT ProcessId, ExecutablePath, CommandLine FROM Win32_Process";
            using var searcher = new ManagementObjectSearcher(WmiQueryString);
            using var results = searcher.Get();
            var query = from p in Process.GetProcesses()
                        join mo in results.Cast<ManagementObject>()
                            on p.Id equals (int)(uint)mo["ProcessId"]
                        select new { Process = p, Path = (string)mo["ExecutablePath"], };
            foreach (var item in query)
            {
                if (item.Path != adbPath)
                {
                    continue;
                }

                // Some emulators start their ADB with administrator privilege.
                // Not sure if this is necessary
                try
                {
                    item.Process.Kill();
                    item.Process.WaitForExit();
                }
                catch (Exception ex)
                {
                    _logger.Error("Error in HardRestartAdb: {ExMessage}", ex.Message);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Error("Error in HardRestartAdb: {ExMessage}", ex.Message);
        }
    }

    /// <summary>
    /// Selects the emulator to execute.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectEmulatorExec()
    {
        var dialog = new OpenFileDialog {
            Filter = LocalizationHelper.GetString("Executable") + "|*.exe;*.bat;*.lnk",
        };

        if (dialog.ShowDialog() == true)
        {
            EmulatorPath = dialog.FileName;
        }
    }

    /// <summary>
    /// Tests the emulator path by trying to start the emulator.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void TestEmulatorExec()
    {
        if (EmulatorPath.Length == 0)
        {
            MessageBoxHelper.Show(
                LocalizationHelper.GetString(ConnectSettings.IsPCConnectConfig ? "PcClientPathNotFound" : "EmulatorPathEmptyWarning"),
                LocalizationHelper.GetString("Warning"),
                icon: MessageBoxImage.Warning);
            return;
        }

        if (!File.Exists(EmulatorPath))
        {
            MessageBoxHelper.Show(
                LocalizationHelper.GetString(ConnectSettings.IsPCConnectConfig ? "PcClientPathNotFound" : "EmulatorPathNotExist"),
                LocalizationHelper.GetString("Warning"),
                icon: MessageBoxImage.Warning);
            return;
        }

        Task.Run(() => TryToStartEmulator(test: true));
    }
}
