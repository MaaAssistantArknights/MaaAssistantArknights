// <copyright file="TaskQueueViewModel.cs" company="MaaAssistantArknights">
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
using System.Buffers;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using MaaWpfGui.Views.Dialogs;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;
using Application = System.Windows.Application;
using Screen = Stylet.Screen;
using Task = System.Threading.Tasks.Task;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The view model of task queue.
/// </summary>
// 通过 container.Get<TaskQueueViewModel>(); 实例化或获取实例
// ReSharper disable once ClassNeverInstantiated.Global
public class TaskQueueViewModel : Screen
{
    private readonly RunningState _runningState;

    private static readonly ILogger _logger = Log.ForContext<TaskQueueViewModel>();

    /// <summary>
    /// Gets or private sets the view models of task items.
    /// </summary>
    public ObservableCollection<TaskItemViewModel> TaskItemViewModels { get; private set; } = [];

    /// <summary>
    /// Gets the visibility of task setting views.
    /// </summary>
    public static TaskSettingVisibilityInfo TaskSettingVisibilities => TaskSettingVisibilityInfo.Instance;

    public static SettingsViewModel TaskSettingDataContext => Instances.SettingsViewModel;

    /// <summary>
    /// Gets the after action setting.
    /// </summary>
    public PostActionSetting PostActionSetting { get; } = PostActionSetting.Instance;

    #region 长草任务Model

    /// <summary>
    /// Gets 连接任务Model
    /// </summary>
    public static StartUpSettingsUserControlModel StartUpTask => StartUpSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 战斗任务Model
    /// </summary>
    public static FightSettingsUserControlModel FightTask => FightSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 招募任务Model
    /// </summary>
    public static RecruitSettingsUserControlModel RecruitTask => RecruitSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 信用及购物任务Model
    /// </summary>
    public static MallSettingsUserControlModel MallTask => MallSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 基建任务Model
    /// </summary>
    public static InfrastSettingsUserControlModel InfrastTask => InfrastSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 领取奖励任务
    /// </summary>
    public static AwardSettingsUserControlModel AwardTask => AwardSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 肉鸽任务Model
    /// </summary>
    public static RoguelikeSettingsUserControlModel RoguelikeTask => RoguelikeSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 生稀盐酸任务Model
    /// </summary>
    public static ReclamationSettingsUserControlModel ReclamationTask => ReclamationSettingsUserControlModel.Instance;

    /// <summary>
    /// Gets 生稀盐酸任务Model
    /// </summary>
    public static CustomSettingsUserControlModel CustomTask => CustomSettingsUserControlModel.Instance;

    #endregion 长草任务Model

    private static readonly IEnumerable<TaskSettingsViewModel> _taskViewModelTypes = InitTaskViewModelList();

