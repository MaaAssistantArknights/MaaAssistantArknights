// <copyright file="SettingsViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using HandyControl.Themes;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.Web;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Newtonsoft.Json;
using Stylet;
using IContainer = StyletIoC.IContainer;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of settings.
    /// </summary>
    public class SettingsViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;
        private IMaaHotKeyManager _maaHotKeyManager;
        private IMainWindowManager _mainWindowManager;
        private TaskQueueViewModel _taskQueueViewModel;
        private AsstProxy _asstProxy;
        private VersionUpdateViewModel _versionUpdateViewModel;
        private readonly IHttpService _httpService;

        [DllImport("MaaCore.dll")]
        private static extern IntPtr AsstGetVersion();

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        private const int SWMINIMIZE = 6;

        [DllImport("user32.dll")]
        private static extern bool IsIconic(IntPtr hWnd);

        private static readonly string s_versionId = Marshal.PtrToStringAnsi(AsstGetVersion());

        /// <summary>
        /// Gets the version id.
        /// </summary>
        public string VersionId => s_versionId;

        private static readonly string s_versionInfo = LocalizationHelper.GetString("Version") + ": " + s_versionId;

        /// <summary>
        /// Gets the version info.
        /// </summary>
        public string VersionInfo => s_versionInfo;

        /// <summary>
        /// The Pallas language key.
        /// </summary>
        public static readonly string PallasLangKey = "pallas";

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public SettingsViewModel(IContainer container)
        {
            _container = container;
            _windowManager = container.Get<Helper.WindowManager>();
            _httpService = container.Get<IHttpService>();

            DisplayName = LocalizationHelper.GetString("Settings");
            _listTitle.Add(LocalizationHelper.GetString("GameSettings"));
            _listTitle.Add(LocalizationHelper.GetString("BaseSettings"));
            _listTitle.Add(LocalizationHelper.GetString("RoguelikeSettings"));
            _listTitle.Add(LocalizationHelper.GetString("RecruitingSettings"));
            _listTitle.Add(LocalizationHelper.GetString("MallSettings"));
            _listTitle.Add(LocalizationHelper.GetString("OtherCombatSettings"));
            _listTitle.Add(LocalizationHelper.GetString("ConnectionSettings"));
            _listTitle.Add(LocalizationHelper.GetString("StartupSettings"));
            _listTitle.Add(LocalizationHelper.GetString("ScheduleSettings"));
            _listTitle.Add(LocalizationHelper.GetString("UISettings"));
            _listTitle.Add(LocalizationHelper.GetString("HotKeySettings"));
            _listTitle.Add(LocalizationHelper.GetString("UpdateSettings"));
            _listTitle.Add(LocalizationHelper.GetString("AboutUs"));

            InfrastInit();

            SystemEvents.UserPreferenceChanged += SystemEvents_UserPreferenceChanged;
            SwitchDarkMode();
            if (Hangover)
            {
                Hangover = false;
                MessageBoxHelper.Show(
                    LocalizationHelper.GetString("Hangover"),
                    LocalizationHelper.GetString("Burping"),
                    MessageBoxButton.OK,
                    iconKey: "HangoverGeometry",
                    iconBrushKey: "PallasBrush");
                Application.Current.Shutdown();
                System.Windows.Forms.Application.Restart();
            }
        }

        public void Sober()
        {
            if (Cheers && Language == PallasLangKey)
            {
                ConfigurationHelper.SetValue(ConfigurationKeys.Localization, SoberLanguage);
                Hangover = true;
                Cheers = false;
            }
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _maaHotKeyManager = _container.Get<IMaaHotKeyManager>();
            _mainWindowManager = _container.Get<IMainWindowManager>();
            _taskQueueViewModel = _container.Get<TaskQueueViewModel>();
            _asstProxy = _container.Get<AsstProxy>();
            _versionUpdateViewModel = _container.Get<VersionUpdateViewModel>();

            var addressListJson = ConfigurationHelper.GetValue(ConfigurationKeys.AddressHistory, string.Empty);
            if (!string.IsNullOrEmpty(addressListJson))
            {
                ConnectAddressHistory = JsonConvert.DeserializeObject<ObservableCollection<string>>(addressListJson);
            }
        }

        private List<string> _listTitle = new List<string>();

        /// <summary>
        /// Gets or sets the list title.
        /// </summary>
        public List<string> ListTitle
        {
            get => _listTitle;
            set => SetAndNotify(ref _listTitle, value);
        }

        private void InfrastInit()
        {
            /* 基建设置 */
            var facility_list = new string[]
            {
                "Mfg",
                "Trade",
                "Control",
                "Power",
                "Reception",
                "Office",
                "Dorm",
            };

            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[facility_list.Length]);
            for (int i = 0; i != facility_list.Length; ++i)
            {
                var facility = facility_list[i];
                bool parsed = int.TryParse(ConfigurationHelper.GetFacilityOrder(facility, "-1"), out int order);

                if (!parsed || order < 0)
                {
                    temp_order_list[i] = new DragItemViewModel(LocalizationHelper.GetString(facility), facility, "Infrast.");
                }
                else
                {
                    temp_order_list[order] = new DragItemViewModel(LocalizationHelper.GetString(facility), facility, "Infrast.");
                }
            }

            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            DefaultInfrastList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("UserDefined"), Value = _userDefined },
                new CombinedData { Display = LocalizationHelper.GetString("153_3"), Value = "153_layout_3_times_a_day.json" },
                new CombinedData { Display = LocalizationHelper.GetString("243_3"), Value = "243_layout_3_times_a_day.json" },
                new CombinedData { Display = LocalizationHelper.GetString("243_4"), Value = "243_layout_4_times_a_day.json" },
                new CombinedData { Display = LocalizationHelper.GetString("252_3"), Value = "252_layout_3_times_a_day.json" },
                new CombinedData { Display = LocalizationHelper.GetString("333_3"), Value = "333_layout_for_Orundum_3_times_a_day.json" },
            };

            UsesOfDronesList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("DronesNotUse"), Value = "_NotUse" },
                new CombinedData { Display = LocalizationHelper.GetString("Money"), Value = "Money" },
                new CombinedData { Display = LocalizationHelper.GetString("SyntheticJade"), Value = "SyntheticJade" },
                new CombinedData { Display = LocalizationHelper.GetString("CombatRecord"), Value = "CombatRecord" },
                new CombinedData { Display = LocalizationHelper.GetString("PureGold"), Value = "PureGold" },
                new CombinedData { Display = LocalizationHelper.GetString("OriginStone"), Value = "OriginStone" },
                new CombinedData { Display = LocalizationHelper.GetString("Chip"), Value = "Chip" },
            };

            ConnectConfigList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("General"), Value = "General" },
                new CombinedData { Display = LocalizationHelper.GetString("BlueStacks"), Value = "BlueStacks" },
                new CombinedData { Display = LocalizationHelper.GetString("MuMuEmulator"), Value = "MuMuEmulator" },
                new CombinedData { Display = LocalizationHelper.GetString("LDPlayer"), Value = "LDPlayer" },
                new CombinedData { Display = LocalizationHelper.GetString("Nox"), Value = "Nox" },
                new CombinedData { Display = LocalizationHelper.GetString("XYAZ"), Value = "XYAZ" },
                new CombinedData { Display = LocalizationHelper.GetString("WSA"), Value = "WSA" },
                new CombinedData { Display = LocalizationHelper.GetString("Compatible"), Value = "Compatible" },
                new CombinedData { Display = LocalizationHelper.GetString("SecondResolution"), Value = "SecondResolution" },
                new CombinedData { Display = LocalizationHelper.GetString("NotKillAdb"), Value = "NotKillAdb" },
                new CombinedData { Display = LocalizationHelper.GetString("GeneralWithoutScreencapErr"), Value = "GeneralWithoutScreencapErr" },
            };

            TouchModeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("MiniTouchMode"), Value = "minitouch" },
                new CombinedData { Display = LocalizationHelper.GetString("MaaTouchMode"), Value = "maatouch" },
                new CombinedData { Display = LocalizationHelper.GetString("AdbTouchMode"), Value = "adb" },
            };

            _dormThresholdLabel = LocalizationHelper.GetString("DormThreshold") + ": " + _dormThreshold + "%";

            RoguelikeModeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("RoguelikeStrategyExp"), Value = "0" },
                new CombinedData { Display = LocalizationHelper.GetString("RoguelikeStrategyGold"), Value = "1" },

                // new CombData { Display = "两者兼顾，投资过后退出", Value = "2" } // 弃用
                // new CombData { Display = Localization.GetString("3"), Value = "3" },  // 开发中
            };

            RoguelikeThemeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("RoguelikeThemePhantom"), Value = "Phantom" },
                new CombinedData { Display = LocalizationHelper.GetString("RoguelikeThemeMizuki"), Value = "Mizuki" },
            };

            UpdateRoguelikeThemeList();

            RoguelikeRolesList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("DefaultRoles"), Value = string.Empty },
                new CombinedData { Display = LocalizationHelper.GetString("FirstMoveAdvantage"), Value = "先手必胜" },
                new CombinedData { Display = LocalizationHelper.GetString("SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
                new CombinedData { Display = LocalizationHelper.GetString("OvercomingYourWeaknesses"), Value = "取长补短" },
                new CombinedData { Display = LocalizationHelper.GetString("AsYourHeartDesires"), Value = "随心所欲" },
            };

            ClientTypeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
                new CombinedData { Display = LocalizationHelper.GetString("Official"), Value = "Official" },
                new CombinedData { Display = LocalizationHelper.GetString("Bilibili"), Value = "Bilibili" },
                new CombinedData { Display = LocalizationHelper.GetString("YoStarEN"), Value = "YoStarEN" },
                new CombinedData { Display = LocalizationHelper.GetString("YoStarJP"), Value = "YoStarJP" },
                new CombinedData { Display = LocalizationHelper.GetString("YoStarKR"), Value = "YoStarKR" },
                new CombinedData { Display = LocalizationHelper.GetString("txwy"), Value = "txwy" },
            };

            DarkModeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("Light"), Value = "Light" },
                new CombinedData { Display = LocalizationHelper.GetString("Dark"), Value = "Dark" },
                new CombinedData { Display = LocalizationHelper.GetString("SyncWithOS"), Value = "SyncWithOS" },
            };

            InverseClearModeList = new List<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("Clear"), Value = "Clear" },
                new CombinedData { Display = LocalizationHelper.GetString("Inverse"), Value = "Inverse" },
                new CombinedData { Display = LocalizationHelper.GetString("Switchable"), Value = "ClearInverse" },
            };

            VersionTypeList = new List<GenericCombinedData<UpdateVersionType>>
            {
                new GenericCombinedData<UpdateVersionType> { Display = LocalizationHelper.GetString("UpdateCheckNightly"), Value = UpdateVersionType.Nightly },
                new GenericCombinedData<UpdateVersionType> { Display = LocalizationHelper.GetString("UpdateCheckBeta"), Value = UpdateVersionType.Beta },
                new GenericCombinedData<UpdateVersionType> { Display = LocalizationHelper.GetString("UpdateCheckStable"), Value = UpdateVersionType.Stable },
            };

            LanguageList = new List<CombinedData>();
            foreach (var pair in LocalizationHelper.SupportedLanguages)
            {
                if (pair.Key == PallasLangKey && !Cheers)
                {
                    continue;
                }

                LanguageList.Add(new CombinedData { Display = pair.Value, Value = pair.Key });
            }
        }

        private bool _idle = true;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set => SetAndNotify(ref _idle, value);
        }

        /* 启动设置 */
        private bool _startSelf = AutoStart.CheckStart();

        /// <summary>
        /// Gets or sets a value indicating whether to start itself.
        /// </summary>
        public bool StartSelf
        {
            get => _startSelf;
            set
            {
                SetAndNotify(ref _startSelf, value);
                AutoStart.SetStart(value);
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

        private bool _startEmulator = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.StartEmulator, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to start emulator.
        /// </summary>
        public bool StartEmulator
        {
            get => _startEmulator;
            set
            {
                SetAndNotify(ref _startEmulator, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.StartEmulator, value.ToString());
                if (ClientType == string.Empty && Idle)
                {
                    ClientType = "Official";
                }
            }
        }

        private bool _minimizingStartup = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizingStartup, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimally start the emulator
        /// </summary>
        public bool MinimizingStartup
        {
            get => _minimizingStartup;
            set
            {
                SetAndNotify(ref _minimizingStartup, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.MinimizingStartup, value.ToString());
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

        public bool RunStartCommand()
        {
            if (string.IsNullOrWhiteSpace(StartsWithScript))
            {
                return false;
            }

            try
            {
                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = StartsWithScript,
                        // FileName = "cmd.exe",
                        // Arguments = $"/c {StartsWithScript}",
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

        public bool RunEndCommand()
        {
            if (string.IsNullOrWhiteSpace(EndsWithScript))
            {
                return false;
            }

            try
            {
                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = EndsWithScript,
                        // FileName = "cmd.exe",
                        // Arguments = $"/c {EndsWithScript}",
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

        /// <summary>
        /// Tries to start the emulator.
        /// </summary>
        /// <param name="manual">Whether to start manually.</param>
        public void TryToStartEmulator(bool manual = false)
        {
            if ((EmulatorPath.Length == 0
                || !File.Exists(EmulatorPath))
                || !(StartEmulator
                || manual))
            {
                return;
            }

            if (!int.TryParse(EmulatorWaitSeconds, out int delay))
            {
                delay = 60;
            }

            try
            {
                string fileName = string.Empty;
                string arguments = string.Empty;
                ProcessStartInfo startInfo;

                if (Path.GetExtension(EmulatorPath).ToLower() == ".lnk")
                {
                    var link = (IShellLink)new ShellLink();
                    var file = (IPersistFile)link;
                    file.Load(EmulatorPath, 0); // STGM_READ
                    link.Resolve(IntPtr.Zero, 1); // SLR_NO_UI
                    var buf = new char[32768];
                    unsafe
                    {
                        fixed (char* ptr = buf)
                        {
                            link.GetPath(ptr, 260, IntPtr.Zero, 0);  // MAX_PATH
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
                    fileName = EmulatorPath;
                    arguments = EmulatorAddCommand;
                }

                if (arguments.Length != 0)
                {
                    startInfo = new ProcessStartInfo(fileName, arguments);
                }
                else
                {
                    startInfo = new ProcessStartInfo(fileName);
                }

                startInfo.UseShellExecute = false;
                Process process = new Process
                {
                    StartInfo = startInfo,
                };

                AsstProxy.AsstLog("Try to start emulator: \nfileName: " + fileName + "\narguments: " + arguments);
                process.Start();

                try
                {
                    // 如果之前就启动了模拟器，这步有几率会抛出异常
                    process.WaitForInputIdle();
                    if (MinimizingStartup)
                    {
                        AsstProxy.AsstLog("Try minimizing the emulator");
                        int i;
                        for (i = 0; !IsIconic(process.MainWindowHandle) && i < 100; ++i)
                        {
                            ShowWindow(process.MainWindowHandle, SWMINIMIZE);
                            Thread.Sleep(10);
                            if (process.HasExited)
                            {
                                throw new Exception();
                            }
                        }

                        if (i >= 100)
                        {
                            AsstProxy.AsstLog("Attempts to exceed the limit");
                            throw new Exception();
                        }
                    }
                }
                catch (Exception)
                {
                    AsstProxy.AsstLog("The emulator was already start");

                    // 如果之前就启动了模拟器，如果开启了最小化启动，就把所有模拟器最小化
                    // TODO:只最小化需要开启的模拟器
                    string processName = Path.GetFileNameWithoutExtension(fileName);
                    Process[] processes = Process.GetProcessesByName(processName);
                    if (processes.Length > 0)
                    {
                        if (MinimizingStartup)
                        {
                            AsstProxy.AsstLog("Try minimizing the emulator by processName: " + processName);
                            foreach (Process p in processes)
                            {
                                int i;
                                for (i = 0; !IsIconic(p.MainWindowHandle) && !p.HasExited && i < 100; ++i)
                                {
                                    ShowWindow(p.MainWindowHandle, SWMINIMIZE);
                                    Thread.Sleep(10);
                                }

                                if (i >= 100)
                                {
                                    AsstProxy.AsstLog("The emulator minimization failure");
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception)
            {
                AsstProxy.AsstLog("Start emulator error, try to start using the default: \n" +
                    "EmulatorPath: " + EmulatorPath + "\n" +
                    "EmulatorAddCommand: " + EmulatorAddCommand);
                if (EmulatorAddCommand.Length != 0)
                {
                    Process.Start(EmulatorPath);
                }
                else
                {
                    Process.Start(EmulatorPath, EmulatorAddCommand);
                }
            }

            for (var i = 0; i < delay; ++i)
            {
                // TODO: _taskQueueViewModel在SettingsViewModel显示之前为null。所以获取不到Stopping内容，导致无法停止等待,等个有缘人优化下）
                // 一般是点了“停止”按钮了
                /*
                if (_taskQueueViewModel.Stopping)
                {
                    AsstProxy.AsstLog("Stop waiting for the emulator to start");
                    return;
                }
                */
                if (i % 10 == 0)
                {
                    // 同样的问题，因为_taskQueueViewModel为null，所以无法偶在主界面的log里显示
                    AsstProxy.AsstLog("Waiting for the emulator to start: " + (delay - i) + "s");
                }

                Thread.Sleep(1000);
            }
        }

        /// <summary>
        /// Selects the emulator to execute.
        /// </summary>
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
                UpdateWindowTitle(); /* 每次修改客户端时更新WindowTitle */
                _taskQueueViewModel.UpdateStageList(true);
                _taskQueueViewModel.UpdateDatePrompt();
            }
        }

        /// <summary>
        /// Gets the client type.
        /// </summary>
        public string ClientName
        {
            get
            {
                foreach (var item in ClientTypeList)
                {
                    if (item.Value == ClientType)
                    {
                        return item.Display;
                    }
                }

                return "Unknown Client";
            }
        }

        private readonly Dictionary<string, string> _serverMapping = new Dictionary<string, string>
        {
            { string.Empty, "CN" },
            { "Official", "CN" },
            { "Bilibili", "CN" },
            { "YoStarEN", "US" },
            { "YoStarJP", "JP" },
            { "YoStarKR", "KR" },
            { "txwy", "ZH_TW" },
        };

        private bool _autoRestartOnDrop = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, "True"));

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

        /* 基建设置 */

        /// <summary>
        /// Gets or sets the infrast item view models.
        /// </summary>
        public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; }

        /// <summary>
        /// Gets or sets the list of uses of drones.
        /// </summary>
        public List<CombinedData> UsesOfDronesList { get; set; }

        /// <summary>
        /// Gets or sets the list of uses of default infrast.
        /// </summary>
        public List<CombinedData> DefaultInfrastList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike lists.
        /// </summary>
        public List<CombinedData> RoguelikeThemeList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike modes.
        /// </summary>
        public List<CombinedData> RoguelikeModeList { get; set; }

        private ObservableCollection<CombinedData> _roguelikeSquadList = new ObservableCollection<CombinedData>();

        /// <summary>
        /// Gets or sets the list of roguelike squad.
        /// </summary>
        public ObservableCollection<CombinedData> RoguelikeSquadList
        {
            get => _roguelikeSquadList;
            set => SetAndNotify(ref _roguelikeSquadList, value);
        }

        /// <summary>
        /// Gets or sets the list of roguelike roles.
        /// </summary>
        public List<CombinedData> RoguelikeRolesList { get; set; }

        // public List<CombData> RoguelikeCoreCharList { get; set; }

        /// <summary>
        /// Gets or sets the list of the client types.
        /// </summary>
        public List<CombinedData> ClientTypeList { get; set; }

        /// <summary>
        /// Gets or sets the list of the configuration of connection.
        /// </summary>
        public List<CombinedData> ConnectConfigList { get; set; }

        /// <summary>
        /// Gets or sets the list of touch modes
        /// </summary>
        public List<CombinedData> TouchModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of dark mode.
        /// </summary>
        public List<CombinedData> DarkModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of inverse clear modes.
        /// </summary>
        public List<CombinedData> InverseClearModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of the version type.
        /// </summary>
        public List<GenericCombinedData<UpdateVersionType>> VersionTypeList { get; set; }

        /// <summary>
        /// Gets or sets the language list.
        /// </summary>
        public List<CombinedData> LanguageList { get; set; }

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
            var orderList = new List<string>();
            foreach (var item in InfrastItemViewModels)
            {
                if (item.IsChecked == false)
                {
                    continue;
                }

                orderList.Add(item.OriginalName);
            }

            return orderList;
        }

        /// <summary>
        /// 实时更新基建换班顺序
        /// </summary>
        public void InfrastOrderSelectionChanged()
        {
            int index = 0;
            foreach (var item in InfrastItemViewModels)
            {
                ConfigurationHelper.GetFacilityOrder(item.OriginalName, index.ToString());
                ++index;
            }
        }

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

        private string _defaultInfrast = ConfigurationHelper.GetValue(ConfigurationKeys.DefaultInfrast, _userDefined);

        private static readonly string _userDefined = "user_defined";

        /// <summary>
        /// Gets or sets the uses of drones.
        /// </summary>
        public string DefaultInfrast
        {
            get => _defaultInfrast;
            set
            {
                SetAndNotify(ref _defaultInfrast, value);
                if (_defaultInfrast != _userDefined)
                {
                    CustomInfrastFile = "resource\\custom_infrast\\" + value;
                    IsCustomInfrastFileReadOnly = true;
                }
                else
                {
                    IsCustomInfrastFileReadOnly = false;
                }

                ConfigurationHelper.SetValue(ConfigurationKeys.DefaultInfrast, value);
            }
        }

        private string _isCustomInfrastFileReadOnly = ConfigurationHelper.GetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether  CustomInfrastFile is read-only
        /// </summary>
        public bool IsCustomInfrastFileReadOnly
        {
            get => bool.Parse(_isCustomInfrastFileReadOnly);
            set
            {
                SetAndNotify(ref _isCustomInfrastFileReadOnly, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, value.ToString());
            }
        }

        private string _dormFilterNotStationedEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.DormFilterNotStationedEnabled, false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
        /// </summary>
        public bool DormFilterNotStationedEnabled
        {
            get => bool.Parse(_dormFilterNotStationedEnabled);
            set
            {
                SetAndNotify(ref _dormFilterNotStationedEnabled, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.DormFilterNotStationedEnabled, value.ToString());
            }
        }

        private string _dormTrustEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.DormTrustEnabled, true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether get trust in dorm is enabled.
        /// </summary>
        public bool DormTrustEnabled
        {
            get => bool.Parse(_dormTrustEnabled);
            set
            {
                SetAndNotify(ref _dormTrustEnabled, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.DormTrustEnabled, value.ToString());
            }
        }

        private string _originiumShardAutoReplenishment = ConfigurationHelper.GetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
        /// </summary>
        public bool OriginiumShardAutoReplenishment
        {
            get => bool.Parse(_originiumShardAutoReplenishment);
            set
            {
                SetAndNotify(ref _originiumShardAutoReplenishment, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, value.ToString());
            }
        }

        private bool _customInfrastEnabled = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastEnabled, false.ToString()));

        public bool CustomInfrastEnabled
        {
            get => _customInfrastEnabled;
            set
            {
                SetAndNotify(ref _customInfrastEnabled, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastEnabled, value.ToString());
                _taskQueueViewModel.CustomInfrastEnabled = value;
            }
        }

        /// <summary>
        /// Selects infrast config file.
        /// </summary>
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

            DefaultInfrast = _userDefined;
        }

        private string _customInfrastFile = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);

        public string CustomInfrastFile
        {
            get => _customInfrastFile;
            set
            {
                SetAndNotify(ref _customInfrastFile, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastFile, value);
                _taskQueueViewModel.RefreshCustonInfrastPlan();
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
            _resetNotifyTimer.Elapsed += (source, e) =>
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
        /// Gets or sets the list of rectangle vertical offset.
        /// </summary>
        public List<double> RectangleVerticalOffsetList { get; set; }

        private int _selectedIndex = 0;

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

                        var isInRange = RectangleVerticalOffsetList != null
                            && RectangleVerticalOffsetList.Count > 0
                            && value < RectangleVerticalOffsetList.Count;

                        if (isInRange)
                        {
                            ScrollOffset = RectangleVerticalOffsetList[value];
                        }

                        ResetNotifySource();
                        break;

                    case NotifyType.ScrollOffset:
                        SetAndNotify(ref _selectedIndex, value);
                        break;
                }
            }
        }

        private double _scrollOffset = 0;

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
                        var isInRange = RectangleVerticalOffsetList != null && RectangleVerticalOffsetList.Count > 0;

                        if (isInRange)
                        {
                            // 滚动条滚动到底部，返回最后一个 Rectangle 索引
                            if (value + ScrollViewportHeight >= ScrollExtentHeight)
                            {
                                SelectedIndex = RectangleVerticalOffsetList.Count - 1;
                                ResetNotifySource();
                                break;
                            }

                            // 根据出当前 ScrollOffset 选出最后一个在可视范围的 Rectangle 索引
                            var rectangleSelect = RectangleVerticalOffsetList.Select((n, i) => (
                            rectangleAppeared: value >= n,
                            index: i));

                            var index = rectangleSelect.LastOrDefault(n => n.rectangleAppeared).index;
                            SelectedIndex = index;
                        }

                        ResetNotifySource();
                        break;

                    case NotifyType.SelectedIndex:
                        SetAndNotify(ref _scrollOffset, value);
                        break;
                }
            }
        }

        #endregion 设置页面列表和滚动视图联动绑定

        /* 肉鸽设置 */

        private string _roguelikeTheme = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, "Phantom");

        /// <summary>
        /// Gets or sets the Roguelike theme.
        /// </summary>
        public string RoguelikeTheme
        {
            get => _roguelikeTheme;
            set
            {
                SetAndNotify(ref _roguelikeTheme, value);
                UpdateRoguelikeThemeList();
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeTheme, value);
            }
        }

        private string _roguelikeMode = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMode, "0");

        /// <summary>
        /// Gets or sets the roguelike mode.
        /// </summary>
        public string RoguelikeMode
        {
            get => _roguelikeMode;
            set
            {
                SetAndNotify(ref _roguelikeMode, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeMode, value);
                if (value == "1")
                {
                    RoguelikeInvestmentEnabled = true;
                }
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
                SetAndNotify(ref _roguelikeCoreChar, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeCoreChar, value);
            }
        }

        private string _roguelikeUseSupportUnit = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeUseSupportUnit, false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether use support unit.
        /// </summary>
        public bool RoguelikeUseSupportUnit
        {
            get => bool.Parse(_roguelikeUseSupportUnit);
            set
            {
                SetAndNotify(ref _roguelikeUseSupportUnit, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeUseSupportUnit, value.ToString());
            }
        }

        private string _roguelikeEnableNonfriendSupport = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether can roguelike support unit belong to nonfriend
        /// </summary>
        public bool RoguelikeEnableNonfriendSupport
        {
            get => bool.Parse(_roguelikeEnableNonfriendSupport);
            set
            {
                SetAndNotify(ref _roguelikeEnableNonfriendSupport, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, value.ToString());
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

        private string _roguelikeInvestmentEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether investment is enabled.
        /// </summary>
        public bool RoguelikeInvestmentEnabled
        {
            get => bool.Parse(_roguelikeInvestmentEnabled);
            set
            {
                SetAndNotify(ref _roguelikeInvestmentEnabled, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, value.ToString());
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

        private string _roguelikeStopWhenInvestmentFull = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether to stop when investment is full.
        /// </summary>
        public bool RoguelikeStopWhenInvestmentFull
        {
            get => bool.Parse(_roguelikeStopWhenInvestmentFull);
            set
            {
                SetAndNotify(ref _roguelikeStopWhenInvestmentFull, value.ToString());
                ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, value.ToString());
            }
        }

        /* 访问好友设置 */
        private string _lastCreditFightTaskTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYJDate().AddDays(-1).ToString("yyyy/MM/dd HH:mm:ss"));

        public string LastCreditFightTaskTime
        {
            get => _lastCreditFightTaskTime;
            set
            {
                SetAndNotify(ref _lastCreditFightTaskTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LastCreditFightTaskTime, value.ToString());
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
                    if (DateTime.UtcNow.ToYJDate() > DateTime.ParseExact(_lastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
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

        /* 信用商店设置 */

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

        private bool _creditForceShoppingIfCreditFull = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, false.ToString()));

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

        /* 定时设置 */

        public class TimerModel
        {
            public class TimerProperties : INotifyPropertyChanged
            {
                public event PropertyChangedEventHandler PropertyChanged;

                protected void OnPropertyChanged([CallerMemberName] string name = null)
                {
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
                }

                public int TimerId { get; set; }

                private bool _isOn;

                /// <summary>
                /// Gets or sets a value indicating whether the timer is set.
                /// </summary>
                public bool IsOn
                {
                    get => _isOn;
                    set
                    {
                        _isOn = value;
                        OnPropertyChanged();
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
                        _hour = (value >= 0 && value <= 23) ? value : _hour;
                        OnPropertyChanged();
                        ConfigurationHelper.SetTimerHour(TimerId, value.ToString());
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
                        _min = (value >= 0 && value <= 59) ? value : _min;
                        OnPropertyChanged();
                        ConfigurationHelper.SetTimerMin(TimerId, value.ToString());
                    }
                }

                public TimerProperties()
                {
                    PropertyChanged += (sender, args) => { };
                }
            }

            public TimerProperties[] Timers { get; set; } = new TimerProperties[8];

            public TimerModel()
            {
                for (int i = 0; i < 8; i++)
                {
                    Timers[i] = new TimerProperties
                    {
                        TimerId = i,
                        IsOn = ConfigurationHelper.GetTimer(i, bool.FalseString) == bool.TrueString,
                        Hour = int.Parse(ConfigurationHelper.GetTimerHour(i, $"{i * 3}")),
                        Min = int.Parse(ConfigurationHelper.GetTimerMin(i, "0")),
                    };
                }
            }
        }

        public TimerModel TimerModels { get; set; } = new TimerModel();

        /* 刷理智设置 */

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

        /* 自动公招设置 */
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

        private bool _useExpedited = false;

        /// <summary>
        /// Gets or sets a value indicating whether to use expedited.
        /// </summary>
        public bool UseExpedited
        {
            get => _useExpedited;
            set => SetAndNotify(ref _useExpedited, value);
        }

        private bool _isLevel3UseShortTime = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IsLevel3UseShortTime, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to shorten the time for level 3.
        /// </summary>
        public bool IsLevel3UseShortTime
        {
            get => _isLevel3UseShortTime;
            set
            {
                SetAndNotify(ref _isLevel3UseShortTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.IsLevel3UseShortTime, value.ToString());
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

        public enum UpdateVersionType
        {
            /// <summary>
            /// 测试版
            /// </summary>
            Nightly,

            /// <summary>
            /// 开发版
            /// </summary>
            Beta,

            /// <summary>
            /// 稳定版
            /// </summary>
            Stable,
        }

        /* 软件更新设置 */

        private UpdateVersionType _versionType = (UpdateVersionType)Enum.Parse(typeof(UpdateVersionType),
                ConfigurationHelper.GetValue(ConfigurationKeys.VersionType, UpdateVersionType.Stable.ToString()));

        /// <summary>
        /// Gets or sets the type of version to update.
        /// </summary>
        public UpdateVersionType VersionType
        {
            get => _versionType;
            set
            {
                SetAndNotify(ref _versionType, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.VersionType, value.ToString());
            }
        }

        /// <summary>
        /// Gets a value indicating whether to update nightly.
        /// </summary>
        public bool UpdateNightly
        {
            get => _versionType == UpdateVersionType.Nightly;
        }

        /// <summary>
        /// Gets a value indicating whether to update beta version.
        /// </summary>
        public bool UpdateBeta
        {
            get => _versionType == UpdateVersionType.Beta;
        }

        private bool _updateCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdateCheck, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to check update.
        /// </summary>
        public bool UpdateCheck
        {
            get => _updateCheck;
            set
            {
                SetAndNotify(ref _updateCheck, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UpdateCheck, value.ToString());
            }
        }

        private bool _updateAutoCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdatAutoCheck, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to check update.
        /// </summary>
        public bool UpdatAutoCheck
        {
            get => _updateAutoCheck;
            set
            {
                SetAndNotify(ref _updateAutoCheck, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UpdatAutoCheck, value.ToString());
            }
        }

        private string _proxy = ConfigurationHelper.GetValue(ConfigurationKeys.UpdateProxy, string.Empty);

        /// <summary>
        /// Gets or sets the proxy settings.
        /// </summary>
        public string Proxy
        {
            get => _proxy;
            set
            {
                SetAndNotify(ref _proxy, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UpdateProxy, value);
            }
        }

        private bool _isCheckingForUpdates = false;

        /// <summary>
        /// Gets or sets a value indicating whether the update is being checked.
        /// </summary>
        public bool IsCheckingForUpdates
        {
            get => _isCheckingForUpdates;
            set
            {
                SetAndNotify(ref _isCheckingForUpdates, value);
            }
        }

        private bool _autoDownloadUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDownloadUpdatePackage, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to auto download update package.
        /// </summary>
        public bool AutoDownloadUpdatePackage
        {
            get => _autoDownloadUpdatePackage;
            set
            {
                SetAndNotify(ref _autoDownloadUpdatePackage, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AutoDownloadUpdatePackage, value.ToString());
            }
        }

        /// <summary>
        /// Updates manually.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task ManualUpdate()
        {
            var versionUpdateViewModel = _container.Get<VersionUpdateViewModel>();
            var ret = await versionUpdateViewModel.CheckAndDownloadUpdate(true);

            string toastMessage = null;
            switch (ret)
            {
                case VersionUpdateViewModel.CheckUpdateRetT.NoNeedToUpdate:
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.AlreadyLatest:
                    toastMessage = LocalizationHelper.GetString("AlreadyLatest");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.UnknownError:
                    toastMessage = LocalizationHelper.GetString("NewVersionDetectFailedTitle");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.NetworkError:
                    toastMessage = LocalizationHelper.GetString("CheckNetworking");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.FailedToGetInfo:
                    toastMessage = LocalizationHelper.GetString("GetReleaseNoteFailed");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.OK:
                    versionUpdateViewModel.AskToRestart();
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.NewVersionIsBeingBuilt:
                    toastMessage = LocalizationHelper.GetString("NewVersionIsBeingBuilt");
                    break;
            }

            if (toastMessage != null)
            {
                Execute.OnUIThread(() =>
                {
                    using var toast = new ToastNotification(toastMessage);
                    toast.Show();
                });
            }
        }

        public void ShowChangelog()
        {
            _windowManager.ShowWindow(_versionUpdateViewModel);
        }

        /* 连接设置 */

        private bool _autoDetectConnection = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDetect, true.ToString()));

        public bool AutoDetectConnection
        {
            get => _autoDetectConnection;
            set
            {
                SetAndNotify(ref _autoDetectConnection, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AutoDetect, value.ToString());
            }
        }

        private bool _alwaysAutoDetectConnection = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.AlwaysAutoDetect, false.ToString()));

        public bool AlwaysAutoDetectConnection
        {
            get => _alwaysAutoDetectConnection;
            set
            {
                SetAndNotify(ref _alwaysAutoDetectConnection, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AlwaysAutoDetect, value.ToString());
            }
        }

        private ObservableCollection<string> _connectAddressHistory = new ObservableCollection<string>();

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
                if (ConnectAddress == value || string.IsNullOrEmpty(value))
                {
                    return;
                }

                Application.Current.Dispatcher.Invoke(() =>
                {
                    if (!ConnectAddressHistory.Contains(value))
                    {
                        ConnectAddressHistory.Insert(0, value);
                        while (ConnectAddressHistory.Count > 5)
                        {
                            ConnectAddressHistory.RemoveAt(ConnectAddressHistory.Count - 1);
                        }
                    }
                    else
                    {
                        ConnectAddressHistory.Remove(value);
                        ConnectAddressHistory.Insert(0, value);
                    }
                });

                SetAndNotify(ref _connectAddress, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AddressHistory, JsonConvert.SerializeObject(ConnectAddressHistory));
                ConfigurationHelper.SetValue(ConfigurationKeys.ConnectAddress, value);
                UpdateWindowTitle(); /* 每次修改连接地址时更新WindowTitle */
            }
        }

        public void RemoveAddress_Click(string address)
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
                SetAndNotify(ref _connectConfig, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ConnectConfig, value);
                UpdateWindowTitle(); /* 每次修改连接配置时更新WindowTitle */
            }
        }

        private bool _retryOnDisconnected = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RetryOnAdbDisconnected, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to retry task after adb disconnected.
        /// </summary>
        public bool RetryOnDisconnected
        {
            get => _retryOnDisconnected;
            set
            {
                SetAndNotify(ref _retryOnDisconnected, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RetryOnAdbDisconnected, value.ToString());
            }
        }

        private bool _deploymentWithPause = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, false.ToString()));

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

        private bool _adbLiteEnabled = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.AdbLiteEnabled, false.ToString()));

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

        /// <summary>
        /// Gets the default addresses.
        /// </summary>
        public Dictionary<string, List<string>> DefaultAddress { get; } = new Dictionary<string, List<string>>
        {
            { "General", new List<string> { string.Empty } },
            { "BlueStacks", new List<string> { "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585", "127.0.0.1:5595", "127.0.0.1:5554" } },
            { "MuMuEmulator", new List<string> { "127.0.0.1:7555" } },
            { "LDPlayer", new List<string> { "emulator-5554", "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5554" } },
            { "Nox", new List<string> { "127.0.0.1:62001", "127.0.0.1:59865" } },
            { "XYAZ", new List<string> { "127.0.0.1:21503" } },
            { "WSA", new List<string> { "127.0.0.1:58526" } },
        };

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
            catch (Exception)
            {
                error = LocalizationHelper.GetString("EmulatorException");
                return false;
            }

            if (emulators.Count == 0)
            {
                error = LocalizationHelper.GetString("EmulatorNotFound");
                return false;
            }
            else if (emulators.Count > 1)
            {
                error = LocalizationHelper.GetString("EmulatorTooMany");
                return false;
            }

            ConnectConfig = emulators.First();
            AdbPath = adapter.GetAdbPathByEmulatorName(ConnectConfig) ?? AdbPath;
            if (string.IsNullOrEmpty(AdbPath))
            {
                error = LocalizationHelper.GetString("AdbException");
                return false;
            }

            if (ConnectAddress.Length == 0)
            {
                var addresses = adapter.GetAdbAddresses(AdbPath);

                // 傻逼雷电已经关掉了，用别的 adb 还能检测出来这个端口 device
                if (addresses.Count == 1 && addresses.First() != "emulator-5554")
                {
                    ConnectAddress = addresses.First();
                }
                else if (addresses.Count > 1)
                {
                    foreach (var address in addresses)
                    {
                        if (address == "emulator-5554" && ConnectConfig != "LDPlayer")
                        {
                            continue;
                        }

                        ConnectAddress = address;
                        break;
                    }
                }

                if (ConnectAddress.Length == 0)
                {
                    ConnectAddress = DefaultAddress[ConnectConfig][0];
                }
            }

            return true;
        }

        /// <summary>
        /// Selects ADB program file.
        /// </summary>
        public void SelectFile()
        {
            var dialog = new OpenFileDialog
            {
                Filter = LocalizationHelper.GetString("ADBProgram") + "|*.exe",
            };

            if (dialog.ShowDialog() == true)
            {
                AdbPath = dialog.FileName;
            }
        }

        /// <summary>
        /// 标题栏显示模拟器名称和IP端口。
        /// </summary>
        public void UpdateWindowTitle()
        {
            var rvm = (RootViewModel)this.Parent;
            var connectConfigName = string.Empty;
            foreach (var data in ConnectConfigList)
            {
                if (data.Value == ConnectConfig)
                {
                    connectConfigName = data.Display;
                }
            }

            string prefix = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitlePrefix, string.Empty);
            if (!string.IsNullOrEmpty(prefix))
            {
                prefix += " - ";
            }

            rvm.WindowTitle = $"{prefix}MAA - {VersionId} - {connectConfigName} ({ConnectAddress}) - {ClientName}";
        }

        private readonly string _bluestacksConfig = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigPath, string.Empty);
        private readonly string _bluestacksKeyWord = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigKeyword, "bst.instance.Nougat64.status.adb_port");

        /// <summary>
        /// Tries to set Bluestack Hyper V address.
        /// </summary>
        public void TryToSetBlueStacksHyperVAddress()
        {
            if (_bluestacksConfig.Length == 0)
            {
                return;
            }

            if (!File.Exists(_bluestacksConfig))
            {
                ConfigurationHelper.SetValue(ConfigurationKeys.BluestacksConfigError, "File not exists");
                return;
            }

            var all_lines = File.ReadAllLines(_bluestacksConfig);
            foreach (var line in all_lines)
            {
                if (line.StartsWith(_bluestacksKeyWord))
                {
                    var sp = line.Split('"');
                    ConnectAddress = "127.0.0.1:" + sp[1];
                }
            }
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
                ConfigurationHelper.SetValue(ConfigurationKeys.TouchMode, value);
                UpdateInstanceSettings();
            }
        }

        public void UpdateInstanceSettings()
        {
            _asstProxy.AsstSetInstanceOption(InstanceOptionKey.TouchMode, TouchMode);
            _asstProxy.AsstSetInstanceOption(InstanceOptionKey.DeploymentWithPause, DeploymentWithPause ? "1" : "0");
            _asstProxy.AsstSetInstanceOption(InstanceOptionKey.AdbLiteEnabled, AdbLiteEnabled ? "1" : "0");
        }

        private static readonly string GoogleAdbDownloadUrl = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
        private static readonly string GoogleAdbFilename = "adb.zip";

        public async void ReplaceADB()
        {
            if (!File.Exists(AdbPath))
            {
                Execute.OnUIThread(() =>
                {
                    using var toast = new ToastNotification(LocalizationHelper.GetString("ReplaceADBNotExists"));
                    toast.Show();
                });
                return;
            }

            if (!File.Exists(GoogleAdbFilename))
            {
                var downloadResult = await _httpService.DownloadFileAsync(new Uri(GoogleAdbDownloadUrl), GoogleAdbFilename);
                if (!downloadResult)
                {
                    await Execute.OnUIThreadAsync(() =>
                    {
                        using var toast = new ToastNotification(LocalizationHelper.GetString("AdbDownloadFailedTitle"));
                        toast.AppendContentText(LocalizationHelper.GetString("AdbDownloadFailedDesc")).Show();
                    });
                    return;
                }
            }

            var procTask = Task.Run(() =>
           {
               // ErrorView.xaml.cs里有个报错的逻辑，这里如果改的话那边也要对应改一下
               foreach (var process in Process.GetProcessesByName(Path.GetFileName(AdbPath)))
               {
                   process.Kill();
               }

               File.Copy(AdbPath, AdbPath + ".bak", true);

               const string UnzipDir = "adb_unzip";
               if (Directory.Exists(UnzipDir))
               {
                   Directory.Delete(UnzipDir, true);
               }

               ZipFile.ExtractToDirectory(GoogleAdbFilename, UnzipDir);
               File.Copy(UnzipDir + "/platform-tools/adb.exe", AdbPath, true);
               Directory.Delete(UnzipDir, true);
           });
            await procTask;

            AdbReplaced = true;
            ConfigurationHelper.SetValue(ConfigurationKeys.AdbReplaced, true.ToString());
            Execute.OnUIThread(() =>
            {
                using var toast = new ToastNotification(LocalizationHelper.GetString("SuccessfullyReplacedADB"));
                toast.Show();
            });
        }

        public bool AdbReplaced { get; set; } = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AdbReplaced, false.ToString()));

        /* 界面设置 */

        /// <summary>
        /// Gets a value indicating whether to use tray icon.
        /// </summary>
        public bool UseTray => true;

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
                _mainWindowManager.SetMinimizeToTaskbar(value);
            }
        }

        private bool _useNotify = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseNotify, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to use notification.
        /// </summary>
        public bool UseNotify
        {
            get => _useNotify;
            set
            {
                SetAndNotify(ref _useNotify, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseNotify, value.ToString());
                if (value)
                {
                    Execute.OnUIThread(() =>
                    {
                        using var toast = new ToastNotification("Test test");
                        toast.Show();
                    });
                }
            }
        }

        private bool _loadGUIParameters = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.LoadPositionAndSize, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to load GUI parameters.
        /// </summary>
        public bool LoadGUIParameters
        {
            get => _loadGUIParameters;
            set
            {
                SetAndNotify(ref _loadGUIParameters, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.LoadPositionAndSize, value.ToString());
                if (value)
                {
                    if (SaveGUIParametersOnClosing)
                    {
                        Application.Current.MainWindow.Closing += SaveGUIParameters;
                    }
                    else
                    {
                        SaveGUIParameters();
                    }
                }
            }
        }

        private bool _saveGUIParametersOnClosing = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.SavePositionAndSize, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to save GUI parameters on closing main window.
        /// </summary>
        public bool SaveGUIParametersOnClosing
        {
            get => _saveGUIParametersOnClosing;
            set
            {
                SetAndNotify(ref _saveGUIParametersOnClosing, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.SavePositionAndSize, value.ToString());
                if (value)
                {
                    Application.Current.MainWindow.Closing += SaveGUIParameters;
                }
                else
                {
                    Application.Current.MainWindow.Closing -= SaveGUIParameters;
                }
            }
        }

        public void SaveGUIParameters(object sender, EventArgs e)
        {
            SaveGUIParameters();
        }

        /// <summary>
        /// Save main window left edge, top edge, width and heigth.
        /// </summary>
        public void SaveGUIParameters()
        {
            // 请在配置文件中修改该部分配置，暂不支持从GUI设置
            // Please modify this part of configuration in the configuration file.
            ConfigurationHelper.SetValue(ConfigurationKeys.LoadPositionAndSize, LoadGUIParameters.ToString());
            ConfigurationHelper.SetValue(ConfigurationKeys.SavePositionAndSize, SaveGUIParametersOnClosing.ToString());

            var mainWindow = Application.Current.MainWindow;
            System.Windows.Forms.Screen currentScreen =
                System.Windows.Forms.Screen.FromHandle(new WindowInteropHelper(mainWindow).Handle);
            var screenRect = currentScreen.Bounds;
            ConfigurationHelper.SetValue(ConfigurationKeys.MonitorNumber, currentScreen.DeviceName);
            ConfigurationHelper.SetValue(ConfigurationKeys.MonitorWidth, screenRect.Width.ToString());
            ConfigurationHelper.SetValue(ConfigurationKeys.MonitorHeight, screenRect.Height.ToString());

            ConfigurationHelper.SetValue(ConfigurationKeys.PositionLeft, (mainWindow.Left - screenRect.Left).ToString(CultureInfo.InvariantCulture));
            ConfigurationHelper.SetValue(ConfigurationKeys.PositionTop, (mainWindow.Top - screenRect.Top).ToString(CultureInfo.InvariantCulture));
            ConfigurationHelper.SetValue(ConfigurationKeys.WindowWidth, mainWindow.Width.ToString(CultureInfo.InvariantCulture));
            ConfigurationHelper.SetValue(ConfigurationKeys.WindowHeight, mainWindow.Height.ToString(CultureInfo.InvariantCulture));
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
                _taskQueueViewModel.UseAlternateStage = value;
                ConfigurationHelper.SetValue(ConfigurationKeys.UseAlternateStage, value.ToString());
                if (value)
                {
                    HideUnavailableStage = false;
                }
            }
        }

        private bool _useRemainingSanityStage = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, bool.TrueString));

        public bool UseRemainingSanityStage
        {
            get => _useRemainingSanityStage;
            set
            {
                SetAndNotify(ref _useRemainingSanityStage, value);
                _taskQueueViewModel.UseRemainingSanityStage = value;
                ConfigurationHelper.SetValue(ConfigurationKeys.UseRemainingSanityStage, value.ToString());
            }
        }

        private bool _useExpiringMedicine = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.UseExpiringMedicine, bool.FalseString));

        public bool UseExpiringMedicine
        {
            get => _useExpiringMedicine;
            set
            {
                SetAndNotify(ref _useExpiringMedicine, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.UseExpiringMedicine, value.ToString());
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

                _taskQueueViewModel.UpdateStageList(true);
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
                _taskQueueViewModel.CustomStageCode = value;
            }
        }

        public enum DarkModeType
        {
            Light,
            Dark,
            SyncWithOS
        }

        private DarkModeType _darkModeType =
            Enum.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.DarkMode, DarkModeType.Light.ToString()),
                out DarkModeType temp)
                ? temp
                : DarkModeType.Light;

        /// <summary>
        /// Gets or sets the dark mode.
        /// </summary>
        public string DarkMode
        {
            get => _darkModeType.ToString();
            set
            {
                if (!Enum.TryParse(value, out DarkModeType tempEnumValue))
                {
                    return;
                }

                SetAndNotify(ref _darkModeType, tempEnumValue);
                ConfigurationHelper.SetValue(ConfigurationKeys.DarkMode, value);
                SwitchDarkMode();

                /*
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("DarkModeSetColorsTip"),
                    LocalizationHelper.GetString("Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Question);
                if (result == MessageBoxResult.OK)
                {
                    Application.Current.Shutdown();
                    System.Windows.Forms.Application.Restart();
                }
                */
            }
        }

        public void SwitchDarkMode()
        {
            DarkModeType darkModeType =
                Enum.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.DarkMode, DarkModeType.Light.ToString()),
                    out DarkModeType temp)
                    ? temp : DarkModeType.Light;
            switch (darkModeType)
            {
                case DarkModeType.Light:
                    ThemeManager.Current.UsingSystemTheme = false;
                    ThemeManager.Current.ApplicationTheme = ApplicationTheme.Light;
                    return;

                case DarkModeType.Dark:
                    ThemeManager.Current.UsingSystemTheme = false;
                    ThemeManager.Current.ApplicationTheme = ApplicationTheme.Dark;
                    return;

                case DarkModeType.SyncWithOS:
                    ThemeManager.Current.UsingSystemTheme = true;
                    return;
            }
        }

        private void SystemEvents_UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e)
        {
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        private enum InverseClearType
        {
            Clear,
            Inverse,
            ClearInverse,
        }

        private InverseClearType _inverseClearMode =
            Enum.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.InverseClearMode, InverseClearType.Clear.ToString()),
                out InverseClearType temp)
            ? temp : InverseClearType.Clear;

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
                        _taskQueueViewModel.InverseMode = false;
                        _taskQueueViewModel.ShowInverse = false;
                        _taskQueueViewModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.Inverse:
                        _taskQueueViewModel.InverseMode = true;
                        _taskQueueViewModel.ShowInverse = false;
                        _taskQueueViewModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.ClearInverse:
                        _taskQueueViewModel.ShowInverse = true;
                        _taskQueueViewModel.SelectedAllWidth = TaskQueueViewModel.SelectedAllWidthWhenBoth;
                        break;
                }
            }
        }

        private string _soberLanguage = ConfigurationHelper.GetValue("GUI.SoberLanguage", LocalizationHelper.DefaultLanguage);

        public string SoberLanguage
        {
            get => _soberLanguage;
            set
            {
                SetAndNotify(ref _soberLanguage, value);
                ConfigurationHelper.SetValue("GUI.SoberLanguage", value);
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

                string FormatText(string text, string key)
                    => string.Format(text, LocalizationHelper.GetString(key, value), LocalizationHelper.GetString(key, _language));

                Window mainWindow = Application.Current.MainWindow;
                mainWindow.Show();
                mainWindow.WindowState = mainWindow.WindowState = WindowState.Normal;
                mainWindow.Activate();
                var result = MessageBoxHelper.Show(
                    FormatText("{0}\n{1}", "LanguageChangedTip"),
                    FormatText("{0}({1})", "Tip"),
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Question,
                    ok: FormatText("{0}({1})", "Ok"),
                    cancel: FormatText("{0}({1})", "ManualRestart"));
                if (result == MessageBoxResult.OK)
                {
                    Application.Current.Shutdown();
                    System.Windows.Forms.Application.Restart();
                }

                SetAndNotify(ref _language, value);
            }
        }

        private bool _cheers = bool.Parse(ConfigurationHelper.GetValue("GUI.Cheers", bool.FalseString));

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
                ConfigurationHelper.SetValue("GUI.Cheers", value.ToString());
                if (_cheers)
                {
                    SetPallasLanguage();
                }
            }
        }

        private bool _hangover = bool.Parse(ConfigurationHelper.GetValue("GUI.Hangover", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hangover.
        /// </summary>
        public bool Hangover
        {
            get => _hangover;
            set
            {
                SetAndNotify(ref _hangover, value);
                ConfigurationHelper.SetValue("GUI.Hangover", value.ToString());
            }
        }

        private void SetPallasLanguage()
        {
            ConfigurationHelper.SetValue(ConfigurationKeys.Localization, PallasLangKey);
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("DrunkAndStaggering"),
                LocalizationHelper.GetString("Burping"),
                MessageBoxButton.OK,
                iconKey: "DrunkAndStaggeringGeometry",
                iconBrushKey: "PallasBrush");
            if (result == MessageBoxResult.OK)
            {
                Application.Current.Shutdown();
                System.Windows.Forms.Application.Restart();
            }
        }

        /// <summary>
        /// Gets or sets the hotkey: ShowGui.
        /// </summary>
        public MaaHotKey HotKeyShowGui
        {
            get => _maaHotKeyManager.GetOrNull(MaaHotKeyAction.ShowGui);
            set => SetHotKey(MaaHotKeyAction.ShowGui, value);
        }

        /// <summary>
        /// Gets or sets the hotkey: LinkStart.
        /// </summary>
        public MaaHotKey HotKeyLinkStart
        {
            get => _maaHotKeyManager.GetOrNull(MaaHotKeyAction.LinkStart);
            set => SetHotKey(MaaHotKeyAction.LinkStart, value);
        }

        private void SetHotKey(MaaHotKeyAction action, MaaHotKey value)
        {
            if (value != null)
            {
                _maaHotKeyManager.TryRegister(action, value);
            }
            else
            {
                _maaHotKeyManager.Unregister(action);
            }
        }

        /// <summary>
        /// Did you buy wine?
        /// </summary>
        /// <returns>The answer.</returns>
        public bool DidYouBuyWine()
        {
            if (DateTime.UtcNow.ToYJDate().IsAprilFoolsDay())
            {
                return true;
            }

            var wine_list = new[] { "酒", "drink", "wine", "beer", "술", "🍷", "🍸", "🍺", "🍻", "🥃", "🍶" };
            foreach (var wine in wine_list)
            {
                if (CreditFirstList.Contains(wine))
                {
                    return true;
                }
            }

            return false;
        }

        public void UpdateRoguelikeThemeList()
        {
            var roguelikeSquad = RoguelikeSquad;

            RoguelikeSquadList = new ObservableCollection<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("DefaultSquad"), Value = string.Empty },
            };

            switch (RoguelikeTheme)
            {
                case "Phantom":
                    // No new items
                    break;

                case "Mizuki":
                    foreach (var item in new ObservableCollection<CombinedData>
                    {
                        new CombinedData { Display = LocalizationHelper.GetString("IS2NewSquad1"), Value = "心胜于物分队" },
                        new CombinedData { Display = LocalizationHelper.GetString("IS2NewSquad2"), Value = "物尽其用分队" },
                        new CombinedData { Display = LocalizationHelper.GetString("IS2NewSquad3"), Value = "以人为本分队" },
                    })
                    {
                        RoguelikeSquadList.Add(item);
                    }

                    break;
            }

            // 通用分队
            foreach (var item in new ObservableCollection<CombinedData>
            {
                new CombinedData { Display = LocalizationHelper.GetString("LeaderSquad"), Value = "指挥分队" },
                new CombinedData { Display = LocalizationHelper.GetString("GatheringSquad"), Value = "集群分队" },
                new CombinedData { Display = LocalizationHelper.GetString("SupportSquad"), Value = "后勤分队" },
                new CombinedData { Display = LocalizationHelper.GetString("SpearheadSquad"), Value = "矛头分队" },
                new CombinedData { Display = LocalizationHelper.GetString("TacticalAssaultOperative"), Value = "突击战术分队" },
                new CombinedData { Display = LocalizationHelper.GetString("TacticalFortificationOperative"), Value = "堡垒战术分队" },
                new CombinedData { Display = LocalizationHelper.GetString("TacticalRangedOperative"), Value = "远程战术分队" },
                new CombinedData { Display = LocalizationHelper.GetString("TacticalDestructionOperative"), Value = "破坏战术分队" },
                new CombinedData { Display = LocalizationHelper.GetString("ResearchSquad"), Value = "研究分队" },
                new CombinedData { Display = LocalizationHelper.GetString("First-ClassSquad"), Value = "高规格分队" },
            })
            {
                RoguelikeSquadList.Add(item);
            }

            RoguelikeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquad) ? roguelikeSquad : string.Empty;
        }
    }
}
