// <copyright file="SettingsViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Globalization;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using HandyControl.Controls;
using HandyControl.Data;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json;
using Serilog;
using Stylet;
using ComboBox = System.Windows.Controls.ComboBox;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The view model of settings.
/// </summary>
public class SettingsViewModel : Screen
{
    private readonly RunningState _runningState;

    private static readonly ILogger _logger = Log.ForContext<SettingsViewModel>();

    /// <summary>
    /// Gets the visibility of task setting views.
    /// </summary>
    public TaskSettingVisibilityInfo TaskSettingVisibilities { get; } = TaskSettingVisibilityInfo.Instance;

    #region 设置界面 Model

    /// <summary>
    /// Gets 游戏设置 model
    /// </summary>
    public static GameSettingsUserControlModel GameSettings { get; } = GameSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 连接设置 model
    /// </summary>
    public static ConnectSettingsUserControlModel ConnectSettings { get; } = ConnectSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 启动设置 model
    /// </summary>
    public static StartSettingsUserControlModel StartSettings { get; } = StartSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 界面设置 model
    /// </summary>
    public static GuiSettingsUserControlModel GuiSettings { get; } = GuiSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 背景设置 model
    /// </summary>
    public static BackgroundSettingsUserControlModel BackgroundSettings { get; } = BackgroundSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 定时设置 model
    /// </summary>
    public static TimerSettingsUserControlModel TimerSettings { get; } = TimerSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 远程控制 model
    /// </summary>
    public static RemoteControlUserControlModel RemoteControlSettings { get; } = RemoteControlUserControlModel.Instance;

    /// <summary>
    /// Gets 软件更新 model
    /// </summary>
    public static VersionUpdateSettingsUserControlModel VersionUpdateSettings { get; } = VersionUpdateSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 外部通知 model
    /// </summary>
    public static ExternalNotificationSettingsUserControlModel ExternalNotificationSettings { get; } = ExternalNotificationSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 性能设置 model
    /// </summary>
    public static PerformanceUserControlModel PerformanceSettings { get; } = PerformanceUserControlModel.Instance;

    /// <summary>
    /// Gets 问题反馈 model
    /// </summary>
    public static IssueReportUserControlModel IssueReportSettings { get; } = IssueReportUserControlModel.Instance;

    /// <summary>
    /// Gets 成就 model
    /// </summary>
    public static AchievementSettingsUserControlModel AchievementSettings { get; } = AchievementSettingsUserControlModel.Instance;

    #endregion 设置界面 Model

    /// <summary>
    /// Initializes a new instance of the <see cref="SettingsViewModel"/> class.
    /// </summary>
    public SettingsViewModel()
    {
        DisplayName = LocalizationHelper.GetString("Settings");

        Init();

        HangoverEnd();

        _runningState = RunningState.Instance;
        _runningState.StateChanged += (_, e) => {
            Idle = e.Idle;

            // Inited = e.Inited;
            // Stopping = e.Stopping;
        };
    }

