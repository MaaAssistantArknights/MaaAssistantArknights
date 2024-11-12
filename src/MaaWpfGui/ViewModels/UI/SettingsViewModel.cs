// <copyright file="SettingsViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Configuration;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UserControl;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.ViewModels.UserControl.VersionUpdateSettingsUserControlModel;
using ComboBox = System.Windows.Controls.ComboBox;
using DarkModeType = MaaWpfGui.Configuration.GUI.DarkModeType;
using Timer = System.Timers.Timer;
using Window = HandyControl.Controls.Window;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of settings.
    /// </summary>
    public class SettingsViewModel : Screen
    {
        private readonly RunningState _runningState;

        private static readonly ILogger _logger = Log.ForContext<SettingsViewModel>();

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        private const int SWMINIMIZE = 6;

        [DllImport("user32.dll")]
        private static extern bool IsIconic(IntPtr hWnd);

        /// <summary>
        /// The Pallas language key.
        /// </summary>
        public const string PallasLangKey = "pallas";

        /// <summary>
        /// Gets the visibility of task setting views.
        /// </summary>
        public TaskSettingVisibilityInfo TaskSettingVisibilities { get; } = TaskSettingVisibilityInfo.Current;

        /// <summary>
        /// Gets the after action setting.
        /// </summary>
        public PostActionSetting PostActionSetting { get; } = PostActionSetting.Current;

        /// <summary>
        /// Gets 软件更新model
        /// </summary>
        public static VersionUpdateSettingsUserControlModel VersionUpdateDataContext { get; } = new();

        /// <summary>
        /// Gets 外部通知model
        /// </summary>
        public static ExternalNotificationSettingsUserControlModel ExternalNotificationDataContext { get; } = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsViewModel"/> class.
        /// </summary>
        public SettingsViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Settings");

            Init();

            HangoverEnd();

            _runningState = RunningState.Instance;
        }

        #region Init

        private List<string> _listTitle =
        [
            LocalizationHelper.GetString("SwitchConfiguration"),
            LocalizationHelper.GetString("ScheduleSettings"),
            LocalizationHelper.GetString("PerformanceSettings"),
            LocalizationHelper.GetString("GameSettings"),
            LocalizationHelper.GetString("ConnectionSettings"),
            LocalizationHelper.GetString("StartupSettings"),
            LocalizationHelper.GetString("RemoteControlSettings"),
            LocalizationHelper.GetString("UiSettings"),
            LocalizationHelper.GetString("ExternalNotificationSettings"),
            LocalizationHelper.GetString("HotKeySettings"),
            LocalizationHelper.GetString("UpdateSettings"),
            LocalizationHelper.GetString("AboutUs"),
        ];

        /// <summary>
        /// Gets or sets the list title.
        /// </summary>
        public List<string> ListTitle
        {
            get => _listTitle;
            set => SetAndNotify(ref _listTitle, value);
        }

        private bool _idle;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set => SetAndNotify(ref _idle, value);
        }

        private void Init()
        {
            InitInfrast();
            InitRoguelike();
            InitConfiguration();
            InitUiSettings();
            InitConnectConfig();
            InitVersionUpdate();
        }

        private void InitInfrast()
        {
            var facilityList = new[]
            {
                "Mfg",
                "Trade",
                "Control",
                "Power",
                "Reception",
                "Office",
                "Dorm",
                "Processing",
                "Training",
            };

            var tempOrderList = new List<DragItemViewModel?>(new DragItemViewModel[facilityList.Length]);
            var nonOrderList = new List<DragItemViewModel>();
            for (int i = 0; i != facilityList.Length; ++i)
            {
                var facility = facilityList[i];
                bool parsed = int.TryParse(ConfigurationHelper.GetFacilityOrder(facility, "-1"), out int order);

                DragItemViewModel vm = new DragItemViewModel(
                    LocalizationHelper.GetString(facility),
                    facility,
                    "Infrast.");

                if (!parsed || order < 0 || order >= tempOrderList.Count || tempOrderList[order] != null)
                {
                    nonOrderList.Add(vm);
                }
                else
                {
                    tempOrderList[order] = vm;
                }
            }

            foreach (var newVm in nonOrderList)
            {
                int i = 0;
                while (i < tempOrderList.Count && tempOrderList[i] != null)
                {
                    ++i;
                }

                tempOrderList[i] = newVm;
                ConfigurationHelper.SetFacilityOrder(newVm.OriginalName, i.ToString());
            }

            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(tempOrderList!);
            InfrastItemViewModels.CollectionChanged += InfrastOrderSelectionChanged;

            _dormThresholdLabel = LocalizationHelper.GetString("DormThreshold") + ": " + _dormThreshold + "%";
        }

        private void InitRoguelike()
        {
            UpdateRoguelikeDifficultyList();
            UpdateRoguelikeModeList();
            UpdateRoguelikeSquadList();
            UpdateRoguelikeCoreCharList();
        }

        private void InitConfiguration()
        {
            var configurations = new ObservableCollection<CombinedData>();
            foreach (var conf in ConfigurationHelper.GetConfigurationList())
            {
                configurations.Add(new CombinedData { Display = conf, Value = conf });
            }

            ConfigurationList = configurations;
        }

        private void InitUiSettings()
        {
            var languageList = (from pair in LocalizationHelper.SupportedLanguages
                                where pair.Key != PallasLangKey || Cheers
                                select new CombinedData { Display = pair.Value, Value = pair.Key })
               .ToList();

            LanguageList = languageList;

            SwitchDarkMode();
        }

        private void InitConnectConfig()
        {
            var addressListJson = ConfigurationHelper.GetValue(ConfigurationKeys.AddressHistory, string.Empty);
            if (!string.IsNullOrEmpty(addressListJson))
            {
                ConnectAddressHistory = JsonConvert.DeserializeObject<ObservableCollection<string>>(addressListJson) ?? [];
            }
        }

        private void InitVersionUpdate()
        {
            if (VersionUpdateDataContext.VersionType == UpdateVersionType.Nightly && !VersionUpdateDataContext.AllowNightlyUpdates)
            {
                VersionUpdateDataContext.VersionType = UpdateVersionType.Beta;
            }
        }

        #endregion Init

        #region EasterEggs

        private bool _cheers = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Cheers, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to cheer.
        /// </summary>
        public bool Cheers
        {
            get => _cheers;
            set
            {
                if (_cheers == value)
                {
                    return;
                }

                SetAndNotify(ref _cheers, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Cheers, value.ToString());
                if (_cheers)
                {
                    ConfigurationHelper.SetValue(ConfigurationKeys.Localization, PallasLangKey);
                }
            }
        }

        private bool _hangover = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Hangover, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hangover.
        /// </summary>
        public bool Hangover
        {
            get => _hangover;
            set
            {
                SetAndNotify(ref _hangover, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Hangover, value.ToString());
            }
        }

        private string _lastBuyWineTime = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LastBuyWineTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

        public string LastBuyWineTime
        {
            get => _lastBuyWineTime;
            set
            {
                SetAndNotify(ref _lastBuyWineTime, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.LastBuyWineTime, value);
            }
        }

        public void HangoverEnd()
        {
            if (!Hangover)
            {
                return;
            }

            Hangover = false;
            MessageBoxHelper.Show(
                LocalizationHelper.GetString("Hangover"),
                LocalizationHelper.GetString("Burping"),
                iconKey: "HangoverGeometry",
                iconBrushKey: "PallasBrush");
            Bootstrapper.ShutdownAndRestartWithoutArgs();
        }

        public void Sober()
        {
            if (!Cheers || Language != PallasLangKey)
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.Localization, SoberLanguage);
            Hangover = true;
            Cheers = false;
        }

        private string _soberLanguage = ConfigurationHelper.GetValue(ConfigurationKeys.SoberLanguage, LocalizationHelper.DefaultLanguage);

        public string SoberLanguage
        {
            get => _soberLanguage;
            set
            {
                SetAndNotify(ref _soberLanguage, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.SoberLanguage, value);
            }
        }

        /// <summary>
        /// Did you buy wine?
        /// </summary>
        /// <returns>The answer.</returns>
        public bool DidYouBuyWine()
        {
            var now = DateTime.UtcNow.ToYjDate();
            if (now == DateTime.ParseExact(LastBuyWineTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
            {
                return false;
            }

            if (now.IsAprilFoolsDay())
            {
                return true;
            }

            string[] wineList = ["酒", "liquor", "drink", "wine", "beer", "술", "🍷", "🍸", "🍺", "🍻", "🥃", "🍶"];
            return wineList.Any(CreditFirstList.Contains);
        }

        #endregion EasterEggs

        #region Remote Control

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public async void RemoteControlConnectionTest()
        {
            await RemoteControlService.ConnectionTest();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void RemoteControlRegenerateDeviceIdentity()
        {
            RemoteControlService.RegenerateDeviceIdentity();
        }

        private string _remoteControlGetTaskEndpointUri = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty);

        public string RemoteControlGetTaskEndpointUri
        {
            get => _remoteControlGetTaskEndpointUri;
            set
            {
                if (!SetAndNotify(ref _remoteControlGetTaskEndpointUri, value))
                {
                    return;
                }

                Instances.RemoteControlService.InitializePollJobTask();
                ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, value);
            }
        }

        private string _remoteControlReportStatusUri = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlReportStatusUri, string.Empty);

        public string RemoteControlReportStatusUri
        {
            get => _remoteControlReportStatusUri;
            set
            {
                SetAndNotify(ref _remoteControlReportStatusUri, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlReportStatusUri, value);
            }
        }

        private string _remoteControlUserIdentity = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlUserIdentity, string.Empty);

        public string RemoteControlUserIdentity
        {
            get => _remoteControlUserIdentity;
            set
            {
                SetAndNotify(ref _remoteControlUserIdentity, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlUserIdentity, value);
            }
        }

        private string _remoteControlDeviceIdentity = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty);

        public string RemoteControlDeviceIdentity
        {
            get => _remoteControlDeviceIdentity;
            set
            {
                SetAndNotify(ref _remoteControlDeviceIdentity, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlDeviceIdentity, value);
            }
        }

        #endregion Remote Control

        #region Performance

        public List<GpuOption> GpuOptions => GpuOption.GetGpuOptions();

        public GpuOption ActiveGpuOption
        {
            get => GpuOption.GetCurrent();
            set
            {
                GpuOption.SetCurrent(value);
                AskRestartToApplySettings();
            }
        }

        public bool AllowDeprecatedGpu
        {
            get => GpuOption.AllowDeprecatedGpu;
            set
            {
                GpuOption.AllowDeprecatedGpu = value;
                NotifyOfPropertyChange();
            }
        }

        #endregion Performance

        #region 启动设置

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
                    _logger.Error("Failed to set startup.");
                    MessageBoxHelper.Show(error);
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

        private bool _minimizeDirectly = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizeDirectly, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimize directly.
        /// </summary>
        public bool MinimizeDirectly
        {
            get => _minimizeDirectly;
            set
            {
                SetAndNotify(ref _minimizeDirectly, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.MinimizeDirectly, value.ToString());
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
                if (ClientType == string.Empty && _runningState.GetIdle())
                {
                    ClientType = "Official";
                }
            }
        }

        private string _accountName = ConfigurationHelper.GetValue(ConfigurationKeys.AccountName, string.Empty);

        public string AccountName
        {
            get => _accountName;
            set
            {
                SetAndNotify(ref _accountName, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AccountName, value);
            }
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void AccountSwitchManualRun()
        {
            Instances.TaskQueueViewModel.QuickSwitchAccount();
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

        private string _startsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.StartsWithScript, string.Empty);

        public string StartsWithScript
        {
            get => _startsWithScript;
            set
            {
                SetAndNotify(ref _startsWithScript, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.StartsWithScript, value);
            }
        }

        private string _endsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.EndsWithScript, string.Empty);

        public string EndsWithScript
        {
            get => _endsWithScript;
            set
            {
                SetAndNotify(ref _endsWithScript, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.EndsWithScript, value);
            }
        }

        private bool _copilotWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotWithScript, bool.FalseString));

        public bool CopilotWithScript
        {
            get => _copilotWithScript;
            set
            {
                SetAndNotify(ref _copilotWithScript, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CopilotWithScript, value.ToString());
            }
        }

        private bool _manualStopWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ManualStopWithScript, bool.FalseString));

        public bool ManualStopWithScript
        {
            get => _manualStopWithScript;
            set
            {
                SetAndNotify(ref _manualStopWithScript, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ManualStopWithScript, value.ToString());
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

        private string _screencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), "---", "---", "---", "---");

        public string ScreencapCost
        {
            get => _screencapCost;
            set => SetAndNotify(ref _screencapCost, value);
        }

        public void RunScript(string str, bool showLog = true)
        {
            bool enable = str switch
            {
                "StartsWithScript" => !string.IsNullOrWhiteSpace(StartsWithScript),
                "EndsWithScript" => !string.IsNullOrWhiteSpace(EndsWithScript),
                _ => false,
            };

            if (!enable)
            {
                return;
            }

            Func<bool> func = str switch
            {
                "StartsWithScript" => () => ExecuteScript(StartsWithScript),
                "EndsWithScript" => () => ExecuteScript(EndsWithScript),
                _ => () => false,
            };

            if (!showLog)
            {
                if (!func())
                {
                    _logger.Warning("Failed to execute the script.");
                }

                return;
            }

            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartTask") + LocalizationHelper.GetString(str));
            if (func())
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString(str));
            }
            else
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskError") + LocalizationHelper.GetString(str), UiLogColor.Warning);
            }
        }

        private static bool ExecuteScript(string scriptPath)
        {
            try
            {
                if (string.IsNullOrWhiteSpace(scriptPath))
                {
                    return false;
                }

                string fileName;
                string arguments;

                if (scriptPath.StartsWith('\"'))
                {
                    var parts = scriptPath.Split("\"", 3);
                    fileName = parts[1];
                    arguments = parts.Length > 2 ? parts[2] : string.Empty;
                }
                else
                {
                    fileName = scriptPath;
                    arguments = string.Empty;
                }

                bool createNoWindow = arguments.Contains("-noWindow");
                bool minimized = arguments.Contains("-minimized");

                if (createNoWindow)
                {
                    arguments = arguments.Replace("-noWindow", string.Empty).Trim();
                }

                if (minimized)
                {
                    arguments = arguments.Replace("-minimized", string.Empty).Trim();
                }

                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = fileName,
                        Arguments = arguments,
                        WindowStyle = minimized ? ProcessWindowStyle.Minimized : ProcessWindowStyle.Normal,
                        CreateNoWindow = createNoWindow,
                        UseShellExecute = !createNoWindow,
                    },
                };
                process.Start();
                process.WaitForExit();
                return true;
            }
            catch (Exception)
            {
                return false;
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
            if (!AllowAdbRestart)
            {
                return;
            }

            string adbPath = AdbPath;

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
            string adbPath = AdbPath;
            string address = ConnectAddress;

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
            if (!AllowAdbHardRestart)
            {
                return;
            }

            string adbPath = AdbPath;

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

        private string _clientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty);

        /// <summary>
        /// Gets or sets the client type.
        /// </summary>
        public string ClientType
        {
            get => _clientType;
            set
            {
                SetAndNotify(ref _clientType, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ClientType, value);
                VersionUpdateDataContext.ResourceInfo = GetResourceVersionByClientType(_clientType);
                VersionUpdateDataContext.ResourceVersion = VersionUpdateDataContext.ResourceInfo.VersionName;
                VersionUpdateDataContext.ResourceDateTime = VersionUpdateDataContext.ResourceInfo.DateTime;
                UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
                Instances.TaskQueueViewModel.UpdateStageList();
                Instances.TaskQueueViewModel.UpdateDatePrompt();
                Instances.AsstProxy.LoadResource();
                AskRestartToApplySettings(_clientType is "YoStarEN");
            }
        }

        /// <summary>
        /// Gets the client type.
        /// </summary>
        public string ClientName
        {
            get
            {
                foreach (var item in ClientTypeList.Where(item => item.Value == ClientType))
                {
                    return item.Display;
                }

                return "Unknown Client";
            }
        }

        private readonly Dictionary<string, string> _serverMapping = new()
        {
            { string.Empty, "CN" },
            { "Official", "CN" },
            { "Bilibili", "CN" },
            { "YoStarEN", "US" },
            { "YoStarJP", "JP" },
            { "YoStarKR", "KR" },
            { "txwy", "ZH_TW" },
        };

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

        /// <summary>
        /// Gets the server type.
        /// </summary>
        public string ServerType => _serverMapping[ClientType];

        #endregion 启动设置

        #region 基建设置

        /// <summary>
        /// Gets or sets the infrast item view models.
        /// </summary>
        public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; }

        /// <summary>
        /// Gets the list of uses of drones.
        /// </summary>
        public List<CombinedData> UsesOfDronesList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("DronesNotUse"), Value = "_NotUse" },
                new() { Display = LocalizationHelper.GetString("Money"), Value = "Money" },
                new() { Display = LocalizationHelper.GetString("SyntheticJade"), Value = "SyntheticJade" },
                new() { Display = LocalizationHelper.GetString("CombatRecord"), Value = "CombatRecord" },
                new() { Display = LocalizationHelper.GetString("PureGold"), Value = "PureGold" },
                new() { Display = LocalizationHelper.GetString("OriginStone"), Value = "OriginStone" },
                new() { Display = LocalizationHelper.GetString("Chip"), Value = "Chip" },
            ];

        /// <summary>
        /// Gets the list of uses of default infrast.
        /// </summary>
        public List<CombinedData> DefaultInfrastList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("UserDefined"), Value = UserDefined },
                new() { Display = LocalizationHelper.GetString("153Time3"), Value = "153_layout_3_times_a_day.json" },
                new() { Display = LocalizationHelper.GetString("153Time4"), Value = "153_layout_4_times_a_day.json" },
                new() { Display = LocalizationHelper.GetString("243Time3"), Value = "243_layout_3_times_a_day.json" },
                new() { Display = LocalizationHelper.GetString("243Time4"), Value = "243_layout_4_times_a_day.json" },
                new() { Display = LocalizationHelper.GetString("333Time3"), Value = "333_layout_for_Orundum_3_times_a_day.json" },
            ];

        private int _dormThreshold = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.DormThreshold, "30"));

        /// <summary>
        /// Gets or sets the threshold to enter dormitory.
        /// </summary>
        public int DormThreshold
        {
            get => _dormThreshold;
            set
            {
                SetAndNotify(ref _dormThreshold, value);
                DormThresholdLabel = LocalizationHelper.GetString("DormThreshold") + ": " + _dormThreshold + "%";
                ConfigurationHelper.SetValue(ConfigurationKeys.DormThreshold, value.ToString());
            }
        }

        private string _dormThresholdLabel;

        /// <summary>
        /// Gets or sets the label of dormitory threshold.
        /// </summary>
        public string DormThresholdLabel
        {
            get => _dormThresholdLabel;
            set => SetAndNotify(ref _dormThresholdLabel, value);
        }

        /// <summary>
        /// Gets infrast order list.
        /// </summary>
        /// <returns>The infrast order list.</returns>
        public List<string> GetInfrastOrderList()
        {
            return (from item in InfrastItemViewModels where item.IsChecked select item.OriginalName).ToList();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void InfrastItemSelectedAll()
        {
            foreach (var item in InfrastItemViewModels)
            {
                item.IsChecked = true;
            }
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void InfrastItemUnselectedAll()
        {
            foreach (var item in InfrastItemViewModels)
            {
                item.IsChecked = false;
            }
        }

        /// <summary>
        /// 实时更新基建换班顺序
        /// </summary>
        /// <param name="sender">ignored object</param>
        /// <param name="e">ignored NotifyCollectionChangedEventArgs</param>
        public void InfrastOrderSelectionChanged(object? sender = null, NotifyCollectionChangedEventArgs? e = null)
        {
            _ = (sender, e);
            int index = 0;
            foreach (var item in InfrastItemViewModels)
            {
                ConfigurationHelper.SetFacilityOrder(item.OriginalName, index.ToString());
                ++index;
            }
        }

        public static TaskQueueViewModel CustomInfrastPlanDataContext { get => Instances.TaskQueueViewModel; }

        private string _usesOfDrones = ConfigurationHelper.GetValue(ConfigurationKeys.UsesOfDrones, "Money");

        /// <summary>
        /// Gets or sets the uses of drones.
        /// </summary>
        public string UsesOfDrones
        {
            get => _usesOfDrones;
            set
            {
                SetAndNotify(ref _usesOfDrones, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UsesOfDrones, value);
            }
        }

        private bool _continueTraining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ContinueTraining, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to continue training after current training completed.
        /// </summary>
        public bool ContinueTraining
        {
            get => _continueTraining;
            set
            {
                SetAndNotify(ref _continueTraining, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ContinueTraining, value.ToString());
            }
        }

        private string _defaultInfrast = ConfigurationHelper.GetValue(ConfigurationKeys.DefaultInfrast, UserDefined);

        private const string UserDefined = "user_defined";

        /// <summary>
        /// Gets or sets the uses of drones.
        /// </summary>
        public string DefaultInfrast
        {
            get => _defaultInfrast;
            set
            {
                SetAndNotify(ref _defaultInfrast, value);
                if (_defaultInfrast != UserDefined)
                {
                    CustomInfrastFile = @"resource\custom_infrast\" + value;
                    IsCustomInfrastFileReadOnly = true;
                }
                else
                {
                    IsCustomInfrastFileReadOnly = false;
                }

                ConfigurationHelper.SetValue(ConfigurationKeys.DefaultInfrast, value);
            }
        }

        private bool _isCustomInfrastFileReadOnly = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether  CustomInfrastFile is read-only
        /// </summary>
        public bool IsCustomInfrastFileReadOnly
        {
            get => _isCustomInfrastFileReadOnly;
            set
            {
                SetAndNotify(ref _isCustomInfrastFileReadOnly, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, value.ToString());
            }
        }

        private bool _dormFilterNotStationedEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DormFilterNotStationedEnabled, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
        /// </summary>
        public bool DormFilterNotStationedEnabled
        {
            get => _dormFilterNotStationedEnabled;
            set
            {
                SetAndNotify(ref _dormFilterNotStationedEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.DormFilterNotStationedEnabled, value.ToString());
            }
        }

        private bool _dormTrustEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DormTrustEnabled, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether trust in dorm is enabled.
        /// </summary>
        public bool DormTrustEnabled
        {
            get => _dormTrustEnabled;
            set
            {
                SetAndNotify(ref _dormTrustEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.DormTrustEnabled, value.ToString());
            }
        }

        private bool _originiumShardAutoReplenishment = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
        /// </summary>
        public bool OriginiumShardAutoReplenishment
        {
            get => _originiumShardAutoReplenishment;
            set
            {
                SetAndNotify(ref _originiumShardAutoReplenishment, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, value.ToString());
            }
        }

        private bool _customInfrastEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastEnabled, bool.FalseString));

        public bool CustomInfrastEnabled
        {
            get => _customInfrastEnabled;
            set
            {
                SetAndNotify(ref _customInfrastEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastEnabled, value.ToString());
                Instances.TaskQueueViewModel.CustomInfrastEnabled = value;
            }
        }

        /// <summary>
        /// Selects infrast config file.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void SelectCustomInfrastFile()
        {
            var dialog = new OpenFileDialog
            {
                Filter = LocalizationHelper.GetString("CustomInfrastFile") + "|*.json",
            };

            if (dialog.ShowDialog() == true)
            {
                CustomInfrastFile = dialog.FileName;
            }

            DefaultInfrast = UserDefined;
        }

        private string _customInfrastFile = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);

        public string CustomInfrastFile
        {
            get => _customInfrastFile;
            set
            {
                SetAndNotify(ref _customInfrastFile, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastFile, value);
                Instances.TaskQueueViewModel.RefreshCustomInfrastPlan();

                // SetAndNotify 在值没有变化时不会触发 PropertyChanged 事件，所以这里手动触发一下
                Instances.TaskQueueViewModel.NeedAddCustomInfrastPlanInfo = false;
                {
                    Instances.TaskQueueViewModel.CustomInfrastPlanIndex--;
                    Instances.TaskQueueViewModel.CustomInfrastPlanIndex++;
                }

                Instances.TaskQueueViewModel.NeedAddCustomInfrastPlanInfo = true;
            }
        }

        #region 设置页面列表和滚动视图联动绑定

        private enum NotifyType
        {
            None,
            SelectedIndex,
            ScrollOffset,
        }

        private NotifyType _notifySource = NotifyType.None;

        private Timer _resetNotifyTimer;

        private void ResetNotifySource()
        {
            if (_resetNotifyTimer != null)
            {
                _resetNotifyTimer.Stop();
                _resetNotifyTimer.Close();
            }

            _resetNotifyTimer = new Timer(20);
            _resetNotifyTimer.Elapsed += (_, _) =>
            {
                _notifySource = NotifyType.None;
            };
            _resetNotifyTimer.AutoReset = false;
            _resetNotifyTimer.Enabled = true;
            _resetNotifyTimer.Start();
        }

        /// <summary>
        /// Gets or sets the height of scroll viewport.
        /// </summary>
        public double ScrollViewportHeight { get; set; }

        /// <summary>
        /// Gets or sets the extent height of scroll.
        /// </summary>
        public double ScrollExtentHeight { get; set; }

        /// <summary>
        /// Gets or sets the list of divider vertical offset.
        /// </summary>
        public List<double> DividerVerticalOffsetList { get; set; } = new();

        private int _selectedIndex;

        /// <summary>
        /// Gets or sets the index selected.
        /// </summary>
        public int SelectedIndex
        {
            get => _selectedIndex;
            set
            {
                switch (_notifySource)
                {
                    case NotifyType.None:
                        _notifySource = NotifyType.SelectedIndex;
                        SetAndNotify(ref _selectedIndex, value);

                        if (DividerVerticalOffsetList?.Count > 0 && value < DividerVerticalOffsetList.Count)
                        {
                            ScrollOffset = DividerVerticalOffsetList[value];
                        }

                        ResetNotifySource();
                        break;

                    case NotifyType.ScrollOffset:
                        SetAndNotify(ref _selectedIndex, value);
                        break;

                    case NotifyType.SelectedIndex:
                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        private double _scrollOffset;

        /// <summary>
        /// Gets or sets the scroll offset.
        /// </summary>
        public double ScrollOffset
        {
            get => _scrollOffset;
            set
            {
                switch (_notifySource)
                {
                    case NotifyType.None:
                        _notifySource = NotifyType.ScrollOffset;
                        SetAndNotify(ref _scrollOffset, value);

                        // 设置 ListBox SelectedIndex 为当前 ScrollOffset 索引
                        if (DividerVerticalOffsetList?.Count > 0)
                        {
                            // 滚动条滚动到底部，返回最后一个 Divider 索引
                            if (value + ScrollViewportHeight >= ScrollExtentHeight)
                            {
                                SelectedIndex = DividerVerticalOffsetList.Count - 1;
                                ResetNotifySource();
                                break;
                            }

                            // 根据出当前 ScrollOffset 选出最后一个在可视范围的 Divider 索引
                            var dividerSelect = DividerVerticalOffsetList.Select((n, i) => (
                                dividerAppeared: value >= n,
                                index: i));

                            var index = dividerSelect.LastOrDefault(n => n.dividerAppeared).index;
                            SelectedIndex = index;
                        }

                        ResetNotifySource();
                        break;

                    case NotifyType.SelectedIndex:
                        SetAndNotify(ref _scrollOffset, value);
                        break;

                    case NotifyType.ScrollOffset:
                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        #endregion 设置页面列表和滚动视图联动绑定

        #endregion 基建设置

        #region 肉鸽设置

        private void UpdateRoguelikeDifficultyList()
        {
            RoguelikeDifficultyList = [
                new() { Display = "MAX", Value = int.MaxValue }
            ];

            for (int i = 20; i >= 0; --i)
            {
                var value = i.ToString();
                RoguelikeDifficultyList.Add(new() { Display = value, Value = i });
            }

            RoguelikeDifficultyList.Add(new() { Display = LocalizationHelper.GetString("Current"), Value = -1 });
        }

        private void UpdateRoguelikeModeList()
        {
            var roguelikeMode = RoguelikeMode;

            RoguelikeModeList =
            [
                new() { Display = LocalizationHelper.GetString("RoguelikeStrategyExp"), Value = "0" },
                new() { Display = LocalizationHelper.GetString("RoguelikeStrategyGold"), Value = "1" },

                // new CombData { Display = "两者兼顾，投资过后退出", Value = "2" } // 弃用
                // new CombData { Display = Localization.GetString("3"), Value = "3" },  // 开发中
                new() { Display = LocalizationHelper.GetString("RoguelikeStrategyLastReward"), Value = "4" },
            ];

            switch (RoguelikeTheme)
            {
                case "Sami":

                    RoguelikeModeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStrategyCollapse"), Value = "5" });

                    break;
            }

            RoguelikeMode = RoguelikeModeList.Any(x => x.Value == roguelikeMode) ? roguelikeMode : "0";
        }

        private readonly Dictionary<string, List<(string Key, string Value)>> _squadDictionary = new()
        {
            ["Phantom_Default"] =
            [
                ("ResearchSquad", "研究分队"),
            ],
            ["Mizuki_Default"] =
            [
                ("IS2NewSquad1", "心胜于物分队"),
                ("IS2NewSquad2", "物尽其用分队"),
                ("IS2NewSquad3", "以人为本分队"),
                ("ResearchSquad", "研究分队"),
            ],
            ["Sami_Default"] =
            [
                ("IS3NewSquad1", "永恒狩猎分队"),
                ("IS3NewSquad2", "生活至上分队"),
                ("IS3NewSquad3", "科学主义分队"),
                ("IS3NewSquad4", "特训分队"),
            ],
            ["Sarkaz_1"] =
            [
                ("IS4NewSquad2", "博闻广记分队"),
                ("IS4NewSquad3", "蓝图测绘分队"),
                ("IS4NewSquad6", "点刺成锭分队"),
                ("IS4NewSquad7", "拟态学者分队"),
            ],
            ["Sarkaz_Default"] =
            [
                ("IS4NewSquad1", "魂灵护送分队"),
                ("IS4NewSquad2", "博闻广记分队"),
                ("IS4NewSquad3", "蓝图测绘分队"),
                ("IS4NewSquad4", "因地制宜分队"),
                ("IS4NewSquad5", "异想天开分队"),
                ("IS4NewSquad6", "点刺成锭分队"),
                ("IS4NewSquad7", "拟态学者分队"),
            ],
        };

        // 通用分队
        private readonly List<(string Key, string Value)> _commonSquads =
        [
            ("LeaderSquad", "指挥分队"),
            ("GatheringSquad", "集群分队"),
            ("SupportSquad", "后勤分队"),
            ("SpearheadSquad", "矛头分队"),
            ("TacticalAssaultOperative", "突击战术分队"),
            ("TacticalFortificationOperative", "堡垒战术分队"),
            ("TacticalRangedOperative", "远程战术分队"),
            ("TacticalDestructionOperative", "破坏战术分队"),
            ("First-ClassSquad", "高规格分队"),
        ];

        private void UpdateRoguelikeSquadList()
        {
            var roguelikeSquad = RoguelikeSquad;
            RoguelikeSquadList =
            [
                new() { Display = LocalizationHelper.GetString("DefaultSquad"), Value = string.Empty }
            ];

            // 优先匹配 Theme_Mode，其次匹配 Theme_Default
            string themeKey = $"{RoguelikeTheme}_{RoguelikeMode}";
            if (!_squadDictionary.ContainsKey(themeKey))
            {
                themeKey = $"{RoguelikeTheme}_Default";
            }

            // 添加主题分队
            if (_squadDictionary.TryGetValue(themeKey, out var squads))
            {
                foreach (var (key, value) in squads)
                {
                    RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString(key), Value = value });
                }
            }

            // 添加通用分队
            foreach (var (key, value) in _commonSquads)
            {
                RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString(key), Value = value });
            }

            // 选择当前分队
            RoguelikeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquad) ? roguelikeSquad : string.Empty;
        }

        private void UpdateRoguelikeCoreCharList()
        {
            var filePath = $"resource/roguelike/{RoguelikeTheme}/recruitment.json";
            if (!File.Exists(filePath))
            {
                RoguelikeCoreCharList.Clear();
                return;
            }

            var jsonStr = File.ReadAllText(filePath);
            var json = (JObject?)JsonConvert.DeserializeObject(jsonStr);

            var roguelikeCoreCharList = new ObservableCollection<string>();

            if (json?["priority"] is JArray priorityArray)
            {
                foreach (var priorityItem in priorityArray)
                {
                    if (priorityItem?["opers"] is not JArray opersArray)
                    {
                        continue;
                    }

                    foreach (var operItem in opersArray)
                    {
                        var isStart = (bool?)operItem["is_start"] ?? false;
                        if (!isStart)
                        {
                            continue;
                        }

                        var name = (string?)operItem["name"];
                        if (string.IsNullOrEmpty(name))
                        {
                            continue;
                        }

                        var localizedName = DataHelper.GetLocalizedCharacterName(name, OperNameLocalization);
                        if (!string.IsNullOrEmpty(localizedName) && !(_clientType.Contains("YoStar") && DataHelper.GetLocalizedCharacterName(name, "en-us") == DataHelper.GetLocalizedCharacterName(name, "zh-cn")))
                        {
                            roguelikeCoreCharList.Add(localizedName);
                        }
                    }
                }
            }

            RoguelikeCoreCharList = roguelikeCoreCharList;
        }

        /// <summary>
        /// Gets the list of roguelike lists.
        /// </summary>
        public List<CombinedData> RoguelikeThemeList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("RoguelikeThemePhantom"), Value = "Phantom" },
                new() { Display = LocalizationHelper.GetString("RoguelikeThemeMizuki"), Value = "Mizuki" },
                new() { Display = LocalizationHelper.GetString("RoguelikeThemeSami"), Value = "Sami" },
                new() { Display = LocalizationHelper.GetString("RoguelikeThemeSarkaz"), Value = "Sarkaz" },
            ];

        private ObservableCollection<GenericCombinedData<int>> _roguelikeDifficultyList = [];

        public ObservableCollection<GenericCombinedData<int>> RoguelikeDifficultyList
        {
            get => _roguelikeDifficultyList;
            set => SetAndNotify(ref _roguelikeDifficultyList, value);
        }

        private ObservableCollection<CombinedData> _roguelikeModeList = [];

        /// <summary>
        /// Gets or sets the list of roguelike modes.
        /// </summary>
        public ObservableCollection<CombinedData> RoguelikeModeList
        {
            get => _roguelikeModeList;
            set => SetAndNotify(ref _roguelikeModeList, value);
        }

        private ObservableCollection<CombinedData> _roguelikeSquadList = [];

        /// <summary>
        /// Gets or sets the list of roguelike squad.
        /// </summary>
        public ObservableCollection<CombinedData> RoguelikeSquadList
        {
            get => _roguelikeSquadList;
            set => SetAndNotify(ref _roguelikeSquadList, value);
        }

        /// <summary>
        /// Gets the list of roguelike roles.
        /// </summary>
        public List<CombinedData> RoguelikeRolesList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("DefaultRoles"), Value = string.Empty },
                new() { Display = LocalizationHelper.GetString("FirstMoveAdvantage"), Value = "先手必胜" },
                new() { Display = LocalizationHelper.GetString("SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
                new() { Display = LocalizationHelper.GetString("OvercomingYourWeaknesses"), Value = "取长补短" },
                new() { Display = LocalizationHelper.GetString("AsYourHeartDesires"), Value = "随心所欲" },
            ];

        // public List<CombData> RoguelikeCoreCharList { get; set; }
        private string _roguelikeTheme = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, "Sarkaz");

        /// <summary>
        /// Gets or sets the Roguelike theme.
        /// </summary>
        public string RoguelikeTheme
        {
            get => _roguelikeTheme;
            set
            {
                SetAndNotify(ref _roguelikeTheme, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeTheme, value);

                UpdateRoguelikeModeList();
                UpdateRoguelikeSquadList();
                UpdateRoguelikeCoreCharList();
            }
        }

        private int _roguelikeDifficulty = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDifficulty, int.MaxValue.ToString()));

        public int RoguelikeDifficulty
        {
            get => _roguelikeDifficulty;
            set
            {
                SetAndNotify(ref _roguelikeDifficulty, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDifficulty, value.ToString());
            }
        }

        private string _roguelikeMode = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMode, "0");

        /// <summary>
        /// Gets or sets 策略，往后打 / 刷一层就退 / 烧热水
        /// </summary>
        public string RoguelikeMode
        {
            get => _roguelikeMode;
            set
            {
                if (value == "1")
                {
                    RoguelikeInvestmentEnabled = true;
                }

                SetAndNotify(ref _roguelikeMode, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeMode, value);

                UpdateRoguelikeSquadList();
            }
        }

        private string _roguelikeSquad = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeSquad, string.Empty);

        /// <summary>
        /// Gets or sets the roguelike squad.
        /// </summary>
        public string RoguelikeSquad
        {
            get => _roguelikeSquad;
            set
            {
                SetAndNotify(ref _roguelikeSquad, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeSquad, value);
            }
        }

        public bool RoguelikeSquadIsProfessional =>
            RoguelikeMode == "4" &&
            RoguelikeTheme != "Phantom" &&
            RoguelikeSquad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";

        public bool RoguelikeSquadIsFoldartal =>
            RoguelikeMode == "4" &&
            RoguelikeTheme == "Sami" &&
            RoguelikeSquad == "生活至上分队";

        private string _roguelikeRoles = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRoles, string.Empty);

        /// <summary>
        /// Gets or sets the roguelike roles.
        /// </summary>
        public string RoguelikeRoles
        {
            get => _roguelikeRoles;
            set
            {
                SetAndNotify(ref _roguelikeRoles, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeRoles, value);
            }
        }

        private string _roguelikeCoreChar = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeCoreChar, string.Empty);

        /// <summary>
        /// Gets or sets the roguelike core character.
        /// </summary>
        public string RoguelikeCoreChar
        {
            get => _roguelikeCoreChar;
            set
            {
                if (_roguelikeCoreChar == (value ??= string.Empty))
                {
                    return;
                }

                SetAndNotify(ref _roguelikeCoreChar, value);
                Instances.TaskQueueViewModel.AddLog(value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeCoreChar, value);
            }
        }

        private ObservableCollection<string> _roguelikeCoreCharList = [];

        /// <summary>
        /// Gets the roguelike core character.
        /// </summary>
        public ObservableCollection<string> RoguelikeCoreCharList
        {
            get => _roguelikeCoreCharList;
            private set
            {
                if (!string.IsNullOrEmpty(RoguelikeCoreChar) && !value.Contains(RoguelikeCoreChar))
                {
                    value.Add(RoguelikeCoreChar);
                }

                SetAndNotify(ref _roguelikeCoreCharList, value);
            }
        }

        private bool _roguelikeStartWithEliteTwo = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartWithEliteTwo, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool RoguelikeStartWithEliteTwoRaw
        {
            get => _roguelikeStartWithEliteTwo;
            set
            {
                switch (value)
                {
                    case true when RoguelikeUseSupportUnit:
                        RoguelikeUseSupportUnit = false;
                        break;

                    case false when RoguelikeOnlyStartWithEliteTwoRaw:
                        RoguelikeOnlyStartWithEliteTwoRaw = false;
                        break;
                }

                SetAndNotify(ref _roguelikeStartWithEliteTwo, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeStartWithEliteTwo, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool RoguelikeStartWithEliteTwo => _roguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional;

        private bool _roguelikeOnlyStartWithEliteTwo = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether only need with elite two's core char.
        /// </summary>
        public bool RoguelikeOnlyStartWithEliteTwoRaw
        {
            get => _roguelikeOnlyStartWithEliteTwo;
            set
            {
                SetAndNotify(ref _roguelikeOnlyStartWithEliteTwo, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether only need with elite two's core char.
        /// </summary>
        public bool RoguelikeOnlyStartWithEliteTwo => _roguelikeOnlyStartWithEliteTwo && RoguelikeStartWithEliteTwo;

        private bool _roguelike3FirstFloorFoldartal = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool Roguelike3FirstFloorFoldartalRaw
        {
            get => _roguelike3FirstFloorFoldartal;
            set
            {
                SetAndNotify(ref _roguelike3FirstFloorFoldartal, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool Roguelike3FirstFloorFoldartal => _roguelike3FirstFloorFoldartal && RoguelikeMode == "4" && RoguelikeTheme == "Sami";

        private string _roguelike3StartFloorFoldartal = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3StartFloorFoldartal, string.Empty);

        public string Roguelike3StartFloorFoldartal
        {
            get => _roguelike3StartFloorFoldartal;
            set
            {
                SetAndNotify(ref _roguelike3StartFloorFoldartal, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Roguelike3StartFloorFoldartal, value);
            }
        }

        private bool _roguelike3NewSquad2StartingFoldartal = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool Roguelike3NewSquad2StartingFoldartalRaw
        {
            get => _roguelike3NewSquad2StartingFoldartal;
            set
            {
                SetAndNotify(ref _roguelike3NewSquad2StartingFoldartal, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether core char need start with elite two.
        /// </summary>
        public bool Roguelike3NewSquad2StartingFoldartal => _roguelike3NewSquad2StartingFoldartal && RoguelikeSquadIsFoldartal;

        private string _roguelike3NewSquad2StartingFoldartals = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals, string.Empty);

        public string Roguelike3NewSquad2StartingFoldartals
        {
            get => _roguelike3NewSquad2StartingFoldartals;
            set
            {
                SetAndNotify(ref _roguelike3NewSquad2StartingFoldartals, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals, value);
            }
        }

        private string _roguelikeExpectedCollapsalParadigms = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeExpectedCollapsalParadigms, string.Empty);

        /// <summary>
        /// Gets or sets the expected collapsal paradigms.
        /// 需要刷的坍缩列表，分号分隔
        /// </summary>
        public string RoguelikeExpectedCollapsalParadigms
        {
            get => _roguelikeExpectedCollapsalParadigms;
            set
            {
                SetAndNotify(ref _roguelikeExpectedCollapsalParadigms, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeExpectedCollapsalParadigms, value);
            }
        }

        private bool _roguelikeUseSupportUnit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeUseSupportUnit, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use support unit.
        /// </summary>
        public bool RoguelikeUseSupportUnit
        {
            get => _roguelikeUseSupportUnit;
            set
            {
                if (value && RoguelikeStartWithEliteTwo)
                {
                    RoguelikeStartWithEliteTwoRaw = false;
                }

                SetAndNotify(ref _roguelikeUseSupportUnit, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeUseSupportUnit, value.ToString());
            }
        }

        private bool _roguelikeEnableNonfriendSupport = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether can roguelike support unit belong to nonfriend
        /// </summary>
        public bool RoguelikeEnableNonfriendSupport
        {
            get => _roguelikeEnableNonfriendSupport;
            set
            {
                SetAndNotify(ref _roguelikeEnableNonfriendSupport, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, value.ToString());
            }
        }

        private bool _roguelikeDelayAbortUntilCombatComplete = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether delay abort until battle complete
        /// </summary>
        public bool RoguelikeDelayAbortUntilCombatComplete
        {
            get => _roguelikeDelayAbortUntilCombatComplete;
            set
            {
                SetAndNotify(ref _roguelikeDelayAbortUntilCombatComplete, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, value.ToString());
            }
        }

        private string _roguelikeStartsCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartsCount, "9999999");

        /// <summary>
        /// Gets or sets the start count of roguelike.
        /// </summary>
        public int RoguelikeStartsCount
        {
            get => int.Parse(_roguelikeStartsCount);
            set
            {
                SetAndNotify(ref _roguelikeStartsCount, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeStartsCount, value.ToString());
            }
        }

        private bool _roguelikeInvestmentEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether investment is enabled.
        /// </summary>
        public bool RoguelikeInvestmentEnabled
        {
            get => _roguelikeInvestmentEnabled;
            set
            {
                SetAndNotify(ref _roguelikeInvestmentEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, value.ToString());
            }
        }

        private bool _roguelikeInvestmentWithMoreScore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether investment is enabled.
        /// </summary>
        public bool RoguelikeInvestmentWithMoreScoreRaw
        {
            get => _roguelikeInvestmentWithMoreScore;
            set
            {
                SetAndNotify(ref _roguelikeInvestmentWithMoreScore, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether investment is enabled.
        /// </summary>
        public bool RoguelikeInvestmentWithMoreScore => _roguelikeInvestmentWithMoreScore && RoguelikeMode == "1";

        private bool _roguelikeRefreshTraderWithDice = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, bool.FalseString));

        public bool RoguelikeRefreshTraderWithDiceRaw
        {
            get => _roguelikeRefreshTraderWithDice;
            set
            {
                SetAndNotify(ref _roguelikeRefreshTraderWithDice, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, value.ToString());
            }
        }

        public bool RoguelikeRefreshTraderWithDice
        {
            get => _roguelikeRefreshTraderWithDice && RoguelikeTheme == "Mizuki";
            set
            {
                SetAndNotify(ref _roguelikeRefreshTraderWithDice, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, value.ToString());
            }
        }

        private string _roguelikeInvestsCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestsCount, "9999999");

        /// <summary>
        /// Gets or sets the invests count of roguelike.
        /// </summary>
        public int RoguelikeInvestsCount
        {
            get => int.Parse(_roguelikeInvestsCount);
            set
            {
                SetAndNotify(ref _roguelikeInvestsCount, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeInvestsCount, value.ToString());
            }
        }

        private bool _roguelikeStopWhenInvestmentFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to stop when investment is full.
        /// </summary>
        public bool RoguelikeStopWhenInvestmentFull
        {
            get => _roguelikeStopWhenInvestmentFull;
            set
            {
                SetAndNotify(ref _roguelikeStopWhenInvestmentFull, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, value.ToString());
            }
        }

        private bool _roguelikeStopAtFinalBoss = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopAtFinalBoss, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to stop when investment is full.
        /// </summary>
        public bool RoguelikeStopAtFinalBoss
        {
            get => _roguelikeStopAtFinalBoss;
            set
            {
                SetAndNotify(ref _roguelikeStopAtFinalBoss, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeStopAtFinalBoss, value.ToString());
            }
        }

        #endregion 肉鸽设置

        #region 生息演算设置

        /// <summary>
        /// Gets the list of reclamation themes.
        /// </summary>
        public List<CombinedData> ReclamationThemeList { get; } =
        [
            new() { Display = $"{LocalizationHelper.GetString("ReclamationThemeFire")} ({LocalizationHelper.GetString("ClosedStage")})", Value = "Fire" },
            new() { Display = LocalizationHelper.GetString("ReclamationThemeTales"), Value = "Tales" },
        ];

        private string _reclamationTheme = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationTheme, "Tales");

        /// <summary>
        /// Gets or sets the Reclamation theme.
        /// </summary>
        public string ReclamationTheme
        {
            get => _reclamationTheme;
            set
            {
                SetAndNotify(ref _reclamationTheme, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationTheme, value);
            }
        }

        /// <summary>
        /// Gets the list of reclamation modes.
        /// </summary>
        public List<CombinedData> ReclamationModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityNoSave"), Value = "0" },
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityInSave"), Value = "1" },
        ];

        private string _reclamationMode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMode, "1");

        /// <summary>
        /// Gets or sets 策略，无存档刷生息点数 / 有存档刷生息点数
        /// </summary>
        public string ReclamationMode
        {
            get => _reclamationMode;
            set
            {
                SetAndNotify(ref _reclamationMode, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationMode, value);
            }
        }

        private string _reclamationToolToCraft = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationToolToCraft, string.Empty);

        public string ReclamationToolToCraft
        {
            get
            {
                if (string.IsNullOrEmpty(_reclamationToolToCraft))
                {
                    return LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", _clientLanguageMapper[_clientType]);
                }

                return _reclamationToolToCraft;
            }

            set
            {
                SetAndNotify(ref _reclamationToolToCraft, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationToolToCraft, value);
            }
        }

        private int _reclamationIncrementMode = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationIncrementMode, "0"));

        public int ReclamationIncrementMode
        {
            get => _reclamationIncrementMode;
            set
            {
                SetAndNotify(ref _reclamationIncrementMode, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationIncrementMode, value.ToString());
            }
        }

        /// <summary>
        /// Gets the list of reclamation increment modes.
        /// </summary>
        public List<CombinedData> ReclamationIncrementModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeClick"), Value = "0" },
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeHold"), Value = "1" },
        ];

        private string _reclamationMaxCraftCountPerRound = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, "16");

        public int ReclamationMaxCraftCountPerRound
        {
            get => int.Parse(_reclamationMaxCraftCountPerRound);
            set
            {
                SetAndNotify(ref _reclamationMaxCraftCountPerRound, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, value.ToString());
            }
        }

        #endregion 生息演算设置

        #region 信用相关设置

        private string _lastCreditFightTaskTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

        public string LastCreditFightTaskTime
        {
            get => _lastCreditFightTaskTime;
            set
            {
                SetAndNotify(ref _lastCreditFightTaskTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LastCreditFightTaskTime, value);
            }
        }

        private bool _creditFightTaskEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether credit fight task is enabled.
        /// </summary>
        public bool CreditFightTaskEnabled
        {
            get
            {
                try
                {
                    if (DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(_lastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
                    {
                        return _creditFightTaskEnabled;
                    }
                }
                catch
                {
                    return _creditFightTaskEnabled;
                }

                return false;
            }

            set
            {
                SetAndNotify(ref _creditFightTaskEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightTaskEnabled, value.ToString());
            }
        }

        public bool CreditFightTaskEnabledDisplay
        {
            get
            {
                return _creditFightTaskEnabled;
            }

            set
            {
                SetAndNotify(ref _creditFightTaskEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightTaskEnabled, value.ToString());
            }
        }

        /// <summary>
        /// Gets 设置选择的编队
        /// </summary>
        // ReSharper disable once MemberCanBePrivate.Global
        public List<CombinedData> FormationSelectList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("Current"), Value = "0" },
                new() { Display = "1", Value = "1" },
                new() { Display = "2", Value = "2" },
                new() { Display = "3", Value = "3" },
                new() { Display = "4", Value = "4" },
            ];

        private string _lastCreditVisitFriendsTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditVisitFriendsTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

        public string LastCreditVisitFriendsTime
        {
            get => _lastCreditVisitFriendsTime;
            set
            {
                SetAndNotify(ref _lastCreditVisitFriendsTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LastCreditVisitFriendsTime, value);
            }
        }

        private bool _creditVisitOnceADay = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to bypass the daily limit.
        /// </summary>
        public bool CreditVisitOnceADay
        {
            get => _creditVisitOnceADay;
            set
            {
                SetAndNotify(ref _creditVisitOnceADay, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitOnceADay, value.ToString());
            }
        }

        private bool _creditVisitFriendsEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether visiting friends task is enabled.
        /// </summary>
        public bool CreditVisitFriendsEnabled
        {
            get
            {
                if (!_creditVisitOnceADay)
                {
                    return _creditVisitFriendsEnabled;
                }

                try
                {
                    return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(_lastCreditVisitFriendsTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture)
                           && _creditVisitFriendsEnabled;
                }
                catch
                {
                    return _creditVisitFriendsEnabled;
                }
            }

            set
            {
                SetAndNotify(ref _creditVisitFriendsEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitFriendsEnabled, value.ToString());
            }
        }

        public bool CreditVisitFriendsEnabledDisplay
        {
            get
            {
                return _creditVisitFriendsEnabled;
            }

            set
            {
                SetAndNotify(ref _creditVisitFriendsEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitFriendsEnabled, value.ToString());
            }
        }

        private int _creditFightSelectFormation = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightSelectFormation, "0"));

        /// <summary>
        /// Gets or sets a value indicating which formation will be select in credit fight.
        /// </summary>
        public int CreditFightSelectFormation
        {
            get => _creditFightSelectFormation;
            set
            {
                SetAndNotify(ref _creditFightSelectFormation, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightSelectFormation, value.ToString());
            }
        }

        private bool _creditShopping = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to shop with credit.
        /// </summary>
        public bool CreditShopping
        {
            get => _creditShopping;
            set
            {
                SetAndNotify(ref _creditShopping, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditShopping, value.ToString());
            }
        }

        private string _creditFirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault"));

        /// <summary>
        /// Gets or sets the priority item list of credit shop.
        /// </summary>
        public string CreditFirstList
        {
            get => _creditFirstList;
            set
            {
                SetAndNotify(ref _creditFirstList, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditFirstListNew, value);
            }
        }

        private string _creditBlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault"));

        /// <summary>
        /// Gets or sets the blacklist of credit shop.
        /// </summary>
        public string CreditBlackList
        {
            get => _creditBlackList;
            set
            {
                SetAndNotify(ref _creditBlackList, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditBlackListNew, value);
            }
        }

        private bool _creditForceShoppingIfCreditFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether save credit is enabled.
        /// </summary>
        public bool CreditForceShoppingIfCreditFull
        {
            get => _creditForceShoppingIfCreditFull;
            set
            {
                SetAndNotify(ref _creditForceShoppingIfCreditFull, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, value.ToString());
            }
        }

        private bool _creditOnlyBuyDiscount = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether only buy discount is enabled.
        /// </summary>
        public bool CreditOnlyBuyDiscount
        {
            get => _creditOnlyBuyDiscount;
            set
            {
                SetAndNotify(ref _creditOnlyBuyDiscount, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditOnlyBuyDiscount, value.ToString());
            }
        }

        private bool _creditReserveMaxCredit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether reserve max credit is enabled.
        /// </summary>
        public bool CreditReserveMaxCredit
        {
            get => _creditReserveMaxCredit;
            set
            {
                SetAndNotify(ref _creditReserveMaxCredit, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CreditReserveMaxCredit, value.ToString());
            }
        }

        #endregion 信用相关设置

        #region 领取奖励设置

        private bool _receiveAward = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether receive award is enabled.
        /// </summary>
        public bool ReceiveAward
        {
            get => _receiveAward;
            set
            {
                SetAndNotify(ref _receiveAward, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveAward, value.ToString());
            }
        }

        private bool _receiveMail = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether receive mail is enabled.
        /// </summary>
        public bool ReceiveMail
        {
            get => _receiveMail;
            set
            {
                SetAndNotify(ref _receiveMail, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveMail, value.ToString());
            }
        }

        private bool _receiveFreeRecruit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeRecruit, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether receive mail is enabled.
        /// </summary>
        public bool ReceiveFreeRecruit
        {
            get => _receiveFreeRecruit;
            set
            {
                if (value)
                {
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("GachaWarning"),
                        LocalizationHelper.GetString("Warning"),
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Warning,
                        no: LocalizationHelper.GetString("Confirm"),
                        yes: LocalizationHelper.GetString("Cancel"),
                        iconBrushKey: "DangerBrush");
                    if (result != MessageBoxResult.No)
                    {
                        return;
                    }
                }

                SetAndNotify(ref _receiveFreeRecruit, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveFreeRecruit, value.ToString());
            }
        }

        private bool _receiveOrundum = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether receive orundum is enabled.
        /// </summary>
        public bool ReceiveOrundum
        {
            get => _receiveOrundum;
            set
            {
                SetAndNotify(ref _receiveOrundum, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveOrundum, value.ToString());
            }
        }

        private bool _receiveMining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether receive mining is enabled.
        /// </summary>
        public bool ReceiveMining
        {
            get => _receiveMining;
            set
            {
                SetAndNotify(ref _receiveMining, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveMining, value.ToString());
            }
        }

        private bool _receiveReceiveSpecialAccess = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to collect special access rewards.
        /// </summary>
        public bool ReceiveSpecialAccess
        {
            get => _receiveReceiveSpecialAccess;
            set
            {
                SetAndNotify(ref _receiveReceiveSpecialAccess, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveSpecialAccess, value.ToString());
            }
        }

        #endregion 领取奖励设置

        #region 定时设置

        public class TimerModel
        {
            public class TimerProperties : PropertyChangedBase
            {
                public TimerProperties(int timeId, bool isOn, int hour, int min, string? timerConfig)
                {
                    TimerId = timeId;
                    _isOn = isOn;
                    _hour = hour;
                    _min = min;
                    if (timerConfig == null || !ConfigurationHelper.GetConfigurationList().Contains(timerConfig))
                    {
                        _timerConfig = ConfigurationHelper.GetCurrentConfiguration();
                    }
                    else
                    {
                        _timerConfig = timerConfig;
                    }
                }

                public int TimerId { get; set; }

                private readonly string _timerName = LocalizationHelper.GetString("Timer");

                public string TimerName
                {
                    get => $"{_timerName} {TimerId + 1}";
                }

                private bool _isOn;

                /// <summary>
                /// Gets or sets a value indicating whether the timer is set.
                /// </summary>
                public bool IsOn
                {
                    get => _isOn;
                    set
                    {
                        SetAndNotify(ref _isOn, value);
                        ConfigurationHelper.SetTimer(TimerId, value.ToString());
                    }
                }

                private int _hour;

                /// <summary>
                /// Gets or sets the hour of the timer.
                /// </summary>
                public int Hour
                {
                    get => _hour;
                    set
                    {
                        SetAndNotify(ref _hour, value);
                        ConfigurationHelper.SetTimerHour(TimerId, _hour.ToString());
                    }
                }

                private int _min;

                /// <summary>
                /// Gets or sets the minute of the timer.
                /// </summary>
                public int Min
                {
                    get => _min;
                    set
                    {
                        SetAndNotify(ref _min, value);
                        ConfigurationHelper.SetTimerMin(TimerId, _min.ToString());
                    }
                }

                private string? _timerConfig;

                /// <summary>
                /// Gets or sets the config of the timer.
                /// </summary>
                public string? TimerConfig
                {
                    get => _timerConfig;
                    set
                    {
                        SetAndNotify(ref _timerConfig, value ?? ConfigurationHelper.GetCurrentConfiguration());
                        ConfigurationHelper.SetTimerConfig(TimerId, _timerConfig);
                    }
                }
            }

            public TimerProperties[] Timers { get; set; } = new TimerProperties[8];

            public TimerModel()
            {
                for (int i = 0; i < 8; i++)
                {
                    Timers[i] = new TimerProperties(
                        i,
                        ConfigurationHelper.GetTimer(i, bool.FalseString) == bool.TrueString,
                        int.Parse(ConfigurationHelper.GetTimerHour(i, $"{i * 3}")),
                        int.Parse(ConfigurationHelper.GetTimerMin(i, "0")),
                        ConfigurationHelper.GetTimerConfig(i, ConfigurationHelper.GetCurrentConfiguration()));
                }
            }
        }

        public TimerModel TimerModels { get; set; } = new();

        private bool _forceScheduledStart = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ForceScheduledStart, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to force scheduled start.
        /// </summary>
        public bool ForceScheduledStart
        {
            get => _forceScheduledStart;
            set
            {
                SetAndNotify(ref _forceScheduledStart, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ForceScheduledStart, value.ToString());
            }
        }

        private bool _showWindowBeforeForceScheduledStart = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether show window before force scheduled start.
        /// </summary>
        public bool ShowWindowBeforeForceScheduledStart
        {
            get => _showWindowBeforeForceScheduledStart;
            set
            {
                SetAndNotify(ref _showWindowBeforeForceScheduledStart, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, value.ToString());
            }
        }

        private bool _customConfig = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.CustomConfig, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use custom config.
        /// </summary>
        public bool CustomConfig
        {
            get => _customConfig;
            set
            {
                SetAndNotify(ref _customConfig, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.CustomConfig, value.ToString());
            }
        }

        #endregion 定时设置

        #region 刷理智设置

        private string _penguinId = ConfigurationHelper.GetValue(ConfigurationKeys.PenguinId, string.Empty);

        /// <summary>
        /// Gets or sets the id of PenguinStats.
        /// </summary>
        public string PenguinId
        {
            get => _penguinId;
            set
            {
                SetAndNotify(ref _penguinId, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.PenguinId, value);
            }
        }

        private bool _enablePenguin = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.EnablePenguin, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to enable penguin upload.
        /// </summary>
        public bool EnablePenguin
        {
            get => _enablePenguin;
            set
            {
                SetAndNotify(ref _enablePenguin, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.EnablePenguin, value.ToString());
            }
        }

        private bool _enableYituliu = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.EnableYituliu, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to enable yituliu upload.
        /// </summary>
        public bool EnableYituliu
        {
            get => _enableYituliu;
            set
            {
                SetAndNotify(ref _enableYituliu, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.EnableYituliu, value.ToString());
            }
        }

        private bool _isDrGrandet = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IsDrGrandet, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use DrGrandet mode.
        /// </summary>
        public bool IsDrGrandet
        {
            get => _isDrGrandet;
            set
            {
                SetAndNotify(ref _isDrGrandet, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.IsDrGrandet, value.ToString());
            }
        }

        private bool _useAlternateStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseAlternateStage, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use alternate stage.
        /// </summary>
        public bool UseAlternateStage
        {
            get => _useAlternateStage;
            set
            {
                SetAndNotify(ref _useAlternateStage, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseAlternateStage, value.ToString());
                if (value)
                {
                    HideUnavailableStage = false;
                }
            }
        }

        private bool _useRemainingSanityStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, bool.TrueString));

        public bool UseRemainingSanityStage
        {
            get => _useRemainingSanityStage;
            set
            {
                SetAndNotify(ref _useRemainingSanityStage, value);
                Instances.TaskQueueViewModel.UseRemainingSanityStage = value;
                ConfigurationHelper.SetValue(ConfigurationKeys.UseRemainingSanityStage, value.ToString());
            }
        }

        private bool _allowUseStoneSave = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowUseStoneSave, bool.FalseString));

        public bool AllowUseStoneSave
        {
            get => _allowUseStoneSave;
            set
            {
                if (value)
                {
                    var result = MessageBoxHelper.Show(
                        LocalizationHelper.GetString("AllowUseStoneSaveWarning"),
                        LocalizationHelper.GetString("Warning"),
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Warning,
                        no: LocalizationHelper.GetString("Confirm"),
                        yes: LocalizationHelper.GetString("Cancel"),
                        iconBrushKey: "DangerBrush");
                    if (result != MessageBoxResult.No)
                    {
                        return;
                    }
                }

                SetAndNotify(ref _allowUseStoneSave, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AllowUseStoneSave, value.ToString());
            }
        }

        private bool _useExpiringMedicine = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseExpiringMedicine, bool.FalseString));

        public bool UseExpiringMedicine
        {
            get => _useExpiringMedicine;
            set
            {
                SetAndNotify(ref _useExpiringMedicine, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseExpiringMedicine, value.ToString());
                Instances.TaskQueueViewModel.SetFightParams();
            }
        }

        private bool _hideUnavailableStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideUnavailableStage, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide unavailable stages.
        /// </summary>
        public bool HideUnavailableStage
        {
            get => _hideUnavailableStage;
            set
            {
                SetAndNotify(ref _hideUnavailableStage, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.HideUnavailableStage, value.ToString());

                if (value)
                {
                    UseAlternateStage = false;
                }

                Instances.TaskQueueViewModel.UpdateStageList();
            }
        }

        private bool _hideSeries = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideSeries, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide series.
        /// </summary>
        public bool HideSeries
        {
            get => _hideSeries;
            set
            {
                SetAndNotify(ref _hideSeries, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.HideSeries, value.ToString());
            }
        }

        private bool _customStageCode = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomStageCode, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use custom stage code.
        /// </summary>
        public bool CustomStageCode
        {
            get => _customStageCode;
            set
            {
                SetAndNotify(ref _customStageCode, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomStageCode, value.ToString());
                Instances.TaskQueueViewModel.CustomStageCode = value;
            }
        }

        #endregion 刷理智设置

        #region 自动公招设置

        /// <summary>
        /// Gets the list of auto recruit selecting extra tags.
        /// </summary>
        public List<CombinedData> AutoRecruitSelectExtraTagsList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("DefaultNoExtraTags"), Value = "0" },
                new() { Display = LocalizationHelper.GetString("SelectExtraTags"), Value = "1" },
                new() { Display = LocalizationHelper.GetString("SelectExtraOnlyRareTags"), Value = "2" },
            ];

        private static readonly List<string> _autoRecruitTagList = ["近战位", "远程位", "先锋干员", "近卫干员", "狙击干员", "重装干员", "医疗干员", "辅助干员", "术师干员", "治疗", "费用回复", "输出", "生存", "群攻", "防护", "减速",];

        private static readonly Lazy<List<CombinedData>> _autoRecruitTagShowList = new(() =>
            _autoRecruitTagList.Select<string, (string, string)?>(tag => DataHelper.RecruitTags.TryGetValue(tag, out var value) ? value : null)
                .Where(tag => tag is not null)
                .Cast<(string Display, string Client)>()
                .Select(tag => new CombinedData() { Display = tag.Display, Value = tag.Client })
                .ToList());

        public static List<CombinedData> AutoRecruitTagShowList
        {
            get => _autoRecruitTagShowList.Value;
        }

        private object[] _autoRecruitFirstList = ConfigurationHelper
            .GetValue(ConfigurationKeys.AutoRecruitFirstList, string.Empty)
            .Split(';')
            .Select(tag => _autoRecruitTagShowList.Value.FirstOrDefault(i => i.Value == tag))
            .Where(tag => tag is not null)
            .Cast<CombinedData>()
            .ToArray();

        public object[] AutoRecruitFirstList
        {
            get => _autoRecruitFirstList;
            set
            {
                SetAndNotify(ref _autoRecruitFirstList, value);
                var config = string.Join(';', value.Cast<CombinedData>().Select(i => i.Value));
                ConfigurationHelper.SetValue(ConfigurationKeys.AutoRecruitFirstList, config);
            }
        }

        private string _recruitMaxTimes = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitMaxTimes, "4");

        /// <summary>
        /// Gets or sets the maximum times of recruit.
        /// </summary>
        public string RecruitMaxTimes
        {
            get => _recruitMaxTimes;
            set
            {
                SetAndNotify(ref _recruitMaxTimes, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitMaxTimes, value);
            }
        }

        private bool _refreshLevel3 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RefreshLevel3, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to refresh level 3.
        /// </summary>
        public bool RefreshLevel3
        {
            get => _refreshLevel3;
            set
            {
                SetAndNotify(ref _refreshLevel3, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RefreshLevel3, value.ToString());
            }
        }

        private bool _forceRefresh = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ForceRefresh, bool.TrueString));

        /// <summary>
        /// Gets or Sets a value indicating whether to refresh when recruit permit ran out.
        /// </summary>
        public bool ForceRefresh
        {
            get => _forceRefresh;
            set
            {
                SetAndNotify(ref _forceRefresh, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ForceRefresh, value.ToString());
            }
        }

        private bool _useExpedited;

        /// <summary>
        /// Gets or sets a value indicating whether to use expedited.
        /// </summary>
        public bool UseExpedited
        {
            get => _useExpedited;
            set => SetAndNotify(ref _useExpedited, value);
        }

        private string _selectExtraTags = ConfigurationHelper.GetValue(ConfigurationKeys.SelectExtraTags, "0");

        /// <summary>
        /// Gets or sets a value indicating three tags are always selected or select only rare tags as many as possible .
        /// </summary>
        public string SelectExtraTags
        {
            get
            {
                if (int.TryParse(_selectExtraTags, out _))
                {
                    return _selectExtraTags;
                }

                var value = "0";
                if (bool.TryParse(_selectExtraTags, out bool boolValue))
                {
                    value = boolValue ? "1" : "0";
                }

                SetAndNotify(ref _selectExtraTags, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.SelectExtraTags, value);
                return value;
            }

            set
            {
                SetAndNotify(ref _selectExtraTags, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.SelectExtraTags, value);
            }
        }

        private bool _notChooseLevel1 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.NotChooseLevel1, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether not to choose level 1.
        /// </summary>
        public bool NotChooseLevel1
        {
            get => _notChooseLevel1;
            set
            {
                SetAndNotify(ref _notChooseLevel1, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.NotChooseLevel1, value.ToString());
            }
        }

        private bool _chooseLevel3 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel3, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 3.
        /// </summary>
        public bool ChooseLevel3
        {
            get => _chooseLevel3;
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitChooseLevel3, value.ToString());
            }
        }

        private bool _chooseLevel4 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel4, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 4.
        /// </summary>
        public bool ChooseLevel4
        {
            get => _chooseLevel4;
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitChooseLevel4, value.ToString());
            }
        }

        private bool _chooseLevel5 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel5, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 5.
        /// </summary>
        public bool ChooseLevel5
        {
            get => _chooseLevel5;
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitChooseLevel5, value.ToString());
            }
        }

        private int _chooseLevel3Hour = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3Time, "540")) / 60;

        public int ChooseLevel3Hour
        {
            get => _chooseLevel3Hour;
            set
            {
                if (!SetAndNotify(ref _chooseLevel3Hour, value))
                {
                    return;
                }

                ChooseLevel3Time = (value * 60) + ChooseLevel3Min;
            }
        }

        private int _chooseLevel3Min = (Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3Time, "540")) % 60) / 10 * 10;

        public int ChooseLevel3Min
        {
            get => _chooseLevel3Min;
            set
            {
                if (!SetAndNotify(ref _chooseLevel3Min, value))
                {
                    return;
                }

                ChooseLevel3Time = (ChooseLevel3Hour * 60) + value;
            }
        }

        private int _chooseLevel3Time = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3Time, "540"));

        public int ChooseLevel3Time
        {
            get => _chooseLevel3Time;
            set
            {
                value = value switch
                {
                    < 60 => 540,
                    > 540 => 60,
                    _ => value / 10 * 10,
                };

                SetAndNotify(ref _chooseLevel3Time, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel3Time, value.ToString());
                SetAndNotify(ref _chooseLevel3Hour, value / 60, nameof(ChooseLevel3Hour));
                SetAndNotify(ref _chooseLevel3Min, value % 60, nameof(ChooseLevel3Min));
            }
        }

        private int _chooseLevel4Hour = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4Time, "540")) / 60;

        public int ChooseLevel4Hour
        {
            get => _chooseLevel4Hour;
            set
            {
                if (!SetAndNotify(ref _chooseLevel4Hour, value))
                {
                    return;
                }

                ChooseLevel4Time = (value * 60) + ChooseLevel4Min;
            }
        }

        private int _chooseLevel4Min = (Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4Time, "540")) % 60) / 10 * 10;

        public int ChooseLevel4Min
        {
            get => _chooseLevel4Min;
            set
            {
                if (!SetAndNotify(ref _chooseLevel4Min, value))
                {
                    return;
                }

                ChooseLevel4Time = (ChooseLevel4Hour * 60) + value;
            }
        }

        private int _chooseLevel4Time = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4Time, "540"));

        public int ChooseLevel4Time
        {
            get => _chooseLevel4Time;
            set
            {
                value = value switch
                {
                    < 60 => 540,
                    > 540 => 60,
                    _ => value / 10 * 10,
                };

                SetAndNotify(ref _chooseLevel4Time, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel4Time, value.ToString());
                SetAndNotify(ref _chooseLevel4Hour, value / 60, nameof(ChooseLevel4Hour));
                SetAndNotify(ref _chooseLevel4Min, value % 60, nameof(ChooseLevel4Min));
            }
        }

        private int _chooseLevel5Hour = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5Time, "540")) / 60;

        public int ChooseLevel5Hour
        {
            get => _chooseLevel5Hour;
            set
            {
                if (!SetAndNotify(ref _chooseLevel5Hour, value))
                {
                    return;
                }

                ChooseLevel5Time = (value * 60) + ChooseLevel5Min;
            }
        }

        private int _chooseLevel5Min = (Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5Time, "540")) % 60) / 10 * 10;

        public int ChooseLevel5Min
        {
            get => _chooseLevel5Min;
            set
            {
                if (!SetAndNotify(ref _chooseLevel5Min, value))
                {
                    return;
                }

                ChooseLevel5Time = (ChooseLevel5Hour * 60) + value;
            }
        }

        private int _chooseLevel5Time = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5Time, "540"));

        public int ChooseLevel5Time
        {
            get => _chooseLevel5Time;
            set
            {
                value = value switch
                {
                    < 60 => 540,
                    > 540 => 60,
                    _ => value / 10 * 10,
                };

                SetAndNotify(ref _chooseLevel5Time, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel5Time, value.ToString());
                SetAndNotify(ref _chooseLevel5Hour, value / 60, nameof(ChooseLevel5Hour));
                SetAndNotify(ref _chooseLevel5Min, value % 60, nameof(ChooseLevel5Min));
            }
        }

        #endregion 自动公招设置

        #region 连接设置

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

        /// <summary>
        /// Gets the list of the client types.
        /// </summary>
        public List<CombinedData> ClientTypeList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
                new() { Display = LocalizationHelper.GetString("Official"), Value = "Official" },
                new() { Display = LocalizationHelper.GetString("Bilibili"), Value = "Bilibili" },
                new() { Display = LocalizationHelper.GetString("YoStarEN"), Value = "YoStarEN" },
                new() { Display = LocalizationHelper.GetString("YoStarJP"), Value = "YoStarJP" },
                new() { Display = LocalizationHelper.GetString("YoStarKR"), Value = "YoStarKR" },
                new() { Display = LocalizationHelper.GetString("Txwy"), Value = "txwy" },
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
                UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
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
                    int count = 3;
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
                UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
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

                                using RegistryKey? driverKey = Registry.LocalMachine.OpenSubKey(UninstallKeyPath);
                                if (driverKey == null)
                                {
                                    EmulatorPath = string.Empty;
                                    return;
                                }

                                string? uninstallString = driverKey.GetValue("UninstallString") as string;

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

                                using RegistryKey? driverKey = Registry.CurrentUser.OpenSubKey(UninstallKeyPath);
                                if (driverKey == null)
                                {
                                    EmulatorPath = string.Empty;
                                    return;
                                }

                                string? installDir = driverKey.GetValue(InstallDirValueName) as string;

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
                string emulatorPath = $@"{EmulatorPath}\ldconsole.exe";
                if (!File.Exists(emulatorPath))
                {
                    MessageBoxHelper.Show("LD Emulator Path Not Found");
                    return 0;
                }

                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = emulatorPath,
                    Arguments = "list2",
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };

                using Process? process = Process.Start(startInfo);
                if (process == null)
                {
                    _logger.Warning("Failed to start ldconsole list2");
                    return 0;
                }

                using StreamReader reader = process.StandardOutput;
                string result = reader.ReadToEnd();
                string[] lines = result.Split([Environment.NewLine], StringSplitOptions.RemoveEmptyEntries);

                if (lines.Length <= 0)
                {
                    _logger.Warning("Failed to get emulator PID.");
                    return 0;
                }

                foreach (string line in lines)
                {
                    string[] parts = line.Split(',');
                    if (parts.Length < 6 || !int.TryParse(parts[0], out int currentIndex) || currentIndex != index)
                    {
                        continue;
                    }

                    if (int.TryParse(parts[5], out int pid))
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
                if (string.IsNullOrEmpty(EmulatorPath))
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

        private bool _deploymentWithPause = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, bool.FalseString));

        public bool DeploymentWithPause
        {
            get => _deploymentWithPause;
            set
            {
                SetAndNotify(ref _deploymentWithPause, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, value.ToString());
                UpdateInstanceSettings();
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
                    ConnectAddress = addresses.First();
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

            string errMsg = string.Empty;
            TestLinkInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
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

            Window popupWindow = new Window
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

        /// <summary>
        /// 标题栏显示模拟器名称和IP端口。
        /// </summary>
        public void UpdateWindowTitle()
        {
            var rvm = (RootViewModel)this.Parent;

            string prefix = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitlePrefix, string.Empty);
            if (!string.IsNullOrEmpty(prefix))
            {
                prefix += " - ";
            }

            List<string> windowTitleSelectShowList = _windowTitleSelectShowList
                .Where(x => _windowTitleAllShowDict.ContainsKey(x?.ToString() ?? string.Empty))
                .Select(x => _windowTitleAllShowDict[x?.ToString() ?? string.Empty]).ToList();

            string currentConfiguration = string.Empty;
            string connectConfigName = string.Empty;
            string connectAddress = string.Empty;
            string clientName = string.Empty;

            foreach (var select in windowTitleSelectShowList)
            {
                switch (select)
                {
                    case "1": // 配置名
                        currentConfiguration = $" ({CurrentConfiguration})";
                        break;

                    case "2": // 连接模式
                        foreach (var data in ConnectConfigList.Where(data => data.Value == ConnectConfig))
                        {
                            connectConfigName = $" - {data.Display}";
                        }

                        break;

                    case "3": // 端口地址
                        connectAddress = $" ({ConnectAddress})";
                        break;

                    case "4": // 客户端类型
                        clientName = $" - {ClientName}";
                        break;
                }
            }

            string resourceVersion = !string.IsNullOrEmpty(VersionUpdateDataContext.ResourceVersion)
                ? LocalizationHelper.CustomCultureInfo.Name.ToLowerInvariant() switch
                {
                    "zh-cn" => $" - {VersionUpdateDataContext.ResourceVersion}{VersionUpdateDataContext.ResourceDateTime:#MMdd}",
                    "zh-tw" => $" - {VersionUpdateDataContext.ResourceVersion}{VersionUpdateDataContext.ResourceDateTime:#MMdd}",
                    "en-us" => $" - {VersionUpdateDataContext.ResourceDateTime:dd/MM} {VersionUpdateDataContext.ResourceVersion}",
                    _ => $" - {VersionUpdateDataContext.ResourceDateTime.ToString(LocalizationHelper.CustomCultureInfo.DateTimeFormat.ShortDatePattern.Replace("yyyy", string.Empty).Trim('/', '.'))} {VersionUpdateDataContext.ResourceVersion}",
                }
                : string.Empty;
            rvm.WindowTitle = $"{prefix}MAA{currentConfiguration} - {CoreVersion}{resourceVersion}{connectConfigName}{connectAddress}{clientName}";
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
                AskRestartToApplySettings();
            }
        }

        public void UpdateInstanceSettings()
        {
            Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.TouchMode, TouchMode);
            Instances.AsstProxy.AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, DeploymentWithPause ? "1" : "0");
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

            bool replaced = false;
            if (AdbPath != NewAdb && File.Exists(AdbPath))
            {
                try
                {
                    foreach (var process in Process.GetProcessesByName(Path.GetFileName(AdbPath)))
                    {
                        process.Kill();
                        process.WaitForExit(5000);
                    }

                    string adbBack = AdbPath + ".bak";
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

        #endregion 连接设置

        #region 界面设置

        /// <summary>
        /// Gets or sets the language list.
        /// </summary>
        public List<CombinedData> LanguageList { get; set; }

        /// <summary>
        /// Gets or sets the list of operator name language settings
        /// </summary>
        public List<CombinedData> OperNameLanguageModeList { get; set; } =
            [
                new() { Display = LocalizationHelper.GetString("OperNameLanguageMAA"), Value = "OperNameLanguageMAA" },
                new() { Display = LocalizationHelper.GetString("OperNameLanguageClient"), Value = "OperNameLanguageClient" }
            ];

        /// <summary>
        /// Gets the list of dark mode.
        /// </summary>
        public List<GenericCombinedData<DarkModeType>> DarkModeList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("Light"), Value = DarkModeType.Light },
                new() { Display = LocalizationHelper.GetString("Dark"), Value = DarkModeType.Dark },
                new() { Display = LocalizationHelper.GetString("SyncWithOs"), Value = DarkModeType.SyncWithOs },
            ];

        /// <summary>
        /// Gets the list of inverse clear modes.
        /// </summary>
        public List<CombinedData> InverseClearModeList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("Clear"), Value = "Clear" },
                new() { Display = LocalizationHelper.GetString("Inverse"), Value = "Inverse" },
                new() { Display = LocalizationHelper.GetString("Switchable"), Value = "ClearInverse" },
            ];

        private bool _useTray = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseTray, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to use tray icon.
        /// </summary>
        public bool UseTray
        {
            get => _useTray;
            set
            {
                if (!value)
                {
                    MinimizeToTray = false;
                }

                SetAndNotify(ref _useTray, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseTray, value.ToString());
                Instances.MainWindowManager.SetUseTrayIcon(value);
            }
        }

        private bool _minimizeToTray = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizeToTray, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimize to tray.
        /// </summary>
        public bool MinimizeToTray
        {
            get => _minimizeToTray;
            set
            {
                SetAndNotify(ref _minimizeToTray, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.MinimizeToTray, value.ToString());
                Instances.MainWindowManager.SetMinimizeToTray(value);
            }
        }

        private bool _windowTitleScrollable = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitleScrollable, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to make window title scrollable.
        /// </summary>
        public bool WindowTitleScrollable
        {
            get => _windowTitleScrollable;
            set
            {
                SetAndNotify(ref _windowTitleScrollable, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.WindowTitleScrollable, value.ToString());
                var rvm = (RootViewModel)this.Parent;
                rvm.WindowTitleScrollable = value;
            }
        }

        private bool _hideCloseButton = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideCloseButton, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide close button.
        /// </summary>
        public bool HideCloseButton
        {
            get => _hideCloseButton;
            set
            {
                SetAndNotify(ref _hideCloseButton, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.HideCloseButton, value.ToString());
                var rvm = (RootViewModel)this.Parent;
                rvm.ShowCloseButton = !value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use notification.
        /// </summary>
        public bool UseNotify
        {
            get => ConfigFactory.CurrentConfig.GUI.UseNotify;
            set
            {
                ConfigFactory.CurrentConfig.GUI.UseNotify = value;
                NotifyOfPropertyChange();
                if (value)
                {
                    ToastNotification.ShowDirect("Test test");
                }
            }
        }

        private bool _useLogItemDateFormat = true; // Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseLogItemDateFormat, bool.FalseString));

        public bool UseLogItemDateFormat
        {
            get => _useLogItemDateFormat;
            set
            {
                SetAndNotify(ref _useLogItemDateFormat, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseLogItemDateFormat, value.ToString());
            }
        }

        public List<string> LogItemDateFormatStringList { get; } =
        [
            "HH:mm:ss",
            "MM-dd  HH:mm:ss",
            "MM/dd  HH:mm:ss",
            "MM.dd  HH:mm:ss",
            "dd-MM  HH:mm:ss",
            "dd/MM  HH:mm:ss",
            "dd.MM  HH:mm:ss",
        ];

        private string _logItemDateFormatString = ConfigurationHelper.GetValue(ConfigurationKeys.LogItemDateFormat, "HH:mm:ss");

        public string LogItemDateFormatString
        {
            get => _logItemDateFormatString;
            set
            {
                SetAndNotify(ref _logItemDateFormatString, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LogItemDateFormat, value);
            }
        }

        /// <summary>
        /// Gets or sets the dark mode.
        /// </summary>
        public DarkModeType DarkMode
        {
            get => ConfigFactory.CurrentConfig.GUI.DarkMode;
            set
            {
                ConfigFactory.CurrentConfig.GUI.DarkMode = value;
                NotifyOfPropertyChange();
                SwitchDarkMode();

                /*
                AskToRestartToApplySettings();
                */
            }
        }

        public void SwitchDarkMode()
        {
            DarkModeType darkModeType = ConfigFactory.CurrentConfig.GUI.DarkMode;
            switch (darkModeType)
            {
                case DarkModeType.Light:
                    ThemeHelper.SwitchToLightMode();
                    break;

                case DarkModeType.Dark:
                    ThemeHelper.SwitchToDarkMode();
                    break;

                case DarkModeType.SyncWithOs:
                    ThemeHelper.SwitchToSyncWithOsMode();
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private enum InverseClearType
        {
            Clear,
            Inverse,
            ClearInverse,
        }

        private InverseClearType _inverseClearMode =
            Enum.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.InverseClearMode, InverseClearType.Clear.ToString()), out InverseClearType temp)
                ? temp
                : InverseClearType.Clear;

        /// <summary>
        /// Gets or sets the inverse clear mode.
        /// </summary>
        public string InverseClearMode
        {
            get => _inverseClearMode.ToString();
            set
            {
                if (!Enum.TryParse(value, out InverseClearType tempEnumValue))
                {
                    return;
                }

                SetAndNotify(ref _inverseClearMode, tempEnumValue);
                ConfigurationHelper.SetValue(ConfigurationKeys.InverseClearMode, value);
                switch (tempEnumValue)
                {
                    case InverseClearType.Clear:
                        Instances.TaskQueueViewModel.InverseMode = false;
                        Instances.TaskQueueViewModel.ShowInverse = false;
                        Instances.TaskQueueViewModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.Inverse:
                        Instances.TaskQueueViewModel.InverseMode = true;
                        Instances.TaskQueueViewModel.ShowInverse = false;
                        Instances.TaskQueueViewModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.ClearInverse:
                        Instances.TaskQueueViewModel.ShowInverse = true;
                        Instances.TaskQueueViewModel.SelectedAllWidth = TaskQueueViewModel.SelectedAllWidthWhenBoth;
                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        private static readonly Dictionary<string, string> _windowTitleAllShowDict = new()
        {
            { LocalizationHelper.GetString("ConfigurationName"), "1" },
            { LocalizationHelper.GetString("ConnectionPreset"), "2" },
            { LocalizationHelper.GetString("ConnectionAddress"), "3" },
            { LocalizationHelper.GetString("ClientType"), "4" },
        };

        private List<string> _windowTitleAllShowList = [.. _windowTitleAllShowDict.Keys];

        public List<string> WindowTitleAllShowList
        {
            get => _windowTitleAllShowList;
            set => SetAndNotify(ref _windowTitleAllShowList, value);
        }

        private object[] _windowTitleSelectShowList = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitleSelectShowList, "1 2 3 4")
            .Split(' ')
            .Where(s => _windowTitleAllShowDict.ContainsValue(s.ToString()))
            .Select(s => _windowTitleAllShowDict.FirstOrDefault(pair => pair.Value == s).Key)
            .ToArray();

        public object[] WindowTitleSelectShowList
        {
            get => _windowTitleSelectShowList;
            set
            {
                SetAndNotify(ref _windowTitleSelectShowList, value);
                UpdateWindowTitle();
                var config = string.Join(' ', _windowTitleSelectShowList.Cast<string>().Select(s => _windowTitleAllShowDict[s]));
                ConfigurationHelper.SetValue(ConfigurationKeys.WindowTitleSelectShowList, config);
            }
        }

        private string _language = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);

        /// <summary>
        /// Gets or sets the language.
        /// </summary>
        public string Language
        {
            get => _language;
            set
            {
                if (value == _language)
                {
                    return;
                }

                if (_language == PallasLangKey)
                {
                    Hangover = true;
                    Cheers = false;
                }

                if (value != PallasLangKey)
                {
                    SoberLanguage = value;
                }

                // var backup = _language;
                ConfigurationHelper.SetValue(ConfigurationKeys.Localization, value);

                var mainWindow = Application.Current.MainWindow;

                if (mainWindow != null)
                {
                    mainWindow.Show();
                    mainWindow.WindowState = mainWindow.WindowState = WindowState.Normal;
                    mainWindow.Activate();
                }

                var result = MessageBoxHelper.Show(
                    FormatText("{0}\n{1}", "LanguageChangedTip"),
                    FormatText("{0}({1})", "Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Question,
                    ok: FormatText("{0}({1})", "Ok"),
                    cancel: FormatText("{0}({1})", "ManualRestart"));
                if (result == MessageBoxResult.OK)
                {
                    Bootstrapper.ShutdownAndRestartWithoutArgs();
                }

                SetAndNotify(ref _language, value);

                return;

                string FormatText(string text, string key)
                    => string.Format(text, LocalizationHelper.GetString(key, value), LocalizationHelper.GetString(key, _language));
            }
        }

        /// <summary>
        /// Gets the language info.
        /// </summary>
        public string LanguageInfo
        {
            get
            {
                var language = (string)Application.Current.Resources["Language"];
                return language == "Language" ? language : language + " / Language";
            }
        }

        private static readonly Dictionary<string, string> _clientLanguageMapper = new()
        {
            { string.Empty, "zh-cn" },
            { "Official", "zh-cn" },
            { "Bilibili", "zh-cn" },
            { "YoStarEN", "en-us" },
            { "YoStarJP", "ja-jp" },
            { "YoStarKR", "ko-kr" },
            { "txwy", "zh-tw" },
        };

        /// <summary>
        /// Opername display language, can set force display when it was set as "OperNameLanguageForce.en-us"
        /// </summary>
        private string _operNameLanguage = ConfigurationHelper.GetValue(ConfigurationKeys.OperNameLanguage, "OperNameLanguageMAA");

        public string OperNameLanguage
        {
            get
            {
                if (!_operNameLanguage.Contains('.'))
                {
                    return _operNameLanguage;
                }

                if (_operNameLanguage.Split('.')[0] != "OperNameLanguageForce" || !LocalizationHelper.SupportedLanguages.ContainsKey(_operNameLanguage.Split('.')[1]))
                {
                    return _operNameLanguage;
                }

                OperNameLanguageModeList.Add(new CombinedData { Display = LocalizationHelper.GetString("OperNameLanguageForce"), Value = "OperNameLanguageForce" });
                return "OperNameLanguageForce";
            }

            set
            {
                if (value == _operNameLanguage.Split('.')[0])
                {
                    return;
                }

                switch (value)
                {
                    case "OperNameLanguageClient":
                        ConfigurationHelper.SetValue(ConfigurationKeys.OperNameLanguage, value);
                        break;

                    case "OperNameLanguageMAA":
                    default:
                        ConfigurationHelper.SetValue(ConfigurationKeys.OperNameLanguage, "OperNameLanguageMAA");
                        break;
                }

                var mainWindow = Application.Current.MainWindow;

                if (mainWindow != null)
                {
                    mainWindow.Show();
                    mainWindow.WindowState = mainWindow.WindowState = WindowState.Normal;
                    mainWindow.Activate();
                }

                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("LanguageChangedTip"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Question,
                    ok: LocalizationHelper.GetString("Ok"),
                    cancel: LocalizationHelper.GetString("ManualRestart"));
                if (result == MessageBoxResult.OK)
                {
                    Bootstrapper.ShutdownAndRestartWithoutArgs();
                }

                SetAndNotify(ref _operNameLanguage, value);
            }
        }

        public string OperNameLocalization
        {
            get
            {
                if (_operNameLanguage == "OperNameLanguageClient")
                {
                    return _clientLanguageMapper[_clientType];
                }

                if (!_operNameLanguage.Contains('.'))
                {
                    return _language;
                }

                if (_operNameLanguage.Split('.')[0] == "OperNameLanguageForce" && LocalizationHelper.SupportedLanguages.ContainsKey(_operNameLanguage.Split('.')[1]))
                {
                    return _operNameLanguage.Split('.')[1];
                }

                return _language;
            }
        }

        #endregion 界面设置

        #region HotKey

        /// <summary>
        /// Gets or sets the hotkey: ShowGui.
        /// </summary>
        public static MaaHotKey HotKeyShowGui
        {
            get => Instances.MaaHotKeyManager.GetOrNull(MaaHotKeyAction.ShowGui);
            set => SetHotKey(MaaHotKeyAction.ShowGui, value);
        }

        /// <summary>
        /// Gets or sets the hotkey: LinkStart.
        /// </summary>
        public static MaaHotKey HotKeyLinkStart
        {
            get => Instances.MaaHotKeyManager.GetOrNull(MaaHotKeyAction.LinkStart);
            set => SetHotKey(MaaHotKeyAction.LinkStart, value);
        }

        private static void SetHotKey(MaaHotKeyAction action, MaaHotKey value)
        {
            if (value != null)
            {
                Instances.MaaHotKeyManager.TryRegister(action, value);
            }
            else
            {
                Instances.MaaHotKeyManager.UnRegister(action);
            }
        }

        #endregion HotKey

        #region 配置

        public ObservableCollection<CombinedData> ConfigurationList { get; set; }

        private string? _currentConfiguration = ConfigurationHelper.GetCurrentConfiguration();

        public string? CurrentConfiguration
        {
            get => _currentConfiguration;
            set
            {
                SetAndNotify(ref _currentConfiguration, value);
                ConfigurationHelper.SwitchConfiguration(value);

                Bootstrapper.ShutdownAndRestartWithoutArgs();
            }
        }

        private string _newConfigurationName;

        public string NewConfigurationName
        {
            get => _newConfigurationName;
            set => SetAndNotify(ref _newConfigurationName, value);
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void AddConfiguration()
        {
            if (string.IsNullOrEmpty(NewConfigurationName))
            {
                NewConfigurationName = DateTime.Now.ToString("yy/MM/dd HH:mm:ss");
            }

            if (ConfigurationHelper.AddConfiguration(NewConfigurationName, CurrentConfiguration))
            {
                ConfigurationList.Add(new CombinedData { Display = NewConfigurationName, Value = NewConfigurationName });

                var growlInfo = new GrowlInfo
                {
                    IsCustom = true,
                    Message = string.Format(LocalizationHelper.GetString("AddConfigSuccess"), NewConfigurationName),
                    IconKey = "HangoverGeometry",
                    IconBrushKey = "PallasBrush",
                };
                Growl.Info(growlInfo);
            }
            else
            {
                var growlInfo = new GrowlInfo
                {
                    IsCustom = true,
                    Message = string.Format(LocalizationHelper.GetString("ConfigExists"), NewConfigurationName),
                    IconKey = "HangoverGeometry",
                    IconBrushKey = "PallasBrush",
                };
                Growl.Info(growlInfo);
            }
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void DeleteConfiguration(CombinedData delete)
        {
            if (ConfigurationHelper.DeleteConfiguration(delete.Display))
            {
                ConfigurationList.Remove(delete);
            }
        }

        #endregion 配置

        #region SettingsGuide

        // 目前共4步，再多塞不下了，后续可以整个新功能展示（
        public const int GuideMaxStep = 4;

        private int _guideStepIndex = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, "0"));

        public int GuideStepIndex
        {
            get => _guideStepIndex;
            set
            {
                SetAndNotify(ref _guideStepIndex, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.GuideStepIndex, value.ToString());
            }
        }

        private string _guideTransitionMode = "Bottom2Top";

        public string GuideTransitionMode
        {
            get => _guideTransitionMode;
            set => SetAndNotify(ref _guideTransitionMode, value);
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void NextGuide(StepBar stepBar)
        {
            GuideTransitionMode = "Bottom2Top";
            stepBar.Next();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void PrevGuide(StepBar stepBar)
        {
            GuideTransitionMode = "Top2Bottom";
            stepBar.Prev();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void DoneGuide()
        {
            TaskSettingVisibilities.Guide = false;
            GuideStepIndex = GuideMaxStep;
        }

        #endregion SettingsGuide

        /// <summary>
        /// Requires the user to restart to apply settings.
        /// </summary>
        /// <param name="isYostarEN">Whether to include the YostarEN resolution tip.</param>
        private static void AskRestartToApplySettings(bool isYostarEN = false)
        {
            var resolutionTip = isYostarEN ? "\n" + LocalizationHelper.GetString("SwitchResolutionTip") : string.Empty;

            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("PromptRestartForSettingsChange") + resolutionTip,
                LocalizationHelper.GetString("Tip"),
                MessageBoxButton.OKCancel,
                MessageBoxImage.Question);

            if (result == MessageBoxResult.OK)
            {
                Bootstrapper.ShutdownAndRestartWithoutArgs();
            }
        }

        /// <summary>
        /// Make comboBox searchable
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event args</param>
        // UI 绑定的方法
        // EventArgs 不能省略，否则会报错
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
        public void MakeComboBoxSearchable(object sender, EventArgs e)
        {
            (sender as ComboBox)?.MakeComboBoxSearchable();
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public async Task CheckAndDownloadAnnouncement()
        {
            await Instances.AnnouncementViewModel.CheckAndDownloadAnnouncement();
            _ = Execute.OnUIThreadAsync(() => Instances.WindowManager.ShowWindow(Instances.AnnouncementViewModel));
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void SetAcknowledgedNightlyWarning()
        {
            VersionUpdateDataContext.HasAcknowledgedNightlyWarning = true;
        }
    }
}
