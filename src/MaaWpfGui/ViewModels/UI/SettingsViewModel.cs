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
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using HandyControl.Controls;
using HandyControl.Data;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
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

        _runningState = RunningState.Instance;
        _runningState.StateChanged += (_, e) => {
            Idle = e.NewState.Idle;

            // Inited = e.Inited;
            // Stopping = e.Stopping;
        };

        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    /// <summary>
    /// 刷新本地化文本。
    /// </summary>
    public void RefreshLocalization()
    {
        DisplayName = LocalizationHelper.GetString("Settings");
        RefreshSettingsList();
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
        var keyList = Enum.GetValues<SettingKey>();

        var tempOrderList = new List<SettingItemViewModel?>();

        bool isAdded = false;
        var orderList = ConfigFactory.Root.Gui.SettingOrders.ToList();
        foreach (var key in keyList.Where(k => !orderList.Any(o => o == k)))
        {
            isAdded = true;
            orderList.Add(key);
        }
        if (isAdded)
        {
            ConfigFactory.Root.Gui.SettingOrders = orderList;
        }

        foreach (var (i, key) in orderList.Select((key, index) => (index, key)))
        {
            var item = new SettingItemViewModel(key.ToString(), LocalizationHelper.GetString(key.ToString()), i);
            tempOrderList.Add(item);
        }

        Settings = [.. tempOrderList.OfType<SettingItemViewModel>()];
        Settings.CollectionChanged += Settings_CollectionChanged;
    }

    /// <summary>
    /// 刷新设置菜单项的本地化文本。
    /// </summary>
    private void RefreshSettingsList()
    {
        foreach (var item in Settings)
        {
            item.Display = LocalizationHelper.GetString(item.Key);
        }
    }

    private void Settings_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs? e)
    {
        ConfigFactory.Root.Gui.SettingOrders = [.. Settings.Select(item => Enum.Parse<SettingKey>(item.Key))];
        Execute.OnUIThread(() => {
            // 集合变更后，根据新的顺序更新各 item 的 Value（右边 Grid.Row 绑定依赖此值）
            for (int i = 0; i < Settings.Count; i++)
            {
                Settings[i].Value = i;
            }

            if (e?.Action == NotifyCollectionChangedAction.Move)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.SortingMaster);
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

    private string _searchText = string.Empty;

    public string SearchText
    {
        get => _searchText;
        set {
            if (SetAndNotify(ref _searchText, value))
            {
                SearchRequested?.Invoke(this, value);
            }
        }
    }

    public event EventHandler<string>? SearchRequested;

    public ObservableCollection<SettingItemViewModel> Settings
    {
        get => _settings;
        set => SetAndNotify(ref _settings, value);
    }

    private void InitConfiguration()
    {
        var configurations = new ObservableCollection<CombinedData>();
        foreach (var conf in ConfigFactory.Root.Configurations)
        {
            configurations.Add(new CombinedData { Display = conf.Key, Value = conf.Key });
        }

        ConfigurationList = configurations;
    }

    private void InitUiSettings()
    {
        var languageList = (from pair in LocalizationHelper.SupportedLanguages
                            where pair.Key != PallasLangKey || IsDrunk
                            select new CombinedData { Display = pair.Value, Value = pair.Key })
           .ToList();

        GuiSettings.LanguageList = languageList;
        GuiSettings.SwitchDarkMode();

        // 主题初始化完成后，若莫奈取色已开启，恢复调色板（必须在主题切换之后执行）
        // 跳过防抖延迟，避免界面先闪烁原版颜色再显示莫奈主题
        BackgroundSettings.UpdateMonet(skipDebounce: true);
    }

    private void InitConnectConfig()
    {
        ConnectSettings.ConnectAddressHistory = new(ConfigFactory.CurrentConfig.Gui.ConnectSettings.AddressHistory);
        ConnectSettings.ConnectAddressHistory.CollectionChanged += (_, _) => {
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.AddressHistory = [.. ConnectSettings.ConnectAddressHistory];
        };
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

    /// <summary>
    /// 当前是否处于喝醉状态（语言为 Pallas）。
    /// </summary>
    public bool IsDrunk => GuiSettings.Language == PallasLangKey;

    /// <summary>
    /// 喝醉：切换到 Pallas 语言。
    /// </summary>
    public void GetDrunk()
    {
        // 不走 Language setter 以避免弹出语言切换确认窗
        GuiSettings.SetLanguageInternal(PallasLangKey);
    }

    /// <summary>
    /// Gets or sets a value indicating whether need to show hangover dialog.
    /// </summary>
    public bool Hangover
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.Hangover = value;
        }
    } = ConfigFactory.Root.Gui.Hangover;

    public DateTimeOffset LastBuyWineTime
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.LastBuyWineTime = value;
        }
    } = ConfigFactory.Root.Gui.LastBuyWineTime;

    /// <summary>
    /// 退出时调用：如果当前喝醉，切回清醒语言并留宿醉标记。
    /// </summary>
    public void Sober()
    {
        if (!IsDrunk)
        {
            return;
        }

        GuiSettings.SetLanguageInternal(SoberLanguage);
        Hangover = true;
    }

    public void HangoverEnd()
    {
        // 同时检查标记和语言：正常退出走 Sober() 会留标记；
        // 异常退出标记可能缺失，但语言仍是 pallas，据此兜底。
        if (!Hangover && !IsDrunk)
        {
            return;
        }

        Hangover = false;
        if (IsDrunk)
        {
            // 异常退出兜底：语言仍为 pallas，这里补切回来
            GuiSettings.SetLanguageInternal(SoberLanguage);
        }

        ShowEasterEggDialog(
            LocalizationHelper.GetString("Burping"),
            LocalizationHelper.GetString("Hangover"),
            LocalizationHelper.GetString("Ok"));
    }

    /// <summary>
    /// 显示非阻塞彩蛋弹窗，用户点确认后执行回调。
    /// </summary>
    /// <param name="caption">标题</param>
    /// <param name="message">提示内容</param>
    /// <param name="confirmText">确认按钮文本</param>
    /// <param name="onConfirm">用户点击确认后的回调（可为 null）</param>
    public static void ShowEasterEggDialog(string caption, string message, string confirmText, Action? onConfirm = null)
    {
        var dialog = new Views.Dialogs.EasterEggDialogView(caption, message, confirmText);
        var hcDialog = Dialog.Show(dialog, nameof(Views.UI.RootView));
        dialog.ConfirmClicked += (_, _) => {
            hcDialog.Close();
            onConfirm?.Invoke();
        };
    }

    public string SoberLanguage
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.SoberLanguage = value;
        }
    } = ConfigFactory.Root.Gui.SoberLanguage;

    /// <summary>
    /// Did you buy wine?
    /// </summary>
    /// <returns>The answer.</returns>
    public bool DidYouBuyWine()
    {
        var now = DateTimeOffset.UtcNow.ToYjDateTime();
        if (now == ConfigFactory.Root.Gui.LastBuyWineTime)
        {
            return false;
        }

        if (now.IsAprilFoolsDay())
        {
            return true;
        }

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

    private string? _currentConfiguration = ConfigFactory.Root.Current;

    public string? CurrentConfiguration
    {
        get => _currentConfiguration;
        set {
            if (string.IsNullOrEmpty(value) || value == _currentConfiguration)
            {
                return;
            }

            var previousConfiguration = _currentConfiguration;
            bool ret = ConfigFactory.SwitchConfig(value);

            if (!ret)
            {
                if (!string.IsNullOrEmpty(previousConfiguration))
                {
                    ConfigFactory.SwitchConfig(previousConfiguration);
                }

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

        bool existsInFactory = ConfigFactory.ConfigurationExists(NewConfigurationName);

        if (existsInFactory)
        {
            Growl.Info(new GrowlInfo {
                IsCustom = true,
                Message = LocalizationHelper.GetStringFormat("ConfigExists", NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            });
            return;
        }

        if (!ConfigFactory.AddConfiguration(NewConfigurationName, CurrentConfiguration))
        {
            Growl.Info(new GrowlInfo {
                IsCustom = true,
                Message = LocalizationHelper.GetStringFormat("ConfigExists", NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            });
            return;
        }

        ConfigurationList.Add(new CombinedData { Display = NewConfigurationName, Value = NewConfigurationName });

        // 配置数量大于 1 时，标题栏显示配置名
        UpdateWindowTitle();

        Growl.Info(new GrowlInfo {
            IsCustom = true,
            Message = LocalizationHelper.GetStringFormat("AddConfigSuccess", NewConfigurationName),
            IconKey = "HangoverGeometry",
            IconBrushKey = "PallasBrush",
        });
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DeleteConfiguration(CombinedData delete)
    {
        if (ConfigFactory.DeleteConfiguration(delete.Display))
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

    public int GuideStepIndex
    {
        get; set {
            ConfigFactory.Root.Gui.GuideStep = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.Gui.GuideStep;

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

    // UI 绑定的方法
    [UsedImplicitly]
    public void RestartGuide()
    {
        GuideStepIndex = 0;
        TaskSettingVisibilities.Guide = true;
        var result = MessageBoxHelper.Show(
            LocalizationHelper.GetString("RestartGuidePrompt"),
            LocalizationHelper.GetString("Tip"),
            MessageBoxButton.OKCancel,
            MessageBoxImage.Question);
        if (result == MessageBoxResult.OK)
        {
            Bootstrapper.ShutdownAndRestartWithoutArgs();
        }
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

            if (value < 0 || value >= DividerVerticalOffsetList.Count)
            {
                return;
            }

            switch (_notifySource)
            {
                case NotifyType.None:
                    _notifySource = NotifyType.SelectedIndex;
                    SetAndNotify(ref _selectedIndex, value);

                    ScrollAnimationTarget = DividerVerticalOffsetList[value];

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

            // 平滑滚动动画落地的回写（ScrollViewerBinding 在动画结束后把目标值路由回绑定源）：
            // 与动画目标一致说明本次滚动源于导航定位，同步值即可，不反向重算导航高亮——
            // 目标偏移被 ScrollViewer 钳制时（分节下方内容不足一屏，实际停不到目标），
            // 回写会命中下方“滚到底选最后一项”的分支，高亮跳离所点击的分节
            if (Math.Abs(value - ScrollAnimationTarget) < 1)
            {
                SetAndNotify(ref _scrollOffset, value);
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

    private double _scrollAnimationTarget = double.NaN;

    /// <summary>
    /// Gets or sets the target offset of the smooth scroll animation,
    /// which is performed by <see cref="Styles.Properties.ScrollViewerBinding"/> on the UI side.
    /// </summary>
    public double ScrollAnimationTarget
    {
        get => _scrollAnimationTarget;
        set => SetAndNotify(ref _scrollAnimationTarget, value);
    }

    #endregion 设置页面列表和滚动视图联动绑定

    #region 折叠框展开状态

    private bool GetExpanderState(SettingKey key) => !ConfigFactory.Root.Gui.CollapesStates.Contains(key);

    private void SetExpanderState(SettingKey key, bool value, [CallerMemberName] string propertyName = "")
    {
        if (!value)
        {
            ConfigFactory.Root.Gui.CollapesStates.Add(key);
        }
        else
        {
            ConfigFactory.Root.Gui.CollapesStates.Remove(key);
        }
        NotifyOfPropertyChange(propertyName);
    }

    public bool IsSwitchConfigurationExpanded
    {
        get => GetExpanderState(SettingKey.SwitchConfiguration);
        set => SetExpanderState(SettingKey.SwitchConfiguration, value);
    }

    public bool IsScheduleSettingsExpanded
    {
        get => GetExpanderState(SettingKey.ScheduleSettings);
        set => SetExpanderState(SettingKey.ScheduleSettings, value);
    }

    public bool IsPerformanceSettingsExpanded
    {
        get => GetExpanderState(SettingKey.PerformanceSettings);
        set => SetExpanderState(SettingKey.PerformanceSettings, value);
    }

    public bool IsGameSettingsExpanded
    {
        get => GetExpanderState(SettingKey.GameSettings);
        set => SetExpanderState(SettingKey.GameSettings, value);
    }

    public bool IsConnectionSettingsExpanded
    {
        get => GetExpanderState(SettingKey.ConnectionSettings);
        set => SetExpanderState(SettingKey.ConnectionSettings, value);
    }

    public bool IsStartupSettingsExpanded
    {
        get => GetExpanderState(SettingKey.StartupSettings);
        set => SetExpanderState(SettingKey.StartupSettings, value);
    }

    public bool IsRemoteControlSettingsExpanded
    {
        get => GetExpanderState(SettingKey.RemoteControlSettings);
        set => SetExpanderState(SettingKey.RemoteControlSettings, value);
    }

    public bool IsUiSettingsExpanded
    {
        get => GetExpanderState(SettingKey.UiSettings);
        set => SetExpanderState(SettingKey.UiSettings, value);
    }

    public bool IsBackgroundSettingsExpanded
    {
        get => GetExpanderState(SettingKey.BackgroundSettings);
        set => SetExpanderState(SettingKey.BackgroundSettings, value);
    }

    public bool IsExternalNotificationSettingsExpanded
    {
        get => GetExpanderState(SettingKey.ExternalNotificationSettings);
        set => SetExpanderState(SettingKey.ExternalNotificationSettings, value);
    }

    public bool IsHotKeySettingsExpanded
    {
        get => GetExpanderState(SettingKey.HotKeySettings);
        set => SetExpanderState(SettingKey.HotKeySettings, value);
    }

    public bool IsAchievementSettingsExpanded
    {
        get => GetExpanderState(SettingKey.AchievementSettings);
        set => SetExpanderState(SettingKey.AchievementSettings, value);
    }

    public bool IsUpdateSettingsExpanded
    {
        get => GetExpanderState(SettingKey.UpdateSettings);
        set => SetExpanderState(SettingKey.UpdateSettings, value);
    }

    public bool IsIssueReportExpanded
    {
        get => GetExpanderState(SettingKey.IssueReport);
        set => SetExpanderState(SettingKey.IssueReport, value);
    }

    public bool IsAboutUsExpanded
    {
        get => GetExpanderState(SettingKey.AboutUs);
        set => SetExpanderState(SettingKey.AboutUs, value);
    }

    #endregion 折叠框展开状态

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

        string prefix = ConfigFactory.CurrentConfig.Gui.WindowTitlePrefix;
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
        string adminTag = Bootstrapper.IsAdministratorWithUac() ? $" ({LocalizationHelper.GetString("Administrator")})" : string.Empty;
        rvm.WindowTitle = $"{prefix}MAA{adminTag}{currentConfiguration} - {uiVersionDisplay}{resourceVersionDisplay}{connectConfigName}{connectAddress}{clientName}";
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

    private static readonly Dictionary<ClientType, string> _serverMapping = new()
    {
        { ClientType.Official, "CN" },
        { ClientType.Bilibili, "CN" },
        { ClientType.EN, "US" },
        { ClientType.JP, "JP" },
        { ClientType.KR, "KR" },
        { ClientType.Txwy, "ZH_TW" },
    };

    /// <summary>
    /// Gets the server type.
    /// </summary>
    public string ServerType => _serverMapping[GameSettings.ClientType];
}
