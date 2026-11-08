// <copyright file="SettingsViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using HandyControl.Controls;
using HandyControl.Data;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Serilog;
using Stylet;
using Stylet.Xaml;
using static MaaWpfGui.Extensions.LocalizationKeyExtensions;
using ComboBox = System.Windows.Controls.ComboBox;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The view model of settings.
/// </summary>
public class SettingsViewModel : Screen
{
    private static readonly ILogger _logger = Log.ForContext<SettingsViewModel>();

    /// <summary>
    /// Gets the shared run control state for run-state bindings.
    /// </summary>
    public RunControlState Run => RunControlState.Instance;

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
    /// Gets 通知设置 model
    /// </summary>
    public static NotificationSettingsUserControlModel NotificationSettings { get; } = new();

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

    /// <summary>
    /// Gets 三方服务 model
    /// </summary>
    public static ThirdPartyServiceSettingsUserControlModel ThirdPartyServiceSettings { get; } = ThirdPartyServiceSettingsUserControlModel.Instance;

    #endregion 设置界面 Model

    /// <summary>
    /// Initializes a new instance of the <see cref="SettingsViewModel"/> class.
    /// </summary>
    public SettingsViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);

        DisplayName = LocalizationHelper.GetString("Settings.Title");

        Init();

        GuideDemoTasks.CollectionChanged += (_, _) =>
        {
            RefreshGuideDemoTaskOrder();
            RecomputeGuideGate();
        };
        ResetGuideDemoTasks();

        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    /// <summary>
    /// 刷新本地化文本。
    /// </summary>
    public void RefreshLocalization()
    {
        DisplayName = LocalizationHelper.GetString("Settings.Title");
        RefreshSettingsList();
        NotifyOfPropertyChange(nameof(GuideNextGateTip));
        NotifyOfPropertyChange(nameof(GuideNightThemeName));
        _guideDemoStageList.RefreshLocalization();
        foreach (var weekly in GuideDemoTasks.Where(t => t.IsFightTask).SelectMany(t => t.DemoWeeklySchedule))
        {
            weekly.RefreshLocalization();
        }
    }

    #region Init

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

    public SettingItemViewModel NotificationSettingsSetting => GetSettingItemByKey("NotificationSettings");

    public SettingItemViewModel ThirdPartyServiceSettingsSetting => GetSettingItemByKey("ThirdPartyServiceSettings");

    public SettingItemViewModel HotKeySettingsSetting => GetSettingItemByKey("HotKeySettings");

    public SettingItemViewModel AchievementSettingsSetting => GetSettingItemByKey("AchievementSettings");

    public SettingItemViewModel UpdateSettingsSetting => GetSettingItemByKey("UpdateSettings");

    public SettingItemViewModel IssueReportSetting => GetSettingItemByKey("IssueReport");

    public SettingItemViewModel AboutUsSetting => GetSettingItemByKey("AboutUs");

    private void InitSettings()
    {
        // 已删除设置项的死键由 SettingsPageReorgMigrationConverter 在反序列化前的 JsonNode 层剔除
        var orderList = ConfigFactory.Root.Gui.SettingOrders.Concat(Enum.GetValues<SettingKey>()).Distinct();

        // 稳定分组，保留旧配置中两个组各自的自定义顺序。
        Settings = [.. orderList
            .Select(key => new SettingItemViewModel(key.ToString(), LocalizationHelper.GetString(key.LocalizationKey()), 0))
            .OrderBy(item => item.IsAdvanced)];
        for (int i = 0; i < Settings.Count; i++)
        {
            Settings[i].Value = i;
            Settings[i].AreAdvancedSettingsVisible = ShowAdvancedSettings;
            Settings[i].PropertyChanged += SettingItem_PropertyChanged;
        }

        SaveSettingOrder();
        _selectedSetting = Settings[0];
        SettingsDropHandler = new SettingsDropHandler(Settings);
        Settings.CollectionChanged += Settings_CollectionChanged;
    }

    public SettingsDropHandler SettingsDropHandler { get; private set; } = null!;

    private bool _showAdvancedSettings = ConfigFactory.Root.Gui.ShowAdvancedSettings;

    public bool ShowAdvancedSettings
    {
        get => _showAdvancedSettings;
        set {
            if (SetAndNotify(ref _showAdvancedSettings, value))
            {
                ConfigFactory.Root.Gui.ShowAdvancedSettings = value;
                UpdateAdvancedSettingsVisibility();
            }
        }
    }

    [PropertyDependsOn(nameof(Settings))]
    public int AdvancedSettingsDividerRow => Settings.Count(item => !item.IsAdvanced);

    [PropertyDependsOn(nameof(Settings))]
    public bool HasVisibleAdvancedSettings => Settings.Any(item => item.IsAdvanced && item.IsVisible);

    [UsedImplicitly]
    public void ToggleAdvancedSettings() => ShowAdvancedSettings = !ShowAdvancedSettings;

    private void UpdateAdvancedSettingsVisibility()
    {
        // 搜索期间临时显示高级分区，不改变用户持久化的显示偏好。
        var showAdvanced = ShowAdvancedSettings || !string.IsNullOrEmpty(SearchText);
        foreach (var item in Settings)
        {
            item.AreAdvancedSettingsVisible = showAdvanced;
        }
    }

    private void SettingItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(SettingItemViewModel.IsVisible))
        {
            NotifyOfPropertyChange(nameof(HasVisibleAdvancedSettings));
            OnSettingItemValueChanged();
        }
    }

    private void SaveSettingOrder() =>
        ConfigFactory.Root.Gui.SettingOrders = [.. Settings.Select(item => Enum.Parse<SettingKey>(item.Key))];

    /// <summary>
    /// 刷新设置菜单项的本地化文本。
    /// </summary>
    private void RefreshSettingsList()
    {
        foreach (var item in Settings)
        {
            item.Display = LocalizationHelper.GetString(Enum.Parse<SettingKey>(item.Key).LocalizationKey());
        }
    }

    private bool _isReorderingSettings;

    private void Settings_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs? e)
    {
        _isReorderingSettings = true;
        _settingToSelectAfterLayout = _selectedSetting;
        SaveSettingOrder();
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

    private bool _isSettingsLayoutRefreshPending;

    private void OnSettingItemValueChanged()
    {
        if (_isSettingsLayoutRefreshPending)
        {
            return;
        }

        var oldSelectedOffset = SelectedIndex >= 0 && SelectedIndex < DividerVerticalOffsetList.Count
            ? DividerVerticalOffsetList[SelectedIndex]
            : double.NaN;

        _isSettingsLayoutRefreshPending = true;
        Application.Current.Dispatcher.InvokeAsync(() => {
            _isSettingsLayoutRefreshPending = false;
            _isReorderingSettings = false;
            RefreshDividerOffsetsRequested?.Invoke(this, EventArgs.Empty);

            var preferred = _settingToSelectAfterLayout;
            _settingToSelectAfterLayout = null;
            var shouldSelectPreferred = preferred is { IsVisible: true };
            var isCurrentSelectionInvalid = SelectedIndex < 0 || SelectedIndex >= Settings.Count || !Settings[SelectedIndex].IsVisible;
            var isCollapsingAdvancedSelection = false;
            if (shouldSelectPreferred || isCurrentSelectionInvalid)
            {
                isCollapsingAdvancedSelection = !shouldSelectPreferred
                    && _selectedSetting is { IsAdvanced: true }
                    && string.IsNullOrEmpty(SearchText);
                var target = shouldSelectPreferred ? preferred
                    : isCollapsingAdvancedSelection
                        ? Settings.LastOrDefault(item => !item.IsAdvanced && item.IsVisible)
                        : Settings.FirstOrDefault(item => item.IsVisible);
                _notifySource = isCollapsingAdvancedSelection ? NotifyType.ScrollOffset : NotifyType.None;
                SelectedIndex = target == null ? -1 : Settings.IndexOf(target);
                if (isCollapsingAdvancedSelection)
                {
                    _notifySource = NotifyType.None;
                }
            }

            if (SelectedIndex >= 0 && SelectedIndex < DividerVerticalOffsetList.Count
                && !double.IsNaN(DividerVerticalOffsetList[SelectedIndex]))
            {
                var targetOffset = DividerVerticalOffsetList[SelectedIndex];
                var offsetShifted = !double.IsNaN(oldSelectedOffset) && Math.Abs(targetOffset - oldSelectedOffset) >= 0.5;
                if ((shouldSelectPreferred || isCurrentSelectionInvalid || offsetShifted) && !isCollapsingAdvancedSelection)
                {
                    if (Math.Abs(ScrollAnimationTarget - targetOffset) < 0.5)
                    {
                        ScrollAnimationTarget = double.NaN;
                    }

                    ScrollAnimationTarget = targetOffset;
                }
            }
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
                UpdateAdvancedSettingsVisibility();
            }
        }
    }

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
            LocalizationHelper.GetString("EasterEgg.Burping"),
            LocalizationHelper.GetString("EasterEgg.Hangover"),
            LocalizationHelper.GetString("Common.Ok"));
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

    /// <summary>
    /// Gets a value indicating whether the ShowGui hotkey failed to register, to mark the editor in the UI.
    /// The manager is the single source of truth; UI reads it when the binding initializes.
    /// </summary>
    public bool HotKeyShowGuiRegistrationFailed => Instances.MaaHotKeyManager?.IsRegistrationFailed(MaaHotKeyAction.ShowGui) ?? false;

    /// <summary>
    /// Gets a value indicating whether the LinkStart hotkey failed to register, to mark the editor in the UI.
    /// The manager is the single source of truth; UI reads it when the binding initializes.
    /// </summary>
    public bool HotKeyLinkStartRegistrationFailed => Instances.MaaHotKeyManager?.IsRegistrationFailed(MaaHotKeyAction.LinkStart) ?? false;

    private static void SetHotKey(MaaHotKeyAction action, MaaHotKey? value)
    {
        if (value != null)
        {
            var result = Instances.MaaHotKeyManager.TryRegister(action, value);
            if (result == MaaHotKeyRegistrationResult.DuplicateHotKey)
            {
                GrowlHelper.Warning(LocalizationHelper.GetString("Settings.HotKey.RegistrationFailed.Duplicate"));
            }
            else if (result == MaaHotKeyRegistrationResult.OccupiedByOtherApp)
            {
                GrowlHelper.Warning(LocalizationHelper.GetString("Settings.HotKey.RegistrationFailed.Occupied"));
            }
        }
        else
        {
            Instances.MaaHotKeyManager.UnRegister(action);
        }

        NotifyHotKeyRegistrationChanged(action);
    }

    private static void NotifyHotKeyRegistrationChanged(MaaHotKeyAction action)
    {
        var settingsViewModel = Instances.SettingsViewModel;
        if (settingsViewModel == null)
        {
            return;
        }

        switch (action)
        {
            case MaaHotKeyAction.ShowGui:
                settingsViewModel.OnHotKeyShowGuiRegistrationChanged();
                break;
            case MaaHotKeyAction.LinkStart:
                settingsViewModel.OnHotKeyLinkStartRegistrationChanged();
                break;
        }
    }

    /// <summary>Notifies the UI to re-read the ShowGui registration state after a register/unregister attempt.</summary>
    private void OnHotKeyShowGuiRegistrationChanged()
    {
        NotifyOfPropertyChange(nameof(HotKeyShowGuiRegistrationFailed));
    }

    /// <summary>Notifies the UI to re-read the LinkStart registration state after a register/unregister attempt.</summary>
    private void OnHotKeyLinkStartRegistrationChanged()
    {
        NotifyOfPropertyChange(nameof(HotKeyLinkStartRegistrationFailed));
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
            GrowlHelper.Info(new GrowlInfo {
                IsCustom = true,
                Message = LocalizationHelper.GetStringFormat("Settings.Configuration.Exists", NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            });
            return;
        }

        if (!ConfigFactory.AddConfiguration(NewConfigurationName, CurrentConfiguration))
        {
            GrowlHelper.Info(new GrowlInfo {
                IsCustom = true,
                Message = LocalizationHelper.GetStringFormat("Settings.Configuration.Exists", NewConfigurationName),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            });
            return;
        }

        ConfigurationList.Add(new CombinedData { Display = NewConfigurationName, Value = NewConfigurationName });

        // 配置数量大于 1 时，标题栏显示配置名
        UpdateWindowTitle();

        GrowlHelper.Info(new GrowlInfo {
            IsCustom = true,
            Message = LocalizationHelper.GetStringFormat("Settings.Configuration.AddSuccess", NewConfigurationName),
            IconKey = "HangoverGeometry",
            IconBrushKey = "PallasBrush",
        });
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DeleteConfiguration(CombinedData delete)
    {
        var result = MessageBoxHelper.Show(
            LocalizationHelper.GetStringFormat("Settings.Configuration.ConfirmDeleteMessage", delete.Display),
            LocalizationHelper.GetString("TaskQueue.ConfirmDeleteTask"),
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning);

        if (result != MessageBoxResult.Yes)
        {
            return;
        }

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
            value = Math.Clamp(value, 0, GuideMaxStep);
            ConfigFactory.Root.Gui.GuideStep = value;
            SetAndNotify(ref field, value);
        }
    } = Math.Clamp(ConfigFactory.Root.Gui.GuideStep, 0, GuideMaxStep);

    /// <summary>
    /// 步骤条点击跳转：由 XAML 的 <c>{s:Action}</c> 调用，点中哪一格就跳到哪一步。
    /// </summary>
    /// <param name="sender">步骤条控件。</param>
    /// <param name="e">鼠标事件参数。</param>
    [UsedImplicitly]
    public void NavigateGuideToStepFromBar(object? sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (e.Handled
            || sender is not System.Windows.Controls.ItemsControl stepBar
            || e.OriginalSource is not DependencyObject source)
        {
            return;
        }

        if (stepBar.ContainerFromElement(source) is not StepBarItem item)
        {
            return;
        }

        e.Handled = true;
        NavigateGuideToStep(item.Index - 1);
    }

    // ｢使用指引｣（含用户协议确认）步骤在 StepBar 中的索引，与 GuideUserControl.xaml 步骤区的 Visibility 条件对应
    private const int GuideAgreementStepIndex = 1;

    // ｢任务设置｣ 步骤在 StepBar 中的索引，与 GuideUserControl.xaml 步骤区的 Visibility 条件对应
    private const int GuideTaskSettingsStepIndex = 5;

    /// <summary>
    /// Gets the reserved reset target step index for tutorial-version migration：教程版本号递增时，
    /// 已完成教程的用户由 Gui.OnDeserialized 重置到该步重新走一遍；需要完整重走保持 0，只补走部分步骤时改为对应索引。
    /// </summary>
    public static int GuideTutorialResetStepIndex => 0;

    /// <summary>
    /// 设置指引 ｢任务设置｣ 步骤的引导 gate 步骤，按引导顺序排列。
    /// gate 为终态判定：不追踪操作过程，只检查演示列表与设置数据的最终状态；
    /// 未到 <see cref="None" /> 时 ｢下一步｣ 保持禁用并按此提示第一个未完成步骤。
    /// </summary>
    private enum GuideGate
    {
        None,
        RestoreStartUp,
        RestoreFight,
        AddFight,
        SelectStage,
        RenameFight,
        ReorderFight,
        EnableWeekly,
        CheckWeekDays,
        AddSwitchTheme,
        ReorderTheme,
        AddNightTheme,
    }

    private GuideGate _guideGateStep;

    /// <summary>
    /// Gets or sets the current unfinished guide gate step，演示数据任意变化后重算；红点与提示绑定派生的 bool 属性。
    /// </summary>
    private GuideGate GuideGateStep
    {
        get => _guideGateStep;
        set => SetAndNotify(ref _guideGateStep, value);
    }

    /// <summary>
    /// Gets 当前步骤 ｢下一步｣ 前置条件的本地化提示，已满足或无前置条件时为 null（即放行）。
    /// 提示资源 key 在 <see cref="GuideGateStepLocalizationKey"/> 注册，UI 与按钮无需改动。
    /// </summary>
    [PropertyDependsOn(nameof(GuideStepIndex), nameof(GuideGateStep), nameof(GuideUserAgreementOpened), nameof(GuideAgreementSatisfied))]
    public string? GuideNextGateTip => GuideStepIndex switch
    {
        GuideAgreementStepIndex when !GuideAgreementSatisfied
            => LocalizationHelper.GetString("Guide.Agreement.Tip"),
        GuideTaskSettingsStepIndex when GuideGateStep != GuideGate.None
            => LocalizationHelper.GetString(GuideGateStepLocalizationKey(GuideGateStep)),
        _ => null,
    };

    private bool _guideUserAgreementOpened;

    /// <summary>
    /// Gets or sets a value indicating whether the user agreement link has been clicked in the guide：
    /// ｢使用指引｣ 步骤要求点击协议链接后才能进入下一步；状态不落盘，重看教程时重新要求点击。
    /// </summary>
    public bool GuideUserAgreementOpened
    {
        get => _guideUserAgreementOpened;
        set => SetAndNotify(ref _guideUserAgreementOpened, value);
    }

    /// <summary>
    /// Gets 用户协议是否已确认：本会话点击过协议链接，或已越过协议步骤（重启恢复进度时视为已确认）。
    /// </summary>
    [PropertyDependsOn(nameof(GuideUserAgreementOpened), nameof(GuideStepIndex))]
    public bool GuideAgreementSatisfied => GuideUserAgreementOpened || GuideStepIndex > GuideAgreementStepIndex;

    /// <summary>
    /// Gets 是否为本地调试环境：本地 Debug 构建或存在 DEBUG/DEBUG.txt 标记时
    /// （编译期与文件判定，不依赖运行时版本链的求值时序）。
    /// </summary>
    private static bool IsLocalDebugEnvironment
        =>
