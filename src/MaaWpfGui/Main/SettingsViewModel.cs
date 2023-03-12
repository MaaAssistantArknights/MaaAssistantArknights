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
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using MaaWpfGui.Helper;
using MaaWpfGui.MaaHotKeys;
using Newtonsoft.Json;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
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

        private static readonly string s_versionInfo = Localization.GetString("Version") + ": " + s_versionId;

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
        /// <param name="windowManager">The window manager.</param>
        public SettingsViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;

            DisplayName = Localization.GetString("Settings");
            _listTitle.Add(Localization.GetString("GameSettings"));
            _listTitle.Add(Localization.GetString("BaseSettings"));
            _listTitle.Add(Localization.GetString("RoguelikeSettings"));
            _listTitle.Add(Localization.GetString("RecruitingSettings"));
            _listTitle.Add(Localization.GetString("MallSettings"));
            _listTitle.Add(Localization.GetString("OtherCombatSettings"));
            _listTitle.Add(Localization.GetString("ConnectionSettings"));
            _listTitle.Add(Localization.GetString("StartupSettings"));
            _listTitle.Add(Localization.GetString("ScheduleSettings"));
            _listTitle.Add(Localization.GetString("UISettings"));
            _listTitle.Add(Localization.GetString("HotKeySettings"));
            _listTitle.Add(Localization.GetString("UpdateSettings"));
            _listTitle.Add(Localization.GetString("AboutUs"));

            InfrastInit();

            if (Hangover)
            {
                Hangover = false;
                _windowManager.ShowMessageBox(
                    Localization.GetString("Hangover"),
                    Localization.GetString("Burping"),
                    MessageBoxButton.OK, MessageBoxImage.Hand);
                Application.Current.Shutdown();
                System.Windows.Forms.Application.Restart();
            }
        }

        public void Sober()
        {
            if (Cheers && Language == PallasLangKey)
            {
                ViewStatusStorage.Set("GUI.Localization", SoberLanguage);
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

            var addressListJson = ViewStatusStorage.Get("Connect.AddressHistory", string.Empty);
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
                bool parsed = int.TryParse(ViewStatusStorage.Get("Infrast.Order." + facility, "-1"), out int order);

                if (!parsed || order < 0)
                {
                    temp_order_list[i] = new DragItemViewModel(Localization.GetString(facility), facility, "Infrast.");
                }
                else
                {
                    temp_order_list[order] = new DragItemViewModel(Localization.GetString(facility), facility, "Infrast.");
                }
            }

            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            DefaultInfrastList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("UserDefined"), Value = _userDefined },
                new CombData { Display = Localization.GetString("153_3"), Value = "153_layout_3_times_a_day.json" },
                new CombData { Display = Localization.GetString("243_3"), Value = "243_layout_3_times_a_day.json" },
                new CombData { Display = Localization.GetString("243_4"), Value = "243_layout_4_times_a_day.json" },
                new CombData { Display = Localization.GetString("252_3"), Value = "252_layout_3_times_a_day.json" },
                new CombData { Display = Localization.GetString("333_3"), Value = "333_layout_for_Orundum_3_times_a_day.json" },
            };

            UsesOfDronesList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("DronesNotUse"), Value = "_NotUse" },
                new CombData { Display = Localization.GetString("Money"), Value = "Money" },
                new CombData { Display = Localization.GetString("SyntheticJade"), Value = "SyntheticJade" },
                new CombData { Display = Localization.GetString("CombatRecord"), Value = "CombatRecord" },
                new CombData { Display = Localization.GetString("PureGold"), Value = "PureGold" },
                new CombData { Display = Localization.GetString("OriginStone"), Value = "OriginStone" },
                new CombData { Display = Localization.GetString("Chip"), Value = "Chip" },
            };

            ConnectConfigList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("General"), Value = "General" },
                new CombData { Display = Localization.GetString("BlueStacks"), Value = "BlueStacks" },
                new CombData { Display = Localization.GetString("MuMuEmulator"), Value = "MuMuEmulator" },
                new CombData { Display = Localization.GetString("LDPlayer"), Value = "LDPlayer" },
                new CombData { Display = Localization.GetString("Nox"), Value = "Nox" },
                new CombData { Display = Localization.GetString("XYAZ"), Value = "XYAZ" },
                new CombData { Display = Localization.GetString("WSA"), Value = "WSA" },
                new CombData { Display = Localization.GetString("Compatible"), Value = "Compatible" },
                new CombData { Display = Localization.GetString("GeneralWithoutScreencapErr"), Value = "GeneralWithoutScreencapErr" },
            };

            TouchModeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("MiniTouchMode"), Value = "minitouch" },
                new CombData { Display = Localization.GetString("MaaTouchMode"), Value = "maatouch" },
                new CombData { Display = Localization.GetString("AdbTouchMode"), Value = "adb" },
            };

            _dormThresholdLabel = Localization.GetString("DormThreshold") + ": " + _dormThreshold + "%";

            RoguelikeModeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("RoguelikeStrategyExp"), Value = "0" },
                new CombData { Display = Localization.GetString("RoguelikeStrategyGold"), Value = "1" },

                // new CombData { Display = "两者兼顾，投资过后退出", Value = "2" } // 弃用
                // new CombData { Display = Localization.GetString("3"), Value = "3" },  // 开发中
            };

            RoguelikeThemeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("RoguelikeThemePhantom"), Value = "Phantom" },
                new CombData { Display = Localization.GetString("RoguelikeThemeMizuki"), Value = "Mizuki" },
            };

            UpdateRoguelikeThemeList();

            RoguelikeRolesList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("DefaultRoles"), Value = string.Empty },
                new CombData { Display = Localization.GetString("FirstMoveAdvantage"), Value = "先手必胜" },
                new CombData { Display = Localization.GetString("SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
                new CombData { Display = Localization.GetString("OvercomingYourWeaknesses"), Value = "取长补短" },
                new CombData { Display = Localization.GetString("AsYourHeartDesires"), Value = "随心所欲" },
            };

            ClientTypeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("NotSelected"), Value = string.Empty },
                new CombData { Display = Localization.GetString("Official"), Value = "Official" },
                new CombData { Display = Localization.GetString("Bilibili"), Value = "Bilibili" },
                new CombData { Display = Localization.GetString("YoStarEN"), Value = "YoStarEN" },
                new CombData { Display = Localization.GetString("YoStarJP"), Value = "YoStarJP" },
                new CombData { Display = Localization.GetString("YoStarKR"), Value = "YoStarKR" },
                new CombData { Display = Localization.GetString("txwy"), Value = "txwy" },
            };

            InverseClearModeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("Clear"), Value = "Clear" },
                new CombData { Display = Localization.GetString("Inverse"), Value = "Inverse" },
                new CombData { Display = Localization.GetString("Switchable"), Value = "ClearInverse" },
            };

            VersionTypeList = new List<GenericCombData<UpdateVersionType>>
            {
                new GenericCombData<UpdateVersionType> { Display = Localization.GetString("UpdateCheckNightly"), Value = UpdateVersionType.Nightly },
                new GenericCombData<UpdateVersionType> { Display = Localization.GetString("UpdateCheckBeta"), Value = UpdateVersionType.Beta },
                new GenericCombData<UpdateVersionType> { Display = Localization.GetString("UpdateCheckStable"), Value = UpdateVersionType.Stable },
            };

            LanguageList = new List<CombData>();
            foreach (var pair in Localization.SupportedLanguages)
            {
                if (pair.Key == PallasLangKey && !Cheers)
                {
                    continue;
                }

                LanguageList.Add(new CombData { Display = pair.Value, Value = pair.Key });
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
        private bool _startSelf = MaaWpfGui.AutoStart.CheckStart();

        /// <summary>
        /// Gets or sets a value indicating whether to start itself.
        /// </summary>
        public bool StartSelf
        {
            get => _startSelf;
            set
            {
                SetAndNotify(ref _startSelf, value);
                MaaWpfGui.AutoStart.SetStart(value);
            }
        }

        private bool _runDirectly = Convert.ToBoolean(ViewStatusStorage.Get("Start.RunDirectly", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to run directly.
        /// </summary>
        public bool RunDirectly
        {
            get => _runDirectly;
            set
            {
                SetAndNotify(ref _runDirectly, value);
                ViewStatusStorage.Set("Start.RunDirectly", value.ToString());
            }
        }

        private bool _startEmulator = Convert.ToBoolean(ViewStatusStorage.Get("Start.StartEmulator", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to start emulator.
        /// </summary>
        public bool StartEmulator
        {
            get => _startEmulator;
            set
            {
                SetAndNotify(ref _startEmulator, value);
                ViewStatusStorage.Set("Start.StartEmulator", value.ToString());
                if (ClientType == string.Empty && Idle)
                {
                    ClientType = "Official";
                }
            }
        }

        private bool _minimizingStartup = Convert.ToBoolean(ViewStatusStorage.Get("Start.MinimizingStartup", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimally start the emulator
        /// </summary>
        public bool MinimizingStartup
        {
            get => _minimizingStartup;
            set
            {
                SetAndNotify(ref _minimizingStartup, value);
                ViewStatusStorage.Set("Start.MinimizingStartup", value.ToString());
            }
        }

        private string _emulatorPath = ViewStatusStorage.Get("Start.EmulatorPath", string.Empty);

        /// <summary>
        /// Gets or sets the emulator path.
        /// </summary>
        public string EmulatorPath
        {
            get => _emulatorPath;
            set
            {
                SetAndNotify(ref _emulatorPath, value);
                ViewStatusStorage.Set("Start.EmulatorPath", value);
            }
        }

        private string _emulatorAddCommand = ViewStatusStorage.Get("Start.EmulatorAddCommand", string.Empty);

        /// <summary>
        /// Gets or sets the command to append after the emulator command.
        /// </summary>
        public string EmulatorAddCommand
        {
            get => _emulatorAddCommand;
            set
            {
                SetAndNotify(ref _emulatorAddCommand, value);
                ViewStatusStorage.Set("Start.EmulatorAddCommand", value);
            }
        }

        private string _emulatorWaitSeconds = ViewStatusStorage.Get("Start.EmulatorWaitSeconds", "60");

        /// <summary>
        /// Gets or sets the seconds to wait for the emulator.
        /// </summary>
        public string EmulatorWaitSeconds
        {
            get => _emulatorWaitSeconds;
            set
            {
                SetAndNotify(ref _emulatorWaitSeconds, value);
                ViewStatusStorage.Set("Start.EmulatorWaitSeconds", value);
            }
        }

        /// <summary>
        /// Tries to start the emulator.
        /// </summary>
        /// <param name="manual">Whether to start manually.</param>
        public void TryToStartEmulator(bool manual = false)
        {
            if ((EmulatorPath.Length == 0
                || !System.IO.File.Exists(EmulatorPath))
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

            Thread.Sleep(delay * 1000);
        }

        /// <summary>
        /// Selects the emulator to execute.
        /// </summary>
        public void SelectEmulatorExec()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = Localization.GetString("Executable") + "|*.exe;*.bat;*.lnk",
            };

            if (dialog.ShowDialog() == true)
            {
                EmulatorPath = dialog.FileName;
            }
        }

        private string _clientType = ViewStatusStorage.Get("Start.ClientType", string.Empty);

        /// <summary>
        /// Gets or sets the client type.
        /// </summary>
        public string ClientType
        {
            get => _clientType;
            set
            {
                SetAndNotify(ref _clientType, value);
                Utils.ClientType = value;
                ViewStatusStorage.Set("Start.ClientType", value);
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

        private bool _autoRestartOnDrop = bool.Parse(ViewStatusStorage.Get("Start.AutoRestartOnDrop", "True"));

        public bool AutoRestartOnDrop
        {
            get => _autoRestartOnDrop;
            set
            {
                SetAndNotify(ref _autoRestartOnDrop, value);
                ViewStatusStorage.Set("Start.AutoRestartOnDrop", value.ToString());
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
        public List<CombData> UsesOfDronesList { get; set; }

        /// <summary>
        /// Gets or sets the list of uses of default infrast.
        /// </summary>
        public List<CombData> DefaultInfrastList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike lists.
        /// </summary>
        public List<CombData> RoguelikeThemeList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike modes.
        /// </summary>
        public List<CombData> RoguelikeModeList { get; set; }

        private ObservableCollection<CombData> _roguelikeSquadList = new ObservableCollection<CombData>();

        /// <summary>
        /// Gets or sets the list of roguelike squad.
        /// </summary>
        public ObservableCollection<CombData> RoguelikeSquadList
        {
            get => _roguelikeSquadList;
            set => SetAndNotify(ref _roguelikeSquadList, value);
        }

        /// <summary>
        /// Gets or sets the list of roguelike roles.
        /// </summary>
        public List<CombData> RoguelikeRolesList { get; set; }

        // public List<CombData> RoguelikeCoreCharList { get; set; }

        /// <summary>
        /// Gets or sets the list of the client types.
        /// </summary>
        public List<CombData> ClientTypeList { get; set; }

        /// <summary>
        /// Gets or sets the list of the configuration of connection.
        /// </summary>
        public List<CombData> ConnectConfigList { get; set; }

        /// <summary>
        /// Gets or sets the list of touch modes
        /// </summary>
        public List<CombData> TouchModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of inverse clear modes.
        /// </summary>
        public List<CombData> InverseClearModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of the version type.
        /// </summary>
        public List<GenericCombData<UpdateVersionType>> VersionTypeList { get; set; }

        /// <summary>
        /// Gets or sets the language list.
        /// </summary>
        public List<CombData> LanguageList { get; set; }

        private int _dormThreshold = Convert.ToInt32(ViewStatusStorage.Get("Infrast.DormThreshold", "30"));

        /// <summary>
        /// Gets or sets the threshold to enter dormitory.
        /// </summary>
        public int DormThreshold
        {
            get => _dormThreshold;
            set
            {
                SetAndNotify(ref _dormThreshold, value);
                DormThresholdLabel = Localization.GetString("DormThreshold") + ": " + _dormThreshold + "%";
                ViewStatusStorage.Set("Infrast.DormThreshold", value.ToString());
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
                ViewStatusStorage.Set("Infrast.Order." + item.OriginalName, index.ToString());
                ++index;
            }
        }

        private string _usesOfDrones = ViewStatusStorage.Get("Infrast.UsesOfDrones", "Money");

        /// <summary>
        /// Gets or sets the uses of drones.
        /// </summary>
        public string UsesOfDrones
        {
            get => _usesOfDrones;
            set
            {
                SetAndNotify(ref _usesOfDrones, value);
                ViewStatusStorage.Set("Infrast.UsesOfDrones", value);
            }
        }

        private string _defaultInfrast = ViewStatusStorage.Get("Infrast.DefaultInfrast", _userDefined);

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

                ViewStatusStorage.Set("Infrast.DefaultInfrast", value);
            }
        }

        private string _isCustomInfrastFileReadOnly = ViewStatusStorage.Get("Infrast.IsCustomInfrastFileReadOnly", false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether  CustomInfrastFile is read-only
        /// </summary>
        public bool IsCustomInfrastFileReadOnly
        {
            get => bool.Parse(_isCustomInfrastFileReadOnly);
            set
            {
                SetAndNotify(ref _isCustomInfrastFileReadOnly, value.ToString());
                ViewStatusStorage.Set("Infrast.IsCustomInfrastFileReadOnly", value.ToString());
            }
        }

        private string _dormFilterNotStationedEnabled = ViewStatusStorage.Get("Infrast.DormFilterNotStationedEnabled", false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
        /// </summary>
        public bool DormFilterNotStationedEnabled
        {
            get => bool.Parse(_dormFilterNotStationedEnabled);
            set
            {
                SetAndNotify(ref _dormFilterNotStationedEnabled, value.ToString());
                ViewStatusStorage.Set("Infrast.DormFilterNotStationedEnabled", value.ToString());
            }
        }

        private string _dormTrustEnabled = ViewStatusStorage.Get("Infrast.DormTrustEnabled", true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether get trust in dorm is enabled.
        /// </summary>
        public bool DormTrustEnabled
        {
            get => bool.Parse(_dormTrustEnabled);
            set
            {
                SetAndNotify(ref _dormTrustEnabled, value.ToString());
                ViewStatusStorage.Set("Infrast.DormTrustEnabled", value.ToString());
            }
        }

        private string _originiumShardAutoReplenishment = ViewStatusStorage.Get("Infrast.OriginiumShardAutoReplenishment", true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
        /// </summary>
        public bool OriginiumShardAutoReplenishment
        {
            get => bool.Parse(_originiumShardAutoReplenishment);
            set
            {
                SetAndNotify(ref _originiumShardAutoReplenishment, value.ToString());
                ViewStatusStorage.Set("Infrast.OriginiumShardAutoReplenishment", value.ToString());
            }
        }

        private bool _customInfrastEnabled = bool.Parse(ViewStatusStorage.Get("Infrast.CustomInfrastEnabled", false.ToString()));

        public bool CustomInfrastEnabled
        {
            get => _customInfrastEnabled;
            set
            {
                SetAndNotify(ref _customInfrastEnabled, value);
                ViewStatusStorage.Set("Infrast.CustomInfrastEnabled", value.ToString());
                _taskQueueViewModel.CustomInfrastEnabled = value;
            }
        }

        /// <summary>
        /// Selects infrast config file.
        /// </summary>
        public void SelectCustomInfrastFile()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = Localization.GetString("CustomInfrastFile") + "|*.json",
            };

            if (dialog.ShowDialog() == true)
            {
                CustomInfrastFile = dialog.FileName;
            }

            DefaultInfrast = _userDefined;
        }

        private string _customInfrastFile = ViewStatusStorage.Get("Infrast.CustomInfrastFile", string.Empty);

        public string CustomInfrastFile
        {
            get => _customInfrastFile;
            set
            {
                SetAndNotify(ref _customInfrastFile, value);
                ViewStatusStorage.Set("Infrast.CustomInfrastFile", value);
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

        private System.Timers.Timer _resetNotifyTimer;

        private void ResetNotifySource()
        {
            if (_resetNotifyTimer != null)
            {
                _resetNotifyTimer.Stop();
                _resetNotifyTimer.Close();
            }

            _resetNotifyTimer = new System.Timers.Timer(20);
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

        private string _roguelikeTheme = ViewStatusStorage.Get("Roguelike.RoguelikeTheme", "Phantom");

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
                ViewStatusStorage.Set("Roguelike.RoguelikeTheme", value);
            }
        }

        private string _roguelikeMode = ViewStatusStorage.Get("Roguelike.Mode", "0");

        /// <summary>
        /// Gets or sets the roguelike mode.
        /// </summary>
        public string RoguelikeMode
        {
            get => _roguelikeMode;
            set
            {
                SetAndNotify(ref _roguelikeMode, value);
                ViewStatusStorage.Set("Roguelike.Mode", value);
                if (value == "1")
                {
                    RoguelikeInvestmentEnabled = true;
                }
            }
        }

        private string _roguelikeSquad = ViewStatusStorage.Get("Roguelike.Squad", string.Empty);

        /// <summary>
        /// Gets or sets the roguelike squad.
        /// </summary>
        public string RoguelikeSquad
        {
            get => _roguelikeSquad;
            set
            {
                SetAndNotify(ref _roguelikeSquad, value);
                ViewStatusStorage.Set("Roguelike.Squad", value);
            }
        }

        private string _roguelikeRoles = ViewStatusStorage.Get("Roguelike.Roles", string.Empty);

        /// <summary>
        /// Gets or sets the roguelike roles.
        /// </summary>
        public string RoguelikeRoles
        {
            get => _roguelikeRoles;
            set
            {
                SetAndNotify(ref _roguelikeRoles, value);
                ViewStatusStorage.Set("Roguelike.Roles", value);
            }
        }

        private string _roguelikeCoreChar = ViewStatusStorage.Get("Roguelike.CoreChar", string.Empty);

        /// <summary>
        /// Gets or sets the roguelike core character.
        /// </summary>
        public string RoguelikeCoreChar
        {
            get => _roguelikeCoreChar;
            set
            {
                SetAndNotify(ref _roguelikeCoreChar, value);
                ViewStatusStorage.Set("Roguelike.CoreChar", value);
            }
        }

        private string _roguelikeUseSupportUnit = ViewStatusStorage.Get("Roguelike.RoguelikeUseSupportUnit", false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether use support unit.
        /// </summary>
        public bool RoguelikeUseSupportUnit
        {
            get => bool.Parse(_roguelikeUseSupportUnit);
            set
            {
                SetAndNotify(ref _roguelikeUseSupportUnit, value.ToString());
                ViewStatusStorage.Set("Roguelike.RoguelikeUseSupportUnit", value.ToString());
            }
        }

        private string _roguelikeEnableNonfriendSupport = ViewStatusStorage.Get("Roguelike.RoguelikeEnableNonfriendSupport", false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether can roguelike support unit belong to nonfriend
        /// </summary>
        public bool RoguelikeEnableNonfriendSupport
        {
            get => bool.Parse(_roguelikeEnableNonfriendSupport);
            set
            {
                SetAndNotify(ref _roguelikeEnableNonfriendSupport, value.ToString());
                ViewStatusStorage.Set("Roguelike.RoguelikeEnableNonfriendSupport", value.ToString());
            }
        }

        private string _roguelikeStartsCount = ViewStatusStorage.Get("Roguelike.StartsCount", "9999999");

        /// <summary>
        /// Gets or sets the start count of roguelike.
        /// </summary>
        public int RoguelikeStartsCount
        {
            get => int.Parse(_roguelikeStartsCount);
            set
            {
                SetAndNotify(ref _roguelikeStartsCount, value.ToString());
                ViewStatusStorage.Set("Roguelike.StartsCount", value.ToString());
            }
        }

        private string _roguelikeInvestmentEnabled = ViewStatusStorage.Get("Roguelike.InvestmentEnabled", true.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether investment is enabled.
        /// </summary>
        public bool RoguelikeInvestmentEnabled
        {
            get => bool.Parse(_roguelikeInvestmentEnabled);
            set
            {
                SetAndNotify(ref _roguelikeInvestmentEnabled, value.ToString());
                ViewStatusStorage.Set("Roguelike.InvestmentEnabled", value.ToString());
            }
        }

        private string _roguelikeInvestsCount = ViewStatusStorage.Get("Roguelike.InvestsCount", "9999999");

        /// <summary>
        /// Gets or sets the invests count of roguelike.
        /// </summary>
        public int RoguelikeInvestsCount
        {
            get => int.Parse(_roguelikeInvestsCount);
            set
            {
                SetAndNotify(ref _roguelikeInvestsCount, value.ToString());
                ViewStatusStorage.Set("Roguelike.InvestsCount", value.ToString());
            }
        }

        private string _roguelikeStopWhenInvestmentFull = ViewStatusStorage.Get("Roguelike.StopWhenInvestmentFull", false.ToString());

        /// <summary>
        /// Gets or sets a value indicating whether to stop when investment is full.
        /// </summary>
        public bool RoguelikeStopWhenInvestmentFull
        {
            get => bool.Parse(_roguelikeStopWhenInvestmentFull);
            set
            {
                SetAndNotify(ref _roguelikeStopWhenInvestmentFull, value.ToString());
                ViewStatusStorage.Set("Roguelike.StopWhenInvestmentFull", value.ToString());
            }
        }

        /* 访问好友设置 */
        private string _lastCreditFightTaskTime = ViewStatusStorage.Get("Visit.LastCreditFightTaskTime", Utils.GetYJTimeDate().AddDays(-1).ToString("yyyy/MM/dd HH:mm:ss", DateTimeFormatInfo.InvariantInfo));

        public string LastCreditFightTaskTime
        {
            get => _lastCreditFightTaskTime;
            set
            {
                SetAndNotify(ref _lastCreditFightTaskTime, value);
                ViewStatusStorage.Set("Visit.LastCreditFightTaskTime", value.ToString());
            }
        }

        private bool _creditFightTaskEnabled = Convert.ToBoolean(ViewStatusStorage.Get("Visit.CreditFightTaskEnabled", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether credit fight task is enabled.
        /// </summary>
        public bool CreditFightTaskEnabled
        {
            get
            {
                try
                {
                    if (Utils.GetYJTimeDate() > DateTime.ParseExact(_lastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
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
                ViewStatusStorage.Set("Visit.CreditFightTaskEnabled", value.ToString());
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
                ViewStatusStorage.Set("Visit.CreditFightTaskEnabled", value.ToString());
            }
        }

        /* 信用商店设置 */

        private bool _creditShopping = Convert.ToBoolean(ViewStatusStorage.Get("Mall.CreditShopping", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to shop with credit.
        /// </summary>
        public bool CreditShopping
        {
            get => _creditShopping;
            set
            {
                SetAndNotify(ref _creditShopping, value);
                ViewStatusStorage.Set("Mall.CreditShopping", value.ToString());
            }
        }

        private string _creditFirstList = ViewStatusStorage.Get("Mall.CreditFirstListNew", Localization.GetString("HighPriorityDefault"));

        /// <summary>
        /// Gets or sets the priority item list of credit shop.
        /// </summary>
        public string CreditFirstList
        {
            get => _creditFirstList;
            set
            {
                SetAndNotify(ref _creditFirstList, value);
                ViewStatusStorage.Set("Mall.CreditFirstListNew", value);
            }
        }

        private string _creditBlackList = ViewStatusStorage.Get("Mall.CreditBlackListNew", Localization.GetString("BlacklistDefault"));

        /// <summary>
        /// Gets or sets the blacklist of credit shop.
        /// </summary>
        public string CreditBlackList
        {
            get => _creditBlackList;
            set
            {
                SetAndNotify(ref _creditBlackList, value);
                ViewStatusStorage.Set("Mall.CreditBlackListNew", value);
            }
        }

        private bool _creditForceShoppingIfCreditFull = bool.Parse(ViewStatusStorage.Get("Mall.CreditForceShoppingIfCreditFull", false.ToString()));

        /// <summary>
        /// Gets or sets a value indicating whether save credit is enabled.
        /// </summary>
        public bool CreditForceShoppingIfCreditFull
        {
            get => _creditForceShoppingIfCreditFull;
            set
            {
                SetAndNotify(ref _creditForceShoppingIfCreditFull, value);
                ViewStatusStorage.Set("Mall.CreditForceShoppingIfCreditFull", value.ToString());
            }
        }

        /* 定时设置 */

        public class TimerModel
        {
            public class TimerProperties : System.ComponentModel.INotifyPropertyChanged
            {
                public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;

                protected void OnPropertyChanged([CallerMemberName] string name = null)
                {
                    PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(name));
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
                        ViewStatusStorage.Set($"Timer.Timer{TimerId + 1}", value.ToString());
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
                        ViewStatusStorage.Set($"Timer.Timer{TimerId + 1}Hour", value.ToString());
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
                        ViewStatusStorage.Set($"Timer.Timer{TimerId + 1}Min", value.ToString());
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
                        IsOn = ViewStatusStorage.Get($"Timer.Timer{i + 1}", bool.FalseString) == bool.TrueString,
                        Hour = int.Parse(ViewStatusStorage.Get($"Timer.Timer{i + 1}Hour", $"{i * 3}")),
                        Min = int.Parse(ViewStatusStorage.Get($"Timer.Timer{i + 1}Min", "0")),
                    };
                }
            }
        }

        public TimerModel TimerModels { get; set; } = new TimerModel();

        /* 刷理智设置 */

        private string _penguinId = ViewStatusStorage.Get("Penguin.Id", string.Empty);

        /// <summary>
        /// Gets or sets the id of PenguinStats.
        /// </summary>
        public string PenguinId
        {
            get => _penguinId;
            set
            {
                SetAndNotify(ref _penguinId, value);
                ViewStatusStorage.Set("Penguin.Id", value);
            }
        }

        private bool _isDrGrandet = Convert.ToBoolean(ViewStatusStorage.Get("Penguin.IsDrGrandet", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use DrGrandet mode.
        /// </summary>
        public bool IsDrGrandet
        {
            get => _isDrGrandet;
            set
            {
                SetAndNotify(ref _isDrGrandet, value);
                ViewStatusStorage.Set("Penguin.IsDrGrandet", value.ToString());
            }
        }

        /* 自动公招设置 */
        private string _recruitMaxTimes = ViewStatusStorage.Get("AutoRecruit.MaxTimes", "4");

        /// <summary>
        /// Gets or sets the maximum times of recruit.
        /// </summary>
        public string RecruitMaxTimes
        {
            get => _recruitMaxTimes;
            set
            {
                SetAndNotify(ref _recruitMaxTimes, value);
                ViewStatusStorage.Set("AutoRecruit.MaxTimes", value);
            }
        }

        private bool _refreshLevel3 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.RefreshLevel3", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to refresh level 3.
        /// </summary>
        public bool RefreshLevel3
        {
            get => _refreshLevel3;
            set
            {
                SetAndNotify(ref _refreshLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.RefreshLevel3", value.ToString());
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

        private bool _isLevel3UseShortTime = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.IsLevel3UseShortTime", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to shorten the time for level 3.
        /// </summary>
        public bool IsLevel3UseShortTime
        {
            get => _isLevel3UseShortTime;
            set
            {
                SetAndNotify(ref _isLevel3UseShortTime, value);
                ViewStatusStorage.Set("AutoRecruit.IsLevel3UseShortTime", value.ToString());
            }
        }

        private bool _notChooseLevel1 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.NotChooseLevel1", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether not to choose level 1.
        /// </summary>
        public bool NotChooseLevel1
        {
            get => _notChooseLevel1;
            set
            {
                SetAndNotify(ref _notChooseLevel1, value);
                ViewStatusStorage.Set("AutoRecruit.NotChooseLevel1", value.ToString());
            }
        }

        private bool _chooseLevel3 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel3", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 3.
        /// </summary>
        public bool ChooseLevel3
        {
            get => _chooseLevel3;
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel3", value.ToString());
            }
        }

        private bool _chooseLevel4 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel4", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 4.
        /// </summary>
        public bool ChooseLevel4
        {
            get => _chooseLevel4;
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel4", value.ToString());
            }
        }

        private bool _chooseLevel5 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel5", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 5.
        /// </summary>
        public bool ChooseLevel5
        {
            get => _chooseLevel5;
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel5", value.ToString());
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
                ViewStatusStorage.Get("VersionUpdate.VersionType", UpdateVersionType.Stable.ToString()));

        /// <summary>
        /// Gets or sets the type of version to update.
        /// </summary>
        public UpdateVersionType VersionType
        {
            get => _versionType;
            set
            {
                SetAndNotify(ref _versionType, value);
                ViewStatusStorage.Set("VersionUpdate.VersionType", value.ToString());
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

        private bool _updateCheck = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UpdateCheck", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to check update.
        /// </summary>
        public bool UpdateCheck
        {
            get => _updateCheck;
            set
            {
                SetAndNotify(ref _updateCheck, value);
                ViewStatusStorage.Set("VersionUpdate.UpdateCheck", value.ToString());
            }
        }

        private string _proxy = ViewStatusStorage.Get("VersionUpdate.Proxy", string.Empty);

        /// <summary>
        /// Gets or sets the proxy settings.
        /// </summary>
        public string Proxy
        {
            get => _proxy;
            set
            {
                WebService.Proxy = value;
                SetAndNotify(ref _proxy, value);
                ViewStatusStorage.Set("VersionUpdate.Proxy", value);
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

        private bool _autoDownloadUpdatePackage = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.AutoDownloadUpdatePackage", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to auto download update package.
        /// </summary>
        public bool AutoDownloadUpdatePackage
        {
            get => _autoDownloadUpdatePackage;
            set
            {
                SetAndNotify(ref _autoDownloadUpdatePackage, value);
                ViewStatusStorage.Set("VersionUpdate.AutoDownloadUpdatePackage", value.ToString());
            }
        }

        /// <summary>
        /// Updates manually.
        /// </summary>
        // TODO: 你确定要用 async void 不是 async Task？
        public async void ManualUpdate()
        {
            var task = Task.Run(() =>
            {
                return _versionUpdateViewModel.CheckAndDownloadUpdate(true);
            });
            var ret = await task;

            string toastMessage = null;
            switch (ret)
            {
                case VersionUpdateViewModel.CheckUpdateRetT.NoNeedToUpdate:
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.AlreadyLatest:
                    toastMessage = Localization.GetString("AlreadyLatest");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.UnknownError:
                    toastMessage = Localization.GetString("NewVersionDetectFailedTitle");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.NetworkError:
                    toastMessage = Localization.GetString("CheckNetworking");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.FailedToGetInfo:
                    toastMessage = Localization.GetString("GetReleaseNoteFailed");
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.OK:
                    _versionUpdateViewModel.IsFirstBootAfterUpdate = true;
                    _versionUpdateViewModel.AskToRestart();
                    break;

                case VersionUpdateViewModel.CheckUpdateRetT.NewVersionIsBeingBuilt:
                    toastMessage = Localization.GetString("NewVersionIsBeingBuilt");
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

        private bool _autoDetectConnection = bool.Parse(ViewStatusStorage.Get("Connect.AutoDetect", true.ToString()));

        public bool AutoDetectConnection
        {
            get => _autoDetectConnection;
            set
            {
                SetAndNotify(ref _autoDetectConnection, value);
                ViewStatusStorage.Set("Connect.AutoDetect", value.ToString());
            }
        }

        private bool _alwaysAutoDetectConnection = bool.Parse(ViewStatusStorage.Get("Connect.AlwaysAutoDetect", false.ToString()));

        public bool AlwaysAutoDetectConnection
        {
            get => _alwaysAutoDetectConnection;
            set
            {
                SetAndNotify(ref _alwaysAutoDetectConnection, value);
                ViewStatusStorage.Set("Connect.AlwaysAutoDetect", value.ToString());
            }
        }

        private ObservableCollection<string> _connectAddressHistory = new ObservableCollection<string>();

        public ObservableCollection<string> ConnectAddressHistory
        {
            get => _connectAddressHistory;
            set => SetAndNotify(ref _connectAddressHistory, value);
        }

        private string _connectAddress = ViewStatusStorage.Get("Connect.Address", string.Empty);

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
                ViewStatusStorage.Set("Connect.AddressHistory", JsonConvert.SerializeObject(ConnectAddressHistory));
                ViewStatusStorage.Set("Connect.Address", value);
                UpdateWindowTitle(); /* 每次修改连接地址时更新WindowTitle */
            }
        }

        public void RemoveAddress_Click(string address)
        {
            ConnectAddressHistory.Remove(address);
            ViewStatusStorage.Set("Connect.AddressHistory", JsonConvert.SerializeObject(ConnectAddressHistory));
        }

        private string _adbPath = ViewStatusStorage.Get("Connect.AdbPath", string.Empty);

        /// <summary>
        /// Gets or sets the ADB path.
        /// </summary>
        public string AdbPath
        {
            get => _adbPath;
            set
            {
                SetAndNotify(ref _adbPath, value);
                ViewStatusStorage.Set("Connect.AdbPath", value);
            }
        }

        private string _connectConfig = ViewStatusStorage.Get("Connect.ConnectConfig", "General");

        /// <summary>
        /// Gets or sets the connection config.
        /// </summary>
        public string ConnectConfig
        {
            get => _connectConfig;
            set
            {
                SetAndNotify(ref _connectConfig, value);
                ViewStatusStorage.Set("Connect.ConnectConfig", value);
                UpdateWindowTitle(); /* 每次修改连接配置时更新WindowTitle */
            }
        }

        private bool _retryOnDisconnected = Convert.ToBoolean(ViewStatusStorage.Get("Connect.RetryOnDisconnected", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to retry task after adb disconnected.
        /// </summary>
        public bool RetryOnDisconnected
        {
            get => _retryOnDisconnected;
            set
            {
                SetAndNotify(ref _retryOnDisconnected, value);
                ViewStatusStorage.Set("Connect.RetryOnDisconnected", value.ToString());
            }
        }

        private bool _deploymentWithPause = bool.Parse(ViewStatusStorage.Get("Roguelike.DeploymentWithPause", false.ToString()));

        public bool DeploymentWithPause
        {
            get => _deploymentWithPause;
            set
            {
                SetAndNotify(ref _deploymentWithPause, value);
                ViewStatusStorage.Set("Roguelike.DeploymentWithPause", value.ToString());
                UpdateInstanceSettings();
            }
        }

        private bool _adbLiteEnabled = bool.Parse(ViewStatusStorage.Get("Connect.AdbLiteEnabled", false.ToString()));

        public bool AdbLiteEnabled
        {
            get => _adbLiteEnabled;
            set
            {
                SetAndNotify(ref _adbLiteEnabled, value);
                ViewStatusStorage.Set("Connect.AdbLiteEnabled", value.ToString());
                UpdateInstanceSettings();
            }
        }

        private readonly Dictionary<string, List<string>> _defaultAddress = new Dictionary<string, List<string>>
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
        /// Gets the default addresses.
        /// </summary>
        public Dictionary<string, List<string>> DefaultAddress => _defaultAddress;

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
                error = Localization.GetString("EmulatorException");
                return false;
            }

            if (emulators.Count == 0)
            {
                error = Localization.GetString("EmulatorNotFound");
                return false;
            }
            else if (emulators.Count > 1)
            {
                error = Localization.GetString("EmulatorTooMany");
                return false;
            }

            ConnectConfig = emulators.First();
            AdbPath = adapter.GetAdbPathByEmulatorName(ConnectConfig) ?? AdbPath;
            if (string.IsNullOrEmpty(AdbPath))
            {
                error = Localization.GetString("AdbException");
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
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = Localization.GetString("ADBProgram") + "|*.exe",
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

            rvm.WindowTitle = $"MAA - {VersionId} - {connectConfigName} ({ConnectAddress}) - {ClientName}";
        }

        private readonly string _bluestacksConfig = ViewStatusStorage.Get("Bluestacks.Config.Path", string.Empty);
        private readonly string _bluestacksKeyWord = ViewStatusStorage.Get("Bluestacks.Config.Keyword", "bst.instance.Nougat64.status.adb_port");

        /// <summary>
        /// Tries to set Bluestack Hyper V address.
        /// </summary>
        public void TryToSetBlueStacksHyperVAddress()
        {
            if (_bluestacksConfig.Length == 0)
            {
                return;
            }

            if (!System.IO.File.Exists(_bluestacksConfig))
            {
                ViewStatusStorage.Set("Bluestacks.Config.Error", "File not exists");
                return;
            }

            var all_lines = System.IO.File.ReadAllLines(_bluestacksConfig);
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

        private string _touchMode = ViewStatusStorage.Get("Connect.TouchMode", "minitouch");

        public string TouchMode
        {
            get => _touchMode;
            set
            {
                SetAndNotify(ref _touchMode, value);
                ViewStatusStorage.Set("Connect.TouchMode", value);
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
            if (!System.IO.File.Exists(AdbPath))
            {
                Execute.OnUIThread(() =>
                {
                    using var toast = new ToastNotification(Localization.GetString("ReplaceADBNotExists"));
                    toast.Show();
                });
                return;
            }

            if (!System.IO.File.Exists(GoogleAdbFilename))
            {
                var downloadTask = Task.Run(() =>
                {
                    return VersionUpdateViewModel.DownloadFile(GoogleAdbDownloadUrl, GoogleAdbFilename);
                });
                var downloadResult = await downloadTask;
                if (!downloadResult)
                {
                    Execute.OnUIThread(() =>
                    {
                        using var toast = new ToastNotification(Localization.GetString("AdbDownloadFailedTitle"));
                        toast.AppendContentText(Localization.GetString("AdbDownloadFailedDesc")).Show();
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

               System.IO.File.Copy(AdbPath, AdbPath + ".bak", true);

               const string UnzipDir = "adb_unzip";
               if (Directory.Exists(UnzipDir))
               {
                   Directory.Delete(UnzipDir, true);
               }

               System.IO.Compression.ZipFile.ExtractToDirectory(GoogleAdbFilename, UnzipDir);
               System.IO.File.Copy(UnzipDir + "/platform-tools/adb.exe", AdbPath, true);
               Directory.Delete(UnzipDir, true);
           });
            await procTask;

            AdbReplaced = true;
            ViewStatusStorage.Set("Connect.AdbReplaced", true.ToString());
            Execute.OnUIThread(() =>
            {
                using var toast = new ToastNotification(Localization.GetString("SuccessfullyReplacedADB"));
                toast.Show();
            });
        }

        public bool AdbReplaced { get; set; } = Convert.ToBoolean(ViewStatusStorage.Get("Connect.AdbReplaced", false.ToString()));

        /* 界面设置 */

        /// <summary>
        /// Gets a value indicating whether to use tray icon.
        /// </summary>
        public bool UseTray => true;

        private bool _minimizeToTray = Convert.ToBoolean(ViewStatusStorage.Get("GUI.MinimizeToTray", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimize to tray.
        /// </summary>
        public bool MinimizeToTray
        {
            get => _minimizeToTray;
            set
            {
                SetAndNotify(ref _minimizeToTray, value);
                ViewStatusStorage.Set("GUI.MinimizeToTray", value.ToString());
                _mainWindowManager.SetMinimizeToTaskbar(value);
            }
        }

        private bool _useNotify = Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseNotify", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to use notification.
        /// </summary>
        public bool UseNotify
        {
            get => _useNotify;
            set
            {
                SetAndNotify(ref _useNotify, value);
                ViewStatusStorage.Set("GUI.UseNotify", value.ToString());
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

        private bool _loadGUIParameters = Convert.ToBoolean(ViewStatusStorage.Get("GUI.PositionAndSize.Load", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to load GUI parameters.
        /// </summary>
        public bool LoadGUIParameters
        {
            get => _loadGUIParameters;
            set
            {
                SetAndNotify(ref _loadGUIParameters, value);
                ViewStatusStorage.Set("GUI.PositionAndSize.Load", value.ToString());
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

        private bool _saveGUIParametersOnClosing = Convert.ToBoolean(ViewStatusStorage.Get("GUI.PositionAndSize.SaveOnClosing", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to save GUI parameters on closing main window.
        /// </summary>
        public bool SaveGUIParametersOnClosing
        {
            get => _saveGUIParametersOnClosing;
            set
            {
                SetAndNotify(ref _saveGUIParametersOnClosing, value);
                ViewStatusStorage.Set("GUI.PositionAndSize.SaveOnClosing", value.ToString());
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
            ViewStatusStorage.Set("GUI.PositionAndSize.Load", LoadGUIParameters.ToString());
            ViewStatusStorage.Set("GUI.PositionAndSize.SaveOnClosing", SaveGUIParametersOnClosing.ToString());

            var mainWindow = Application.Current.MainWindow;
            System.Windows.Forms.Screen currentScreen =
                System.Windows.Forms.Screen.FromHandle(new WindowInteropHelper(mainWindow).Handle);
            var screenRect = currentScreen.Bounds;
            ViewStatusStorage.Set("GUI.Monitor.Number", currentScreen.DeviceName);
            ViewStatusStorage.Set("GUI.Monitor.Width", screenRect.Width.ToString());
            ViewStatusStorage.Set("GUI.Monitor.Height", screenRect.Height.ToString());

            ViewStatusStorage.Set("GUI.Position.Left", (mainWindow.Left - screenRect.Left).ToString(CultureInfo.InvariantCulture));
            ViewStatusStorage.Set("GUI.Position.Top", (mainWindow.Top - screenRect.Top).ToString(CultureInfo.InvariantCulture));
            ViewStatusStorage.Set("GUI.Size.Width", mainWindow.Width.ToString(CultureInfo.InvariantCulture));
            ViewStatusStorage.Set("GUI.Size.Height", mainWindow.Height.ToString(CultureInfo.InvariantCulture));
        }

        private bool _useAlternateStage = Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseAlternateStage", bool.FalseString));

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
                ViewStatusStorage.Set("GUI.UseAlternateStage", value.ToString());
                if (value)
                {
                    HideUnavailableStage = false;
                }
            }
        }

        private bool _useRemainingSanityStage = bool.Parse(ViewStatusStorage.Get("Fight.UseRemainingSanityStage", bool.TrueString));

        public bool UseRemainingSanityStage
        {
            get => _useRemainingSanityStage;
            set
            {
                SetAndNotify(ref _useRemainingSanityStage, value);
                _taskQueueViewModel.UseRemainingSanityStage = value;
                ViewStatusStorage.Set("Fight.UseRemainingSanityStage", value.ToString());
            }
        }

        private bool _useExpiringMedicine = bool.Parse(ViewStatusStorage.Get("Fight.UseExpiringMedicine", bool.FalseString));

        public bool UseExpiringMedicine
        {
            get => _useExpiringMedicine;
            set
            {
                SetAndNotify(ref _useExpiringMedicine, value);
                ViewStatusStorage.Set("Fight.UseExpiringMedicine", value.ToString());
            }
        }

        private bool _hideUnavailableStage = Convert.ToBoolean(ViewStatusStorage.Get("GUI.HideUnavailableStage", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide unavailable stages.
        /// </summary>
        public bool HideUnavailableStage
        {
            get => _hideUnavailableStage;
            set
            {
                SetAndNotify(ref _hideUnavailableStage, value);
                ViewStatusStorage.Set("GUI.HideUnavailableStage", value.ToString());

                if (value)
                {
                    UseAlternateStage = false;
                }

                _taskQueueViewModel.UpdateStageList(true);
            }
        }

        private bool _customStageCode = Convert.ToBoolean(ViewStatusStorage.Get("GUI.CustomStageCode", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use custom stage code.
        /// </summary>
        public bool CustomStageCode
        {
            get => _customStageCode;
            set
            {
                SetAndNotify(ref _customStageCode, value);
                ViewStatusStorage.Set("GUI.CustomStageCode", value.ToString());
                _taskQueueViewModel.CustomStageCode = value;
            }
        }

        private enum InverseClearType
        {
            Clear,
            Inverse,
            ClearInverse,
        }

        private InverseClearType _inverseClearMode =
            Enum.TryParse(ViewStatusStorage.Get("GUI.InverseClearMode", InverseClearType.Clear.ToString()),
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
                ViewStatusStorage.Set("GUI.InverseClearMode", value);
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

        private string _soberLanguage = ViewStatusStorage.Get("GUI.SoberLanguage", Localization.DefaultLanguage);

        public string SoberLanguage
        {
            get => _soberLanguage;
            set
            {
                SetAndNotify(ref _soberLanguage, value);
                ViewStatusStorage.Set("GUI.SoberLanguage", value);
            }
        }

        private string _language = ViewStatusStorage.Get("GUI.Localization", Localization.DefaultLanguage);

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
                ViewStatusStorage.Set("GUI.Localization", value);

                string FormatText(string text, string key)
                    => string.Format(text, Localization.GetString(key, value), Localization.GetString(key, _language));
                System.Windows.Forms.MessageBoxManager.Unregister();
                System.Windows.Forms.MessageBoxManager.Yes = FormatText("{0}({1})", "Ok");
                System.Windows.Forms.MessageBoxManager.No = FormatText("{0}({1})", "ManualRestart");
                System.Windows.Forms.MessageBoxManager.Register();
                Window mainWindow = Application.Current.MainWindow;
                mainWindow.Show();
                mainWindow.WindowState = mainWindow.WindowState = WindowState.Normal;
                mainWindow.Activate();
                var result = MessageBox.Show(
                    FormatText("{0}\n{1}", "LanguageChangedTip"),
                    FormatText("{0}({1})", "Tip"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Question);
                System.Windows.Forms.MessageBoxManager.Unregister();
                SetAndNotify(ref _language, value);
                if (result == MessageBoxResult.Yes)
                {
                    Application.Current.Shutdown();
                    System.Windows.Forms.Application.Restart();
                }
            }
        }

        private bool _cheers = bool.Parse(ViewStatusStorage.Get("GUI.Cheers", bool.FalseString));

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
                ViewStatusStorage.Set("GUI.Cheers", value.ToString());
                if (_cheers)
                {
                    SetPallasLanguage();
                }
            }
        }

        private bool _hangover = bool.Parse(ViewStatusStorage.Get("GUI.Hangover", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hangover.
        /// </summary>
        public bool Hangover
        {
            get => _hangover;
            set
            {
                SetAndNotify(ref _hangover, value);
                ViewStatusStorage.Set("GUI.Hangover", value.ToString());
            }
        }

        private void SetPallasLanguage()
        {
            ViewStatusStorage.Set("GUI.Localization", PallasLangKey);
            var result = _windowManager.ShowMessageBox(
                Localization.GetString("DrunkAndStaggering"),
                Localization.GetString("Burping"),
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
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

            RoguelikeSquadList = new ObservableCollection<CombData>
            {
                new CombData { Display = Localization.GetString("DefaultSquad"), Value = string.Empty },
            };

            switch (RoguelikeTheme)
            {
                case "Phantom":
                    // No new items
                    break;

                case "Mizuki":
                    foreach (var item in new ObservableCollection<CombData>
                    {
                        new CombData { Display = Localization.GetString("IS2NewSquad1"), Value = "心胜于物分队" },
                        new CombData { Display = Localization.GetString("IS2NewSquad2"), Value = "物尽其用分队" },
                        new CombData { Display = Localization.GetString("IS2NewSquad3"), Value = "以人为本分队" },
                    })
                    {
                        RoguelikeSquadList.Add(item);
                    }

                    break;
            }

            // 通用分队
            foreach (var item in new ObservableCollection<CombData>
            {
                new CombData { Display = Localization.GetString("LeaderSquad"), Value = "指挥分队" },
                new CombData { Display = Localization.GetString("GatheringSquad"), Value = "集群分队" },
                new CombData { Display = Localization.GetString("SupportSquad"), Value = "后勤分队" },
                new CombData { Display = Localization.GetString("SpearheadSquad"), Value = "矛头分队" },
                new CombData { Display = Localization.GetString("TacticalAssaultOperative"), Value = "突击战术分队" },
                new CombData { Display = Localization.GetString("TacticalFortificationOperative"), Value = "堡垒战术分队" },
                new CombData { Display = Localization.GetString("TacticalRangedOperative"), Value = "远程战术分队" },
                new CombData { Display = Localization.GetString("TacticalDestructionOperative"), Value = "破坏战术分队" },
                new CombData { Display = Localization.GetString("ResearchSquad"), Value = "研究分队" },
                new CombData { Display = Localization.GetString("First-ClassSquad"), Value = "高规格分队" },
            })
            {
                RoguelikeSquadList.Add(item);
            }

            RoguelikeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquad) ? roguelikeSquad : string.Empty;
        }
    }
}
