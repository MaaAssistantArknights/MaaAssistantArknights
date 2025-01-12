// <copyright file="StartSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Windows;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
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
        set
        {
            if (!AutoStart.SetStart(value, out var error))
            {
                _logger.Error($"Failed to set startup: {error}");
                MessageBoxHelper.Show(error, LocalizationHelper.GetString("Warning"), icon: MessageBoxImage.Warning);
                return;
            }

            SetAndNotify(ref _startSelf, value);
        }
    }

    private bool _runDirectly = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RunDirectly, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to run directly.
    /// </summary>
    public bool RunDirectly
    {
        get => _runDirectly;
        set
        {
            SetAndNotify(ref _runDirectly, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RunDirectly, value.ToString());
        }
    }

    private bool _minimizeDirectly = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MinimizeDirectly, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to minimize directly.
    /// </summary>
    public bool MinimizeDirectly
    {
        get => _minimizeDirectly;
        set
        {
            SetAndNotify(ref _minimizeDirectly, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MinimizeDirectly, value.ToString());
        }
    }

    private bool _openEmulatorAfterLaunch = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.StartEmulator, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to start emulator.
    /// </summary>
    public bool OpenEmulatorAfterLaunch
    {
        get => _openEmulatorAfterLaunch;
        set
        {
            SetAndNotify(ref _openEmulatorAfterLaunch, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.StartEmulator, value.ToString());
            if (SettingsViewModel.GameSettings.ClientType == string.Empty && _runningState.GetIdle())
            {
                SettingsViewModel.GameSettings.ClientType = "Official";
            }
        }
    }

    private string _emulatorPath = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorPath, string.Empty);

    /// <summary>
    /// Gets or sets the emulator path.
    /// </summary>
    public string EmulatorPath
    {
        get => _emulatorPath;
        set
        {
            if (Path.GetFileName(value).ToLower().Contains("maa"))
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

            SetAndNotify(ref _emulatorPath, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EmulatorPath, value);
        }
    }

    private string _emulatorAddCommand = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorAddCommand, string.Empty);

    /// <summary>
    /// Gets or sets the command to append after the emulator command.
    /// </summary>
    public string EmulatorAddCommand
    {
        get => _emulatorAddCommand;
        set
        {
            SetAndNotify(ref _emulatorAddCommand, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EmulatorAddCommand, value);
        }
    }

    private string _emulatorWaitSeconds = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorWaitSeconds, "60");

    /// <summary>
    /// Gets or sets the seconds to wait for the emulator.
    /// </summary>
    public string EmulatorWaitSeconds
    {
        get => _emulatorWaitSeconds;
        set
        {
            SetAndNotify(ref _emulatorWaitSeconds, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EmulatorWaitSeconds, value);
        }
    }

    private bool _blockSleep = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleep, bool.FalseString));

    public bool BlockSleep
    {
        get => _blockSleep;
        set
        {
            SetAndNotify(ref _blockSleep, value);
            SleepManagement.SetBlockSleep(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleep, value.ToString());
        }
    }

    private bool _blockSleepWithScreenOn = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, bool.TrueString));

    public bool BlockSleepWithScreenOn
    {
        get => _blockSleepWithScreenOn;
        set
        {
            SetAndNotify(ref _blockSleepWithScreenOn, value);
            SleepManagement.SetBlockSleepWithScreenOn(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleepWithScreenOn, value.ToString());
        }
    }

    private (string FileName, string Arguments) ResolveShortcut(string path)
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
            arguments = EmulatorAddCommand;
        }

        return (fileName, arguments);
    }

    private void WaitForEmulatorStart(int delay)
    {
        bool idle = _runningState.GetIdle();
        _runningState.SetIdle(false);

        for (var i = 0; i < delay; ++i)
        {
            if (Instances.TaskQueueViewModel.Stopping)
            {
                _logger.Information("Stop waiting for the emulator to start");
                return;
            }

            if (i % 10 == 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("WaitForEmulator") + ": " + (delay - i) + "s");
                _logger.Information("Waiting for the emulator to start: " + (delay - i) + "s");
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
    public void TryToStartEmulator(bool openWithMaaLaunch = false)
    {
        if (EmulatorPath.Length == 0 || !File.Exists(EmulatorPath) || (!OpenEmulatorAfterLaunch && openWithMaaLaunch))
        {
            return;
        }

        if (!int.TryParse(EmulatorWaitSeconds, out int delay))
        {
            delay = 60;
        }

        try
        {
            var (fileName, arguments) = ResolveShortcut(EmulatorPath);
            Process process = new Process
            {
                StartInfo = new ProcessStartInfo(fileName, arguments)
                {
                    UseShellExecute = false,
                },
            };

            _logger.Information("Try to start emulator: \nfileName: " + fileName + "\narguments: " + arguments);
            process.Start();
        }
        catch (Exception)
        {
            _logger.Information("Start emulator error, try to start using the default: \n" +
                "EmulatorPath: " + EmulatorPath + "\n" +
                "EmulatorAddCommand: " + EmulatorAddCommand);
            try
            {
                if (EmulatorAddCommand.Length != 0)
                {
                    Process.Start(EmulatorPath);
                }
                else
                {
                    Process.Start(EmulatorPath, EmulatorAddCommand);
                }
            }
            catch (Exception e)
            {
                if (e is Win32Exception { NativeErrorCode: 740 })
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("EmulatorStartFailed"), UiLogColor.Warning);

                    _logger.Warning(
                        "Insufficient permissions to start the emulator:\n" +
                        "EmulatorPath: " + EmulatorPath + "\n");
                }
                else
                {
                    _logger.Warning("Emulator start failed with error: " + e.Message);
                }

                return;
            }
        }

        WaitForEmulatorStart(delay);
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

        ProcessStartInfo processStartInfo = new ProcessStartInfo
        {
            FileName = "cmd.exe",
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            CreateNoWindow = true,
            UseShellExecute = false,
        };

        Process process = new Process
        {
            StartInfo = processStartInfo,
        };

        process.Start();
        process.StandardInput.WriteLine($"{adbPath} kill-server");
        process.StandardInput.WriteLine($"{adbPath} start-server");
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

        ProcessStartInfo processStartInfo = new ProcessStartInfo
        {
            FileName = "cmd.exe",
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            CreateNoWindow = true,
            UseShellExecute = false,
        };

        Process process = new Process { StartInfo = processStartInfo, };

        process.Start();
        process.StandardInput.WriteLine($"{adbPath} disconnect {address}");
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

        // This allows for SQL injection, but since it is not on a real database nothing horrible would happen.
        // The following query string does what I want, but WMI does not accept it.
        // var wmiQueryString = string.Format("SELECT ProcessId, CommandLine FROM Win32_Process WHERE ExecutablePath='{0}'", adbPath);
        const string WmiQueryString = "SELECT ProcessId, ExecutablePath, CommandLine FROM Win32_Process";
        using var searcher = new ManagementObjectSearcher(WmiQueryString);
        using var results = searcher.Get();
        var query = from p in Process.GetProcesses()
                    join mo in results.Cast<ManagementObject>()
                        on p.Id equals (int)(uint)mo["ProcessId"]
                    select new
                    {
                        Process = p,
                        Path = (string)mo["ExecutablePath"],
                    };
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
            catch
            {
                // ignored
            }
        }
    }

    /// <summary>
    /// Selects the emulator to execute.
    /// </summary>
    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public void SelectEmulatorExec()
    {
        var dialog = new OpenFileDialog
        {
            Filter = LocalizationHelper.GetString("Executable") + "|*.exe;*.bat;*.lnk",
        };

        if (dialog.ShowDialog() == true)
        {
            EmulatorPath = dialog.FileName;
        }
    }

    private bool _autoRestartOnDrop = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, "True"));

    public bool AutoRestartOnDrop
    {
        get => _autoRestartOnDrop;
        set
        {
            SetAndNotify(ref _autoRestartOnDrop, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AutoRestartOnDrop, value.ToString());
        }
    }
}
