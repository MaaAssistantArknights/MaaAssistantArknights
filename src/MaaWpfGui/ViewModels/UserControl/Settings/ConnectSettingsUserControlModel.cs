// <copyright file="ConnectSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using Window = HandyControl.Controls.Window;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 连接设置
/// </summary>
public class ConnectSettingsUserControlModel : PropertyChangedBase
{
    static ConnectSettingsUserControlModel()
    {
        Instance = new();
    }

    public static ConnectSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<ConnectSettingsUserControlModel>();

    private static RunningState _runningState => RunningState.Instance;

    /// <summary>
    /// Gets the list of the configuration of connection.
    /// </summary>
    public List<CombinedData> ConnectConfigList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("General"), Value = "General" },
            new() { Display = LocalizationHelper.GetString("BlueStacks"), Value = "BlueStacks" },
            new() { Display = LocalizationHelper.GetString("MuMuEmulator12"), Value = "MuMuEmulator12" },
            new() { Display = LocalizationHelper.GetString("LDPlayer"), Value = "LDPlayer" },
            new() { Display = LocalizationHelper.GetString("Nox"), Value = "Nox" },
            new() { Display = LocalizationHelper.GetString("XYAZ"), Value = "XYAZ" },
            new() { Display = LocalizationHelper.GetString("WSA"), Value = "WSA" },
            new() { Display = LocalizationHelper.GetString("Compatible"), Value = "Compatible" },
            new() { Display = LocalizationHelper.GetString("SecondResolution"), Value = "SecondResolution" },
            new() { Display = LocalizationHelper.GetString("GeneralWithoutScreencapErr"), Value = "GeneralWithoutScreencapErr" },
        ];

    /// <summary>
    /// Gets the list of touch modes
    /// </summary>
    public List<CombinedData> TouchModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("MiniTouchMode"), Value = "minitouch" },
            new() { Display = LocalizationHelper.GetString("MaaTouchMode"), Value = "maatouch" },
            new() { Display = LocalizationHelper.GetString("AdbTouchMode"), Value = "adb" },
        ];

    private bool _autoDetectConnection = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDetect, bool.TrueString));

    public bool AutoDetectConnection
    {
        get => _autoDetectConnection;
        set
        {
            if (!SetAndNotify(ref _autoDetectConnection, value))
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.AutoDetect, value.ToString());

            if (value)
            {
                Instances.AsstProxy.Connected = false;
            }
        }
    }

    private bool _alwaysAutoDetectConnection = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AlwaysAutoDetect, bool.FalseString));

    public bool AlwaysAutoDetectConnection
    {
        get => _alwaysAutoDetectConnection;
        set
        {
            SetAndNotify(ref _alwaysAutoDetectConnection, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AlwaysAutoDetect, value.ToString());
        }
    }

    private ObservableCollection<string> _connectAddressHistory = [];

    public ObservableCollection<string> ConnectAddressHistory
    {
        get => _connectAddressHistory;
        set => SetAndNotify(ref _connectAddressHistory, value);
    }

    private string _connectAddress = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);

    /// <summary>
    /// Gets or sets the connection address.
    /// </summary>
    public string ConnectAddress
    {
        get => _connectAddress;
        set
        {
            if (ConnectAddress == value)
            {
                return;
            }

            Instances.AsstProxy.Connected = false;

            UpdateConnectionHistory(value);

            SetAndNotify(ref _connectAddress, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AddressHistory, JsonConvert.SerializeObject(ConnectAddressHistory));
            ConfigurationHelper.SetValue(ConfigurationKeys.ConnectAddress, value);
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
        }
    }

    private void UpdateConnectionHistory(string address)
    {
        var history = ConnectAddressHistory.ToList();
        if (history.Remove(address))
        {
            history.Insert(0, address);
        }
        else
        {
            history.Insert(0, address);
            const int MaxHistoryCount = 5;
            if (history.Count > MaxHistoryCount)
            {
                history.RemoveRange(MaxHistoryCount, history.Count - MaxHistoryCount);
            }
        }

        ConnectAddressHistory = new ObservableCollection<string>(history);
    }

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public void RemoveAddressClick(string address)
    {
        ConnectAddressHistory.Remove(address);
        ConfigurationHelper.SetValue(ConfigurationKeys.AddressHistory, JsonConvert.SerializeObject(ConnectAddressHistory));
    }

    private string _adbPath = ConfigurationHelper.GetValue(ConfigurationKeys.AdbPath, string.Empty);

    /// <summary>
    /// Gets or sets the ADB path.
    /// </summary>
    public string AdbPath
    {
        get => _adbPath;
        set
        {
            if (!Path.GetFileName(value).ToLower().Contains("adb"))
            {
                var count = 3;
                while (count-- > 0)
                {
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("AdbPathFileSelectionErrorPrompt"),
                        LocalizationHelper.GetString("Tip"),
                        MessageBoxButton.OKCancel,
                        MessageBoxImage.Warning,
                        ok: LocalizationHelper.GetString("AdbPathFileSelectionErrorImSure") + $"({count + 1})",
                        cancel: LocalizationHelper.GetString("AdbPathFileSelectionErrorSelectAgain"));
                    if (result == MessageBoxResult.Cancel)
                    {
                        return;
                    }
                }
            }

            Instances.AsstProxy.Connected = false;

            SetAndNotify(ref _adbPath, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AdbPath, value);
        }
    }

    private string _connectConfig = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectConfig, "General");

    /// <summary>
    /// Gets or sets the connection config.
    /// </summary>
    public string ConnectConfig
    {
        get => _connectConfig;
        set
        {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _connectConfig, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ConnectConfig, value);
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
        }
    }

    public string ScreencapMethod { get; set; } = string.Empty;

    public string ScreencapTestCost { get; set; } = string.Empty;

    public class MuMuEmulator12ConnectionExtras : PropertyChangedBase
    {
        private bool _enable = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12ExtrasEnabled, bool.FalseString));

        public bool Enable
        {
            get => _enable;
            set
            {
                if (!SetAndNotify(ref _enable, value))
                {
                    return;
                }

                if (value)
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("MuMu12ExtrasEnabledTip"));

                    // 读取mumu注册表地址 并填充GUI
                    if (string.IsNullOrEmpty(EmulatorPath))
                    {
                        try
                        {
                            const string UninstallKeyPath = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayer-12.0";
                            const string UninstallExeName = @"\uninstall.exe";

                            using var driverKey = Registry.LocalMachine.OpenSubKey(UninstallKeyPath);
                            if (driverKey == null)
                            {
                                EmulatorPath = string.Empty;
                                return;
                            }

                            var uninstallString = driverKey.GetValue("UninstallString") as string;

                            if (string.IsNullOrEmpty(uninstallString) || !uninstallString.Contains(UninstallExeName))
                            {
                                EmulatorPath = string.Empty;
                                return;
                            }

                            var match = Regex.Match(uninstallString,
                                $"""
                                     ^"(.*?){Regex.Escape(UninstallExeName)}
                                     """,
                                RegexOptions.IgnoreCase);
                            EmulatorPath = match.Success ? match.Groups[1].Value : string.Empty;
                        }
                        catch (Exception e)
                        {
                            _logger.Warning($"An error occurred: {e.Message}");
                            EmulatorPath = string.Empty;
                        }
                    }
                }

                Instances.AsstProxy.Connected = false;
                ConfigurationHelper.SetValue(ConfigurationKeys.MuMu12ExtrasEnabled, value.ToString());
            }
        }

        private static readonly string _configuredPath = ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12EmulatorPath, @"C:\Program Files\Netease\MuMuPlayer-12.0");
        private string _emulatorPath = Directory.Exists(_configuredPath) ? _configuredPath : string.Empty;

        /// <summary>
        /// Gets or sets a value indicating the path of the emulator.
        /// </summary>
        public string EmulatorPath
        {
            get => _emulatorPath;
            set
            {
                if (_enable && !string.IsNullOrEmpty(value) && !Directory.Exists(value))
                {
                    MessageBoxHelper.Show("MuMu Emulator 12 Path Not Found");
                    value = string.Empty;
                }

                Instances.AsstProxy.Connected = false;
                SetAndNotify(ref _emulatorPath, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.MuMu12EmulatorPath, value);
            }
        }

        private bool _mumuBridgeConnection = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MumuBridgeConnection, bool.FalseString));

        public bool MuMuBridgeConnection
        {
            get => _mumuBridgeConnection;
            set
            {
                if (_mumuBridgeConnection == value)
                {
                    return;
                }

                if (value)
                {
                    var result = MessageBoxHelper.Show(LocalizationHelper.GetString("MuMuBridgeConnectionTip"), icon: MessageBoxImage.Information, buttons: MessageBoxButton.OKCancel);
                    if (result != MessageBoxResult.OK)
                    {
                        return;
                    }
                }

                SetAndNotify(ref _mumuBridgeConnection, value);

                Instances.AsstProxy.Connected = false;
                ConfigurationHelper.SetValue(ConfigurationKeys.MumuBridgeConnection, value.ToString());
            }
        }

        private string _index = ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12Index, "0");

        /// <summary>
        /// Gets or sets the index of the emulator.
        /// </summary>
        public string Index
        {
            get => _index;
            set
            {
                Instances.AsstProxy.Connected = false;
                SetAndNotify(ref _index, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.MuMu12Index, value);
            }
        }

        public string Config
        {
            get
            {
                if (!Enable)
                {
                    return JsonConvert.SerializeObject(new JObject());
                }

                var configObject = new JObject
                {
                    ["path"] = EmulatorPath,
                };

                if (MuMuBridgeConnection)
                {
                    configObject["index"] = int.TryParse(Index, out var indexParse) ? indexParse : 0;
                }

                return JsonConvert.SerializeObject(configObject);
            }
        }
    }

    public MuMuEmulator12ConnectionExtras MuMuEmulator12Extras { get; set; } = new();

    public class LdPlayerConnectionExtras : PropertyChangedBase
    {
        private bool _enable = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerExtrasEnabled, bool.FalseString));

        public bool Enable
        {
            get => _enable;
            set
            {
                if (!SetAndNotify(ref _enable, value))
                {
                    return;
                }

                if (value)
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("LdExtrasEnabledTip"));

                    // 读取 LD 注册表地址 并填充GUI
                    if (string.IsNullOrEmpty(EmulatorPath))
                    {
                        try
                        {
                            const string UninstallKeyPath = @"Software\leidian\ldplayer9";
                            const string InstallDirValueName = "InstallDir";

                            using var driverKey = Registry.CurrentUser.OpenSubKey(UninstallKeyPath);
                            if (driverKey == null)
                            {
                                EmulatorPath = string.Empty;
                                return;
                            }

                            var installDir = driverKey.GetValue(InstallDirValueName) as string;

                            if (string.IsNullOrEmpty(installDir))
                            {
                                EmulatorPath = string.Empty;
                                return;
                            }

                            EmulatorPath = installDir;
                        }
                        catch (Exception e)
                        {
                            _logger.Warning($"An error occurred: {e.Message}");
                            EmulatorPath = string.Empty;
                        }
                    }
                }

                Instances.AsstProxy.Connected = false;
                ConfigurationHelper.SetValue(ConfigurationKeys.LdPlayerExtrasEnabled, value.ToString());
            }
        }

        private static readonly string _configuredPath = ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerEmulatorPath, @"C:\leidian\LDPlayer9");
        private string _emulatorPath = Directory.Exists(_configuredPath) ? _configuredPath : string.Empty;

        /// <summary>
        /// Gets or sets a value indicating the path of the emulator.
        /// </summary>
        public string EmulatorPath
        {
            get => _emulatorPath;
            set
            {
                if (_enable && !string.IsNullOrEmpty(value) && !Directory.Exists(value))
                {
                    MessageBoxHelper.Show("LD Emulator Path Not Found");
                    value = string.Empty;
                }

                Instances.AsstProxy.Connected = false;
                SetAndNotify(ref _emulatorPath, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LdPlayerEmulatorPath, value);
            }
        }

        private string _index = ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerIndex, "0");

        /// <summary>
        /// Gets or sets the index of the emulator.
        /// </summary>
        public string Index
        {
            get => _index;
            set
            {
                Instances.AsstProxy.Connected = false;
                SetAndNotify(ref _index, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LdPlayerIndex, value);
            }
        }

        private int GetEmulatorPid(int index)
        {
            var emulatorPath = $@"{EmulatorPath}\ldconsole.exe";
            if (!File.Exists(emulatorPath))
            {
                MessageBoxHelper.Show("LD Emulator Path Not Found");
                return 0;
            }

            var startInfo = new ProcessStartInfo
            {
                FileName = emulatorPath,
                Arguments = "list2",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                _logger.Warning("Failed to start ldconsole list2");
                return 0;
            }

            using var reader = process.StandardOutput;
            var result = reader.ReadToEnd();
            var lines = result.Split([Environment.NewLine], StringSplitOptions.RemoveEmptyEntries);

            if (lines.Length <= 0)
            {
                _logger.Warning("Failed to get emulator PID.");
                return 0;
            }

            foreach (var line in lines)
            {
                var parts = line.Split(',');
                if (parts.Length < 6 || !int.TryParse(parts[0], out var currentIndex) || currentIndex != index)
                {
                    continue;
                }

                if (int.TryParse(parts[5], out var pid))
                {
                    return pid;
                }
            }

            _logger.Warning("Failed to get emulator PID.");
            return 0;
        }

        public string Config
        {
            get
            {
                if (!Enable)
                {
                    return JsonConvert.SerializeObject(new JObject());
                }

                var configObject = new JObject
                {
                    ["path"] = EmulatorPath,
                    ["index"] = int.TryParse(Index, out var indexParse) ? indexParse : 0,
                    ["pid"] = GetEmulatorPid(indexParse),
                };
                return JsonConvert.SerializeObject(configObject);
            }
        }
    }

    public LdPlayerConnectionExtras LdPlayerExtras { get; set; } = new();

    private bool _retryOnDisconnected = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RetryOnAdbDisconnected, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to retry task after ADB disconnected.
    /// </summary>
    public bool RetryOnDisconnected
    {
        get => _retryOnDisconnected;
        set
        {
            if (string.IsNullOrEmpty(SettingsViewModel.StartSettings.EmulatorPath))
            {
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("RetryOnDisconnectedEmulatorPathEmptyError"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                value = false;
            }

            SetAndNotify(ref _retryOnDisconnected, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RetryOnAdbDisconnected, value.ToString());
        }
    }

    private bool _allowAdbRestart = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbRestart, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to retry task after ADB disconnected.
    /// </summary>
    public bool AllowAdbRestart
    {
        get => _allowAdbRestart;
        set
        {
            SetAndNotify(ref _allowAdbRestart, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AllowAdbRestart, value.ToString());
        }
    }

    private bool _allowAdbHardRestart = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbHardRestart, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to allow for killing ADB process.
    /// </summary>
    public bool AllowAdbHardRestart
    {
        get => _allowAdbHardRestart;
        set
        {
            SetAndNotify(ref _allowAdbHardRestart, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AllowAdbHardRestart, value.ToString());
        }
    }

    private bool _adbLiteEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AdbLiteEnabled, bool.FalseString));

    public bool AdbLiteEnabled
    {
        get => _adbLiteEnabled;
        set
        {
            SetAndNotify(ref _adbLiteEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AdbLiteEnabled, value.ToString());
            UpdateInstanceSettings();
        }
    }

    private bool _killAdbOnExit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.KillAdbOnExit, bool.FalseString));

    public bool KillAdbOnExit
    {
        get => _killAdbOnExit;
        set
        {
            SetAndNotify(ref _killAdbOnExit, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.KillAdbOnExit, value.ToString());
            UpdateInstanceSettings();
        }
    }

    /// <summary>
    /// Gets the default addresses.
    /// </summary>
    public Dictionary<string, List<string>> DefaultAddress { get; } = new()
        {
            { "General", [string.Empty] },
            { "BlueStacks", ["127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585", "127.0.0.1:5595", "127.0.0.1:5554"] },
            { "MuMuEmulator12", ["127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448", "127.0.0.1:16480", "127.0.0.1:16512", "127.0.0.1:16544", "127.0.0.1:16576"] },
            { "LDPlayer", ["emulator-5554", "emulator-5556", "emulator-5558", "emulator-5560", "127.0.0.1:5555", "127.0.0.1:5557", "127.0.0.1:5559", "127.0.0.1:5561"] },
            { "Nox", ["127.0.0.1:62001", "127.0.0.1:59865"] },
            { "XYAZ", ["127.0.0.1:21503"] },
            { "WSA", ["127.0.0.1:58526"] },
        };

    /// <summary>
    /// RegisterKey of Bluestacks_Nxt
    /// </summary>
    private const string BluestacksNxtRegistryKey = @"SOFTWARE\BlueStacks_nxt";

    private const string BluestacksNxtValueName = "UserDefinedDir";

    /// <summary>
    /// Refreshes ADB config.
    /// </summary>
    /// <param name="error">Errors when doing this operation.</param>
    /// <returns>Whether the operation is successful.</returns>
    public bool DetectAdbConfig(ref string error)
    {
        var adapter = new WinAdapter();
        List<string> emulators;
        try
        {
            emulators = adapter.RefreshEmulatorsInfo();
        }
        catch (Exception e)
        {
            _logger.Information(e.Message);
            error = LocalizationHelper.GetString("EmulatorException");
            return false;
        }

        switch (emulators.Count)
        {
            case 0:
                error = LocalizationHelper.GetString("EmulatorNotFound");
                return false;

            case > 1:
                error = LocalizationHelper.GetString("EmulatorTooMany");
                break;
        }

        ConnectConfig = emulators.First();
        AdbPath = adapter.GetAdbPathByEmulatorName(ConnectConfig) ?? AdbPath;
        if (string.IsNullOrEmpty(AdbPath))
        {
            error = LocalizationHelper.GetString("AdbException");
            return false;
        }

        var addresses = adapter.GetAdbAddresses(AdbPath);

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
                    foreach (var address in addresses.Where(address => address != "emulator-5554" && address != "1234567890ABCDEF"))
                    {
                        ConnectAddress = address;
                        break;
                    }

                    break;
                }
        }

        if (ConnectAddress.Length == 0)
        {
            ConnectAddress = DefaultAddress[ConnectConfig][0];
        }

        return true;
    }

    /// <summary>
    /// Get the path of bluestacks.conf
    /// </summary>
    /// <returns>path</returns>
    private static string? GetBluestacksConfig()
    {
        var conf = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigPath, string.Empty);
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
    /// </summary>
    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public void SelectFile()
    {
        var dialog = new OpenFileDialog();
        if (MaaWineBridge.Availability == WineBridgeAvailability.NotAvailable)
        {
            dialog.Filter = LocalizationHelper.GetString("AdbProgram") + "|*.exe";
        }

        if (dialog.ShowDialog() == true)
        {
            AdbPath = dialog.FileName;
        }
    }

    /// <summary>
    /// Test Link And Get Image.
    /// </summary>
    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public async void TestLinkAndGetImage()
    {
        _runningState.SetIdle(false);

        var errMsg = string.Empty;
        TestLinkInfo = LocalizationHelper.GetString("ConnectingToEmulator");
        var caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (!caught)
        {
            TestLinkInfo = errMsg;
            _runningState.SetIdle(true);
            return;
        }

        TestLinkImage = await Instances.AsstProxy.AsstGetFreshImageAsync();
        await Instances.TaskQueueViewModel.Stop();
        Instances.TaskQueueViewModel.SetStopped();

        if (TestLinkImage is null)
        {
            TestLinkInfo = "Image is null";
            return;
        }

        switch (ConnectConfig)
        {
            case "MuMuEmulator12":
                if (MuMuEmulator12Extras.Enable && ScreencapMethod != "MumuExtras")
                {
                    TestLinkInfo = $"{LocalizationHelper.GetString("MuMuExtrasNotEnabledMessage")}\n{ScreencapTestCost}";
                    return;
                }

                break;
        }

        TestLinkInfo = ScreencapTestCost;

        var popupWindow = new Window
        {
            Width = 800,
            Height = 481, // (800 - 1 - 1) * 9 / 16 + 32 + 1,
            Content = new Image { Source = TestLinkImage, },
            WindowStartupLocation = WindowStartupLocation.CenterScreen,
        };
        popupWindow.ShowDialog();
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
    private string _bluestacksKeyWord = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigKeyword, string.Empty);

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
            ConfigurationHelper.SetValue(ConfigurationKeys.BluestacksConfigError, "File not exists");
            return string.Empty;
        }

        var allLines = File.ReadAllLines(_bluestacksConfig);

        // ReSharper disable once InvertIf
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

    public bool IsAdbTouchMode()
    {
        return TouchMode == "adb";
    }

    private string _touchMode = ConfigurationHelper.GetValue(ConfigurationKeys.TouchMode, "minitouch");

    public string TouchMode
    {
        get => _touchMode;
        set
        {
            SetAndNotify(ref _touchMode, value);
            UpdateInstanceSettings();
            ConfigurationHelper.SetValue(ConfigurationKeys.TouchMode, value);
            SettingsViewModel.AskRestartToApplySettings();
        }
    }

    public void UpdateInstanceSettings()
    {
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.TouchMode, TouchMode);
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, SettingsViewModel.GameSettings.DeploymentWithPause ? "1" : "0");
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, AdbLiteEnabled ? "1" : "0");
        Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.KillAdbOnExit, KillAdbOnExit ? "1" : "0");
    }

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public async void ReplaceAdb()
    {
        if (string.IsNullOrEmpty(AdbPath))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("NoAdbPathSpecifiedMessage"));
            return;
        }

        if (!File.Exists(MaaUrls.GoogleAdbFilename))
        {
            var downloadResult = await Instances.HttpService.DownloadFileAsync(new Uri(MaaUrls.GoogleAdbDownloadUrl), MaaUrls.GoogleAdbFilename);

            if (!downloadResult)
            {
                downloadResult = await Instances.HttpService.DownloadFileAsync(new Uri(MaaUrls.AdbMaaMirrorDownloadUrl), MaaUrls.GoogleAdbFilename);
            }

            if (!downloadResult)
            {
                using var toast = new ToastNotification(LocalizationHelper.GetString("AdbDownloadFailedTitle"));
                toast.AppendContentText(LocalizationHelper.GetString("AdbDownloadFailedDesc")).Show();
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
            _logger.Error($"An error occurred while deleting directory: {ex.GetType()}: {ex.Message}");
            ToastNotification.ShowDirect(LocalizationHelper.GetString("AdbDeletionFailedMessage"));
            return;
        }

        try
        {
            ZipFile.ExtractToDirectory(MaaUrls.GoogleAdbFilename, UnzipDir);
        }
        catch (Exception ex)
        {
            _logger.Error(ex.ToString());
            ToastNotification.ShowDirect(LocalizationHelper.GetString("UnzipFailedMessage"));
            return;
        }

        var replaced = false;
        if (AdbPath != NewAdb && File.Exists(AdbPath))
        {
            try
            {
                foreach (var process in Process.GetProcessesByName(Path.GetFileName(AdbPath)))
                {
                    process.Kill();
                    process.WaitForExit(5000);
                }

                var adbBack = AdbPath + ".bak";
                if (!File.Exists(adbBack))
                {
                    File.Copy(AdbPath, adbBack, true);
                }

                File.Copy(NewAdb, AdbPath, true);
                replaced = true;
            }
            catch (Exception ex)
            {
                _logger.Error(ex.ToString());
                replaced = false;
            }
        }

        if (replaced)
        {
            AdbReplaced = true;

            ConfigurationHelper.SetValue(ConfigurationKeys.AdbReplaced, bool.TrueString);

            ToastNotification.ShowDirect(LocalizationHelper.GetString("SuccessfullyReplacedAdb"));
        }
        else
        {
            AdbPath = NewAdb;

            using var toast = new ToastNotification(LocalizationHelper.GetString("FailedToReplaceAdbAndUseLocal"));
            toast.AppendContentText(LocalizationHelper.GetString("FailedToReplaceAdbAndUseLocalDesc")).Show();
        }
    }

    public bool AdbReplaced { get; set; } = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AdbReplaced, bool.FalseString));
}