    /// <summary>
    /// 实时更新任务顺序
    /// </summary>
    /// <param name="sender">ignored object</param>
    /// <param name="e">ignored NotifyCollectionChangedEventArgs</param>
    public void TaskItemSelectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        Application.Current.Dispatcher.InvokeAsync(() => {
            if (e.Action == NotifyCollectionChangedAction.Move)
            {
                int oldIndex = e.OldStartingIndex;
                int newIndex = e.NewStartingIndex;

                ConfigFactory.CurrentConfig.TaskQueue.Move(oldIndex, newIndex);

                int start = Math.Min(oldIndex, newIndex);
                int end = Math.Max(oldIndex, newIndex);

                // Move 会导致区间内所有任务顺序变化，必须全部更新
                for (int i = start; i <= end; i++)
                {
                    TaskItemViewModels[i].Index = i;
                }

                if (TaskSettingVisibilities.CurrentIndex == oldIndex)
                {
                    TaskSettingVisibilities.CurrentIndex = newIndex;
                }
            }
            else if (e.Action == NotifyCollectionChangedAction.Add)
            {
                EnableAfterActionSetting = false;
                TaskItemViewModels[e.NewStartingIndex].Index = e.NewStartingIndex;
                TaskItemViewModels.FirstOrDefault(i => i.EnableSetting)?.EnableSetting = false;
                TaskItemViewModels[e.NewStartingIndex].EnableSetting = true;
            }
            else if (e.Action == NotifyCollectionChangedAction.Remove)
            {
                if (e.OldStartingIndex >= 0 && e.OldStartingIndex < ConfigFactory.CurrentConfig.TaskQueue.Count)
                {
                    TaskSettingVisibilities.SetTaskSettingVisible(ConfigFactory.CurrentConfig.TaskQueue[e.OldStartingIndex], false);
                    ConfigFactory.CurrentConfig.TaskQueue.RemoveAt(e.OldStartingIndex);
                }

                // 更新后续任务的索引
                for (int i = e.OldStartingIndex; i < TaskItemViewModels.Count; i++)
                {
                    TaskItemViewModels[i].Index = i;
                }
                if (e.OldStartingIndex == TaskSettingVisibilities.CurrentIndex)
                {
                    var task = TaskItemViewModels.FirstOrDefault(i => i.Index == e.OldStartingIndex);
                    task ??= TaskItemViewModels.FirstOrDefault(i => i.Index == e.OldStartingIndex - 1);
                    task?.EnableSetting = true;
                }
            }
        });
    }

    public static void ChooseOverlayTarget()
    {
        try
        {
            var owner = Application.Current.MainWindow;
            var picker = new ProcessPickerDialogView { Owner = owner };
            var ok = picker.ShowDialog();
            if (ok == true && picker.SelectedHwnd != IntPtr.Zero)
            {
                var overlayVm = Instances.OverlayViewModel;
                if (overlayVm != null)
                {
                    overlayVm.SetTargetHwnd(picker.SelectedHwnd);
                    if (overlayVm.IsCreated)
                    {
                        overlayVm.Close();
                        overlayVm.EnsureCreated();
                    }
                }
            }
        }
        catch
        {
            // ignored
        }
    }

    public bool IsOverlayEnabled { get => field; set => SetAndNotify(ref field, value); }

    public static void EnableOverlay()
    {
        if (Instances.TaskQueueViewModel.IsOverlayEnabled)
        {
            return;
        }

        Instances.TaskQueueViewModel.IsOverlayEnabled = true;
        Instances.OverlayViewModel?.EnsureCreated();
    }

    public static void DisableOverlay()
    {
        if (!Instances.TaskQueueViewModel.IsOverlayEnabled)
        {
            return;
        }

        Instances.TaskQueueViewModel.IsOverlayEnabled = false;
        Instances.OverlayViewModel?.Close();
    }

    public static void ToggleOverlay()
    {
        if (Instances.TaskQueueViewModel.IsOverlayEnabled)
        {
            DisableOverlay();
        }
        else
        {
            EnableOverlay();
        }
    }

    /// <summary>
    /// Gets or private sets the view models of log items.
    /// </summary>
    public ObservableCollection<LogItemViewModel> LogItemViewModels { get; private set; } = [];

    /// <summary>
    /// Gets the grouped log cards. Each card contains multiple <see cref="LogItemViewModel"/>.
    /// </summary>
    public ObservableCollection<LogCardItemViewModel> LogCardViewModels { get; private set; } = [];

    private bool TryMergeIntoLastCard(string content, string color, string weight, ToolTip? toolTip)
    {
        // Merge into last existing card when it exists and is not sealed.
        if (LogCardViewModels.Count == 0)
        {
            return false;
        }

        var lastCard = LogCardViewModels[^1];
        var log = new LogItemViewModel(content, color, weight, toolTip: toolTip);
        LogItemViewModels.Add(log);
        lastCard.Items.Add(log);
        return true;
    }

    /// <summary>
    /// Gets or private sets the single download-related log item.
    /// Use a single LogItemViewModel instead of a collection because only one entry is shown.
    /// </summary>
    private LogItemViewModel _downloadLogItemViewModel = new(string.Empty);

    public LogItemViewModel DownloadLogItemViewModel
    {
        get => _downloadLogItemViewModel;
        private set => SetAndNotify(ref _downloadLogItemViewModel, value);
    }

    #region LogThumbnails

    private readonly SemaphoreSlim _logThumbnailSemaphore = new(1, 1);

    private const int LogThumbnailWidth = 640;
    private const int LogThumbnailHeight = 360;

    private static int MaxLogItemsWithThumbnails => SettingsViewModel.GuiSettings.MaxNumberOfLogThumbnails;

    private async Task AttachThumbnailToCardAsync(LogCardItemViewModel card, bool forceScreencap, bool setToolTipOnLastLogItem = false)
    {
        if (card is null)
        {
            _logger.Warning("Cannot attach thumbnail to null log card.");
            return;
        }

        try
        {
            var thumbnail = await GetOrCaptureLogThumbnailAsync(forceScreencap).ConfigureAwait(false);
            if (thumbnail is null)
            {
                return;
            }

            await Execute.OnUIThreadAsync(() => {
                // 检查卡片是否还在集合中，避免给已清空的卡片赋值
                if (!LogCardViewModels.Contains(card))
                {
                    return;
                }
                card.Thumbnail = thumbnail;
                TrimOldThumbnails();

                // 若需要将当前 Card 图片作为 ToolTip，在缩略图挂载完成后设置最后一条日志的 ToolTip
                if (setToolTipOnLastLogItem && card.Items.Count > 0)
                {
                    var lastLogItem = card.Items[^1];
                    lastLogItem.ToolTip = thumbnail?.CreateTooltip();
                }
            });
        }
        catch
        {
            _logger.Warning("Failed to attach thumbnail to log card.");
        }
    }

    private async Task<BitmapSource?> GetOrCaptureLogThumbnailAsync(bool forceScreencap = false)
    {
        if (!await _logThumbnailSemaphore.WaitAsync(100))
        {
            return null;
        }

        try
        {
            var frameData = await Instances.AsstProxy.AsstGetImageBgrDataAsync(forceScreencap: forceScreencap).ConfigureAwait(false);
            if (frameData is null || frameData.Length == 0)
            {
                return null;
            }

            try
            {
                // 只保留小图，避免日志列表长期运行时占用过多内存。
                var thumbnail = AsstProxy.CreateBgrBitmapSourceScaled(frameData, LogThumbnailWidth, LogThumbnailHeight);
                return thumbnail;
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(frameData);
            }
        }
        finally
        {
            _logThumbnailSemaphore.Release();
        }
    }

    private void TrimOldThumbnails()
    {
        var thumbnailIndices = LogCardViewModels
            .Select((vm, index) => new { vm, index })
            .Where(x => x.vm.Thumbnail != null)
            .Select(x => x.index)
            .ToList();

        if (thumbnailIndices.Count > MaxLogItemsWithThumbnails)
        {
            for (int i = 0; i < thumbnailIndices.Count - MaxLogItemsWithThumbnails; i++)
            {
                LogCardViewModels[thumbnailIndices[i]].Thumbnail = null;
            }
        }
    }

    #endregion

    #region ActionAfterTasks

    private bool _enableAfterActionSetting;

    /// <summary>
    ///  Gets or sets a value indicating whether to show after task queue actions
    /// </summary>
    public bool EnableAfterActionSetting
    {
        get => _enableAfterActionSetting;
        set {
            SetAndNotify(ref _enableAfterActionSetting, value);
            TaskSettingVisibilityInfo.Instance.SetPostAction(value);
        }
    }

    /// <summary>
    /// Checks after completion.
    /// </summary>
    /// <returns>Task</returns>
    public async Task CheckAfterCompleted()
    {
        await Task.Run(() => SettingsViewModel.GameSettings.RunScript("EndsWithScript"));
        var actions = PostActionSetting;
        _logger.Information("Post actions: " + actions.ActionDescription);

        if (actions.BackToAndroidHome)
        {
            Instances.AsstProxy.AsstBackToHome();
            await Task.Delay(1000);
        }

        if (actions.ExitArknights)
        {
            var clientType = SettingsViewModel.GameSettings.ClientType;
            if (!Instances.AsstProxy.AsstStartCloseDown(clientType))
            {
                AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
            }

            await Task.Delay(1000);
        }

        if (actions.ExitEmulator)
        {
            DoKillEmulator();
            await Task.Delay(1000);
        }

        if (actions.ExitSelf && !(actions.Hibernate || actions.Shutdown || actions.Sleep))
        {
            Bootstrapper.Shutdown();
        }

        if (actions.Hibernate)
        {
            if (actions.IfNoOtherMaa && HasOtherMaa())
            {
                Bootstrapper.Shutdown();
            }
            else
            {
                await DoHibernate();
            }
        }

        if (actions.Shutdown)
        {
            if (actions.IfNoOtherMaa && HasOtherMaa())
            {
                Bootstrapper.Shutdown();
            }
            else
            {
                await DoShutDown();
            }
        }

        if (actions.Sleep)
        {
            if (actions.IfNoOtherMaa && HasOtherMaa())
            {
                Bootstrapper.Shutdown();
            }
            else
            {
                await DoSleep();
            }
        }

        if (actions.ExitSelf)
        {
            Bootstrapper.Shutdown();
        }

        actions.LoadPostActions();
        return;

        bool HasOtherMaa()
        {
            var processesCount = Process.GetProcessesByName("MAA").Length;
            _logger.Information("MAA processes count: {ProcessesCount}", processesCount);
            return processesCount > 1;
        }

        void DoKillEmulator()
        {
            if (!EmulatorHelper.KillEmulatorModeSwitcher())
            {
                AddLog(LocalizationHelper.GetString("ExitEmulatorFailed"), UiLogColor.Error);
            }
        }

        async Task DoHibernate()
        {
            actions.LoadPostActions();

            await Execute.OnUIThreadAsync(() => Instances.MainWindowManager?.Show());
            if (await TimerCanceledAsync(
                    LocalizationHelper.GetString("Hibernate"),
                    LocalizationHelper.GetString("HibernatePrompt"),
                    LocalizationHelper.GetString("Cancel"),
                    60))
            {
                return;
            }

            _logger.Information("Hibernate not canceled, proceeding to hibernate.");
            PowerManagement.Hibernate();
        }

        async Task DoShutDown()
        {
            PowerManagement.Shutdown();

            await Execute.OnUIThreadAsync(() => Instances.MainWindowManager?.Show());
            if (await TimerCanceledAsync(
                    LocalizationHelper.GetString("Shutdown"),
                    LocalizationHelper.GetString("AboutToShutdown"),
                    LocalizationHelper.GetString("Cancel"),
                    60))
            {
                PowerManagement.AbortShutdown();
                return;
            }

            _logger.Information("Shutdown not canceled, proceeding to exit application.");
            Bootstrapper.Shutdown();
        }

        async Task DoSleep()
        {
            actions.LoadPostActions();

            await Execute.OnUIThreadAsync(() => Instances.MainWindowManager?.Show());
            if (await TimerCanceledAsync(
                    LocalizationHelper.GetString("Sleep"),
                    LocalizationHelper.GetString("SleepPrompt"),
                    LocalizationHelper.GetString("Cancel"),
                    60))
            {
                return;
            }

            _logger.Information("Sleep not canceled, proceeding to sleep.");
            PowerManagement.Sleep();
        }
    }

    #endregion

    /// <summary>
    /// Initializes a new instance of the <see cref="TaskQueueViewModel"/> class.
    /// </summary>
    public TaskQueueViewModel()
    {
        _runningState = RunningState.Instance;
        _runningState.StateChanged += (_, e) => {
            Idle = e.Idle;
            Inited = e.Inited;
            Stopping = e.Stopping;

            Instances.SettingsViewModel.Idle = e.Idle;
            if (!e.Idle)
            {
                Instances.Data.ClearCache();
            }
        };
        _runningState.TimeoutOccurred += RunningState_TimeOut;

        if (Instances.VersionUpdateDialogViewModel.IsDebugVersion() || File.Exists("DEBUG") || File.Exists("DEBUG.txt"))
        {
            CanShowAutoReload = true;
            ShowDebugTask = true;
        }
    }

    private void RunningState_TimeOut(object? sender, string message)
    {
        Execute.OnUIThread(() => {
            AddLog(message, UiLogColor.Warning);
            ToastNotification.ShowDirect(message);
            if (!SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenTimeout)
            {
                return;
            }

            var lastLogs = LogItemViewModels
                .TakeLast(5)
                .Aggregate(string.Empty, (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n");
            ExternalNotificationService.Send(message, lastLogs);
        });
    }

    protected override void OnInitialActivate()
    {
        base.OnInitialActivate();

        DisplayName = LocalizationHelper.GetString("Farming");
        LogItemViewModels = [];
        InitializeItems();
        InitTimer();

        _ = UpdateDatePromptAndStagesWeb();
    }

    /// <inheritdoc/>
    protected override void OnClose()
    {
        base.OnClose();
    }

    /*
    public void ShowButton()
    {
       Visible = Visibility.Visible;
       Hibernate = true;
    }

    private Visibility _visible = Visibility.Collapsed;

    public Visibility Visible
    {
       get => _visible;
       set => SetAndNotify(ref _visible, value);
    }
    */

    public bool Running { get; set; }

    public bool Closing { get; set; }

    private readonly System.Timers.Timer _timer = new();

    public bool ConfirmExit()
    {
        if (Closing)
        {
            return false;
        }

        Closing = true;
        if (Application.Current.IsShuttingDown())
        {
            // allow close if application is shutting down
            return true;
        }

        if (SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates)
        {
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("UpdateConfirmExitText"),
                LocalizationHelper.GetString("UpdateConfirmExitTitle"),
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            Closing = false;
            return result == MessageBoxResult.Yes;
        }

        if (!Running)
        {
            // no need to confirm if no running task
            return true;
        }

        var confirmResult = MessageBoxHelper.Show(
            LocalizationHelper.GetString("ConfirmExitText"),
            LocalizationHelper.GetString("ConfirmExitTitle"),
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);
        Closing = false;
        return confirmResult == MessageBoxResult.Yes;
    }

    public override Task<bool> CanCloseAsync()
    {
        return Task.FromResult(this.ConfirmExit());
    }

    private void InitTimer()
    {
        _timer.Interval = 30 * 1000;
        _timer.Elapsed += Timer1_Elapsed;
        _timer.Start();
    }

    private DateTime _lastTimerElapsed = DateTime.MinValue;

    private async void Timer1_Elapsed(object? sender, EventArgs e)
    {
        try
        {
            // 提前记录时间，避免等待超过定时时间
            DateTime currentTime = DateTime.Now;
            currentTime = new(currentTime.Year, currentTime.Month, currentTime.Day, currentTime.Hour, currentTime.Minute, 0);

            if (currentTime <= _lastTimerElapsed)
            {
                return;
            }

            _lastTimerElapsed = currentTime;

            if ((currentTime.Hour == 3 || currentTime.Hour == 13 || currentTime.Hour == 23) && currentTime.Minute == 25 && !Idle)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.Time325);
            }

            VersionUpdateSettingsUserControlModel.Instance.RefreshMirrorChyanCdkRemaining();
            HandleDatePromptUpdate();
            HandleCheckForUpdates();

            InfrastTask.RefreshInfrastTimeRotationDisplay();

            await HandleTimerLogic(currentTime);
        }
        catch
        {
            // ignored
        }
    }

    private static int CalculateRandomDelay()
    {
        Random random = new Random();
        int delayTime = random.Next(0, 60 * 60 * 1000);
        return delayTime;
    }

    private bool _isUpdatingDatePrompt;

    private void HandleDatePromptUpdate()
    {
        if (!NeedToUpdateDatePrompt() || _isUpdatingDatePrompt)
        {
            return;
        }

        _isUpdatingDatePrompt = true;
        UpdateDatePromptAndStagesLocally();

        var delayTime = CalculateRandomDelay();
        _ = Task.Run(async () => {
            await Task.Delay(delayTime);
            await _runningState.UntilIdleAsync(60000);
            await UpdateDatePromptAndStagesWeb();
            _isUpdatingDatePrompt = false;
        });
    }

    private bool _isCheckingForUpdates;

    private void HandleCheckForUpdates()
    {
        if (!SettingsViewModel.VersionUpdateSettings.UpdateAutoCheck)
        {
            return;
        }

        if (!NeedToCheckForUpdates() || _isCheckingForUpdates)
        {
            return;
        }

        _isCheckingForUpdates = true;
        var delayTime = CalculateRandomDelay();
        _ = Task.Run(async () => {
            _logger.Information("waiting for update check: {DelayTime}", delayTime);
            await Task.Delay(delayTime);
            await Instances.VersionUpdateDialogViewModel.VersionUpdateAndAskToRestartAsync();
            await ResourceUpdater.ResourceUpdateAndReloadAsync();

            _isCheckingForUpdates = false;
        });
    }

    private static (bool _timeToStart, bool _timeToChangeConfig, int _configIndex) CheckTimers(DateTime currentTime)
    {
        bool timeToStart = false;
        bool timeToChangeConfig = false;
        int configIndex = 0;

        for (int i = 0; i < 8; ++i)
        {
            if (SettingsViewModel.TimerSettings.TimerModels.Timers[i].IsOn == false)
            {
                continue;
            }

            DateTime startTime = new DateTime(currentTime.Year,
                currentTime.Month,
                currentTime.Day,
                SettingsViewModel.TimerSettings.TimerModels.Timers[i].Hour,
                SettingsViewModel.TimerSettings.TimerModels.Timers[i].Min,
                0);
            DateTime restartDateTime = startTime.AddMinutes(-2);

            // 确保0点的定时会在当日的23:58重启
            if (restartDateTime.Day != startTime.Day)
            {
                restartDateTime = restartDateTime.AddDays(1);
            }

            if (currentTime == restartDateTime &&
                Instances.SettingsViewModel.CurrentConfiguration != SettingsViewModel.TimerSettings.TimerModels.Timers[i].TimerConfig)
            {
                timeToChangeConfig = true;
                configIndex = i;
                break;
            }

            // ReSharper disable once InvertIf
            if (currentTime == startTime)
            {
                timeToStart = true;
                configIndex = i;
                break;
            }
        }

        return (timeToStart, timeToChangeConfig, configIndex);
    }

    private async Task HandleTimerLogic(DateTime currentTime)
    {
        if (!_runningState.GetIdle() && !SettingsViewModel.TimerSettings.ForceScheduledStart)
        {
            return;
        }

        var (timeToStart, timeToChangeConfig, configIndex) = CheckTimers(currentTime);

        if (timeToChangeConfig)
        {
            _logger.Information("Scheduled configuration change: Timer Index: {ConfigIndex}", configIndex);
            HandleConfigChange(configIndex);
            return;
        }

        if (timeToStart)
        {
            _logger.Information("Scheduled start: Timer Index: {ConfigIndex}", configIndex);
            await HandleScheduledStart(configIndex);

            SettingsViewModel.TimerSettings.TimerModels.Timers[configIndex].IsOn ??= false;
        }
    }

    private void HandleConfigChange(int configIndex)
    {
        if (SettingsViewModel.TimerSettings.CustomConfig &&
            (_runningState.GetIdle() || SettingsViewModel.TimerSettings.ForceScheduledStart))
        {
            Instances.SettingsViewModel.CurrentConfiguration = SettingsViewModel.TimerSettings.TimerModels.Timers[configIndex].TimerConfig;
        }
    }

    private async Task HandleScheduledStart(int configIndex)
    {
        if (SettingsViewModel.TimerSettings.ForceScheduledStart)
        {
            if (SettingsViewModel.TimerSettings.CustomConfig &&
                Instances.SettingsViewModel.CurrentConfiguration != SettingsViewModel.TimerSettings.TimerModels.Timers[configIndex].TimerConfig)
            {
                _logger.Warning(
                    "Scheduled start skipped: Custom configuration is enabled, but the current configuration does not match the scheduled timer configuration (Timer Index: {ConfigIndex}). Current Configuration: {CurrentConfiguration}, Scheduled Configuration: {TimerConfig}",
                    configIndex,
                    Instances.SettingsViewModel.CurrentConfiguration,
                    SettingsViewModel.TimerSettings.TimerModels.Timers[configIndex].TimerConfig);
                return;
            }

            if (SettingsViewModel.TimerSettings.ShowWindowBeforeForceScheduledStart)
            {
                await Execute.OnUIThreadAsync(() => Instances.MainWindowManager?.Show());
            }

            if (await TimerCanceledAsync(
                    LocalizationHelper.GetString("ForceScheduledStart"),
                    LocalizationHelper.GetString("ForceScheduledStartTip"),
                    LocalizationHelper.GetString("Cancel")))
            {
                return;
            }

            if (!_runningState.GetIdle())
            {
                _logger.Information("Not idle, Stop and CloseDown");
                await Stop();
                SetStopped();
            }

            var mode = SettingsViewModel.GameSettings.ClientType;
            if (!Instances.AsstProxy.AsstAppendCloseDown(mode))
            {
                AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
            }

            ResetAllTemporaryVariable();
        }

        await LinkStart();

        AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.ScheduleMasterGroup);
    }

    private static async Task<bool> TimerCanceledAsync(string content = "", string tipContent = "", string buttonContent = "", int seconds = 10)
    {
        if (Application.Current.Dispatcher.CheckAccess())
        {
            return await ShowDialogAsync();
        }

        return await await Application.Current.Dispatcher.InvokeAsync(ShowDialogAsync);

        async Task<bool> ShowDialogAsync()
        {
            var canceled = false;
            var delay = TimeSpan.FromSeconds(seconds);
            var dialogUserControl = new Views.UserControl.TextDialogWithTimerUserControl(
                content,
                tipContent,
                buttonContent,
                delay.TotalMilliseconds);
            var dialog = HandyControl.Controls.Dialog.Show(dialogUserControl, nameof(Views.UI.RootView));
            var tcs = new TaskCompletionSource<bool>();
            dialogUserControl.Click += (_, _) => {
                canceled = true;
                dialog.Close();
                tcs.TrySetResult(true);
            };
            _logger.Information("Timer wait time: {Seconds}", seconds);
            await Task.WhenAny(Task.Delay(delay), tcs.Task);
            dialog.Close();
            _logger.Information("Timer canceled: {Canceled}", canceled);
            return canceled;
        }
    }

    /// <summary>
    /// Initializes items.
    /// </summary>
    private void InitializeItems()
    {
        List<TaskItemViewModel> taskqueue = [];
        for (int i = 0; i < ConfigFactory.CurrentConfig.TaskQueue.Count; i++)
        {
            var task = ConfigFactory.CurrentConfig.TaskQueue.ElementAt(i);
            if (task is not null)
            {
                taskqueue.Add(new TaskItemViewModel(task.Name, task.IsEnable) { Index = i });
            }
        }

        TaskItemViewModels = [.. taskqueue];
        TaskItemViewModels.CollectionChanged += TaskItemSelectionChanged;
        var taskItem = TaskItemViewModels.ElementAtOrDefault(ConfigFactory.CurrentConfig.TaskSelectedIndex);
        taskItem ??= TaskItemViewModels.FirstOrDefault(i => ConfigFactory.CurrentConfig.TaskQueue[i.Index] is FightTask);
        taskItem ??= TaskItemViewModels.FirstOrDefault();
        if (taskItem is { })
        {
            taskItem.EnableSetting = true;
        }

        if (TaskItemViewModels.Count == 0)
        {
            AddTaskQueueTask(typeof(StartUpTask));
            AddTaskQueueTask(typeof(FightTask));
            AddTaskQueueTask(typeof(InfrastTask));
            AddTaskQueueTask(typeof(RecruitTask));
            AddTaskQueueTask(typeof(MallTask));
            AddTaskQueueTask(typeof(AwardTask));
            AddTaskQueueTask(typeof(RoguelikeTask));
            AddTaskQueueTask(typeof(ReclamationTask));
        }

        NeedToUpdateDatePrompt();
        UpdateDatePromptAndStagesLocally();

        if (DateTime.UtcNow.ToYjDate().IsAprilFoolsDay())
        {
            AddLog(LocalizationHelper.GetString("BuyWineOnAprilFoolsDay"), UiLogColor.Info);
        }
    }

    public DayOfWeek CurDayOfWeek { get; private set; }

    /// <summary>
    /// Determine whether the specified stage is open
    /// </summary>
    /// <param name="name">stage name</param>
    /// <returns>Whether the specified stage is open</returns>
    public bool IsStageOpen(string name) => Instances.StageManager.IsStageOpen(name, CurDayOfWeek);

    /// <summary>
    /// Returns the valid stage if it is open, otherwise returns an empty string.
    /// </summary>
    /// <param name="stage">The stage to check.</param>
    /// <returns>The valid stage or an empty string.</returns>
    public string GetValidStage(string stage) => IsStageOpen(stage) ? stage : string.Empty;

    /// <summary>
    /// Returns whether the task is enabled
    /// </summary>
    /// <param name="baseTask">The TaskQueue task</param>
    /// <returns>whether the task is enabled</returns>
    public static bool IsTaskEnable(BaseTask baseTask) => baseTask.IsEnable is true || (baseTask.IsEnable is null && !GuiSettingsUserControlModel.Instance.MainTasksInvertNullFunction);

    /// <summary>
    /// 更新日期提示和关卡列表
    /// </summary>
    public void UpdateDatePromptAndStagesLocally()
    {
        UpdateDatePrompt();
        FightTask.UpdateStageList();
        ToolboxViewModel.UpdateMiniGameTaskList();
    }

    /// <summary>
    /// 访问 api 获取更新后更新日期提示和关卡列表
    /// </summary>
    /// <returns>可等待</returns>
    public async Task UpdateDatePromptAndStagesWeb()
    {
        await Instances.StageManager.UpdateStageWeb();
        UpdateDatePromptAndStagesLocally();
    }

    private DateOnly _lastPromptDate;

    private bool NeedToUpdateDatePrompt()
    {
        var now = DateTime.UtcNow.ToYjDateTime();

        CurDayOfWeek = now.DayOfWeek;

        // yj历的 4/16 点
        var today = DateOnly.FromDateTime(now);
        bool isCriticalTime = now is { Minute: 0, Hour: 0 or 12 };
        bool isNewDate = today != _lastPromptDate;

        if (!isCriticalTime && !isNewDate)
        {
            return false;
        }

        _lastPromptDate = today;
        return true;
    }

    private static bool NeedToCheckForUpdates()
    {
        var now = DateTime.UtcNow.ToYjDateTime();

        // yj历的 4/22 点
        return now is { Minute: 0, Hour: 0 or 18 };
    }

    /// <summary>
    /// Updates date prompt.
    /// </summary>
    // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
    // 这个函数被列为public可见，意味着他注入对象前被调用
    public void UpdateDatePrompt()
    {
        var builder = new StringBuilder(LocalizationHelper.GetString("TodaysStageTip") + "\n");

        // Closed activity stages
        /*
        foreach (var stage in FightTask.Stages)
        {
            if (stage == null || Instances.StageManager.GetStageInfo(stage).IsActivityClosed() != true)
            {
                continue;
            }

            builder.Append(stage).Append(": ").AppendLine(LocalizationHelper.GetString("ClosedStage"));
        }*/

        // Open stages today
        var openStages = Instances.StageManager.GetStageTips(CurDayOfWeek);
        if (!string.IsNullOrEmpty(openStages))
        {
            builder.Append(openStages);
        }

        var prompt = builder.ToString();
        if (StagesOfToday == prompt)
        {
            return;
        }

        StagesOfToday = prompt;
    }

    private string _stagesOfToday = string.Empty;

    /// <summary>
    /// Gets or private sets the stages of today.
    /// </summary>
    public string StagesOfToday
    {
        get => _stagesOfToday;
        private set => SetAndNotify(ref _stagesOfToday, value);
    }

    public enum LogCardSplitMode
    {
        /// <summary>
        /// 不拆分日志卡片
        /// </summary>
        None = 0,

        /// <summary>
        /// 插入日志前拆分卡片
        /// </summary>
        Before = 1,

        /// <summary>
        /// 插入日志后拆分卡片
        /// </summary>
        After = 2,

        /// <summary>
        /// 插入日志前后都拆分卡片
        /// </summary>
        Both = 3,
    }

    /// <summary>
    /// Adds log.
    /// </summary>
    /// <param name="content">The content.</param>
    /// <param name="color">The font color.</param>
    /// <param name="weight">The font weight.</param>
    /// <param name="toolTip">The toolTip</param>
    /// <param name="updateCardImage">Whether to update the containing card's image/thumbnail.</param>
    /// <param name="fetchLatestImage">Whether to force fetching a fresh screenshot instead of using cache.</param>
    /// <param name="useCardImageAsToolTip">Whether to use the current card's image as toolTip.</param>
    /// <param name="splitMode">Whether to split cards before/after this log.</param>
    public void AddLog(string? content,
        string color = UiLogColor.Trace,
        string weight = "Regular",
        ToolTip? toolTip = null,
        bool updateCardImage = false,
        bool fetchLatestImage = false,
        bool useCardImageAsToolTip = false,
        LogCardSplitMode splitMode = LogCardSplitMode.None)
    {
        bool isEmpty = string.IsNullOrEmpty(content);
        bool needsBeforeSplit = splitMode == LogCardSplitMode.Before || splitMode == LogCardSplitMode.Both;
        bool needsAfterSplit = splitMode == LogCardSplitMode.After || splitMode == LogCardSplitMode.Both;

        Execute.OnUIThread(() => {
            if (needsBeforeSplit)
            {
                createNewCard();
            }

            // 确保至少有一个卡片（如果没有内容且不需要切割，也需要确保有卡片才能更新图片）
            if (LogCardViewModels.Count <= 0 && (!isEmpty || updateCardImage))
            {
                createNewCard();
            }

            if (LogCardViewModels.Count > 0)
            {
                if (!isEmpty)
                {
                    TryMergeIntoLastCard(content!, color, weight, toolTip);
                }

                if (updateCardImage)
                {
                    _ = AttachThumbnailToCardAsync(LogCardViewModels[^1], fetchLatestImage, setToolTipOnLastLogItem: useCardImageAsToolTip);
                }
            }

            if (needsAfterSplit)
            {
                createNewCard();
            }
        });

        // 记录日志
        if (!isEmpty)
        {
            switch (color)
            {
                case UiLogColor.Error:
                    _logger.Error("{Content}", content);
                    break;
                case UiLogColor.Warning:
                    _logger.Warning("{Content}", content);
                    break;
                default:
                    _logger.Information("{Content}", content);
                    break;
            }
        }
    }

    private void createNewCard()
    {
        if (LogCardViewModels.Count > 0 && LogCardViewModels[^1].Items.Count <= 0)
        {
            return;
        }

        var card = new LogCardItemViewModel();
        LogCardViewModels.Add(card);
    }

    /// <summary>
    /// Clears log.
    /// </summary>
    private void ClearLog()
    {
        Execute.OnUIThread(() => {
            LogItemViewModels.Clear();
            LogCardViewModels.Clear();
            DownloadLogItemViewModel = new(string.Empty);
            _logger.Information("Main windows log clear.");
            _logger.Information("{Empty}", string.Empty);
        });
    }

    /// <summary>
    /// Update the dedicated download log area. Thread-safe and will run on UI thread.
    /// Mirrors previous logic which updated the first download log entry.
    /// </summary>
    /// <param name="fullText">The full text to show in download area.</param>
    /// <param name="toolTip">Optional tooltip.</param>
    public void UpdateDownloadLog(string fullText, string? toolTip = null)
    {
        Execute.OnUIThread(() => {
            // Keep download area limited to a single entry.
            if (string.IsNullOrEmpty(fullText))
            {
                DownloadLogItemViewModel = new(string.Empty);
                return;
            }

            var log = new LogItemViewModel(fullText, UiLogColor.Download, toolTip: toolTip?.CreateTooltip());
            DownloadLogItemViewModel = log;
        });
    }

    /// <summary>
    /// Selects all.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectedAll()
    {
        foreach (var item in TaskItemViewModels)
        {
            switch (ConfigFactory.CurrentConfig.TaskQueue[item.Index].TaskType)
            {
                case TaskType.Roguelike:
                case TaskType.Reclamation:
                case TaskType.Custom:
                    continue;
            }

            item.IsEnable = true;
        }
    }

    public static ReadOnlyCollection<GenericCombinedData<Type>> TaskTypeList { get; } = Array.AsReadOnly(
        [
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("StartUp"), Value = typeof(StartUpTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Fight"), Value = typeof(FightTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Infrast"), Value = typeof(InfrastTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Recruit"), Value = typeof(RecruitTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Mall"), Value = typeof(MallTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Award"), Value = typeof(AwardTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Roguelike"), Value = typeof(RoguelikeTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Reclamation"), Value = typeof(ReclamationTask) },
            new GenericCombinedData<Type> { Display = LocalizationHelper.GetString("Custom"), Value = typeof(CustomTask) },
        ]);

    public void AddTaskQueueTask(Type taskName)
    {
        if (Activator.CreateInstance(taskName) is BaseTask task)
        {
            task.Name = TaskTypeList.FirstOrDefault(t => t.Value == taskName)?.Display ?? taskName.Name;
            ConfigFactory.CurrentConfig.TaskQueue.Add(task);
            TaskItemViewModels.Add(new TaskItemViewModel(task.Name));
        }
        else
        {
            AddLog("could NOT create instance of " + taskName, UiLogColor.Error);
        }
    }

    /// <summary>
    /// 重命名任务
    /// </summary>
    /// <param name="taskItem">任务项</param>
    [UsedImplicitly]
    public void RenameTask(TaskItemViewModel taskItem)
    {
        if (taskItem == null || !Idle)
        {
            return;
        }

        var taskType = ConfigFactory.CurrentConfig.TaskQueue[taskItem.Index].TaskType;
        var currentName = taskItem.Name;
        var dialog = new Views.Dialogs.TextDialogUserControl(
            LocalizationHelper.GetString("RenameTask") + $" {taskItem.Index + 1}-{LocalizationHelper.GetString(taskType.ToString())}",
            LocalizationHelper.GetString("RenameTaskPrompt"),
            currentName) {
            Owner = Application.Current.MainWindow,
        };

        var result = dialog.ShowDialog();

        if (result == true && !string.IsNullOrWhiteSpace(dialog.InputText))
        {
            var newName = dialog.InputText.Trim();
            taskItem.Name = newName;
            if (taskItem.Index < ConfigFactory.CurrentConfig.TaskQueue.Count)
            {
                ConfigFactory.CurrentConfig.TaskQueue[taskItem.Index].Name = newName;
            }

            AddLog(string.Format(LocalizationHelper.GetString("TaskRenamed"), newName), UiLogColor.Info);
        }
    }

    /// <summary>
    /// 删除任务
    /// </summary>
    /// <param name="taskItem">任务项</param>
    [UsedImplicitly]
    public void RemoveTask(TaskItemViewModel taskItem)
    {
        if (taskItem == null || !Idle)
        {
            return;
        }

        var taskType = ConfigFactory.CurrentConfig.TaskQueue[taskItem.Index].TaskType;
        var result = MessageBoxHelper.Show(
            string.Format(LocalizationHelper.GetString("ConfirmDeleteTaskMessage"), $"{taskItem.Index + 1}-{LocalizationHelper.GetString(taskType.ToString())}", taskItem.Name),
            LocalizationHelper.GetString("ConfirmDeleteTask"),
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);

        if (result == MessageBoxResult.Yes)
        {
            var index = taskItem.Index;
            if (index < ConfigFactory.CurrentConfig.TaskQueue.Count)
            {
                TaskItemViewModels.RemoveAt(index);
                AddLog(string.Format(LocalizationHelper.GetString("TaskDeleted"), taskItem.Name), UiLogColor.Info);
            }
        }
    }

    private bool _inverseMode = ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, false);

    /// <summary>
    /// Gets or sets a value indicating whether to use inverse mode.
    /// </summary>
    public bool InverseMode
    {
        get => _inverseMode;
        set {
            SetAndNotify(ref _inverseMode, value);
            InverseShowText = value ? LocalizationHelper.GetString("Inverse") : LocalizationHelper.GetString("Clear");
            InverseMenuText = value ? LocalizationHelper.GetString("Clear") : LocalizationHelper.GetString("Inverse");
            ConfigurationHelper.SetValue(ConfigurationKeys.MainFunctionInverseMode, value.ToString());
        }
    }

    /// <summary>
    /// The width of "Select All" when both.
    /// </summary>
    public const int SelectedAllWidthWhenBoth = 80;

    private int _selectedAllWidth =
        ConfigurationHelper.GetGlobalValue(ConfigurationKeys.InverseClearMode, "Clear") == "ClearInverse" ? SelectedAllWidthWhenBoth : 85;

    /// <summary>
    /// Gets or sets the width of "Select All".
    /// </summary>
    public int SelectedAllWidth
    {
        get => _selectedAllWidth;
        set => SetAndNotify(ref _selectedAllWidth, value);
    }

    private bool _showInverse = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.InverseClearMode, "Clear") == "ClearInverse";

    /// <summary>
    /// Gets or sets a value indicating whether "Select inversely" is visible.
    /// </summary>
    public bool ShowInverse
    {
        get => _showInverse;
        set => SetAndNotify(ref _showInverse, value);
    }

    private string _inverseShowText = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString))
        ? LocalizationHelper.GetString("Inverse")
        : LocalizationHelper.GetString("Clear");

    /// <summary>
    /// Gets or private Sets the text to be displayed for "Select inversely".
    /// </summary>
    public string InverseShowText
    {
        get => _inverseShowText;
        private set => SetAndNotify(ref _inverseShowText, value);
    }

    private string _inverseMenuText = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString))
        ? LocalizationHelper.GetString("Clear")
        : LocalizationHelper.GetString("Inverse");

    /// <summary>
    /// Gets or private sets the text of inversion menu.
    /// </summary>
    public string InverseMenuText
    {
        get => _inverseMenuText;
        private set => SetAndNotify(ref _inverseMenuText, value);
    }

    /// <summary>
    /// Changes inversion mode.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ChangeInverseMode()
    {
        InverseMode = !InverseMode;
    }

    /// <summary>
    /// Selects inversely.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void InverseSelected()
    {
        if (InverseMode)
        {
            foreach (var item in TaskItemViewModels)
            {
                switch (ConfigFactory.CurrentConfig.TaskQueue[item.Index].TaskType)
                {
                    case TaskType.Roguelike:
                    case TaskType.Reclamation:
                    case TaskType.Custom:
                        continue;
                }

                item.IsEnable = !(item.IsEnable ?? true);
            }
        }
        else
        {
            foreach (var item in TaskItemViewModels)
            {
                item.IsEnable = false;
            }
        }
    }

    /// <summary>
    /// Reset unsaved task selection.
    /// </summary>
    public void ResetTaskSelection()
    {
        foreach (var item in TaskItemViewModels)
        {
            item.IsEnable ??= GuiSettingsUserControlModel.Instance.MainTasksInvertNullFunction;
        }
    }

    /// <summary>
    /// 还原所有临时变量（右键半选）
    /// </summary>
    /// <param name="refreshUI">是否刷新UI</param>
    public void ResetAllTemporaryVariable(bool refreshUI = true)
    {
        foreach (var item in ConfigFactory.CurrentConfig.TaskQueue)
        {
            switch (item)
            {
                case FightTask fight:
                    FightSettingsUserControlModel.ResetFightVariables(fight);
                    if (refreshUI && TaskSettingVisibilityInfo.CurrentTask == fight)
                    {
                        RefreshTaskModel(fight);
                    }
                    break;

                case RecruitTask recruit:
                    RecruitSettingsUserControlModel.ResetRecruitVariables(recruit);
                    if (refreshUI && TaskSettingVisibilityInfo.CurrentTask == recruit)
                    {
                        RefreshTaskModel(recruit);
                    }
                    break;
            }
        }
        ResetTaskSelection();
    }

    private async Task<bool> ConnectToEmulator()
    {
        string errMsg = string.Empty;
        bool connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));

        if (!connected && SettingsViewModel.ConnectSettings.UseAttachWindow)
        {
            AddLog(errMsg, UiLogColor.Error);
            _runningState.SetIdle(true);
            SetStopped();
            return false;
        }

        // 尝试启动模拟器
        if (!connected && SettingsViewModel.ConnectSettings.RetryOnDisconnected)
        {
            AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("TryToStartEmulator"));

            await Task.Run(() => SettingsViewModel.StartSettings.TryToStartEmulator());

            if (_runningState.GetStopping())
            {
                SetStopped();
                return false;
            }

            connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        }

        // 尝试断开连接, 然后重新连接
        if (!connected)
        {
            AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("TryToReconnectByAdb"));
            await Task.Run(() => SettingsViewModel.StartSettings.ReconnectByAdb());

            if (_runningState.GetStopping())
            {
                SetStopped();
                return false;
            }

            Instances.AsstProxy.Connected = false;
            connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        }

        // 尝试重启 ADB
        if (!connected && SettingsViewModel.ConnectSettings.AllowAdbRestart)
        {
            AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("RestartAdb"));

            await Task.Run(() => SettingsViewModel.StartSettings.RestartAdb());

            if (_runningState.GetStopping())
            {
                SetStopped();
                return false;
            }

            connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        }

        // 尝试杀掉 ADB 进程
        if (!connected && SettingsViewModel.ConnectSettings.AllowAdbHardRestart)
        {
            AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("HardRestartAdb"));

            await Task.Run(() => SettingsViewModel.StartSettings.HardRestartAdb());

            if (_runningState.GetStopping())
            {
                SetStopped();
                return false;
            }

            connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        }

        if (connected)
        {
            return true;
        }

        AddLog(errMsg, UiLogColor.Error);
        _runningState.SetIdle(true);
        SetStopped();
        return false;
    }

    public int MainTasksCompletedCount { get; set; }

    public int MainTasksSelectedCount => TaskItemViewModels.Count(x => (x.IsEnable ?? true));

    /// <summary>
    /// updates the main tasks progress.
    /// </summary>
    /// <param name="completedCount">已完成任务数，留空则代表 +1</param>
    public void UpdateMainTasksProgress(int? completedCount = null)
    {
        var rvm = (RootViewModel)this.Parent;
        if (MainTasksSelectedCount == 0)
        {
            rvm.TaskProgress = null;
            return;
        }

        MainTasksCompletedCount = completedCount ?? ++MainTasksCompletedCount;

        if (MainTasksCompletedCount >= MainTasksSelectedCount)
        {
            rvm.TaskProgress = null;
        }
        else
        {
            rvm.TaskProgress = (MainTasksCompletedCount, MainTasksSelectedCount);
        }
    }

    public bool ShowDebugTask { get => field; set => SetAndNotify(ref field, value); }

    public bool CanShowAutoReload { get => field; set => SetAndNotify(ref field, value); }

    public bool EnableAutoReload { get => field; set => SetAndNotify(ref field, value); }

    private DateTime? _taskStartTime;

    /// <summary>
    /// Starts.
    /// </summary>
    /// <returns>Task</returns>
    public async Task LinkStart()
    {
        if (!_runningState.GetIdle())
        {
            _logger.Information("Not idle, return.");
            return;
        }

        _taskStartTime = DateTime.Now;

        ClearLog();

        Instances.OverlayViewModel.LogItemsSource = LogItemViewModels;

        var buildDateTimeLong = VersionUpdateSettingsUserControlModel.BuildDateTimeCurrentCultureString;
        var resourceDateTimeLong = SettingsViewModel.VersionUpdateSettings.ResourceDateTimeCurrentCultureString;
        AddLog($"Build Time:\n{buildDateTimeLong}\nResource Time:\n{resourceDateTimeLong}");

        var buildTimeInterval = (DateTime.UtcNow - VersionUpdateSettingsUserControlModel.BuildDateTime).TotalDays;
        var resourceTimeInterval = (DateTime.UtcNow - SettingsViewModel.VersionUpdateSettings.ResourceDateTime).TotalDays;
        var maxTimeInterval = Math.Max(buildTimeInterval, resourceTimeInterval);
        if (maxTimeInterval > 90)
        {
            AddLog(
                string.Format(
                    LocalizationHelper.GetString("Achievement.Martian.ConditionsTip"),
                    Math.Round(maxTimeInterval / 30, 1)),
                UiLogColor.Error);
        }

        var uiVersion = VersionUpdateSettingsUserControlModel.UiVersion;
        var coreVersion = VersionUpdateSettingsUserControlModel.CoreVersion;
        if (!Instances.VersionUpdateDialogViewModel.IsDebugVersion() && uiVersion != coreVersion)
        {
            AddLog(string.Format(LocalizationHelper.GetString("VersionMismatch"), uiVersion, coreVersion), UiLogColor.Error);
            return;
        }

        MainTasksCompletedCount = 0;

        // 所有提前 return 都要放在 _runningState.SetIdle(false) 之前，否则会导致无法再次点击开始
        _runningState.SetIdle(false);

        // 虽然更改时已经保存过了，不过保险起见在点击开始之后再次保存任务和基建列表
        // TaskItemSelectionChanged();
        // InfrastTask.InfrastOrderSelectionChanged();
        await Task.Run(() => SettingsViewModel.GameSettings.RunScript("StartsWithScript"));

        AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));

        /*
        // 现在的主流模拟器都已经更新过自带的 adb 了，不再需要替换
        if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
        {
            AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
        }
        */

        // 一般是点了“停止”按钮了
        if (_runningState.GetStopping())
        {
            SetStopped();
            return;
        }

        if (!await ConnectToEmulator())
        {
            return;
        }

        // 一般是点了“停止”按钮了
        if (_runningState.GetStopping())
        {
            SetStopped();
            return;
        }

        bool taskRet = true;

        // 直接遍历TaskItemViewModels里面的内容，是排序后的
        int count = 0;
        foreach (var (index, item) in ConfigFactory.CurrentConfig.TaskQueue.Select((task, i) => (i, task)))
        {
            _logger.Information("Index {Index}, Type {TaskType}, Name {TaskName}, IsEnable {IsEnable}",
                index,
                item.TaskType,
                item.Name,
                item.IsEnable);
            if (!IsTaskEnable(item))
            {
                Instances.TaskQueueViewModel.TaskItemViewModels[index].Status = 4;
                continue;
            }

            try
            {
                switch (SerializeTask(item))
                {
                    case true:
                        ++count;
                        Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskId = Instances.AsstProxy.TasksStatus.Last().Key;
                        break;
                    case false:
                        taskRet = false;
                        AddLog(LocalizationHelper.GetStringFormat("TaskAppend.Error", LocalizationHelper.GetString(item.TaskType.ToString()), item.Name), UiLogColor.Error);
                        Instances.TaskQueueViewModel.TaskItemViewModels[index].Status = (int)Main.TaskStatus.Error;
                        break;
                    case null:
                        AddLog(LocalizationHelper.GetStringFormat("TaskAppend.Skip", LocalizationHelper.GetString(item.TaskType.ToString()), item.Name), UiLogColor.Info);
                        Instances.TaskQueueViewModel.TaskItemViewModels[index].Status = 4;
                        break;
                }
            }
            catch (Exception ex)
            {
                taskRet = false;
                AddLog(LocalizationHelper.GetStringFormat("TaskAppend.Error", LocalizationHelper.GetString(item.TaskType.ToString()), item.Name) + "\n" + ex.Message, UiLogColor.Error);
            }
        }

        if (count == 0)
        {
            AddLog(LocalizationHelper.GetString("UnselectedTask"));
            _runningState.SetIdle(true);
            Instances.AsstProxy.AsstStop();
            SetStopped();
            return;
        }

        AchievementTrackerHelper.Instance.SetProgress(AchievementIds.TaskChainKing, count);

        taskRet &= Instances.AsstProxy.AsstStart();

        if (taskRet)
        {
            AddLog(LocalizationHelper.GetString("Running"));
            Instances.AsstProxy.StartTaskTime = DateTimeOffset.Now;
        }
        else
        {
            AddLog(LocalizationHelper.GetString("UnknownErrorOccurs"));
            await Stop();
            SetStopped();
        }

        AchievementTrackerHelper.Instance.MissionStartCountAdd();
        AchievementTrackerHelper.Instance.UseDailyAdd();
    }

    public void ManualStop()
    {
        if (Stopping || _runningState.GetIdle())
        {
            _logger.Information("Already stopping or idle, return.");
            return;
        }

        _ = Stop();
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.TacticalRetreat);

        if (_taskStartTime is null)
        {
            return;
        }

        var duration = DateTime.Now - _taskStartTime.Value;
        if (duration.TotalSeconds < 5)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.TaskStartCancel);
        }
    }

    /// <summary>
    /// <para>通常要和 <see cref="SetStopped()"/> 一起使用，除非能保证回调消息能收到 `AsstMsg.TaskChainStopped`</para>
    /// <para>This is usually done with <see cref="SetStopped()"/> Unless you are guaranteed to receive the callback message `AsstMsg.TaskChainStopped`</para>
    /// </summary>
    /// <param name="timeout">Timeout millisecond</param>
    /// <returns>A <see cref="Task"/>
    /// <para>尝试等待 core 成功停止运行，默认超时时间一分钟</para>
    /// <para>Try to wait for the core to stop running, the default timeout is one minute</para>
    /// </returns>
    public async Task<bool> Stop(int timeout = 60 * 1000)
    {
        _runningState.SetStopping(true);
        AddLog(LocalizationHelper.GetString("Stopping"), splitMode: LogCardSplitMode.Both);
        await Task.Run(() => {
            if (!Instances.AsstProxy.AsstStop())
            {
                _logger.Warning("Failed to stop Asst");
            }
        });

        int count = 0;
        while (Instances.AsstProxy.AsstRunning() && count <= timeout / 100)
        {
            await Task.Delay(100);
            count++;
        }

        return !Instances.AsstProxy.AsstRunning();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async Task WaitAndStop()
    {
        Waiting = true;
        AddLog(LocalizationHelper.GetString("Waiting"));
        if (RoguelikeTask.RoguelikeDelayAbortUntilCombatComplete)
        {
            await WaitUntilRoguelikeCombatComplete();

            if (Instances.AsstProxy.AsstRunning() && !_runningState.GetStopping())
            {
                await Stop();
            }
        }
    }

    /// <summary>
    /// 等待肉鸽战斗结束，10分钟强制退出
    /// </summary>
    private async Task WaitUntilRoguelikeCombatComplete()
    {
        int time = 0;
        while (RoguelikeTask.RoguelikeDelayAbortUntilCombatComplete && RoguelikeInCombatAndShowWait && time < 600 && !Stopping)
        {
            await Task.Delay(1000);
            ++time;
        }
    }

    private bool _roguelikeInCombatAndShowWait;

    public bool RoguelikeInCombatAndShowWait
    {
        get => _roguelikeInCombatAndShowWait;
        set => SetAndNotify(ref _roguelikeInCombatAndShowWait, value);
    }

    public void SetStopped()
    {
        SleepManagement.AllowSleep();
        if (SettingsViewModel.GameSettings.ManualStopWithScript)
        {
            Task.Run(() => SettingsViewModel.GameSettings.RunScript("EndsWithScript"));
        }

        if (!_runningState.GetIdle() || _runningState.GetStopping())
        {
            AddLog(LocalizationHelper.GetString("Stopped"), splitMode: LogCardSplitMode.Both);
        }

        Waiting = false;
        _runningState.SetStopping(false);
        _runningState.SetIdle(true);

        // 只抑制“本轮任务期间”的自动开启；任务结束后应允许下一轮自动开启 LiveView。
    }

    // 该函数将于未来被废弃，改用 LinkStart 代替
    public async Task QuickSwitchAccount()
    {
        if (!_runningState.GetIdle())
        {
            return;
        }

        _runningState.SetIdle(false);
        _taskStartTime = DateTime.Now; // 快速修复

        // 虽然更改时已经保存过了，不过保险起见在点击开始之后再次保存任务和基建列表
        // TaskItemSelectionChanged();
        // InfrastTask.InfrastOrderSelectionChanged();
        ClearLog();

        await Task.Run(() => SettingsViewModel.GameSettings.RunScript("StartsWithScript"));

        AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));

        /*
        // 现在的主流模拟器都已经更新过自带的 adb 了，不再需要替换
        if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
        {
            AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
        }
        */

        // 一般是点了“停止”按钮了
        if (_runningState.GetStopping())
        {
            SetStopped();
            return;
        }

        if (!await ConnectToEmulator())
        {
            return;
        }

        // 一般是点了“停止”按钮了
        if (_runningState.GetStopping())
        {
            SetStopped();
            return;
        }

        bool taskRet = true;
        taskRet &= StartUpTask.SerializeTask(TaskSettingVisibilityInfo.CurrentTask) is true;
        taskRet &= Instances.AsstProxy.AsstStart();

        if (taskRet)
        {
            AddLog(LocalizationHelper.GetString("Running"));
        }
        else
        {
            AddLog(LocalizationHelper.GetString("UnknownErrorOccurs"));
            await Stop();
            SetStopped();
        }
    }

    public bool EnableSetFightParams { get; set; } = true;

    private bool _inited = false;

    public bool Inited
    {
        get => _inited;
        set => SetAndNotify(ref _inited, value);
    }

    private bool _idle;

    /// <summary>
    /// Gets or sets a value indicating whether it is idle.
    /// </summary>
    public bool Idle
    {
        get => _idle;
        set {
            SetAndNotify(ref _idle, value);
            if (!value)
            {
                return;
            }

            UpdateMainTasksProgress(0);
        }
    }

    private bool _stopping;

    /// <summary>
    /// Gets a value indicating whether `stop` is awaiting.
    /// </summary>
    public bool Stopping
    {
        get => _stopping;
        private set => SetAndNotify(ref _stopping, value);
    }

    private bool _waiting;

    /// <summary>
    /// Gets a value indicating whether waiting for roguelike combat complete.
    /// </summary>
    public bool Waiting
    {
        // UI 会根据这个值来改变 Visibility
        [UsedImplicitly]
        get => _waiting;
        private set => SetAndNotify(ref _waiting, value);
    }

    /*
    private bool _shutdown = false;

    public bool Shutdown
    {
        get => return _shutdown;
        set
        {
            SetAndNotify(ref _shutdown, value);

            if (value)
            {
                Hibernate = false;
                Suspend = false;
            }
        }
    }

    private bool _hibernate = false;  // 休眠

    public bool Hibernate
    {
        get => return _hibernate;
        set
        {
            SetAndNotify(ref _hibernate, value);

            if (value)
            {
                Shutdown = false;
                Suspend = false;
            }
        }
    }

    private bool _suspend = false;  // 待机

    public bool Suspend
    {
        get => return _suspend;
        set
        {
            SetAndNotify(ref _suspend, value);

            if (value)
            {
                Shutdown = false;
                Hibernate = false;
            }
        }
    }
    */

    private static IEnumerable<TaskSettingsViewModel> InitTaskViewModelList()
    {
        var types = Assembly.GetExecutingAssembly()
            .GetTypes()
            .Where(t => t is { Namespace: "MaaWpfGui.ViewModels.UserControl.TaskQueue", IsClass: true, IsAbstract: false } && t.IsSubclassOf(typeof(TaskSettingsViewModel)));
        foreach (var type in types)
        {
            // 获取 Instance 字段
            if (type.GetProperty("Instance", BindingFlags.Public | BindingFlags.Static) is not { } property)
            {
                continue;
            }

            // 获取实例
            if (property.GetValue(null) is TaskSettingsViewModel instance)
            {
                yield return instance;
            }
        }
    }

    public static void InvokeProcSubTaskMsg(AsstMsg msg, JObject details)
    {
        foreach (var instance in _taskViewModelTypes)
        {
            // 调用 ProcSubTaskMsg 方法
            instance.ProcSubTaskMsg(msg, details);
        }
    }

    public void RefreshTaskModel(BaseTask task)
    {
        foreach (var instance in _taskViewModelTypes)
        {
            instance.RefreshUI(task);
        }
    }

    /// <summary>序列化任务</summary>
    /// <param name="task">存储的任务</param>
    /// <param name="taskId">任务id, null时追加任务, 非null为设置任务参数</param>
    /// <returns>null为未序列化, false失败, true成功</returns>
    private static bool? SerializeTask(BaseTask task, int? taskId = null)
    {
        bool? ret = null;
        foreach (var instance in _taskViewModelTypes)
        {
            ret ??= instance.SerializeTask(task, taskId);
        }
        return ret;
    }
}
