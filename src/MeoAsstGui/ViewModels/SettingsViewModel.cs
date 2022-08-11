// <copyright file="SettingsViewModel.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of settings.
    /// </summary>
    public class SettingsViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        [DllImport("MeoAssistant.dll")]
        private static extern IntPtr AsstGetVersion();

        private readonly string _versionInfo = Localization.GetString("Version") + ": " + Marshal.PtrToStringAnsi(AsstGetVersion());

        /// <summary>
        /// Gets the version info.
        /// </summary>
        public string VersionInfo
        {
            get { return _versionInfo; }
        }

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

            _listTitle.Add(Localization.GetString("BaseSettings"));
            _listTitle.Add(Localization.GetString("RoguelikeSettings"));
            _listTitle.Add(Localization.GetString("RecruitingSettings"));
            _listTitle.Add(Localization.GetString("MallSettings"));
            _listTitle.Add(Localization.GetString("PenguinSettings"));
            _listTitle.Add(Localization.GetString("ConnectionSettings"));
            _listTitle.Add(Localization.GetString("StartupSettings"));
            _listTitle.Add(Localization.GetString("ScheduleSettings"));
            _listTitle.Add(Localization.GetString("UISettings"));
            _listTitle.Add(Localization.GetString("UpdateSettings"));
            _listTitle.Add(Localization.GetString("AboutUs"));

            InfrastInit();

            var trayObj = _container.Get<TrayIcon>();
            trayObj.SetVisible(UseTray);
            trayObj.SetMinimizeToTaskbar(MinimizeToTray);
            trayObj.SetSettingsViewModel(this);
            Bootstrapper.SetTrayIconInSettingsViewModel(this);

            if (Hangover)
            {
                _windowManager.ShowMessageBox(
                    Localization.GetString("Hangover"),
                    Localization.GetString("Burping"),
                    MessageBoxButton.OK, MessageBoxImage.Hand);
                Hangover = false;
            }
        }

        private List<string> _listTitle = new List<string>();

        /// <summary>
        /// Gets or sets the list title.
        /// </summary>
        public List<string> ListTitle
        {
            get { return _listTitle; }
            set { SetAndNotify(ref _listTitle, value); }
        }

        private void InfrastInit()
        {
            /* 基建设置 */
            string[] facility_list = new string[]
            {
                Localization.GetString("Mfg"),
                Localization.GetString("Trade"),
                Localization.GetString("Control"),
                Localization.GetString("Power"),
                Localization.GetString("Reception"),
                Localization.GetString("Office"),
                Localization.GetString("Dorm"),
            };

            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[facility_list.Length]);
            for (int i = 0; i != facility_list.Length; ++i)
            {
                var facility = facility_list[i];
                int order;
                bool parsed = int.TryParse(ViewStatusStorage.Get("Infrast.Order." + facility, "-1"), out order);

                if (!parsed || order < 0)
                {
                    temp_order_list[i] = new DragItemViewModel(facility, "Infrast.");
                }
                else
                {
                    temp_order_list[order] = new DragItemViewModel(facility, "Infrast.");
                }
            }

            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            _facilityKey.Add(Localization.GetString("Dorm"), "Dorm");
            _facilityKey.Add(Localization.GetString("Mfg"), "Mfg");
            _facilityKey.Add(Localization.GetString("Trade"), "Trade");
            _facilityKey.Add(Localization.GetString("Power"), "Power");
            _facilityKey.Add(Localization.GetString("Reception"), "Reception");
            _facilityKey.Add(Localization.GetString("Office"), "Office");
            _facilityKey.Add(Localization.GetString("Control"), "Control");

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
            };

            _dormThresholdLabel = Localization.GetString("DormThreshold") + ": " + _dormThreshold + "%";

            RoguelikeModeList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("RoguelikeStrategyCandle"), Value = "0" },
                new CombData { Display = Localization.GetString("RoguelikeStrategyGold"), Value = "1" },

                // new CombData { Display = "两者兼顾，投资过后退出", Value = "2" } // 弃用
                // new CombData { Display = Localization.GetString("3"), Value = "3" },  // 开发中
            };

            RoguelikeSquadList = new List<CombData>
            {
                new CombData { Display = Localization.GetString("DefaultSquad"), Value = string.Empty },
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
            };

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
            get { return _idle; }
            set { SetAndNotify(ref _idle, value); }
        }

        /* 启动设置 */
        private bool _startSelf = StartSelfModel.CheckStart();

        /// <summary>
        /// Gets or sets a value indicating whether to start itself.
        /// </summary>
        public bool StartSelf
        {
            get
            {
                return _startSelf;
            }

            set
            {
                SetAndNotify(ref _startSelf, value);
                StartSelfModel.SetStart(value);
            }
        }

        private bool _runDirectly = Convert.ToBoolean(ViewStatusStorage.Get("Start.RunDirectly", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to run directly.
        /// </summary>
        public bool RunDirectly
        {
            get
            {
                return _runDirectly;
            }

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
            get
            {
                return _startEmulator;
            }

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

        private string _emulatorPath = ViewStatusStorage.Get("Start.EmulatorPath", string.Empty);

        /// <summary>
        /// Gets or sets the emulator path.
        /// </summary>
        public string EmulatorPath
        {
            get
            {
                return _emulatorPath;
            }

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
            get
            {
                return _emulatorAddCommand;
            }

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
            get
            {
                return _emulatorWaitSeconds;
            }

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
                || !File.Exists(EmulatorPath))
                || !(StartEmulator
                || manual))
            {
                return;
            }

            if (EmulatorAddCommand.Length != 0)
            {
                Process.Start(EmulatorPath, EmulatorAddCommand);
            }
            else
            {
                Process.Start(EmulatorPath);
            }

            int delay;
            if (!int.TryParse(EmulatorWaitSeconds, out delay))
            {
                delay = 60;
            }

            Thread.Sleep(delay * 1000);
        }

        /// <summary>
        /// Selects the emulator to execute.
        /// </summary>
        public void SelectEmulatorExec()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();

            dialog.Filter = Localization.GetString("Executable") + "|*.exe;*.bat;*.lnk";

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
            get
            {
                return _clientType;
            }

            set
            {
                SetAndNotify(ref _clientType, value);
                ViewStatusStorage.Set("Start.ClientType", value);
            }
        }

        /* 基建设置 */
        private readonly Dictionary<string, string> _facilityKey = new Dictionary<string, string>();

        /// <summary>
        /// Gets or sets the infrast item view models.
        /// </summary>
        public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; }

        /// <summary>
        /// Gets or sets the list of uses of drones.
        /// </summary>
        public List<CombData> UsesOfDronesList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike modes.
        /// </summary>
        public List<CombData> RoguelikeModeList { get; set; }

        /// <summary>
        /// Gets or sets the list of roguelike squad.
        /// </summary>
        public List<CombData> RoguelikeSquadList { get; set; }

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
        /// Gets or sets the list of inverse clear modes.
        /// </summary>
        public List<CombData> InverseClearModeList { get; set; }

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
            get
            {
                return _dormThreshold;
            }

            set
            {
                DormThresholdLabel = Localization.GetString("DormThreshold") + ": " + _dormThreshold + "%";
                SetAndNotify(ref _dormThreshold, value);
                ViewStatusStorage.Set("Infrast.DormThreshold", value.ToString());
            }
        }

        private string _dormThresholdLabel;

        /// <summary>
        /// Gets or sets the label of dormitory threshold.
        /// </summary>
        public string DormThresholdLabel
        {
            get { return _dormThresholdLabel; }
            set { SetAndNotify(ref _dormThresholdLabel, value); }
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

                orderList.Add(_facilityKey[item.Name]);
            }

            return orderList;
        }

        /// <summary>
        /// Saves infrast order list.
        /// </summary>
        public void SaveInfrastOrderList()
        {
            for (int i = 0; i < InfrastItemViewModels.Count; i++)
            {
                ViewStatusStorage.Set("Infrast.Order." + InfrastItemViewModels[i].Name, i.ToString());
            }
        }

        private string _usesOfDrones = ViewStatusStorage.Get("Infrast.UsesOfDrones", "Money");

        /// <summary>
        /// Gets or sets the uses of drones.
        /// </summary>
        public string UsesOfDrones
        {
            get
            {
                return _usesOfDrones;
            }

            set
            {
                SetAndNotify(ref _usesOfDrones, value);
                ViewStatusStorage.Set("Infrast.UsesOfDrones", value);
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
            get
            {
                return _selectedIndex;
            }

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
            get
            {
                return _scrollOffset;
            }

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

        private string _roguelikeMode = ViewStatusStorage.Get("Roguelike.Mode", "0");

        /// <summary>
        /// Gets or sets the roguelike mode.
        /// </summary>
        public string RoguelikeMode
        {
            get
            {
                return _roguelikeMode;
            }

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
            get
            {
                return _roguelikeSquad;
            }

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
            get
            {
                return _roguelikeRoles;
            }

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
            get
            {
                return _roguelikeCoreChar;
            }

            set
            {
                SetAndNotify(ref _roguelikeCoreChar, value);
                ViewStatusStorage.Set("Roguelike.CoreChar", value);
            }
        }

        private string _roguelikeStartsCount = ViewStatusStorage.Get("Roguelike.StartsCount", "9999999");

        /// <summary>
        /// Gets or sets the start count of roguelike.
        /// </summary>
        public int RoguelikeStartsCount
        {
            get
            {
                return int.Parse(_roguelikeStartsCount);
            }

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
            get
            {
                return bool.Parse(_roguelikeInvestmentEnabled);
            }

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
            get
            {
                return int.Parse(_roguelikeInvestsCount);
            }

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
            get
            {
                return bool.Parse(_roguelikeStopWhenInvestmentFull);
            }

            set
            {
                SetAndNotify(ref _roguelikeStopWhenInvestmentFull, value.ToString());
                ViewStatusStorage.Set("Roguelike.StopWhenInvestmentFull", value.ToString());
            }
        }

        /* 信用商店设置 */

        private bool _creditShopping = Convert.ToBoolean(ViewStatusStorage.Get("Mall.CreditShopping", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to shop with credit.
        /// </summary>
        public bool CreditShopping
        {
            get
            {
                return _creditShopping;
            }

            set
            {
                SetAndNotify(ref _creditShopping, value);
                ViewStatusStorage.Set("Mall.CreditShopping", value.ToString());
            }
        }

        private string _creditFirstList = ViewStatusStorage.Get("Mall.CreditFirstListNew", "招聘许可；龙门币");

        /// <summary>
        /// Gets or sets the priority item list of credit shop.
        /// </summary>
        public string CreditFirstList
        {
            get
            {
                return _creditFirstList;
            }

            set
            {
                SetAndNotify(ref _creditFirstList, value);
                ViewStatusStorage.Set("Mall.CreditFirstListNew", value);
            }
        }

        private string _creditBlackList = ViewStatusStorage.Get("Mall.CreditBlackListNew", "碳；家具");

        /// <summary>
        /// Gets or sets the blacklist of credit shop.
        /// </summary>
        public string CreditBlackList
        {
            get
            {
                return _creditBlackList;
            }

            set
            {
                SetAndNotify(ref _creditBlackList, value);
                ViewStatusStorage.Set("Mall.CreditBlackListNew", value);
            }
        }

        /* 定时设置 */

        private bool _timer1 = ViewStatusStorage.Get("Timer.Timer1", bool.FalseString) == bool.TrueString;
        private bool _timer2 = ViewStatusStorage.Get("Timer.Timer2", bool.FalseString) == bool.TrueString;
        private bool _timer3 = ViewStatusStorage.Get("Timer.Timer3", bool.FalseString) == bool.TrueString;
        private bool _timer4 = ViewStatusStorage.Get("Timer.Timer4", bool.FalseString) == bool.TrueString;
        private bool _timer5 = ViewStatusStorage.Get("Timer.Timer5", bool.FalseString) == bool.TrueString;
        private bool _timer6 = ViewStatusStorage.Get("Timer.Timer6", bool.FalseString) == bool.TrueString;
        private bool _timer7 = ViewStatusStorage.Get("Timer.Timer7", bool.FalseString) == bool.TrueString;
        private bool _timer8 = ViewStatusStorage.Get("Timer.Timer8", bool.FalseString) == bool.TrueString;

        private int _timer1hour = int.Parse(ViewStatusStorage.Get("Timer.Timer1Hour", "0"));
        private int _timer2hour = int.Parse(ViewStatusStorage.Get("Timer.Timer2Hour", "6"));
        private int _timer3hour = int.Parse(ViewStatusStorage.Get("Timer.Timer3Hour", "12"));
        private int _timer4hour = int.Parse(ViewStatusStorage.Get("Timer.Timer4Hour", "18"));
        private int _timer5hour = int.Parse(ViewStatusStorage.Get("Timer.Timer5Hour", "3"));
        private int _timer6hour = int.Parse(ViewStatusStorage.Get("Timer.Timer6Hour", "9"));
        private int _timer7hour = int.Parse(ViewStatusStorage.Get("Timer.Timer7Hour", "15"));
        private int _timer8hour = int.Parse(ViewStatusStorage.Get("Timer.Timer8Hour", "21"));

        /// <summary>
        /// Gets or sets a value indicating whether the 1st timer is set.
        /// </summary>
        public bool Timer1
        {
            get
            {
                return _timer1;
            }

            set
            {
                SetAndNotify(ref _timer1, value);
                ViewStatusStorage.Set("Timer.Timer1", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 2nd timer is set.
        /// </summary>
        public bool Timer2
        {
            get
            {
                return _timer2;
            }

            set
            {
                SetAndNotify(ref _timer2, value);
                ViewStatusStorage.Set("Timer.Timer2", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 3rd timer is set.
        /// </summary>
        public bool Timer3
        {
            get
            {
                return _timer3;
            }

            set
            {
                SetAndNotify(ref _timer3, value);
                ViewStatusStorage.Set("Timer.Timer3", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 4th timer is set.
        /// </summary>
        public bool Timer4
        {
            get
            {
                return _timer4;
            }

            set
            {
                SetAndNotify(ref _timer4, value);
                ViewStatusStorage.Set("Timer.Timer4", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 5th timer is set.
        /// </summary>
        public bool Timer5
        {
            get
            {
                return _timer5;
            }

            set
            {
                SetAndNotify(ref _timer5, value);
                ViewStatusStorage.Set("Timer.Timer5", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 6th timer is set.
        /// </summary>
        public bool Timer6
        {
            get
            {
                return _timer6;
            }

            set
            {
                SetAndNotify(ref _timer6, value);
                ViewStatusStorage.Set("Timer.Timer6", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 7th timer is set.
        /// </summary>
        public bool Timer7
        {
            get
            {
                return _timer7;
            }

            set
            {
                SetAndNotify(ref _timer7, value);
                ViewStatusStorage.Set("Timer.Timer7", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 8th timer is set.
        /// </summary>
        public bool Timer8
        {
            get
            {
                return _timer8;
            }

            set
            {
                SetAndNotify(ref _timer8, value);
                ViewStatusStorage.Set("Timer.Timer8", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 1st timer.
        /// </summary>
        public int Timer1Hour
        {
            get
            {
                return _timer1hour;
            }

            set
            {
                SetAndNotify(ref _timer1hour, value);
                ViewStatusStorage.Set("Timer.Timer1Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 2nd timer.
        /// </summary>
        public int Timer2Hour
        {
            get
            {
                return _timer2hour;
            }

            set
            {
                SetAndNotify(ref _timer2hour, value);
                ViewStatusStorage.Set("Timer.Timer2Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 3rd timer.
        /// </summary>
        public int Timer3Hour
        {
            get
            {
                return _timer3hour;
            }

            set
            {
                SetAndNotify(ref _timer3hour, value);
                ViewStatusStorage.Set("Timer.Timer3Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 4th timer.
        /// </summary>
        public int Timer4Hour
        {
            get
            {
                return _timer4hour;
            }

            set
            {
                SetAndNotify(ref _timer4hour, value);
                ViewStatusStorage.Set("Timer.Timer4Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 5th timer.
        /// </summary>
        public int Timer5Hour
        {
            get
            {
                return _timer5hour;
            }

            set
            {
                SetAndNotify(ref _timer5hour, value);
                ViewStatusStorage.Set("Timer.Timer5Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 6th timer.
        /// </summary>
        public int Timer6Hour
        {
            get
            {
                return _timer6hour;
            }

            set
            {
                SetAndNotify(ref _timer6hour, value);
                ViewStatusStorage.Set("Timer.Timer6Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 7th timer.
        /// </summary>
        public int Timer7Hour
        {
            get
            {
                return _timer7hour;
            }

            set
            {
                SetAndNotify(ref _timer7hour, value);
                ViewStatusStorage.Set("Timer.Timer7Hour", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the hour of the 8th timer.
        /// </summary>
        public int Timer8Hour
        {
            get
            {
                return _timer8hour;
            }

            set
            {
                SetAndNotify(ref _timer8hour, value);
                ViewStatusStorage.Set("Timer.Timer8Hour", value.ToString());
            }
        }

        /* 企鹅物流设置 */

        private string _penguinId = ViewStatusStorage.Get("Penguin.Id", string.Empty);

        /// <summary>
        /// Gets or sets the id of PenguinStats.
        /// </summary>
        public string PenguinId
        {
            get
            {
                return _penguinId;
            }

            set
            {
                SetAndNotify(ref _penguinId, value);
                ViewStatusStorage.Set("Penguin.Id", value);
            }
        }

        /* 自动公招设置 */
        private string _recruitMaxTimes = ViewStatusStorage.Get("AutoRecruit.MaxTimes", "4");

        /// <summary>
        /// Gets or sets the maximum times of recruit.
        /// </summary>
        public string RecruitMaxTimes
        {
            get
            {
                return _recruitMaxTimes;
            }

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
            get
            {
                return _refreshLevel3;
            }

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
            get { return _useExpedited; }
            set { SetAndNotify(ref _useExpedited, value); }
        }

        private bool _notChooseLevel1 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.NotChooseLevel1", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether not to choose level 1.
        /// </summary>
        public bool NotChooseLevel1
        {
            get
            {
                return _notChooseLevel1;
            }

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
            get
            {
                return _chooseLevel3;
            }

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
            get
            {
                return _chooseLevel4;
            }

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
            get
            {
                return _chooseLevel5;
            }

            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel5", value.ToString());
            }
        }

        /* 软件更新设置 */
        private bool _updateBeta = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UpdateBeta", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to update beta version.
        /// </summary>
        public bool UpdateBeta
        {
            get
            {
                return _updateBeta;
            }

            set
            {
                SetAndNotify(ref _updateBeta, value);
                ViewStatusStorage.Set("VersionUpdate.UpdateBeta", value.ToString());
            }
        }

        private bool _updateCheck = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UpdateCheck", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to check update.
        /// </summary>
        public bool UpdateCheck
        {
            get
            {
                return _updateCheck;
            }

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
            get
            {
                return _proxy;
            }

            set
            {
                SetAndNotify(ref _proxy, value);
                ViewStatusStorage.Set("VersionUpdate.Proxy", value);
            }
        }

        private bool _useAria2 = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UseAria2", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to use Aria 2.
        /// </summary>
        public bool UseAria2
        {
            get
            {
                return _useAria2;
            }

            set
            {
                SetAndNotify(ref _useAria2, value);
                ViewStatusStorage.Set("VersionUpdate.UseAria2", value.ToString());
            }
        }

        private bool _autoDownloadUpdatePackage = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.AutoDownloadUpdatePackage", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to auto download update package.
        /// </summary>
        public bool AutoDownloadUpdatePackage
        {
            get
            {
                return _autoDownloadUpdatePackage;
            }

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
            var updateModel = _container.Get<VersionUpdateViewModel>();
            var task = Task.Run(() =>
            {
                if (!updateModel.CheckAndDownloadUpdate(true))
                {
                    Execute.OnUIThread(() =>
                    {
                        using (var toast = new ToastNotification(Localization.GetString("AlreadyLatest")))
                        {
                            toast.Show();
                        }
                    });
                }
            });
            await task;
        }

        /* 连接设置 */

        private string _connectAddress = ViewStatusStorage.Get("Connect.Address", string.Empty);

        /// <summary>
        /// Gets or sets the connection address.
        /// </summary>
        public string ConnectAddress
        {
            get
            {
                return _connectAddress;
            }

            set
            {
                SetAndNotify(ref _connectAddress, value);
                ViewStatusStorage.Set("Connect.Address", value);
                UpdateWindowTitle(); /* 每次修改连接地址时更新WindowTitle */
            }
        }

        private string _adbPath = ViewStatusStorage.Get("Connect.AdbPath", string.Empty);

        /// <summary>
        /// Gets or sets the ADB path.
        /// </summary>
        public string AdbPath
        {
            get
            {
                return _adbPath;
            }

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
            get
            {
                return _connectConfig;
            }

            set
            {
                SetAndNotify(ref _connectConfig, value);
                ViewStatusStorage.Set("Connect.ConnectConfig", value);
            }
        }

        private readonly Dictionary<string, List<string>> _defaultAddress = new Dictionary<string, List<string>>
        {
            { "General", new List<string> { string.Empty } },
            { "BlueStacks", new List<string> { "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5554" } },
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
        public bool RefreshAdbConfig(ref string error)
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
            var dialog = new Microsoft.Win32.OpenFileDialog();

            dialog.Filter = Localization.GetString("ADBProgram") + "|*.exe";

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
            string connectConfigName = string.Empty;
            foreach (CombData data in ConnectConfigList)
            {
                if (data.Value == ConnectConfig)
                {
                    connectConfigName = data.Display;
                }
            }

            rvm.WindowTitle = string.Format("MaaAssistantArknights - {0} ({1})", connectConfigName, ConnectAddress);
        }

        private readonly string _bluestacksConfig = ViewStatusStorage.Get("Bluestacks.Config.Path", string.Empty);

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
                ViewStatusStorage.Set("Bluestacks.Config.Error", "File not exists");
                return;
            }

            var all_lines = File.ReadAllLines(_bluestacksConfig);
            foreach (var line in all_lines)
            {
                if (line.StartsWith("bst.instance.Nougat64.status.adb_port"))
                {
                    var sp = line.Split('"');
                    ConnectAddress = "127.0.0.1:" + sp[1];
                }
            }
        }

        /* 界面设置 */
#pragma warning disable SA1401 // Fields should be private

        /// <summary>
        /// Gets or sets a value indicating whether to use tray icon.
        /// </summary>
        public bool UseTray = true;

#pragma warning restore SA1401 // Fields should be private

        // private bool _useTray = Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseTray", bool.TrueString));

        // public bool UseTray
        // {
        //    get { return _useTray; }
        //    set
        //    {
        //        SetAndNotify(ref _useTray, value);
        //        ViewStatusStorage.Set("GUI.UseTray", value.ToString());
        //        var trayObj = _container.Get<TrayIcon>();
        //        trayObj.SetVisible(value);

        // if (!Convert.ToBoolean(value))
        //        {
        //            MinimizeToTray = false;
        //        }
        //    }
        // }
        private bool _minimizeToTray = Convert.ToBoolean(ViewStatusStorage.Get("GUI.MinimizeToTray", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to minimize to tray.
        /// </summary>
        public bool MinimizeToTray
        {
            get
            {
                return _minimizeToTray;
            }

            set
            {
                SetAndNotify(ref _minimizeToTray, value);
                ViewStatusStorage.Set("GUI.MinimizeToTray", value.ToString());
                var trayObj = _container.Get<TrayIcon>();
                trayObj.SetMinimizeToTaskbar(value);
            }
        }

        private bool _useNotify = Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseNotify", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to use notification.
        /// </summary>
        public bool UseNotify
        {
            get
            {
                return _useNotify;
            }

            set
            {
                SetAndNotify(ref _useNotify, value);
                ViewStatusStorage.Set("GUI.UseNotify", value.ToString());
                if (value)
                {
                    Execute.OnUIThread(() =>
                    {
                        using (var toast = new ToastNotification("Test test"))
                        {
                            toast.Show();
                        }
                    });
                }
            }
        }

        private bool _hideUnavailableStage = Convert.ToBoolean(ViewStatusStorage.Get("GUI.HideUnavailableStage", bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide unavailable stages.
        /// </summary>
        public bool HideUnavailableStage
        {
            get
            {
                return _hideUnavailableStage;
            }

            set
            {
                SetAndNotify(ref _hideUnavailableStage, value);
                ViewStatusStorage.Set("GUI.HideUnavailableStage", value.ToString());
                var mainModel = _container.Get<TaskQueueViewModel>();
                mainModel.UpdateStageList();
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
            get
            {
                return _inverseClearMode.ToString();
            }

            set
            {
                bool parsed = Enum.TryParse(value, out InverseClearType tempEnumValue);
                if (!parsed)
                {
                    return;
                }

                SetAndNotify(ref _inverseClearMode, tempEnumValue);
                ViewStatusStorage.Set("GUI.InverseClearMode", value);
                var taskQueueModel = _container.Get<TaskQueueViewModel>();
                switch (tempEnumValue)
                {
                    case InverseClearType.Clear:
                        taskQueueModel.InverseMode = false;
                        taskQueueModel.InverseShowVisibility = Visibility.Collapsed;
                        taskQueueModel.NotInverseShowVisibility = Visibility.Visible;
                        taskQueueModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.Inverse:
                        taskQueueModel.InverseMode = true;
                        taskQueueModel.InverseShowVisibility = Visibility.Collapsed;
                        taskQueueModel.NotInverseShowVisibility = Visibility.Visible;
                        taskQueueModel.SelectedAllWidth = 90;
                        break;

                    case InverseClearType.ClearInverse:
                        taskQueueModel.InverseShowVisibility = Visibility.Visible;
                        taskQueueModel.NotInverseShowVisibility = Visibility.Collapsed;
                        taskQueueModel.SelectedAllWidth = TaskQueueViewModel.SelectedAllWidthWhenBoth;
                        break;
                }
            }
        }

        private string _language = ViewStatusStorage.Get("GUI.Localization", Localization.DefaultLanguage);

        /// <summary>
        /// Gets or sets the language.
        /// </summary>
        public string Language
        {
            get
            {
                return _language;
            }

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

                var backup = _language;
                ViewStatusStorage.Set("GUI.Localization", value);
                System.Windows.Forms.MessageBoxManager.Yes = Localization.GetString("Ok", value);
                System.Windows.Forms.MessageBoxManager.No = Localization.GetString("ManualRestart", value);
                System.Windows.Forms.MessageBoxManager.Register();
                var result = MessageBox.Show(
                    Localization.GetString("LanguageChangedTip", value),
                    Localization.GetString("Tip", value),
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
            get
            {
                return _cheers;
            }

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
                    setPallasLanguage();
                }
            }
        }

        private bool _hangover = bool.Parse(ViewStatusStorage.Get("GUI.Hangover", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hangover.
        /// </summary>
        public bool Hangover
        {
            get
            {
                return _hangover;
            }

            set
            {
                SetAndNotify(ref _hangover, value);
                ViewStatusStorage.Set("GUI.Hangover", value.ToString());
            }
        }

        private void setPallasLanguage()
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
    }
}