#if DEBUG
            true;
#else
            File.Exists("DEBUG") || File.Exists("DEBUG.txt");
#endif

    /// <summary>
    /// Gets 设置指引左上角 ｢跳过｣ 按钮是否显示：须先确认用户协议并到达协议步骤之后。
    /// </summary>
    [PropertyDependsOn(nameof(GuideStepIndex), nameof(GuideAgreementSatisfied))]
    public bool GuideSkipVisible => GuideStepIndex >= GuideAgreementStepIndex && GuideAgreementSatisfied;

    /// <summary>
    /// Gets ｢上一步｣ 按钮是否可用：首步禁用。
    /// </summary>
    [PropertyDependsOn(nameof(GuideStepIndex))]
    public bool GuidePrevEnabled => GuideStepIndex > 0;

    /// <summary>
    /// Gets 当前进度文字 ｢3 / 7｣ 形式，本地化前缀由 <c>Guide.Progress</c> 提供；
    /// 步骤条本身只能看出走到哪一格，补一行数字让新用户对剩余步数有预期。
    /// 完成后 <see cref="GuideStepIndex"/> 为 <see cref="GuideMaxStep"/>，此处钳制到末步，避免出现 ｢8 / 7｣。
    /// </summary>
    [PropertyDependsOn(nameof(GuideStepIndex))]
    public string GuideProgressText => $"{Math.Min(GuideStepIndex + 1, GuideMaxStep)} / {GuideMaxStep}";

    // UI 绑定的方法（协议链接点击）
    [UsedImplicitly]
    public void OnGuideUserAgreementClick()
    {
        GuideUserAgreementOpened = true;
    }

    private static string GuideGateStepLocalizationKey(GuideGate step) => step switch
    {
        GuideGate.RestoreStartUp => "Guide.Demo.Gate.RestoreStartUp",
        GuideGate.RestoreFight => "Guide.Demo.Gate.RestoreFight",
        GuideGate.AddFight => "Guide.Demo.Gate.AddFight",
        GuideGate.RenameFight => "Guide.Demo.Gate.RenameFight",
        GuideGate.SelectStage => "Guide.Demo.Gate.SelectStage",
        GuideGate.ReorderFight => "Guide.Demo.Gate.ReorderFight",
        GuideGate.EnableWeekly => "Guide.Demo.Gate.EnableWeekly",
        GuideGate.CheckWeekDays => "Guide.Demo.Gate.CheckWeekDays",
        GuideGate.AddSwitchTheme => "Guide.Demo.Gate.AddSwitchTheme",
        GuideGate.ReorderTheme => "Guide.Demo.Gate.ReorderTheme",
        GuideGate.AddNightTheme => "Guide.Demo.Gate.AddNightTheme",
        _ => string.Empty,
    };

    /// <summary>
    /// Gets 设置指引 ｢添加任务｣ 按钮的红点是否显示：下一步为添加类步骤时引导用户注意该按钮。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideAddTaskBadgeVisible => GuideGateStep
        is GuideGate.RestoreStartUp or GuideGate.RestoreFight
        or GuideGate.AddFight or GuideGate.AddSwitchTheme;

    /// <summary>
    /// Gets 演示关卡选择下拉框的红点是否显示：下一步为 ｢选择剿灭关卡｣ 且已选中目标任务齿轮时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep), nameof(GuideTargetGearSelected))]
    public bool GuideStageBadgeVisible => GuideGateStep == GuideGate.SelectStage && GuideTargetGearSelected;

    /// <summary>
    /// Gets ｢高级设置｣ 切换按钮的红点是否显示：下一步为周计划相关步骤、已选中目标任务齿轮且尚未切到高级页时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep), nameof(GuideTargetGearSelected), nameof(GuideDemoAdvancedSettings))]
    public bool GuideAdvancedBadgeVisible => GuideGateStep is GuideGate.EnableWeekly or GuideGate.CheckWeekDays
        && GuideTargetGearSelected && !GuideDemoAdvancedSettings;

    /// <summary>
    /// Gets ｢启用周计划｣ 开关的红点是否显示：下一步为 ｢启用周计划｣ 且已选中目标任务齿轮时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep), nameof(GuideTargetGearSelected))]
    public bool GuideWeeklyBadgeVisible => GuideGateStep == GuideGate.EnableWeekly && GuideTargetGearSelected;

    /// <summary>
    /// Gets 周计划 ｢周一｣ ｢周日｣ 勾选框的红点是否显示：下一步为 ｢勾选周一与周日｣ 且已选中目标任务齿轮时（已勾选的项由模板隐藏红点）。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep), nameof(GuideTargetGearSelected))]
    public bool GuideWeekDaysBadgeVisible => GuideGateStep == GuideGate.CheckWeekDays && GuideTargetGearSelected;

    /// <summary>
    /// Gets ｢添加主题｣ 按钮的红点是否显示：下一步为 ｢添加夜间主题｣ 且已选中目标任务齿轮时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep), nameof(GuideTargetGearSelected))]
    public bool GuideThemeAddBadgeVisible => GuideGateStep == GuideGate.AddNightTheme && GuideTargetGearSelected;

    /// <summary>
    /// Gets 添加菜单 ｢理智作战｣ 菜单项的红点是否显示：下一步为 ｢添加理智作战｣ 时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideFightMenuBadgeVisible => GuideGateStep == GuideGate.AddFight;

    /// <summary>
    /// Gets 添加菜单 ｢更换主题｣ 菜单项的红点是否显示：下一步为 ｢添加更换主题｣ 时。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideSwitchThemeMenuBadgeVisible => GuideGateStep == GuideGate.AddSwitchTheme;

    /// <summary>
    /// Gets 演示关卡选择是否可编辑：仅在 ｢改关卡｣ 步骤（流程锁白名单，其余步骤控件禁用）。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideStageEditingAllowed => GuideGateStep == GuideGate.SelectStage;

    /// <summary>
    /// Gets 演示周计划是否可编辑：仅在周计划相关步骤。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideWeeklyEditingAllowed => GuideGateStep is GuideGate.EnableWeekly or GuideGate.CheckWeekDays;

    /// <summary>
    /// Gets 演示主题列表是否可编辑：仅在 ｢添加夜间主题｣ 步骤。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideThemeEditingAllowed => GuideGateStep == GuideGate.AddNightTheme;

    /// <summary>
    /// Gets 演示任务列表是否允许拖拽或移动排序：仅在重排教学步骤或教程完成后。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideDemoDragAllowed => GuideGateStep is GuideGate.None or GuideGate.ReorderFight or GuideGate.ReorderTheme;

    /// <summary>
    /// Gets 演示 ｢添加任务｣ 按钮是否可用：仅在添加类步骤（含补回类），按钮禁用时菜单不可展开。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideAddTaskEnabled => GuideGateStep
        is GuideGate.AddFight or GuideGate.RestoreFight
        or GuideGate.AddSwitchTheme or GuideGate.RestoreStartUp;

    /// <summary>
    /// Gets 演示任务的 ｢重命名｣ 按钮是否可用：仅在 ｢重命名｣ 步骤。
    /// </summary>
    [PropertyDependsOn(nameof(GuideGateStep))]
    public bool GuideRenameTaskEnabled => GuideGateStep == GuideGate.RenameFight;

    /// <summary>
    /// 流程锁判定下 gate 步骤对应 ｢下一步操作涉及的任务｣：改关卡指向新添加的理智作战（append 在列表尾部），
    /// 其后指向剿灭理智作战，主题步骤指向更换主题任务；添加类步骤无目标任务。
    /// </summary>
    private GuideDemoTaskItem? GuideTargetTask(GuideGate step)
    {
        var fights = GuideDemoTasks.Where(t => t.IsFightTask).ToList();
        return step switch
        {
            GuideGate.SelectStage => fights.LastOrDefault(),
            GuideGate.RenameFight or GuideGate.ReorderFight or GuideGate.EnableWeekly or GuideGate.CheckWeekDays
                => fights.FirstOrDefault(t => t.DemoStage == FightTaskUserControlModel.AnnihilationName),
            GuideGate.ReorderTheme or GuideGate.AddNightTheme => GuideDemoTasks.FirstOrDefault(t => t.IsSwitchThemeTask),
            _ => null,
        };
    }

    /// <summary>
    /// 流程锁：以 Growl 提示当前操作不在引导流程内，并附当前步骤的引导提示。
    /// </summary>
    private void NotifyGuideOperationBlocked()
    {
        var message = LocalizationHelper.GetString("Guide.Demo.BlockedOperation");
        if (GuideNextGateTip is { } tip)
        {
            message += "\n" + tip;
        }

        GrowlHelper.Warning(message);
    }

    private string _guideTransitionMode = "Bottom2Top";

    public string GuideTransitionMode
    {
        get => _guideTransitionMode;
        set => SetAndNotify(ref _guideTransitionMode, value);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void NextGuide()
    {
        NavigateGuideToStep(GuideStepIndex + 1);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void PrevGuide()
    {
        NavigateGuideToStep(GuideStepIndex - 1);
    }

    /// <summary>
    /// 跳转到指定引导步骤：向后跳转自由；向前跨越协议页需已确认协议，跳到末步需任务设置 gate 已完成。
    /// </summary>
    /// <param name="target">目标步骤索引（0 起，末项为 <see cref="GuideMaxStep"/> - 1）。</param>
    public void NavigateGuideToStep(int target)
    {
        target = Math.Clamp(target, 0, GuideMaxStep - 1);
        if (target == GuideStepIndex)
        {
            return;
        }

        bool forward = target > GuideStepIndex;
        if (forward && target > GuideAgreementStepIndex && !GuideAgreementSatisfied)
        {
            GrowlHelper.Warning(LocalizationHelper.GetString("Guide.Agreement.Tip"));
            return;
        }

        if (forward && target == GuideMaxStep - 1 && GuideGateStep != GuideGate.None)
        {
            GrowlHelper.Warning(LocalizationHelper.GetString(GuideGateStepLocalizationKey(GuideGateStep)));
            return;
        }

        GuideTransitionMode = forward ? "Bottom2Top" : "Top2Bottom";
        GuideStepIndex = target;
    }

    // UI 绑定的方法：跳过设置指引；调试环境直接结束，其余先弹确认（含重看入口指引）再结束
    [UsedImplicitly]
    public void SkipGuide()
    {
        if (!IsLocalDebugEnvironment)
        {
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("Guide.SkipPrompt"),
                LocalizationHelper.GetString("Common.Tip"),
                MessageBoxButton.OKCancel,
                MessageBoxImage.Question,
                ok: LocalizationHelper.GetString("Guide.Skip"));
            if (result != MessageBoxResult.OK)
            {
                return;
            }
        }

        CompleteGuide(false);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DoneGuide()
    {
        CompleteGuide(true);
    }

    /// <summary>
    /// 收尾设置指引：隐藏指引界面、写入完成进度与教程版本号。
    /// </summary>
    /// <param name="notify">是否提示完成后如何重新打开指引；主动跳过时不再重复引导文案。</param>
    private void CompleteGuide(bool notify)
    {
        TaskSettingVisibilities.Guide = false;
        GuideStepIndex = GuideMaxStep;
        ConfigFactory.Root.Gui.GuideTutorialVersion = Gui.CurrentGuideTutorialVersion;

        // 走完教程后界面直接切回主界面，新用户会失去｢之后去哪重看｣的线索，这里补一条指向入口的提示
        if (notify)
        {
            GrowlHelper.Success(LocalizationHelper.GetString("Guide.Done.Tip"));
        }
    }

    // UI 绑定的方法：先确认再重置，取消时保持现状不变
    [UsedImplicitly]
    public void RestartGuide()
    {
        var result = MessageBoxHelper.Show(
            LocalizationHelper.GetString("Guide.RestartPrompt"),
            LocalizationHelper.GetString("Common.Tip"),
            MessageBoxButton.OKCancel,
            MessageBoxImage.Question,
            ok: LocalizationHelper.GetString("Common.Confirm"));
        if (result != MessageBoxResult.OK)
        {
            return;
        }

        GuideStepIndex = 0;
        GuideUserAgreementOpened = false;
        ResetGuideDemoTasks();
        TaskSettingVisibilities.Guide = true;
        Bootstrapper.ShutdownAndRestartWithoutArgs();
    }

    /// <summary>
    /// 演示列表容纳的任务数上限，超出后提示不要继续添加。
    /// </summary>
    private const int GuideDemoTaskLimit = 5;

    /// <summary>
    /// 弹窗提示演示任务列表已满。
    /// </summary>
    private void NotifyGuideDemoTaskLimit()
    {
        MessageBoxHelper.Show(
            LocalizationHelper.GetString("Guide.Demo.TaskAddLimit.Tip"),
            LocalizationHelper.GetString("Common.Tip"),
            MessageBoxButton.OK,
            MessageBoxImage.Information);
    }

    /// <summary>
    /// Gets the demo task list for the ｢任务设置｣ guide step's interactive simulation.
    /// </summary>
    public ObservableCollection<GuideDemoTaskItem> GuideDemoTasks { get; } = new();

    /// <summary>
    /// Gets the demo task whose gear button is toggled on，右侧演示设置面板跟随该任务切换；未选中任何齿轮时为 null。
    /// </summary>
    public GuideDemoTaskItem? SelectedGuideDemoTask => GuideDemoTasks.FirstOrDefault(t => t.EnableSetting);

    /// <summary>
    /// Gets 演示 ｢夜间｣ 主题名（随界面语言取对应客户端的主题译名），添加主题预填与 gate 判定共用。
    /// </summary>
    public string GuideNightThemeName => LocalizationHelper.GetString("Guide.Demo.NightTheme");

    private readonly LocalizedObservableList<string> _guideDemoStageList = new(
        (string.Empty, "TaskQueue.Stage.Label.Default"),
        (FightTaskUserControlModel.AnnihilationName, "TaskQueue.Annihilation.Current"));

    /// <summary>
    /// Gets 演示关卡选择的候选列表（当前/上次、当期剿灭），随语言热切换刷新。
    /// </summary>
    public LocalizedObservableList<string> GuideDemoStageList => _guideDemoStageList;

    private bool _guideDemoAdvancedSettings;

    /// <summary>
    /// Gets or sets a value indicating whether the demo settings column shows advanced entries，由常规/高级设置按钮组切换。
    /// 流程锁：仅在周计划相关步骤与引导完成后允许切到高级页，其余步骤拒绝赋值（切回常规页随时放行）。
    /// </summary>
    public bool GuideDemoAdvancedSettings
    {
        get => _guideDemoAdvancedSettings;
        set
        {
            if (value && GuideGateStep is not (GuideGate.None or GuideGate.EnableWeekly or GuideGate.CheckWeekDays))
            {
                NotifyGuideOperationBlocked();
                return;
            }

            SetAndNotify(ref _guideDemoAdvancedSettings, value);
        }
    }

    /// <summary>
    /// 重置指引演示任务列表为初始任务（开始唤醒 + 理智作战）并重算 gate。名称项只存任务类型，显示名随语言热切换刷新。
    /// </summary>
    public void ResetGuideDemoTasks()
    {
        // 先归位流程锁残留状态（RestartGuide 不重启时复用同一实例，armed/上次合法齿轮仍指向上轮），
        // 使下方初始选中落位与构造路径一致，不被流程锁拦截；armed 在落位后重新启用
        _guideFlowLockArmed = false;
        _guideLastAllowedGear = null;
        GuideDemoAdvancedSettings = false;

        foreach (var task in GuideDemoTasks)
        {
            DetachGuideDemoTask(task);
            (task as IDisposable)?.Dispose();
        }

        GuideDemoTasks.Clear();
        foreach (var type in new[] { AsstProxy.TaskType.StartUp, AsstProxy.TaskType.Fight })
        {
            var item = new GuideDemoTaskItem(type);
            AttachGuideDemoTask(item);
            GuideDemoTasks.Add(item);
        }

        GuideDemoTasks[0].EnableSetting = true;
        _guideFlowLockArmed = true;
        RecomputeGuideGate();
    }

    private CommandAction? _addGuideDemoTaskCommand;

    /// <summary>
    /// Gets ｢添加任务｣ 菜单命令，包装 <see cref="AddGuideDemoTask"/>。生成式菜单的容器样式里 Setter.Value
    /// 不接受 s:Action（WPF 仅放行 DynamicResource 与 Binding），故以显式 target 构造 Stylet 的 CommandAction（s:Action 的底层实现）经 Binding 绑定。
    /// </summary>
    public CommandAction AddGuideDemoTaskCommand => _addGuideDemoTaskCommand ??= new CommandAction(
        Instances.SettingsViewModel,
        nameof(AddGuideDemoTask),
        ActionUnavailableBehaviour.Throw,
        ActionUnavailableBehaviour.Throw);

    // UI 绑定的方法
    [UsedImplicitly]
    public void AddGuideDemoTask(Type taskType)
    {
        if (GuideDemoTasks.Count >= GuideDemoTaskLimit)
        {
            NotifyGuideDemoTaskLimit();
            return;
        }

        // 任务链名与类型名同构（XxxTask → Xxx），链名解析为 TaskType 后构造演示项
        var chainName = taskType.Name.EndsWith("Task", StringComparison.Ordinal) ? taskType.Name[..^"Task".Length] : taskType.Name;
        if (!Enum.TryParse(chainName, out AsstProxy.TaskType type))
        {
            return;
        }

        var allowed = GuideGateStep switch
        {
            GuideGate.AddFight or GuideGate.RestoreFight => type == AsstProxy.TaskType.Fight,
            GuideGate.RestoreStartUp => type == AsstProxy.TaskType.StartUp,
            GuideGate.AddSwitchTheme => type == AsstProxy.TaskType.SwitchTheme,
            _ => false,
        };
        if (!allowed)
        {
            NotifyGuideOperationBlocked();
            return;
        }

        var item = new GuideDemoTaskItem(type);
        AttachGuideDemoTask(item);
        GuideDemoTasks.Add(item);
    }

    // UI 绑定的方法（复制按钮常灰，此处拦截旁路调用）
    [UsedImplicitly]
    public void CopyGuideDemoTask(GuideDemoTaskItem taskItem)
    {
        if (taskItem == null)
        {
            return;
        }

        // 流程锁：教程内恒禁复制（无对应教学步骤，副本会引入流程外状态）
        NotifyGuideOperationBlocked();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void AddGuideDemoTheme(GuideDemoTaskItem taskItem)
    {
        if (taskItem?.DemoThemes is not { } themes)
        {
            return;
        }

        // 流程锁：仅添加夜间主题步骤允许对目标任务操作（按钮已按 gate 禁用，此处拦截旁路调用）
        if (GuideGateStep != GuideGate.AddNightTheme || taskItem != GuideTargetTask(GuideGateStep))
        {
            NotifyGuideOperationBlocked();
            return;
        }

        // 演示中添加主题直接预填 ｢夜间｣（取当前语言译名），免打字
        themes.Add(new GuideDemoThemeItem(GuideNightThemeName));
    }

    // UI 绑定的方法（删除按钮按白名单禁用，此处拦截右键等旁路调用）
    [UsedImplicitly]
    public void RemoveGuideDemoTheme(GuideDemoThemeItem themeItem)
    {
        // 流程锁：教程内恒禁删除主题（无对应教学步骤）
        NotifyGuideOperationBlocked();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void RenameGuideDemoTask(GuideDemoTaskItem taskItem)
    {
        if (taskItem == null)
        {
            return;
        }

        if (GuideGateStep != GuideGate.RenameFight || taskItem != GuideTargetTask(GuideGateStep))
        {
            NotifyGuideOperationBlocked();
            return;
        }

        var dialog = new Views.Dialogs.TextDialogView(
            LocalizationHelper.GetString("TaskQueue.RenameTask"),
            LocalizationHelper.GetString("TaskQueue.RenameTaskPrompt"),
            taskItem.Name) {
            Owner = Application.Current.MainWindow,
        };

        if (dialog.ShowDialog() != true || string.IsNullOrWhiteSpace(dialog.InputText))
        {
            return;
        }

        var newName = dialog.InputText.Trim().Replace("\r", string.Empty).Replace("\n", string.Empty);
        taskItem.Name = newName;
    }

    // UI 绑定的方法（删除按钮常灰，此处拦截旁路调用）
    [UsedImplicitly]
    public void RemoveGuideDemoTask(GuideDemoTaskItem taskItem)
    {
        if (taskItem == null)
        {
            return;
        }

        // 流程锁：教程内恒禁删除任务（无对应教学步骤）
        NotifyGuideOperationBlocked();
    }

    // UI 绑定的方法（与拖拽等价的重排手段，重排教学步骤由此完成）
    [UsedImplicitly]
    public void MoveGuideDemoTaskUp(GuideDemoTaskItem taskItem)
    {
        MoveGuideDemoTask(taskItem, -1);
    }

    // UI 绑定的方法（与拖拽等价的重排手段，重排教学步骤由此完成）
    [UsedImplicitly]
    public void MoveGuideDemoTaskDown(GuideDemoTaskItem taskItem)
    {
        MoveGuideDemoTask(taskItem, 1);
    }

    /// <summary>
    /// 移动演示任务一行，负值上移、正值下移；序号与 gate 重算由集合变更处理器完成
    /// </summary>
    /// <param name="taskItem">演示任务项</param>
    /// <param name="offset">移动偏移</param>
    private void MoveGuideDemoTask(GuideDemoTaskItem taskItem, int offset)
    {
        if (taskItem == null)
        {
            return;
        }

        // 流程锁：仅在重排教学步骤或教程完成后允许移动，避免提前改变列表顺序破坏后续 gate 判定
        if (!GuideDemoDragAllowed)
        {
            NotifyGuideOperationBlocked();
            return;
        }

        var index = GuideDemoTasks.IndexOf(taskItem);
        var newIndex = index + offset;
        if (index < 0 || newIndex < 0 || newIndex >= GuideDemoTasks.Count)
        {
            return;
        }

        GuideDemoTasks.Move(index, newIndex);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void StartGuideDemo()
    {
        GrowlHelper.Info(LocalizationHelper.GetString("Guide.Demo.Start"));
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void SelectAllGuideDemoTasks()
    {
        foreach (var task in GuideDemoTasks)
        {
            task.IsChecked = true;
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void ClearGuideDemoTasks()
    {
        foreach (var task in GuideDemoTasks)
        {
            task.IsChecked = false;
        }
    }

    /// <summary>
    /// 集合任意变化后统一重算演示任务的上移/下移可用性（含行数变化时位置不变的行）。
    /// </summary>
    private void RefreshGuideDemoTaskOrder()
    {
        for (int i = 0; i < GuideDemoTasks.Count; i++)
        {
            GuideDemoTasks[i].CanMoveUp = i > 0;
            GuideDemoTasks[i].CanMoveDown = i < GuideDemoTasks.Count - 1;
        }
    }

    /// <summary>
    /// 重算引导 gate：按引导顺序求第一个未满足的终态条件，并同步任务项级红点的落点。
    /// 由演示数据（列表增删移动、重命名、关卡、周计划、主题）的变更通知触发。
    /// </summary>
    private void RecomputeGuideGate()
    {
        GuideGate step = ComputeGuideGateStep();
        GuideGateStep = step;

        foreach (var task in GuideDemoTasks)
        {
            task.IsNameBadgeVisible = false;
            task.IsMoveUpBadgeVisible = false;
            task.IsMoveDownBadgeVisible = false;
            task.IsGearBadgeVisible = false;
        }

        var target = GuideTargetTask(step);
        GuideTargetGearSelected = target is { EnableSetting: true };
        if (target == null)
        {
            return;
        }

        switch (step)
        {
            // 在设置面板内操作的步骤：未选中目标齿轮时红点引导点齿轮，选中后交给面板内的红点
            case GuideGate.SelectStage or GuideGate.EnableWeekly or GuideGate.CheckWeekDays or GuideGate.AddNightTheme:
                target.IsGearBadgeVisible = !target.EnableSetting;
                break;
            case GuideGate.RenameFight:
                target.IsNameBadgeVisible = true;
                break;
            case GuideGate.ReorderFight:
                target.IsMoveUpBadgeVisible = true;
                break;
            case GuideGate.ReorderTheme:
                target.IsMoveDownBadgeVisible = true;
                break;
        }
    }

    private bool _guideTargetGearSelected;

    /// <summary>
    /// Gets or sets a value indicating whether the current gate step's target task has its gear selected；
    /// 设置面板内的红点仅在此时显示，保证同一时刻只有一个红点指向下一步。
    /// </summary>
    private bool GuideTargetGearSelected
    {
        get => _guideTargetGearSelected;
        set => SetAndNotify(ref _guideTargetGearSelected, value);
    }

    /// <summary>
    /// 按引导顺序求第一个未满足的终态条件（终态判定，不追踪操作过程）。
    /// </summary>
    private GuideGate ComputeGuideGateStep()
    {
        if (!GuideDemoTasks.Any(t => t.TaskType == AsstProxy.TaskType.StartUp))
        {
            return GuideGate.RestoreStartUp;
        }

        var fights = GuideDemoTasks.Where(t => t.IsFightTask).ToList();
        if (fights.Count == 0)
        {
            return GuideGate.RestoreFight;
        }

        var annihilationFight = fights.FirstOrDefault(t => t.DemoStage == FightTaskUserControlModel.AnnihilationName);
        if (annihilationFight == null)
        {
            // 引导顺序：添加任务 → 改关卡（当期剿灭）→ 重命名 → 拖动；
            // 无剿灭理智作战时，已有两个理智作战则引导改关卡，否则先引导添加
            return fights.Count >= 2 ? GuideGate.SelectStage : GuideGate.AddFight;
        }

        // 重命名步的终态是 ｢与默认任务区分｣：显示名仍是任务类型的本地化名（未改名或改成同名）即未完成
        if (annihilationFight.Name == LocalizationHelper.GetString(annihilationFight.LocalizationKey))
        {
            return GuideGate.RenameFight;
        }

        // 剿灭理智作战须位于所有默认理智作战（关卡为 当前/上次）之上
        var defaultFights = fights.Where(t => t.DemoStage != FightTaskUserControlModel.AnnihilationName).ToList();
        if (defaultFights.Any(d => GuideDemoTasks.IndexOf(annihilationFight) > GuideDemoTasks.IndexOf(d)))
        {
            return GuideGate.ReorderFight;
        }

        if (!annihilationFight.DemoUseWeeklySchedule)
        {
            return GuideGate.EnableWeekly;
        }

        bool mondayChecked = annihilationFight.DemoWeeklySchedule.First(d => d.DayOfWeek == DayOfWeek.Monday).Value;
        bool sundayChecked = annihilationFight.DemoWeeklySchedule.First(d => d.DayOfWeek == DayOfWeek.Sunday).Value;
        if (!mondayChecked || !sundayChecked)
        {
            return GuideGate.CheckWeekDays;
        }

        var themeTasks = GuideDemoTasks.Where(t => t.IsSwitchThemeTask).ToList();
        if (themeTasks.Count == 0)
        {
            return GuideGate.AddSwitchTheme;
        }

        if (GuideDemoTasks[^1].TaskType != AsstProxy.TaskType.SwitchTheme)
        {
            return GuideGate.ReorderTheme;
        }

        var nightTheme = GuideNightThemeName;
        if (!themeTasks.Any(t => t.DemoThemes?.Any(theme => theme.Name.Trim() == nightTheme) == true))
        {
            return GuideGate.AddNightTheme;
        }

        return GuideGate.None;
    }

    private void AttachGuideDemoTask(GuideDemoTaskItem item)
    {
        item.PropertyChanged += OnGuideDemoTaskPropertyChanged;
        foreach (var weekly in item.DemoWeeklySchedule)
        {
            weekly.PropertyChanged += OnGuideDemoSubItemPropertyChanged;
        }

        if (item.DemoThemes is { } themes)
        {
            themes.CollectionChanged += OnGuideDemoThemesChanged;
            foreach (var theme in themes)
            {
                theme.PropertyChanged += OnGuideDemoSubItemPropertyChanged;
            }
        }
    }

    private void DetachGuideDemoTask(GuideDemoTaskItem item)
    {
        item.PropertyChanged -= OnGuideDemoTaskPropertyChanged;
        foreach (var weekly in item.DemoWeeklySchedule)
        {
            weekly.PropertyChanged -= OnGuideDemoSubItemPropertyChanged;
        }

        if (item.DemoThemes is { } themes)
        {
            themes.CollectionChanged -= OnGuideDemoThemesChanged;
            foreach (var theme in themes)
            {
                theme.PropertyChanged -= OnGuideDemoSubItemPropertyChanged;
            }
        }
    }

    private GuideDemoTaskItem? _guideLastAllowedGear;

    // 流程锁在构造期的初始选中落位后才启用，避免初始齿轮被无目标步骤的锁误拦
    private bool _guideFlowLockArmed;

    private void OnGuideDemoTaskPropertyChanged(object? sender, PropertyChangedEventArgs args)
    {
        if (sender is not GuideDemoTaskItem item)
        {
            return;
        }

        // 红点与箭头是 gate 的输出，写回不再作为输入触发重算，否则形成 重算→写标记→再重算 的死循环（栈溢出）
        if (args.PropertyName is nameof(GuideDemoTaskItem.IsNameBadgeVisible)
            or nameof(GuideDemoTaskItem.IsMoveUpBadgeVisible)
            or nameof(GuideDemoTaskItem.IsMoveDownBadgeVisible)
            or nameof(GuideDemoTaskItem.IsGearBadgeVisible))
        {
            return;
        }

        if (args.PropertyName == nameof(GuideDemoTaskItem.EnableSetting))
        {
            // 流程锁：齿轮只允许切到当前引导步骤的目标任务或切回上一个合法选中（完成后自由），非法选中回滚；
            // 添加类步骤无目标任务，保持初始选中（开始唤醒）不拦
            if (_guideFlowLockArmed
                && item.EnableSetting
                && GuideGateStep != GuideGate.None
                && item != GuideTargetTask(GuideGateStep)
                && item != _guideLastAllowedGear)
            {
                item.EnableSetting = false;
                if (_guideLastAllowedGear is { } last && GuideDemoTasks.Contains(last))
                {
                    last.EnableSetting = true;
                }

                NotifyGuideOperationBlocked();
                return;
            }

            if (item.EnableSetting)
            {
                _guideLastAllowedGear = item;

                // 与主界面一致：切换任务后回常规页（高级页仅理智作战有内容，入口也已按任务类型禁用）
                GuideDemoAdvancedSettings = false;
            }

            NotifyOfPropertyChange(nameof(SelectedGuideDemoTask));
        }

        RecomputeGuideGate();
    }

    private void OnGuideDemoSubItemPropertyChanged(object? sender, PropertyChangedEventArgs args)
    {
        RecomputeGuideGate();
    }

    private void OnGuideDemoThemesChanged(object? sender, NotifyCollectionChangedEventArgs args)
    {
        // 增删主题条目时同步子项订阅（Name 变更影响夜间主题判定）
        if (args.NewItems != null)
        {
            foreach (GuideDemoThemeItem theme in args.NewItems)
            {
                theme.PropertyChanged += OnGuideDemoSubItemPropertyChanged;
            }
        }

        if (args.OldItems != null)
        {
            foreach (GuideDemoThemeItem theme in args.OldItems)
            {
                theme.PropertyChanged -= OnGuideDemoSubItemPropertyChanged;
            }
        }

        RecomputeGuideGate();
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

            SetAndNotify(ref _dividerVerticalOffsetList, value);
        }
    }

    private int _selectedIndex;

    private SettingItemViewModel? _selectedSetting;

    private SettingItemViewModel? _settingToSelectAfterLayout;

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

            if (value == -1)
            {
                _selectedSetting = null;
                SetAndNotify(ref _selectedIndex, value);
                return;
            }

            if (value < 0 || value >= Settings.Count || !Settings[value].IsVisible
                || value >= DividerVerticalOffsetList.Count || double.IsNaN(DividerVerticalOffsetList[value]))
            {
                return;
            }

            if (_isReorderingSettings)
            {
                SetAndNotify(ref _selectedIndex, value);
                _selectedSetting = Settings[value];
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

            if (_selectedIndex == value)
            {
                _selectedSetting = Settings[value];
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
                    var visibleDividers = DividerVerticalOffsetList
                        .Select((offset, index) => (offset, index))
                        .Where(item => !double.IsNaN(item.offset) && Settings[item.index].IsVisible)
                        .ToArray();
                    if (visibleDividers.Length > 0)
                    {
                        // 滚动条滚动到底部，返回最后一个 Divider 索引
                        if (value + ScrollViewportHeight >= ScrollExtentHeight)
                        {
                            SelectedIndex = visibleDividers[^1].index;
                            ResetNotifySource();
                            break;
                        }

                        // 根据出当前 ScrollOffset 选出最后一个在可视范围的 Divider 索引
                        var index = visibleDividers.LastOrDefault(item => value >= item.offset, visibleDividers[0]).index;
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

    public bool IsNotificationSettingsExpanded
    {
        get => GetExpanderState(SettingKey.NotificationSettings);
        set => SetExpanderState(SettingKey.NotificationSettings, value);
    }

    public bool IsThirdPartyServiceSettingsExpanded
    {
        get => GetExpanderState(SettingKey.ThirdPartyServiceSettings);
        set => SetExpanderState(SettingKey.ThirdPartyServiceSettings, value);
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
        var resolutionTip = isYostarEN ? "\n" + LocalizationHelper.GetString("Settings.Game.SwitchResolution.Tip") : string.Empty;

        var result = MessageBoxHelper.Show(
            LocalizationHelper.GetString("Settings.PromptRestartForSettingsChange") + resolutionTip,
            LocalizationHelper.GetString("Common.Tip"),
            MessageBoxButton.OKCancel,
            MessageBoxImage.Question,
            ok: LocalizationHelper.GetString("Common.Confirm"));

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

        string resourceVersionDisplay = DemoWindowTitleResourceVersionOverride is { Length: > 0 } demoResVersion
            ? $" - {demoResVersion}"
            : !string.IsNullOrEmpty(EffectiveResourceVersion)
                ? $" - {LocalizationHelper.FormatVersion(EffectiveResourceVersion, VersionUpdateSettings.ResourceDateTime)}"
                : string.Empty;
        string uiVersionDisplay = DemoWindowTitleVersionOverride is { Length: > 0 } demoUiVersion
            ? demoUiVersion
            : LocalizationHelper.FormatVersion(uiVersion, VersionUpdateSettingsUserControlModel.BuildDateTime);
        string adminTag = Bootstrapper.IsAdministratorWithUac() ? $" ({LocalizationHelper.GetString("Settings.About.Administrator")})" : string.Empty;

        // 常驻预览标记：防止带参数预览后忘记本进程未加载 Core
        string previewTag = Bootstrapper.IsCoreInitSkipped ? $" ({LocalizationHelper.GetString("Settings.About.UiPreviewMode")})" : string.Empty;
        rvm.WindowTitle = $"{prefix}MAA{adminTag}{currentConfiguration} - {uiVersionDisplay}{resourceVersionDisplay}{connectConfigName}{connectAddress}{clientName}{previewTag}";
    }

    /// <summary>
    /// README 截图演示模式的窗口标题 UI 版本段覆盖。null 表示字段缺省走原行为（真实构建版本），
    /// 仅 <see cref="Main.Bootstrapper.IsDemoMode"/> 流程会设置。
    /// </summary>
    internal static string? DemoWindowTitleVersionOverride { get; set; }

    /// <summary>
    /// README 截图演示模式的窗口标题资源版本段覆盖。null 走原行为，空串隐藏整段，
    /// 非 null 时替代真实资源版本参与拼接（一并消除 culture 相关的日期格式差异）。仅演示流程会设置。
    /// </summary>
    internal static string? DemoWindowTitleResourceVersionOverride { get; set; }

    /// <summary>
    /// Gets 标题拼接实际使用的资源版本：演示覆盖优先于真实资源版本。
    /// </summary>
    private static string EffectiveResourceVersion => DemoWindowTitleResourceVersionOverride ?? VersionUpdateSettings.ResourceVersion;

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
