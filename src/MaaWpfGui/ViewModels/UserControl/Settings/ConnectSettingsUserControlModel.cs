// <copyright file="ConnectSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Collections.ObjectModel;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Constants.Enums.Core;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models.EmulatorConnectionExtra;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;
using Serilog;
using Stylet;
using Window = HandyControl.Controls.Window;
using WindowManager = MaaWpfGui.Helper.WindowManager;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 连接设置
/// </summary>
public class ConnectSettingsUserControlModel : PropertyChangedBase
{
    private readonly RunningState _runningState = RunningState.Instance;

    static ConnectSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;

        // MuMu 触控勾选框的跨实例依赖须在 Instance 就绪后注册，
        // 放进构造链会因静态构造重入拿到 null 的 Instance 而静默失败
        PropertyDependsOnUtility.InitializePropertyDependencies(Instance._extras.Mumu12);
    }

    private ConnectSettingsUserControlModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);

        // 刷新截图方式选项的可用状态
        _extras.Win32.UpdateScreencapMethodAvailability();

        _extras.Win32.PropertyChanged += (_, e) => {
            if (e.PropertyName == nameof(Win32Extra.MouseMethod))
            {
                NotifyOfPropertyChange(nameof(ShowWindowRestoreButton));
                NotifyOfPropertyChange(nameof(ShowWindowMinimizeButton));
            }
        };

        // 从配置恢复时，若 MuMu 截图增强已启用，需将 MuMu 触控加入下拉列表
        if (ExtraConfig is MuMu12Extra { Enable: true })
        {
            if (!TouchModeList.Items.Any(item => item.Value == TouchMode.MumuExtras))
            {
                TouchModeList.Add(TouchMode.MumuExtras, "Connection.TouchMode.MumuExtras");
            }
        }
    }

    public static ConnectSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<ConnectSettingsUserControlModel>();

    /// <summary>
    /// Gets the list of the configuration of connection.
    /// </summary>
    public LocalizedObservableList<ConnectConfig> ConnectConfigList { get; } = new(
        (ConnectConfig.General, "Connection.Config.General"),
        (ConnectConfig.BlueStacks, "Connection.Config.BlueStacks"),
        (ConnectConfig.MuMuEmulator12, "Connection.Config.MuMu"),
        (ConnectConfig.MuMuArm, "Connection.Config.MuMuArm"),
        (ConnectConfig.LDPlayer, "Connection.Config.LDPlayer"),
        (ConnectConfig.Androws, "Connection.Config.Androws"),
        (ConnectConfig.AVD, "Connection.Config.AVD"),
        (ConnectConfig.Nox, "Connection.Config.Nox"),
        (ConnectConfig.XYAZ, "Connection.Config.XYAZ"),
        (ConnectConfig.PC, "Connection.Config.PC"),
        (ConnectConfig.WSA, "Connection.Config.WSA"),
        (ConnectConfig.Compatible, "Connection.Config.Compatible"),
        (ConnectConfig.SecondResolution, "Connection.Config.SecondResolution"),
        (ConnectConfig.GeneralWithoutScreencapErr, "Connection.Config.GeneralWithoutScreencapErr"));

    public static string TouchModeVideoPath => Path.Combine(PathsHelper.BaseDir, "Res", "Video", "TouchMode.mp4");

    public bool AutoDetectConnection
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AutoDetect = value;
            if (value)
            {
                Instances.AsstProxy.Connected = false;
            }
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AutoDetect;

    public bool AlwaysAutoDetectConnection
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            if (value)
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("Connection.AlwaysAutoDetectConnection.Tip"),
                    LocalizationHelper.GetString("Common.Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
            }

            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AlwaysAutoDetect = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AlwaysAutoDetect;

    public ObservableCollection<string> ConnectAddressHistory { get; set => SetAndNotify(ref field, value); } = [];

    /// <summary>
    /// Gets or sets the connection address.
    /// </summary>
    public string ConnectAddress
    {
        get; set {
            value = value
                .Replace(" ", string.Empty)
                .Replace("：", ":")
                .Replace(";", ":")
                .Replace("；", ":")
                .Trim();

            if (ConnectAddress == value)
            {
                return;
            }

            Instances.AsstProxy.Connected = false;

            UpdateConnectionHistory(value);

            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Address = value;
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Address;

    private void UpdateConnectionHistory(string address)
    {
        Execute.OnUIThread(() => {
            var index = ConnectAddressHistory.IndexOf(address);
            if (index >= 0)
            {
                ConnectAddressHistory.Move(index, 0);
            }
            else
            {
                ConnectAddressHistory.Insert(0, address);
                const int MaxHistoryCount = 5;
                while (ConnectAddressHistory.Count > MaxHistoryCount)
                {
                    ConnectAddressHistory.RemoveAt(ConnectAddressHistory.Count - 1);
                }
            }
        });
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void RemoveAddressClick(string address)
    {
        ConnectAddressHistory.Remove(address);
    }

    /// <summary>
    /// Gets or sets the ADB path.
    /// </summary>
    public string AdbPath
    {
        get; set {
            // 空路径视为用户清空输入、交由自动检测重填，不属于“文件名不含 ADB”
            if (!string.IsNullOrWhiteSpace(value) && !Path.GetFileName(value).ToLower().Contains("adb"))
            {
                var count = 3;
                while (count-- > 0)
                {
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("Connection.AdbPathFileSelectionErrorPrompt"),
                        LocalizationHelper.GetString("Common.Tip"),
                        MessageBoxButton.OKCancel,
                        MessageBoxImage.Warning,
                        ok: LocalizationHelper.GetString("Connection.AdbPathFileSelectionErrorImSure") + $"({count + 1})",
                        cancel: LocalizationHelper.GetString("Connection.AdbPathFileSelectionErrorSelectAgain"));
                    if (result == MessageBoxResult.Cancel)
                    {
                        return;
                    }
                }
            }

            Instances.AsstProxy.Connected = false;

            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbPath = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbPath;

    /// <summary>
    /// Gets or sets the connection config.
    /// </summary>
    public ConnectConfig ConnectConfig
    {
        get;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Config = value;
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle

            // 切换连接配置时，若不再使用 MuMu 截图增强，需移除 MuMu 触控选项
            var mumuEnabled = ExtraConfig is MuMu12Extra { Enable: true };
            OnMuMuExtrasEnableChanged(mumuEnabled);
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Config;

    [PropertyDependsOn(nameof(ConnectConfig))]
    public ExtraConfig? ExtraConfig => ConnectConfig switch {
        ConnectConfig.LDPlayer => _extras.LdPlayer,
        ConnectConfig.MuMuEmulator12 => _extras.Mumu12,
        ConnectConfig.PC => _extras.Win32,
        _ => null,
    };

    private readonly ExtraConfigs _extras = new();

    private class ExtraConfigs
    {
        public LDPlayerExtra LdPlayer { get; set; } = new();

        public MuMu12Extra Mumu12 { get; set; } = new();

        public Models.EmulatorConnectionExtra.Win32Extra Win32 { get; set; } = new();

        public Bluestacks BluestacksExtra { get; set; } = new();

        public record class Bluestacks
        {
            public string ConfigKeyword { get; set; } = string.Empty;

            public string ConfigPath { get; set; } = string.Empty;
        }
    }

    public string ScreencapMethod { get; set; } = string.Empty;

    public string ScreencapTestCost { get; set; } = string.Empty;

    private string _screencapCost = LocalizationHelper.GetStringFormat("Asst.Connection.ScreencapCost", "---", "---", "---", "---");

    public string ScreencapCost
    {
        get => _screencapCost;
        set => SetAndNotify(ref _screencapCost, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to retry task after ADB disconnected.
    /// </summary>
    public bool RetryOnDisconnected
    {
        get; set {
            if (string.IsNullOrEmpty(SettingsViewModel.StartSettings.EmulatorPath))
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("Connection.RetryOnDisconnectedEmulatorPathEmptyError"),
                    LocalizationHelper.GetString("Common.Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                value = false;
            }

            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.RestartEmulatorWhenAdbFailed = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.RestartEmulatorWhenAdbFailed;

    /// <summary>
    /// Gets or sets a value indicating whether to start the PC client again after a window connection failure.
    /// </summary>
    public bool RetryPcClientOnDisconnected
    {
        get; set {
            if (value && string.IsNullOrEmpty(SettingsViewModel.StartSettings.PcClientPath))
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("Connection.RetryOnPcDisconnectedPathEmptyError"),
                    LocalizationHelper.GetString("Common.Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                value = false;
            }

            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.StartUpSettings.RestartPcClientWhenConnectFailed = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.StartUpSettings.RestartPcClientWhenConnectFailed;

    /// <summary>
    /// Gets or sets a value indicating whether to retry task after ADB disconnected.
    /// </summary>
    public bool AllowAdbRestart
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbRestart = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbRestart;

    /// <summary>
    /// Gets or sets a value indicating whether to allow for killing ADB process.
    /// </summary>
    public bool AllowAdbHardRestart
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbHardRestart = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbHardRestart;

    public bool AdbLiteEnabled
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.EnableAdbLite = value;
            UpdateInstanceSettings();
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.EnableAdbLite;

    public bool KillAdbOnExit
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.KillAdbOnExit = value;
            UpdateInstanceSettings();
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.KillAdbOnExit;

    /// <summary>
    /// Gets the default addresses.
    /// </summary>
    public Dictionary<string, List<string>> DefaultAddress { get; } = new()
        {
            { "General", [string.Empty] },
            { "MuMuArm", [] },
            { "BlueStacks", ["127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585", "127.0.0.1:5595", "127.0.0.1:5554"] },
            { "MuMuEmulator12", ["127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448", "127.0.0.1:16480", "127.0.0.1:16512", "127.0.0.1:16544", "127.0.0.1:16576"] },
            { "LDPlayer", ["emulator-5554", "emulator-5556", "emulator-5558", "emulator-5560", "127.0.0.1:5555", "127.0.0.1:5557", "127.0.0.1:5559", "127.0.0.1:5561"] },
            { "Nox", ["127.0.0.1:62001", "127.0.0.1:59865"] },
            { "XYAZ", ["127.0.0.1:21503"] },
            { "Androws", ["127.0.0.1:5555"] },
            { "WSA", ["127.0.0.1:58526"] },
        };

    /// <summary>
    /// RegisterKey of Bluestacks_Nxt
    /// </summary>
    private const string BluestacksNxtRegistryKey = @"SOFTWARE\BlueStacks_nxt";

    private const string BluestacksNxtValueName = "UserDefinedDir";

    public static string? ShowItemSelectionDialog(IEnumerable<string> items, string windowTitle, string promptMessage)
    {
        string? ShowDialogCore()
        {
            var selectionWindow = new Views.Dialogs.ItemSelectionDialogView(items, windowTitle, promptMessage) {
                Owner = Application.Current.MainWindow,
            };

            return selectionWindow.ShowDialog() == true && !string.IsNullOrEmpty(selectionWindow.SelectedItem)
                ? selectionWindow.SelectedItem
                : null;
        }

        var application = Application.Current;
        if (application?.Dispatcher == null)
        {
            return null;
        }

        return application.Dispatcher.CheckAccess()
            ? ShowDialogCore()
            : application.Dispatcher.Invoke(ShowDialogCore);
    }

    /// <summary>
    /// Refreshes ADB config.
    /// </summary>
    /// <param name="error">Errors when doing this operation.</param>
    /// <returns>Whether the operation is successful.</returns>
    public bool DetectAdbConfig(ref string error)
    {
        var adapter = new WinAdapter();
        List<WinAdapter.DetectedEmulatorInfo> emulators;
        WinAdapter.DetectedEmulatorInfo? selectedEmulator = null;
        try
        {
            emulators = adapter.RefreshEmulatorsInfo();
            if (ConnectConfig == ConnectConfig.MuMuArm)
            {
                emulators.RemoveAll(emulator => emulator.EmulatorName != ConnectConfig.MuMuArm);
            }
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Exception caught");
            error = LocalizationHelper.GetString("Connection.EmulatorException");
            return false;
        }

        switch (emulators.Count)
        {
            case 0:
                error = LocalizationHelper.GetString("Connection.EmulatorNotFound");
                return false;

            case > 1:
                {
                    var selectedEmulatorDisplayText = ShowItemSelectionDialog(
                        [.. emulators.Select(emulator => emulator.SelectionDisplayText)],
                        LocalizationHelper.GetString("Connection.SelectEmulator"),
                        LocalizationHelper.GetString("Connection.MultipleEmulatorsDetectedForConnection"));

                    if (string.IsNullOrEmpty(selectedEmulatorDisplayText))
                    {
                        error = LocalizationHelper.GetString("Connection.EmulatorSelectionCancelled");
                        return false;
                    }

                    selectedEmulator = emulators.First(emulator => emulator.SelectionDisplayText == selectedEmulatorDisplayText);
                    ConnectConfig = selectedEmulator.EmulatorName;
                    break;
                }

            default:
                selectedEmulator = emulators.First();
                ConnectConfig = selectedEmulator.EmulatorName;
                break;
        }

        AdbPath = selectedEmulator?.AdbPath ?? AdbPath;
        if (string.IsNullOrEmpty(AdbPath))
        {
            error = LocalizationHelper.GetString("Connection.AdbException");
            return false;
        }

        // MuMu ARM exposes its endpoint in the running instance's state file,
        // even before it appears in `adb devices`. Never select an unrelated device.
        if (selectedEmulator?.EmulatorName == ConnectConfig.MuMuArm)
        {
            if (string.IsNullOrEmpty(selectedEmulator.Address))
            {
                error = LocalizationHelper.GetString("Connection.MuMuArm.ConnectionNotReady");
                return false;
            }

            ConnectAddress = selectedEmulator.Address;
            return true;
        }

        var addresses = WinAdapter.GetAdbAddresses(AdbPath);

        switch (addresses.Count)
        {
            case 1:
                if (addresses.First() != "1234567890ABCDEF")
                {
                    ConnectAddress = addresses.First();
                }

                break;

            case > 1:
                {
                    // 过滤掉默认地址
                    var filteredAddresses = addresses
                        .Where(address => address != "1234567890ABCDEF")
                        .ToList();

                    if (filteredAddresses.Count == 1)
                    {
                        ConnectAddress = filteredAddresses.First();
                    }
                    else if (filteredAddresses.Count > 1)
                    {
                        var selectedAddress = ShowItemSelectionDialog(
                            filteredAddresses,
                            LocalizationHelper.GetString("Connection.SelectAddress"),
                            LocalizationHelper.GetString("Connection.MultipleAddressesDetected"));

                        if (!string.IsNullOrEmpty(selectedAddress))
                        {
                            ConnectAddress = selectedAddress;
                        }
                        else
                        {
                            error = LocalizationHelper.GetString("Connection.EmulatorSelectionCancelled");
                            return false;
                        }
                    }

                    break;
                }
        }

        if (ConnectAddress.Length == 0)
        {
            ConnectAddress = DefaultAddress[ConnectConfig.ToString()][0];
        }

        return true;
    }

    /// <summary>
    /// Get the path of bluestacks.conf
    /// </summary>
    /// <returns>path</returns>
    private static string? GetBluestacksConfig()
    {
        var conf = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.BluestacksExtra.ConfigPath;
        if (!string.IsNullOrEmpty(conf))
        {
            return conf;
        }

        using var key = Registry.LocalMachine.OpenSubKey(BluestacksNxtRegistryKey);
        var value = key?.GetValue(BluestacksNxtValueName);
        if (value != null)
        {
            return (string)value + @"\bluestacks.conf";
        }

        return null;
    }

    /// <summary>
    /// Selects ADB program file.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectFile()
    {
        var dialog = new OpenFileDialog();
        if (MaaWineBridge.Availability == WineBridgeAvailability.NotAvailable)
        {
            dialog.Filter = LocalizationHelper.GetString("Connection.AdbProgram") + "|*.exe";
        }

        if (dialog.ShowDialog() == true)
        {
            AdbPath = dialog.FileName;
        }
    }

    private static Window? _imagePopupWindow;

    /// <summary>
    /// Enables persistent detection of the running MuMu ARM instance and tests its connection and screenshot.
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task DetectMuMuArmConnection()
    {
        if (!_runningState.GetIdle())
        {
            return;
        }

        AutoDetectConnection = true;

        // MuMu ARM can receive a new IP address when the instance restarts.
        AlwaysAutoDetectConnection = true;
        await TestLinkAndGetImage();
    }

    /// <summary>
    /// Test Link And Get Image.
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task TestLinkAndGetImage()
    {
        if (!_runningState.GetIdle())
        {
            return;
        }

        if (Bootstrapper.TryGetTaskBlockReason() is { } reason)
        {
            TestLinkInfo = reason;
            return;
        }

        _runningState.SetIdle(false);

        var errMsg = string.Empty;
        TestLinkInfo = LocalizationHelper.GetString("Connection.Connecting");
        Instances.AsstProxy.Connected = false;
        var caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (!caught)
        {
            TestLinkInfo = errMsg;
            _runningState.SetIdle(true);
            return;
        }

        try
        {
            TestLinkImage = await Instances.AsstProxy.AsstGetImageAsync(forceScreencap: true);
        }
        finally
        {
            if (ShowWindowRestoreButton)
            {
                Instances.AsstProxy.RestoreGameWindowPosition();
            }

            _runningState.SetIdle(true);
        }

        if (TestLinkImage is null)
        {
            TestLinkInfo = LocalizationHelper.GetString("Connection.ImageIsNull");
            return;
        }

        switch (ConnectConfig)
        {
            case ConnectConfig.MuMuEmulator12:
                if (ExtraConfig is MuMu12Extra muMu12Extra && muMu12Extra.Enable && ScreencapMethod != "MumuExtras")
                {
                    var mumuExtrasMsg = string.IsNullOrEmpty(muMu12Extra.EmulatorPath)
                        ? LocalizationHelper.GetString("Connection.MuMu.EmulatorPathEmptyError")
                        : LocalizationHelper.GetString("Connection.MuMu.ExtrasNotEnabledMessage");
                    TestLinkInfo = $"{mumuExtrasMsg}\n{ScreencapTestCost}";
                    return;
                }

                break;

            case ConnectConfig.LDPlayer:
                if (ExtraConfig is LDPlayerExtra ldPlayerExtra && ldPlayerExtra.Enable && ScreencapMethod != "LDExtras")
                {
                    var ldExtrasMsg = string.IsNullOrEmpty(ldPlayerExtra.EmulatorPath)
                        ? LocalizationHelper.GetString("Connection.LdPlayer.EmulatorPathEmptyError")
                        : LocalizationHelper.GetString("Connection.LdPlayer.ExtrasNotEnabledMessage");
                    TestLinkInfo = $"{ldExtrasMsg}\n{ScreencapTestCost}";
                    return;
                }

                break;
        }

        TestLinkInfo = ScreencapTestCost;

        if (_imagePopupWindow == null)
        {
            const double TotalWindowWidth = 800;

            var nc = SystemParameters.WindowNonClientFrameThickness;
            var rb = SystemParameters.WindowResizeBorderThickness;

            double contentWidth = TotalWindowWidth - (nc.Left + nc.Right + rb.Left + rb.Right);
            double contentHeight = contentWidth * 9.0 / 16.0;

            double totalWindowHeight = contentHeight + (nc.Top + nc.Bottom + rb.Top + rb.Bottom);
            _imagePopupWindow = new() {
                Width = TotalWindowWidth,
                Height = totalWindowHeight,
                ResizeMode = ResizeMode.NoResize,
                Content = new Image {
                    Source = TestLinkImage,
                },
            };
            _imagePopupWindow.Loaded += (_, _) => {
                WindowManager.MoveWindowToRootCenter(_imagePopupWindow);
            };
            _imagePopupWindow.Closed += (_, _) => {
                _imagePopupWindow = null;
            };
            var img = (Image)_imagePopupWindow.Content;
            img.MouseLeftButtonUp += (_, _) => {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.OneMoreLook);
                _ = TestLinkAndGetImage();
            };
        }
        else
        {
            if (_imagePopupWindow.Content is Image image)
            {
                image.Source = TestLinkImage;
            }
        }

        AchievementTrackerHelper.Instance.Unlock(AchievementIds.ConnectionTester);
        WindowManager.ShowWindow(_imagePopupWindow);
    }

    private BitmapImage? _testLinkImage;

    public BitmapImage? TestLinkImage
    {
        get => _testLinkImage;
        set => SetAndNotify(ref _testLinkImage, value);
    }

    private string _testLinkInfo = string.Empty;

    public string TestLinkInfo
    {
        get => _testLinkInfo;
        set => SetAndNotify(ref _testLinkInfo, value);
    }

    private readonly string? _bluestacksConfig = GetBluestacksConfig();
    private string _bluestacksKeyWord = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.BluestacksExtra.ConfigKeyword;

    /// <summary>
    /// Tries to set BlueStack Hyper V address.
    /// </summary>
    /// <returns>success</returns>
    public string? TryToSetBlueStacksHyperVAddress()
    {
        if (string.IsNullOrEmpty(_bluestacksConfig))
        {
            return string.Empty;
        }

        if (!File.Exists(_bluestacksConfig))
        {
            _logger.Error("File not exists");
            return string.Empty;
        }

        var allLines = File.ReadAllLines(_bluestacksConfig);

        if (string.IsNullOrEmpty(_bluestacksKeyWord))
        {
            foreach (var line in allLines)
            {
                if (!line.StartsWith("bst.installed_images"))
                {
                    continue;
                }

                var images = line.Split('"')[1].Split(',');
                _bluestacksKeyWord = "bst.instance." + images[0] + ".status.adb_port";
                break;
            }
        }

        return (from line in allLines
                where line.StartsWith(_bluestacksKeyWord)
                select line.Split('"') into sp
                select "127.0.0.1:" + sp[1])
            .FirstOrDefault();
    }

    /// <summary>
    /// Gets the list of touch modes.
    /// MuMu 触控选项仅在 MuMu 截图增强启用时动态加入列表。
    /// </summary>
    public LocalizedObservableList<TouchMode> TouchModeList { get; } = new(
        (TouchMode.MiniTouch, "Connection.TouchMode.MiniTouch"),
        (TouchMode.MaaTouch, "Connection.TouchMode.MaaTouch"),
        (TouchMode.Adb, "Connection.TouchMode.AdbInput"),
        (TouchMode.MaaFwAdb, "Connection.TouchMode.MaaFwAdb"));

    public bool IsAdbTouchMode() => TouchMode == TouchMode.Adb;

    public TouchMode TouchMode
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            UpdateInstanceSettings();
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.TouchMode = value;

            // 触控模式决定控制器子类，Core 侧需重连才能重建控制器实例
            Instances.AsstProxy.Connected = false;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.TouchMode;

    /// <summary>
    /// 根据 MuMu 截图增强的开关状态，动态增删触控模式下拉列表中的 ｢MuMu 触控｣ 选项。
    /// 仅增删下拉项，不自动切换当前触控模式——切换由触控增强勾选框负责。
    /// </summary>
    /// <param name="mumuExtrasEnabled">MuMu 截图增强是否已启用。</param>
    public void OnMuMuExtrasEnableChanged(bool mumuExtrasEnabled)
    {
        Execute.OnUIThread(() => {
            var hasMumu = TouchModeList.Items.Any(item => item.Value == TouchMode.MumuExtras);
            if (mumuExtrasEnabled && !hasMumu)
            {
                TouchModeList.Add(TouchMode.MumuExtras, "Connection.TouchMode.MumuExtras");
            }
            else if (!mumuExtrasEnabled && hasMumu)
            {
                TouchModeList.Remove(TouchMode.MumuExtras);
                if (TouchMode == TouchMode.MumuExtras)
                {
                    TouchMode = TouchMode.MiniTouch; // 回到默认
                }
            }
        });
    }

    public void UpdateInstanceSettings()
    {
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.TouchMode, TouchMode.ToCustomString());
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, SettingsViewModel.GameSettings.DeploymentWithPause ? "1" : "0");
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, AdbLiteEnabled ? "1" : "0");
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.KillAdbOnExit, KillAdbOnExit ? "1" : "0");
    }

    /// <summary>
    /// Gets or sets a value indicating whether 替换 ADB 进行中（含下载与解压全过程），期间禁用按钮防止并发写同一 adb.zip。
    /// </summary>
    public bool IsReplacingAdb
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(CanReplaceAdb));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 当前可触发替换 ADB（非进行中）。
    /// </summary>
    public bool CanReplaceAdb => !IsReplacingAdb;

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task ReplaceAdb()
    {
        IsReplacingAdb = true;
        try
        {
            if (!File.Exists(MaaUrls.GoogleAdbFilename))
            {
                string[] downloadUrls =
                [
                    MaaUrls.GoogleAdbDownloadUrl,
                    MaaUrls.AdbMaaMirrorDownloadUrl,
                    MaaUrls.AdbMaaMirror2DownloadUrl
                ];

                bool downloadResult = false;
                foreach (var url in downloadUrls)
                {
                    downloadResult = await Instances.HttpService.DownloadFileAsync(new(url), MaaUrls.GoogleAdbFilename);
                    if (downloadResult)
                    {
                        break;
                    }
                }

                if (!downloadResult)
                {
                    using var toast = new ToastNotification(LocalizationHelper.GetString("Connection.AdbDownloadFailed.Title"));
                    toast.AppendContentText(LocalizationHelper.GetString("Connection.AdbDownloadFailed.Description")).Show();
                    return;
                }
            }

            const string UnzipDir = "adb";
            const string NewAdb = UnzipDir + "/platform-tools/adb.exe";

            try
            {
                if (Directory.Exists(UnzipDir))
                {
                    Directory.Delete(UnzipDir, true);
                }
            }
            catch (Exception ex)
            {
                _logger.Error("An error occurred while deleting directory: {Type}: {ExMessage}", ex.GetType(), ex.Message);
                ToastNotification.ShowDirect(LocalizationHelper.GetString("Connection.AdbDeletionFailed.Message"));
                return;
            }

            try
            {
                ZipFile.ExtractToDirectory(MaaUrls.GoogleAdbFilename, UnzipDir);
            }
            catch (Exception e)
            {
                _logger.Error(e, "UnzipFailedMessage");
                ToastNotification.ShowDirect(LocalizationHelper.GetString("Connection.UnzipFailed.Message"));
                return;
            }

            if (File.Exists(NewAdb))
            {
                AdbPath = NewAdb;
                AdbReplaced = true;
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbReplaced = true;
                ToastNotification.ShowDirect(LocalizationHelper.GetString("Connection.AdbReplaced"));
            }
            else
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("Connection.AdbReplaceFailed"));
            }
        }
        finally
        {
            IsReplacingAdb = false;
        }
    }

    public bool AdbReplaced { get; set; } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbReplaced;

    #region AttachWindow (Win32窗口绑定) 配置

    /// <summary>
    /// Gets a value indicating whether to show the window restore button (PC 端 + *WithWindowPos 输入方式).
    /// </summary>
    [PropertyDependsOn(nameof(ConnectConfig))]
    public bool ShowWindowRestoreButton =>
        IsPCConnectConfig && ExtraConfig is Models.EmulatorConnectionExtra.Win32Extra { MouseMethod: AsstWin32InputMethod.SendMessageWithWindowPos or AsstWin32InputMethod.PostMessageWithWindowPos };

    /// <summary>
    /// Gets a value indicating whether to show the window minimize button for PC PostMsg-WindowPos input.
    /// </summary>
    [PropertyDependsOn(nameof(ConnectConfig))]
    public bool ShowWindowMinimizeButton =>
        IsPCConnectConfig && ExtraConfig is Models.EmulatorConnectionExtra.Win32Extra { MouseMethod: AsstWin32InputMethod.PostMessageWithWindowPos };

    [PropertyDependsOn(nameof(ConnectConfig))]
    public bool IsPCConnectConfig => ConnectConfig == ConnectConfig.PC;

    #endregion

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        ConnectConfigList.RefreshLocalization();
        TouchModeList.RefreshLocalization();
    }
}