    #region Init

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
        InitSettings();
        TaskQueueViewModel.RoguelikeTask.InitRoguelike();
        InitConfiguration();
        InitUiSettings();
        InitConnectConfig();
        InitVersionUpdate();
    }

    public SettingItemViewModel GetSettingItemByKey(string key)
    {
        return Settings.First(s => s.Key == key);
    }

    public SettingItemViewModel SwitchConfigurationSetting => GetSettingItemByKey("SwitchConfiguration");

    public SettingItemViewModel ScheduleSettingsSetting => GetSettingItemByKey("ScheduleSettings");

    public SettingItemViewModel PerformanceSettingsSetting => GetSettingItemByKey("PerformanceSettings");

    public SettingItemViewModel GameSettingsSetting => GetSettingItemByKey("GameSettings");

    public SettingItemViewModel ConnectionSettingsSetting => GetSettingItemByKey("ConnectionSettings");

    public SettingItemViewModel StartupSettingsSetting => GetSettingItemByKey("StartupSettings");

    public SettingItemViewModel RemoteControlSettingsSetting => GetSettingItemByKey("RemoteControlSettings");

    public SettingItemViewModel UiSettingsSetting => GetSettingItemByKey("UiSettings");

    public SettingItemViewModel BackgroundSettingsSetting => GetSettingItemByKey("BackgroundSettings");

    public SettingItemViewModel ExternalNotificationSettingsSetting => GetSettingItemByKey("ExternalNotificationSettings");

    public SettingItemViewModel HotKeySettingsSetting => GetSettingItemByKey("HotKeySettings");

    public SettingItemViewModel AchievementSettingsSetting => GetSettingItemByKey("AchievementSettings");

    public SettingItemViewModel UpdateSettingsSetting => GetSettingItemByKey("UpdateSettings");

    public SettingItemViewModel IssueReportSetting => GetSettingItemByKey("IssueReport");

    public SettingItemViewModel AboutUsSetting => GetSettingItemByKey("AboutUs");

    private void InitSettings()
    {
        List<string> keyList =
        [
            "SwitchConfiguration",
            "ScheduleSettings",
            "PerformanceSettings",
            "GameSettings",
            "ConnectionSettings",
            "StartupSettings",
            "RemoteControlSettings",
            "UiSettings",
            "BackgroundSettings",
            "ExternalNotificationSettings",
            "HotKeySettings",
            "AchievementSettings",
            "UpdateSettings",
            "IssueReport",
            "AboutUs",
        ];

        var tempOrderList = new List<SettingItemViewModel?>(new SettingItemViewModel[keyList.Count]);
        var nonOrderList = new List<SettingItemViewModel?>();

        foreach (var key in keyList)
        {
            int order = ConfigurationHelper.GetSettingOrder(key, -1);

            var item = new SettingItemViewModel(key, LocalizationHelper.GetString(key), -1);

            if (order < 0 || order >= tempOrderList.Count || tempOrderList[order] != null)
            {
                nonOrderList.Add(item);
            }
            else
            {
                item.Value = order;
                tempOrderList[order] = item;
            }
        }

        int fillIndex = 0;
        foreach (var item in nonOrderList.OfType<SettingItemViewModel>())
        {
            while (fillIndex < tempOrderList.Count && tempOrderList[fillIndex] != null)
            {
                fillIndex++;
            }

            if (fillIndex < tempOrderList.Count)
            {
                item.Value = fillIndex;
                tempOrderList[fillIndex] = item;
                ConfigurationHelper.SetSettingOrder(item.Key, fillIndex);
            }
        }

        Settings = [.. tempOrderList.OfType<SettingItemViewModel>()];

        Settings.CollectionChanged += Settings_CollectionChanged;
    }

    private void Settings_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs? e)
    {
        Execute.OnUIThread(() => {
            for (int i = 0; i < Settings.Count; i++)
            {
                var item = Settings[i];
                if (item.Value == i)
                {
                    continue;
                }

                item.Value = i;
                ConfigurationHelper.SetSettingOrder(item.Key, i);
            }

            OnSettingItemValueChanged();
        });
    }

    private void OnSettingItemValueChanged()
    {
        Application.Current.Dispatcher.InvokeAsync(() => {
            RefreshDividerOffsetsRequested?.Invoke(this, EventArgs.Empty);
        }, System.Windows.Threading.DispatcherPriority.Loaded);
    }

    public event EventHandler? RefreshDividerOffsetsRequested;

    private ObservableCollection<SettingItemViewModel> _settings = [];

    public ObservableCollection<SettingItemViewModel> Settings
    {
        get => _settings;
        set => SetAndNotify(ref _settings, value);
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

        GuiSettings.LanguageList = languageList;
        GuiSettings.SwitchDarkMode();
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
        if (VersionUpdateSettings is { VersionType: VersionUpdateSettingsUserControlModel.UpdateVersionType.Nightly, AllowNightlyUpdates: false })
        {
            VersionUpdateSettings.VersionType = VersionUpdateSettingsUserControlModel.UpdateVersionType.Beta;
        }
    }

    #endregion Init

    #region EasterEggs

    /// <summary>
    /// The Pallas language key.
    /// </summary>
    public const string PallasLangKey = "pallas";

    private bool _cheers = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Cheers, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to cheer.
    /// </summary>
    public bool Cheers
    {
        get => _cheers;
        set {
            if (_cheers == value)
            {
                return;
            }

            SetAndNotify(ref _cheers, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Cheers, value.ToString());
            if (_cheers)
            {
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Localization, PallasLangKey);
            }
        }
    }

    private bool _hangover = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Hangover, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to hangover.
    /// </summary>
    public bool Hangover
    {
        get => _hangover;
        set {
            SetAndNotify(ref _hangover, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Hangover, value.ToString());
        }
    }

    private string _lastBuyWineTime = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LastBuyWineTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

    public string LastBuyWineTime
    {
        get => _lastBuyWineTime;
        set {
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

        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.Localization, SoberLanguage);
        Hangover = true;
        Cheers = false;
    }

    private string _soberLanguage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.SoberLanguage, LocalizationHelper.DefaultLanguage);

    public string SoberLanguage
    {
        get => _soberLanguage;
        set {
            SetAndNotify(ref _soberLanguage, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.SoberLanguage, value);
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

        // if (now.IsAprilFoolsDay())
        // {
        //     return true;
        // }
        string[] wineList = ["酒", "liquor", "drink", "wine", "beer", "술", "🍷", "🍸", "🍺", "🍻", "🥃", "🍶"];
        foreach (var task in ConfigFactory.CurrentConfig.TaskQueue.OfType<MallTask>())
        {
            if (wineList.Any(task.FirstList.Contains))
            {
                return true;
            }
        }

        return false;
    }

    #endregion EasterEggs

    #region HotKey

    /// <summary>
    /// Gets or sets the hotkey: ShowGui.
    /// </summary>
    public static MaaHotKey? HotKeyShowGui
    {
        get => Instances.MaaHotKeyManager.GetOrNull(MaaHotKeyAction.ShowGui);
        set => SetHotKey(MaaHotKeyAction.ShowGui, value);
    }

    /// <summary>
    /// Gets or sets the hotkey: LinkStart.
    /// </summary>
    public static MaaHotKey? HotKeyLinkStart
    {
        get => Instances.MaaHotKeyManager.GetOrNull(MaaHotKeyAction.LinkStart);
        set => SetHotKey(MaaHotKeyAction.LinkStart, value);
    }

    private static void SetHotKey(MaaHotKeyAction action, MaaHotKey? value)
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

    public ObservableCollection<CombinedData> ConfigurationList { get; set; } = [];

    private string _currentConfiguration = ConfigurationHelper.GetCurrentConfiguration();

    public string CurrentConfiguration
    {
        get => _currentConfiguration;
        set {
            bool ret = ConfigurationHelper.SwitchConfiguration(value);
            ret &= ConfigFactory.SwitchConfig(value);

            if (!ret)
            {
                ConfigurationHelper.SwitchConfiguration(_currentConfiguration);
                ConfigFactory.SwitchConfig(_currentConfiguration);
                return;
            }
            SetAndNotify(ref _currentConfiguration, value);
            Bootstrapper.ShutdownAndRestartWithoutArgs();
        }
    }

    private string _newConfigurationName = string.Empty;

    public string NewConfigurationName
    {
        get => _newConfigurationName;
        set => SetAndNotify(ref _newConfigurationName, value);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void AddConfiguration()
    {
        if (string.IsNullOrEmpty(NewConfigurationName))
        {
            NewConfigurationName = DateTime.Now.ToString("yy/MM/dd HH:mm:ss");
        }

        if (ConfigurationHelper.AddConfiguration(NewConfigurationName, CurrentConfiguration) && ConfigFactory.AddConfiguration(NewConfigurationName, CurrentConfiguration))
        {
            ConfigurationList.Add(new CombinedData { Display = NewConfigurationName, Value = NewConfigurationName });

            // 配置数量大于 1 时，标题栏显示配置名
            UpdateWindowTitle();

            var growlInfo = new GrowlInfo {
                IsCustom = true,
                Message = string.Format(LocalizationHelper.GetString("AddConfigSuccess"), NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            };
            Growl.Info(growlInfo);
        }
        else
        {
            ConfigurationHelper.DeleteConfiguration(NewConfigurationName);
            ConfigFactory.DeleteConfiguration(NewConfigurationName);
            var growlInfo = new GrowlInfo {
                IsCustom = true,
                Message = string.Format(LocalizationHelper.GetString("ConfigExists"), NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            };
            Growl.Info(growlInfo);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DeleteConfiguration(CombinedData delete)
    {
        if (ConfigurationHelper.DeleteConfiguration(delete.Display) && ConfigFactory.DeleteConfiguration(delete.Display))
        {
            ConfigurationList.Remove(delete);
            if (ConfigurationList.Count <= 1)
            {
                UpdateWindowTitle();
            }
        }
    }

    #endregion 配置

    #region SettingsGuide

    public static int GuideMaxStep => 7;

    private int _guideStepIndex = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, "0"));

    public int GuideStepIndex
    {
        get => _guideStepIndex;
        set {
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
    [UsedImplicitly]
    public void NextGuide(StepBar stepBar)
    {
        GuideTransitionMode = "Bottom2Top";
        stepBar.Next();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void PrevGuide(StepBar stepBar)
    {
        GuideTransitionMode = "Top2Bottom";
        stepBar.Prev();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DoneGuide()
    {
        TaskSettingVisibilities.Guide = false;
        GuideStepIndex = GuideMaxStep;
    }

    #endregion SettingsGuide

    #region 设置页面列表和滚动视图联动绑定

    private enum NotifyType
    {
        None,
        SelectedIndex,
        ScrollOffset,
    }

    private NotifyType _notifySource = NotifyType.None;

    private Timer? _resetNotifyTimer;

    private void ResetNotifySource()
    {
        if (_resetNotifyTimer != null)
        {
            _resetNotifyTimer.Stop();
            _resetNotifyTimer.Close();
        }

        _resetNotifyTimer = new Timer(20);
        _resetNotifyTimer.Elapsed += (_, _) => {
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

    private List<double> _dividerVerticalOffsetList = [];

    /// <summary>
    /// Gets or sets the list of divider vertical offset.
    /// </summary>
    public List<double> DividerVerticalOffsetList
    {
        get => _dividerVerticalOffsetList;
        set {
            if (_dividerVerticalOffsetList == value)
            {
                return;
            }

            _dividerVerticalOffsetList = value;
            SetAndNotify(ref _dividerVerticalOffsetList, value);
        }
    }

    private int _selectedIndex;

    /// <summary>
    /// Gets or sets the index selected.
    /// </summary>
    public int SelectedIndex
    {
        get => _selectedIndex;
        set {
            if (_selectedIndex == value)
            {
                return;
            }

            if (value < 0 || value > DividerVerticalOffsetList.Count)
            {
                return;
            }

            switch (_notifySource)
            {
                case NotifyType.None:
                    _notifySource = NotifyType.SelectedIndex;
                    SetAndNotify(ref _selectedIndex, value);

                    ScrollOffset = DividerVerticalOffsetList[value];

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
        set {
            if (!AllowScrollOffsetChange)
            {
                return;
            }

            switch (_notifySource)
            {
                case NotifyType.None:
                    _notifySource = NotifyType.ScrollOffset;
                    SetAndNotify(ref _scrollOffset, value);

                    // 设置 ListBox SelectedIndex 为当前 ScrollOffset 索引
                    if (DividerVerticalOffsetList.Count > 0)
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

    public bool AllowScrollOffsetChange { get; set; } = true;

    #endregion 设置页面列表和滚动视图联动绑定

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
    /// UI 绑定的方法
    /// </summary>
    /// <param name="sender">Event sender</param>
    /// <param name="e">Event args</param>
    [UsedImplicitly]
    public static void MakeComboBoxSearchable(object sender, EventArgs e)
    {
        (sender as ComboBox)?.MakeComboBoxSearchable();
    }

    private bool _isCheckingAnnouncement = false;

    public bool IsCheckingAnnouncement
    {
        get => _isCheckingAnnouncement;
        set {
            SetAndNotify(ref _isCheckingAnnouncement, value);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task CheckAndDownloadAnnouncement()
    {
        if (IsCheckingAnnouncement)
        {
            return;
        }

        IsCheckingAnnouncement = true;

        try
        {
            if (Instances.AnnouncementDialogViewModel.View is System.Windows.Window window)
            {
                if (window.WindowState == WindowState.Minimized)
                {
                    window.WindowState = WindowState.Normal;
                }

                window.Activate();
            }
            else
            {
                Instances.WindowManager.ShowWindow(Instances.AnnouncementDialogViewModel);
            }

            await Instances.AnnouncementDialogViewModel.CheckAndDownloadAnnouncement();
        }
        finally
        {
            IsCheckingAnnouncement = false;
        }
    }

    /// <summary>
    /// 标题栏显示模拟器名称和IP端口。
    /// </summary>
    public void UpdateWindowTitle()
    {
        var rvm = (RootViewModel)this.Parent;

        var newVersionFoundInfo = VersionUpdateSettings.NewVersionFoundInfo;
        var uiVersion = VersionUpdateSettingsUserControlModel.UiVersion;
        var startupUpdateCheck = VersionUpdateSettings.StartupUpdateCheck;
        var isDebug = Instances.VersionUpdateDialogViewModel.IsDebugVersion();

        if (newVersionFoundInfo != uiVersion && !isDebug && !string.IsNullOrEmpty(newVersionFoundInfo) && startupUpdateCheck)
        {
            rvm.WindowVersionUpdateInfo = $"{newVersionFoundInfo}".Trim();
        }

        rvm.WindowResourceUpdateInfo = VersionUpdateSettings.NewResourceFoundInfo;

        string prefix = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitlePrefix, string.Empty);
        if (!string.IsNullOrEmpty(prefix))
        {
            prefix += " - ";
        }

        List<string> windowTitleSelectShowList = [.. GuiSettings.WindowTitleSelectShowList
            .Cast<KeyValuePair<string, string>>().Select(pair => pair.Key)];

        string currentConfiguration = string.Empty;
        string connectConfigName = string.Empty;
        string connectAddress = string.Empty;
        string clientName = string.Empty;

        foreach (var select in windowTitleSelectShowList)
        {
            switch (select)
            {
                case "1": // 配置名
                    if (ConfigurationList.Count > 1)
                    {
                        currentConfiguration = $" ({CurrentConfiguration})";
                    }

                    break;

                case "2": // 连接模式
                    foreach (var data in ConnectSettings.ConnectConfigList.Where(data => data.Value == ConnectSettings.ConnectConfig))
                    {
                        connectConfigName = $" - {data.Display}";
                    }

                    break;

                case "3": // 端口地址
                    connectAddress = $" ({ConnectSettings.ConnectAddress})".Replace("127.0.0.1:", string.Empty).Replace("localhost:", string.Empty);
                    break;

                case "4": // 客户端类型
                    clientName = $" - {ClientName}";
                    break;
            }
        }

        string resourceVersionDisplay = !string.IsNullOrEmpty(VersionUpdateSettings.ResourceVersion)
            ? $" - {LocalizationHelper.FormatVersion(VersionUpdateSettings.ResourceVersion, VersionUpdateSettings.ResourceDateTime)}"
            : string.Empty;
        string uiVersionDisplay = LocalizationHelper.FormatVersion(uiVersion, VersionUpdateSettingsUserControlModel.BuildDateTime);
        rvm.WindowTitle = $"{prefix}MAA{currentConfiguration} - {uiVersionDisplay}{resourceVersionDisplay}{connectConfigName}{connectAddress}{clientName}";
    }

    /// <summary>
    /// Gets the client type.
    /// </summary>
    private string ClientName
    {
        get {
            foreach (var item in GameSettings.ClientTypeList.Where(item => item.Value == GameSettings.ClientType))
            {
                return item.Display;
            }

            return "Unknown Client";
        }
    }

    private static readonly Dictionary<string, string> _serverMapping = new()
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
    public string ServerType => _serverMapping[GameSettings.ClientType];
}
