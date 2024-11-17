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
using System.Linq;
using System.Management;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Configuration;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using Windows.Globalization;
using ComboBox = System.Windows.Controls.ComboBox;
using DarkModeType = MaaWpfGui.Configuration.GUI.DarkModeType;
using Timer = System.Timers.Timer;

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
        /// Gets the visibility of task setting views.
        /// </summary>
        public TaskSettingVisibilityInfo TaskSettingVisibilities { get; } = TaskSettingVisibilityInfo.Current;

        /// <summary>
        /// Gets the after action setting.
        /// </summary>
        public PostActionSetting PostActionSetting { get; } = PostActionSetting.Current;

        #region 长草任务Model

        /// <summary>
        /// Gets 生稀盐酸任务Model
        /// </summary>
        public static ReclamationSettingsUserControlModel ReclamationTask { get; } = new();

        #endregion 长草任务Model

        #region 设置界面Model

        /// <summary>
        /// Gets 界面设置model
        /// </summary>
        public static GUISettingsUserControlModel GuiSettings { get; } = new();

        /// <summary>
        /// Gets 连接设置model
        /// </summary>
        public static ConnectSettingsUserControlModel ConnectSettings { get; } = new();

        /// <summary>
        /// Gets 启动设置Model
        /// </summary>
        public static StartSettingsUserControlModel StartSettings { get; } = new();

        /// <summary>
        /// Gets 软件更新model
        /// </summary>
        public static VersionUpdateSettingsUserControlModel VersionUpdateDataContext { get; } = new();

        /// <summary>
        /// Gets 外部通知model
        /// </summary>
        public static ExternalNotificationSettingsUserControlModel ExternalNotificationDataContext { get; } = new();

        #endregion 设置界面Model

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

        private void InitUiSettings()
        {
            var languageList = (from pair in LocalizationHelper.SupportedLanguages
                                where pair.Key != PallasLangKey || Cheers
                                select new CombinedData { Display = pair.Value, Value = pair.Key })
               .ToList();

            GuiSettings.LanguageList = languageList;
            GuiSettings.SwitchDarkMode();
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

        private void InitConnectConfig()
        {
            var addressListJson = ConfigurationHelper.GetValue(ConfigurationKeys.AddressHistory, string.Empty);
            if (!string.IsNullOrEmpty(addressListJson))
            {
                ConnectSettings.ConnectAddressHistory = JsonConvert.DeserializeObject<ObservableCollection<string>>(addressListJson) ?? [];
            }
        }

        private void InitVersionUpdate()
        {
            if (VersionUpdateDataContext.VersionType == VersionUpdateSettingsUserControlModel.UpdateVersionType.Nightly && !VersionUpdateDataContext.AllowNightlyUpdates)
            {
                VersionUpdateDataContext.VersionType = VersionUpdateSettingsUserControlModel.UpdateVersionType.Beta;
            }
        }

        #endregion Init

        #region EasterEggs

        /// <summary>
        /// The Pallas language key.
        /// </summary>
        public const string PallasLangKey = "pallas";

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
            if (!Cheers || GuiSettings.Language != PallasLangKey)
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
            return wineList.Any(Instances.SettingsViewModel.CreditFirstList.Contains);
        }

        #endregion EasterEggs

        #region Remote Control

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

        private string _screencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), "---", "---", "---", "---");

        public string ScreencapCost
        {
            get => _screencapCost;
            set => SetAndNotify(ref _screencapCost, value);
        }

        #endregion Performance

        #region 游戏设置

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

        #endregion

        #region 开始唤醒

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
                VersionUpdateDataContext.ResourceInfo = VersionUpdateSettingsUserControlModel.GetResourceVersionByClientType(_clientType);
                VersionUpdateDataContext.ResourceVersion = VersionUpdateDataContext.ResourceInfo.VersionName;
                VersionUpdateDataContext.ResourceDateTime = VersionUpdateDataContext.ResourceInfo.DateTime;
                Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
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
                foreach (var item in Instances.SettingsViewModel.ClientTypeList.Where(item => item.Value == ClientType))
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

        /// <summary>
        /// Gets the server type.
        /// </summary>
        public string ServerType => _serverMapping[ClientType];

        #endregion 开始唤醒

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

                        var localizedName = DataHelper.GetLocalizedCharacterName(name, GuiSettings.OperNameLocalization);
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
        public static void AskRestartToApplySettings(bool isYostarEN = false)
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

            List<string> windowTitleSelectShowList = GuiSettings.WindowTitleSelectShowList
                .Where(x => GUISettingsUserControlModel.WindowTitleAllShowDict.ContainsKey(x?.ToString() ?? string.Empty))
                .Select(x => GUISettingsUserControlModel.WindowTitleAllShowDict[x?.ToString() ?? string.Empty]).ToList();

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
                        foreach (var data in ConnectSettings.ConnectConfigList.Where(data => data.Value == ConnectSettings.ConnectConfig))
                        {
                            connectConfigName = $" - {data.Display}";
                        }

                        break;

                    case "3": // 端口地址
                        connectAddress = $" ({ConnectSettings.ConnectAddress})";
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
            rvm.WindowTitle = $"{prefix}MAA{currentConfiguration} - {VersionUpdateSettingsUserControlModel.CoreVersion}{resourceVersion}{connectConfigName}{connectAddress}{clientName}";
        }
    }
}
