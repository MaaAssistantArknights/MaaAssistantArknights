// <copyright file="TaskQueueViewModel.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using StyletIoC;
using Application = System.Windows.Application;
using ComboBox = System.Windows.Controls.ComboBox;
using Screen = Stylet.Screen;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of task queue.
    /// </summary>
    // 通过 container.Get<TaskQueueViewModel>(); 实例化或获取实例
    // ReSharper disable once ClassNeverInstantiated.Global
    public class TaskQueueViewModel : Screen
    {
        private readonly IContainer _container;
        private StageManager _stageManager;
        private readonly RunningState _runningState;

        private static readonly ILogger _logger = Log.ForContext<TaskQueueViewModel>();

        /// <summary>
        /// Gets or private sets the view models of task items.
        /// </summary>
        public ObservableCollection<DragItemViewModel> TaskItemViewModels { get; private set; }

        /// <summary>
        /// Gets the visibility of task setting views.
        /// </summary>
        public static TaskSettingVisibilityInfo TaskSettingVisibilities => TaskSettingVisibilityInfo.Current;

        public static SettingsViewModel TaskSettingDataContext => Instances.SettingsViewModel;

        /// <summary>
        /// 实时更新任务顺序
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once MemberCanBePrivate.Global
        public void TaskItemSelectionChanged()
        {
            Execute.OnUIThread(() =>
            {
                int index = 0;
                foreach (var item in TaskItemViewModels)
                {
                    ConfigurationHelper.SetTaskOrder(item.OriginalName, index.ToString());
                    ++index;
                }
            });
        }

        /// <summary>
        /// Gets or private sets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; private set; }

        private string _actionAfterCompleted = ConfigurationHelper.GetValue(ConfigurationKeys.ActionAfterCompleted, ActionType.DoNothing.ToString());

        /// <summary>
        /// Gets the list of the actions after completion.
        /// </summary>
        public List<GenericCombinedData<ActionType>> ActionAfterCompletedList { get; } =
            [
                new() { Display = LocalizationHelper.GetString("DoNothing"), Value = ActionType.DoNothing },
                new() { Display = LocalizationHelper.GetString("ExitArknights"), Value = ActionType.StopGame },
                new() { Display = LocalizationHelper.GetString("BackToAndroidHome"), Value = ActionType.BackToAndroidHome },

                new() { Display = LocalizationHelper.GetString("ExitEmulator"), Value = ActionType.ExitEmulator },
                new() { Display = LocalizationHelper.GetString("ExitSelf"), Value = ActionType.ExitSelf },
                new() { Display = LocalizationHelper.GetString("ExitEmulatorAndSelf"), Value = ActionType.ExitEmulatorAndSelf },

                // new() { Display="待机",Value=ActionTypeAfterCompleted.Suspend },
                new() { Display = LocalizationHelper.GetString("ExitEmulatorAndSelfAndHibernate"), Value = ActionType.ExitEmulatorAndSelfAndHibernate },
                new() { Display = LocalizationHelper.GetString("Hibernate"), Value = ActionType.Hibernate },
                new() { Display = LocalizationHelper.GetString("Shutdown"), Value = ActionType.Shutdown },

                // new() { Display = Localization.GetString("ExitEmulatorAndSelfAndHibernate") + "*", Value = ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist },
                new() { Display = LocalizationHelper.GetString("HibernateWithoutPersist"), Value = ActionType.HibernateWithoutPersist },
                new() { Display = LocalizationHelper.GetString("ShutdownWithoutPersist"), Value = ActionType.ShutdownWithoutPersist },

                new() { Display = LocalizationHelper.GetString("ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate"), Value = ActionType.ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate },
                new() { Display = LocalizationHelper.GetString("ExitSelfIfOtherMaaElseShutdown"), Value = ActionType.ExitSelfIfOtherMaaElseShutdown },
            ];

        /// <summary>
        /// Gets or sets the action after completion.
        /// </summary>
        public ActionType ActionAfterCompleted
        {
            get => !Enum.TryParse(_actionAfterCompleted, out ActionType action) ? ActionType.DoNothing : action;
            set
            {
                string storeValue = value.ToString();
                SetAndNotify(ref _actionAfterCompleted, storeValue);

                if (value == ActionType.HibernateWithoutPersist ||
                    value == ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist ||
                    value == ActionType.ShutdownWithoutPersist)
                {
                }
                else
                {
                    ConfigurationHelper.SetValue(ConfigurationKeys.ActionAfterCompleted, storeValue);
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TaskQueueViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public TaskQueueViewModel(IContainer container)
        {
            _container = container;
            _runningState = RunningState.Instance;
            _runningState.IdleChanged += RunningState_IdleChanged;
        }

        private void RunningState_IdleChanged(object sender, bool e)
        {
            Idle = e;
            TaskSettingDataContext.Idle = e;
            if (!e)
            {
                Instances.Data.ClearCache();
            }
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _stageManager = _container.Get<StageManager>();

            DisplayName = LocalizationHelper.GetString("Farming");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            InitializeItems();
            InitTimer();
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

        private readonly DispatcherTimer _timer = new();

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

            if (!Running)
            {
                // no need to confirm if no running task
                return true;
            }

            if (!Instances.RecognizerViewModel.GachaDone)
            {
                // no need to confirm if Gacha running
                return true;
            }

            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("ConfirmExitText"),
                LocalizationHelper.GetString("ConfirmExitTitle"),
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            Closing = false;
            return result == MessageBoxResult.Yes;
        }

        public override Task<bool> CanCloseAsync()
        {
            return Task.FromResult(this.ConfirmExit());
        }

        private void InitTimer()
        {
            _timer.Interval = TimeSpan.FromSeconds(59);
            _timer.Tick += Timer1_Elapsed;
            _timer.Start();
        }

        private async void Timer1_Elapsed(object sender, EventArgs e)
        {
            // 提前记录时间，避免等待超过定时时间
            DateTime currentTime = DateTime.Now;
            currentTime = new DateTime(currentTime.Year, currentTime.Month, currentTime.Day, currentTime.Hour, currentTime.Minute, 0);

            if (NeedToUpdateDatePrompt())
            {
                UpdateDatePrompt();
                UpdateStageList(false);

                // 随机延迟，防止同时更新
                var delayTime = new Random().Next(0, 60 * 60 * 1000);
                _ = Task.Run(async () =>
                {
                    await Task.Delay(delayTime);
                    await _stageManager.UpdateStageWeb();
                    UpdateDatePrompt();
                    UpdateStageList(false);
                });
            }

            if (NeedToCheckForUpdates())
            {
                if (Instances.SettingsViewModel.UpdateAutoCheck)
                {
                    // 随机延迟，防止同时更新
                    var delayTime = new Random().Next(0, 60 * 60 * 1000);
                    _ = Task.Run(async () =>
                    {
                        await Task.Delay(delayTime);
                        _ = Instances.SettingsViewModel.ManualUpdate();
                    });
                }
            }

            RefreshCustomInfrastPlanIndexByPeriod();

            if (!_runningState.GetIdle() && !Instances.SettingsViewModel.ForceScheduledStart && !Instances.SettingsViewModel.CustomConfig)
            {
                return;
            }

            var timeToStart = false;
            var timeToChangeConfig = false;
            var configIndex = 0;
            for (int i = 0; i < 8; ++i)
            {
                if (!Instances.SettingsViewModel.TimerModels.Timers[i].IsOn)
                {
                    continue;
                }

                DateTime startTime = new DateTime(currentTime.Year, currentTime.Month, currentTime.Day,
                    Instances.SettingsViewModel.TimerModels.Timers[i].Hour,
                    Instances.SettingsViewModel.TimerModels.Timers[i].Min,
                    0);
                DateTime restartDateTime = startTime.AddMinutes(-2);

                // 确保0点的定时会在当日的23:58重启
                if (restartDateTime.Day != startTime.Day)
                {
                    restartDateTime = restartDateTime.AddDays(1);
                }

                if (currentTime == restartDateTime &&
                    Instances.SettingsViewModel.CurrentConfiguration != Instances.SettingsViewModel.TimerModels.Timers[i].TimerConfig)
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

            if (timeToChangeConfig)
            {
                if (Instances.SettingsViewModel.CustomConfig &&
                    (_runningState.GetIdle() || Instances.SettingsViewModel.ForceScheduledStart))
                {
                    // CurrentConfiguration设置后会重启
                    Instances.SettingsViewModel.CurrentConfiguration = Instances.SettingsViewModel.TimerModels.Timers[configIndex].TimerConfig;
                    return;
                }
            }

            if (!timeToStart)
            {
                return;
            }

            if (Instances.SettingsViewModel.ForceScheduledStart)
            {
                // 什么时候会遇到这种情况？
                if (Instances.SettingsViewModel.CustomConfig &&
                    Instances.SettingsViewModel.CurrentConfiguration != Instances.SettingsViewModel.TimerModels.Timers[configIndex].TimerConfig)
                {
                    return;
                }

                if (Instances.SettingsViewModel.ShowWindowBeforeForceScheduledStart)
                {
                    Instances.MainWindowManager?.Show();
                }

                if (await TimerCanceledAsync())
                {
                    return;
                }

                if (!_runningState.GetIdle())
                {
                    await Stop();
                }

                if (!Instances.AsstProxy.AsstAppendCloseDown())
                {
                    AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
                }

                ResetFightVariables();
                ResetTaskSelection();
                RefreshCustomInfrastPlanIndexByPeriod();
            }

            LinkStart();
        }

        private static async Task<bool> TimerCanceledAsync()
        {
            var delay = TimeSpan.FromSeconds(10);
            var dialogUserControl = new Views.UserControl.TextDialogWithTimerUserControl(
                LocalizationHelper.GetString("ForceScheduledStart"),
                LocalizationHelper.GetString("ForceScheduledStartTip"),
                LocalizationHelper.GetString("Cancel"),
                delay.TotalMilliseconds);
            var dialog = HandyControl.Controls.Dialog.Show(dialogUserControl, nameof(Views.UI.RootView));
            var canceled = false;
            dialogUserControl.Click += (_, _) =>
            {
                canceled = true;
                dialog.Close();
            };
            await Task.Delay(delay);
            dialog.Close();
            return canceled;
        }

        /// <summary>
        /// Initializes items.
        /// </summary>
        private void InitializeItems()
        {
            List<string> taskList =
            [
                "WakeUp",
                "Recruiting",
                "Base",
                "Combat",
                "Mall",
                "Mission",
                "AutoRoguelike",
            ];

            if (Instances.SettingsViewModel.ClientType is "" or "Official" or "Bilibili")
            {
                taskList.Add("ReclamationAlgorithm2");
            }

            var tempOrderList = new List<DragItemViewModel>(new DragItemViewModel[taskList.Count]);
            var nonOrderList = new List<DragItemViewModel>();
            for (int i = 0; i != taskList.Count; ++i)
            {
                var task = taskList[i];
                bool parsed = int.TryParse(ConfigurationHelper.GetTaskOrder(task, "-1"), out var order);

                var vm = new DragItemViewModel(LocalizationHelper.GetString(task), task, "TaskQueue.");

                if (task == TaskSettingVisibilityInfo.DefaultVisibleTaskSetting)
                {
                    vm.EnableSetting = true;
                }

                if (!parsed || order < 0 || order >= tempOrderList.Count)
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
                while (tempOrderList[i] != null)
                {
                    ++i;
                }

                tempOrderList[i] = newVm;
            }

            TaskItemViewModels = new ObservableCollection<DragItemViewModel>(tempOrderList);

            InitDrops();
            NeedToUpdateDatePrompt();
            UpdateDatePrompt();
            UpdateStageList(true);
            RefreshCustomInfrastPlan();

            if (DateTime.UtcNow.ToYjDate().IsAprilFoolsDay())
            {
                AddLog(LocalizationHelper.GetString("BuyWineOnAprilFoolsDay"), UiLogColor.Info);
            }
        }

        private DayOfWeek _curDayOfWeek;

        /// <summary>
        /// Determine whether the specified stage is open
        /// </summary>
        /// <param name="name">stage name</param>
        /// <returns>Whether the specified stage is open</returns>
        private bool IsStageOpen(string name)
        {
            return _stageManager.IsStageOpen(name, _curDayOfWeek);
        }

        /// <summary>
        /// Updates stage list.
        /// </summary>
        /// <param name="forceUpdate">Whether to update the stage list for selection forcibly</param>
        // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
        // 这个函数被列为public可见，意味着他注入对象前被调用
        public void UpdateStageList(bool forceUpdate)
        {
            var hideUnavailableStage = Instances.SettingsViewModel.HideUnavailableStage;

            // forceUpdate: initializing or settings changing, update stage list forcibly
            if (!forceUpdate && !hideUnavailableStage)
            {
                return;
            }

            EnableSetFightParams = false;

            var stage1 = Stage1 ?? string.Empty;
            var stage2 = Stage2 ?? string.Empty;
            var stage3 = Stage3 ?? string.Empty;
            var rss = RemainingSanityStage ?? string.Empty;

            StageList = hideUnavailableStage
                ? new ObservableCollection<CombinedData>(_stageManager.GetStageList(_curDayOfWeek))
                : new ObservableCollection<CombinedData>(_stageManager.GetStageList());

            AlternateStageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList());

            RemainingSanityStageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList())
            {
                [0] = new() { Display = LocalizationHelper.GetString("NoUse"), Value = string.Empty },
            };

            // reset closed stages to "Last/Current"
            if (!CustomStageCode)
            {
                stage1 = StageList.Any(x => x.Value == stage1) ? stage1 : string.Empty;
                stage2 = AlternateStageList.Any(x => x.Value == stage2) ? stage2 : string.Empty;
                stage3 = AlternateStageList.Any(x => x.Value == stage3) ? stage3 : string.Empty;
                rss = RemainingSanityStageList.Any(x => x.Value == rss) ? rss : string.Empty;
            }
            else if (hideUnavailableStage)
            {
                stage1 = IsStageOpen(stage1) ? stage1 : string.Empty;
                stage2 = IsStageOpen(stage2) ? stage2 : string.Empty;
                stage3 = IsStageOpen(stage3) ? stage3 : string.Empty;
                rss = IsStageOpen(rss) ? rss : string.Empty;
            }

            _stage1Fallback = stage1;
            Stage1 = stage1;
            Stage2 = stage2;
            Stage3 = stage3;
            RemainingSanityStage = rss;

            EnableSetFightParams = true;
        }

        private bool NeedToUpdateDatePrompt()
        {
            var now = DateTime.UtcNow.ToYjDateTime();
            var hour = now.Hour;
            var min = now.Minute;

            // yj历的4/16点
            if (min == 0 && hour == 12)
            {
                return true;
            }

            if (_curDayOfWeek == now.DayOfWeek)
            {
                return false;
            }

            _curDayOfWeek = now.DayOfWeek;
            return true;
        }

        private static bool NeedToCheckForUpdates()
        {
            var now = DateTime.UtcNow.ToYjDateTime();
            var hour = now.Hour;
            var min = now.Minute;

            // yj历的4/22点
            return min == 0 && hour == 18;
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
            var stages = new[] { Stage1, Stage2, Stage3 };
            foreach (var stage in stages)
            {
                if (stage == null || _stageManager.GetStageInfo(stage)?.IsActivityClosed() != true)
                {
                    continue;
                }

                builder.Append(stage).Append(": ").AppendLine(LocalizationHelper.GetString("ClosedStage"));
                break;
            }

            // Open stages today
            var openStages = _stageManager.GetStageTips(_curDayOfWeek);
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

        /// <summary>
        /// Adds log.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLog(string content, string color = UiLogColor.Trace, string weight = "Regular")
        {
            Execute.OnUIThread(() =>
            {
                var log = new LogItemViewModel(content, color, weight);
                LogItemViewModels.Add(log);
                _logger.Information(content);
            });
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        private void ClearLog()
        {
            LogItemViewModels.Clear();
            _logger.Information("Main windows log clear.");
            _logger.Information(string.Empty);
        }

        /// <summary>
        /// Selects all.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void SelectedAll()
        {
            foreach (var item in TaskItemViewModels)
            {
                switch (item.OriginalName)
                {
                    case "AutoRoguelike":
                    case "ReclamationAlgorithm":
                    case "ReclamationAlgorithm2":
                        continue;
                }

                item.IsChecked = true;
            }
        }

        private bool _inverseMode = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use inverse mode.
        /// </summary>
        public bool InverseMode
        {
            get => _inverseMode;
            set
            {
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
            ConfigurationHelper.GetValue(ConfigurationKeys.InverseClearMode, "Clear") == "ClearInverse" ? SelectedAllWidthWhenBoth : 85;

        /// <summary>
        /// Gets or sets the width of "Select All".
        /// </summary>
        public int SelectedAllWidth
        {
            get => _selectedAllWidth;
            set => SetAndNotify(ref _selectedAllWidth, value);
        }

        private bool _showInverse = ConfigurationHelper.GetValue(ConfigurationKeys.InverseClearMode, "Clear") == "ClearInverse";

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
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void ChangeInverseMode()
        {
            InverseMode = !InverseMode;
        }

        /// <summary>
        /// Selects inversely.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void InverseSelected()
        {
            if (InverseMode)
            {
                foreach (var item in TaskItemViewModels)
                {
                    switch (item.OriginalName)
                    {
                        case "AutoRoguelike":
                        case "ReclamationAlgorithm":
                        case "ReclamationAlgorithm2":
                            item.IsChecked = false;
                            continue;
                    }

                    item.IsChecked = !item.IsChecked;
                }
            }
            else
            {
                foreach (var item in TaskItemViewModels)
                {
                    item.IsChecked = false;
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
                if (item.IsCheckedWithNull == null)
                {
                    item.IsChecked = false;
                }
            }
        }

        private async Task<bool> ConnectToEmulator()
        {
            string errMsg = string.Empty;
            bool connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));

            // 尝试启动模拟器
            if (!connected && Instances.SettingsViewModel.RetryOnDisconnected)
            {
                AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("TryToStartEmulator"));

                await Task.Run(() => Instances.SettingsViewModel.TryToStartEmulator(true));

                if (Stopping)
                {
                    SetStopped();
                    return false;
                }

                connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            }

            // 尝试重启 ADB
            if (!connected && Instances.SettingsViewModel.AllowAdbRestart)
            {
                AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("RestartAdb"));

                await Task.Run(() => Instances.SettingsViewModel.RestartAdb());

                if (Stopping)
                {
                    SetStopped();
                    return false;
                }

                connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            }

            // 尝试杀掉 ADB 进程
            if (!connected && Instances.SettingsViewModel.AllowAdbHardRestart)
            {
                AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("HardRestartAdb"));

                await Task.Run(() => Instances.SettingsViewModel.HardRestartAdb());

                if (Stopping)
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

        /// <summary>
        /// Starts.
        /// </summary>
        public async void LinkStart()
        {
            if (!_runningState.GetIdle())
            {
                return;
            }

            _runningState.SetIdle(false);

            Instances.SettingsViewModel.SetupSleepManagement();

            // 虽然更改时已经保存过了，不过保险起见在点击开始之后再次保存任务和基建列表
            TaskItemSelectionChanged();
            Instances.SettingsViewModel.InfrastOrderSelectionChanged();

            InfrastTaskRunning = true;

            ClearLog();

            var uiVersion = SettingsViewModel.UiVersion;
            var coreVersion = SettingsViewModel.CoreVersion;
            if (uiVersion != coreVersion &&
                Instances.VersionUpdateViewModel.IsStdVersion(uiVersion) &&
                Instances.VersionUpdateViewModel.IsStdVersion(coreVersion))
            {
                AddLog(string.Format(LocalizationHelper.GetString("VersionMismatch"), uiVersion, coreVersion), UiLogColor.Warning);
            }

            await Task.Run(() => Instances.SettingsViewModel.RunScript("StartsWithScript"));

            AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));
            if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
            {
                AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            if (!await ConnectToEmulator())
            {
                return;
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            bool taskRet = true;

            // 直接遍历TaskItemViewModels里面的内容，是排序后的
            int count = 0;
            foreach (var item in TaskItemViewModels)
            {
                if (item.IsChecked == false)
                {
                    continue;
                }

                ++count;
                switch (item.OriginalName)
                {
                    case "Base":
                        taskRet &= AppendInfrast();
                        break;

                    case "WakeUp":
                        taskRet &= AppendStart();
                        break;

                    case "Combat":
                        taskRet &= AppendFight();
                        break;

                    case "Recruiting":
                        taskRet &= AppendRecruit();
                        break;

                    case "Mall":
                        taskRet &= AppendMall();
                        break;

                    case "Mission":
                        taskRet &= AppendAward();
                        break;

                    case "AutoRoguelike":
                        taskRet &= AppendRoguelike();
                        break;

                    case "ReclamationAlgorithm":
                        taskRet &= AppendReclamation();
                        break;

                    case "ReclamationAlgorithm2":
                        taskRet &= AppendReclamation2();
                        break;

                    default:
                        --count;
                        _logger.Error("Unknown task: " + item.OriginalName);
                        break;
                }

                if (taskRet)
                {
                    continue;
                }

                AddLog(item.OriginalName + "Error", UiLogColor.Error);
                taskRet = true;
                --count;
            }

            if (count == 0)
            {
                AddLog(LocalizationHelper.GetString("UnselectedTask"));
                _runningState.SetIdle(true);
                Instances.AsstProxy.AsstStop();
                SetStopped();
                return;
            }

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
            Stopping = true;
            AddLog(LocalizationHelper.GetString("Stopping"));
            await Task.Run(() =>
            {
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
        // ReSharper disable once UnusedMember.Global
        public async void WaitAndStop()
        {
            Waiting = true;
            AddLog(LocalizationHelper.GetString("Waiting"));
            if (Instances.SettingsViewModel.RoguelikeDelayAbortUntilCombatComplete)
            {
                await WaitUntilRoguelikeCombatComplete();

                if (Instances.AsstProxy.AsstRunning() && !Stopping)
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
            while (Instances.SettingsViewModel.RoguelikeDelayAbortUntilCombatComplete && RoguelikeInCombatAndShowWait && time < 600 && !Stopping)
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
            if (Instances.SettingsViewModel.ManualStopWithScript)
            {
                Task.Run(() => Instances.SettingsViewModel.RunScript("EndsWithScript"));
            }

            if (!_runningState.GetIdle() || Stopping)
            {
                AddLog(LocalizationHelper.GetString("Stopped"));
            }

            Waiting = false;
            Stopping = false;
            _runningState.SetIdle(true);
        }

        public async void QuickSwitchAccount()
        {
            if (!_runningState.GetIdle())
            {
                return;
            }

            _runningState.SetIdle(false);

            // 虽然更改时已经保存过了，不过保险起见在点击开始之后再次保存任务和基建列表
            TaskItemSelectionChanged();
            Instances.SettingsViewModel.InfrastOrderSelectionChanged();

            ClearLog();

            await Task.Run(() => Instances.SettingsViewModel.RunScript("StartsWithScript"));

            AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));
            if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
            {
                AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            if (!await ConnectToEmulator())
            {
                return;
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            bool taskRet = true;
            taskRet &= AppendStart();

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

        private static bool AppendStart()
        {
            var mode = Instances.SettingsViewModel.ClientType;
            var enable = mode.Length != 0;
            Instances.SettingsViewModel.AccountName = Instances.SettingsViewModel.AccountName.Trim();
            var accountName = Instances.SettingsViewModel.AccountName;
            return Instances.AsstProxy.AsstAppendStartUp(mode, enable, accountName);
        }

        private bool AppendFight()
        {
            int medicine = 0;
            if (UseMedicine)
            {
                if (!int.TryParse(MedicineNumber, out medicine))
                {
                    medicine = 0;
                }
            }

            int stone = 0;
            if (UseStone)
            {
                if (!int.TryParse(StoneNumber, out stone))
                {
                    stone = 0;
                }
            }

            int times = int.MaxValue;
            if (HasTimesLimited)
            {
                if (!int.TryParse(MaxTimes, out times))
                {
                    times = 0;
                }
            }

            if (!int.TryParse(Series, out var series))
            {
                series = 1;
            }

            int dropsQuantity = 0;
            if (IsSpecifiedDrops)
            {
                if (!int.TryParse(DropsQuantity, out dropsQuantity))
                {
                    dropsQuantity = 0;
                }
            }

            string curStage = Stage;

            bool mainFightRet = Instances.AsstProxy.AsstAppendFight(curStage, medicine, stone, times, series, DropsItemId, dropsQuantity);

            if (!mainFightRet)
            {
                AddLog(LocalizationHelper.GetString("UnsupportedStages") + ": " + curStage, UiLogColor.Error);
                return false;
            }

            if ((curStage == "Annihilation") && Instances.SettingsViewModel.UseAlternateStage)
            {
                foreach (var stage in new[] { Stage1, Stage2, Stage3 })
                {
                    if (!IsStageOpen(stage) || (stage == curStage))
                    {
                        continue;
                    }

                    mainFightRet = Instances.AsstProxy.AsstAppendFight(stage, medicine, 0, int.MaxValue, series, string.Empty, 0);
                    break;
                }
            }

            if (mainFightRet && UseRemainingSanityStage && !string.IsNullOrEmpty(RemainingSanityStage))
            {
                return Instances.AsstProxy.AsstAppendFight(RemainingSanityStage, 0, 0, int.MaxValue, 1, string.Empty, 0, false);
            }

            return mainFightRet;
        }

        private bool EnableSetFightParams { get; set; } = true;

        /// <summary>
        /// Sets parameters.
        /// </summary>
        public void SetFightParams()
        {
            if (!EnableSetFightParams)
            {
                return;
            }

            int medicine = 0;
            if (UseMedicine)
            {
                if (!int.TryParse(MedicineNumber, out medicine))
                {
                    medicine = 0;
                }
            }

            int stone = 0;
            if (UseStone)
            {
                if (!int.TryParse(StoneNumber, out stone))
                {
                    stone = 0;
                }
            }

            int times = int.MaxValue;
            if (HasTimesLimited)
            {
                if (!int.TryParse(MaxTimes, out times))
                {
                    times = 0;
                }
            }

            if (!int.TryParse(Series, out var series))
            {
                series = 1;
            }

            int dropsQuantity = 0;
            if (IsSpecifiedDrops)
            {
                if (!int.TryParse(DropsQuantity, out dropsQuantity))
                {
                    dropsQuantity = 0;
                }
            }

            Instances.AsstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, series, DropsItemId, dropsQuantity);
        }

        private void SetFightRemainingSanityParams()
        {
            Instances.AsstProxy.AsstSetFightTaskParams(RemainingSanityStage, 0, 0, int.MaxValue, 1, string.Empty, 0, false);
        }

        private void SetInfrastParams()
        {
            var order = Instances.SettingsViewModel.GetInfrastOrderList();
            Instances.AsstProxy.AsstSetInfrastTaskParams(order.ToArray(), Instances.SettingsViewModel.UsesOfDrones, Instances.SettingsViewModel.ContinueTraining, Instances.SettingsViewModel.DormThreshold / 100.0, Instances.SettingsViewModel.DormFilterNotStationedEnabled, Instances.SettingsViewModel.DormTrustEnabled,
                Instances.SettingsViewModel.OriginiumShardAutoReplenishment, Instances.SettingsViewModel.CustomInfrastEnabled, Instances.SettingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool AppendInfrast()
        {
            if (Instances.SettingsViewModel.CustomInfrastEnabled && !File.Exists(Instances.SettingsViewModel.CustomInfrastFile))
            {
                AddLog(LocalizationHelper.GetString("CustomizeInfrastSelectionEmpty"), UiLogColor.Error);
                return false;
            }

            var order = Instances.SettingsViewModel.GetInfrastOrderList();
            return Instances.AsstProxy.AsstAppendInfrast(order.ToArray(), Instances.SettingsViewModel.UsesOfDrones, Instances.SettingsViewModel.ContinueTraining, Instances.SettingsViewModel.DormThreshold / 100.0, Instances.SettingsViewModel.DormFilterNotStationedEnabled, Instances.SettingsViewModel.DormTrustEnabled,
                Instances.SettingsViewModel.OriginiumShardAutoReplenishment, Instances.SettingsViewModel.CustomInfrastEnabled, Instances.SettingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private readonly Dictionary<string, IEnumerable<string>> _blackCharacterListMapping = new()
        {
            { string.Empty, new[] { "讯使", "嘉维尔", "坚雷" } },
            { "Official", new[] { "讯使", "嘉维尔", "坚雷" } },
            { "Bilibili", new[] { "讯使", "嘉维尔", "坚雷" } },
            { "YoStarEN", new[] { "Courier", "Gavial", "Dur-nar" } },
            { "YoStarJP", new[] { "クーリエ", "ガヴィル", "ジュナー" } },
            { "YoStarKR", new[] { "쿠리어", "가비알", "듀나" } },
            { "txwy", new[] { "訊使", "嘉維爾", "堅雷" } },
        };

        private bool AppendMall()
        {
            var buyFirst = Instances.SettingsViewModel.CreditFirstList.Split(';', '；')
                .Select(s => s.Trim());

            var blackList = Instances.SettingsViewModel.CreditBlackList.Split(';', '；')
                .Select(s => s.Trim());

            blackList = blackList.Union(_blackCharacterListMapping[Instances.SettingsViewModel.ClientType]);

            return Instances.AsstProxy.AsstAppendMall(
                !string.IsNullOrEmpty(this.Stage) && Instances.SettingsViewModel.CreditFightTaskEnabled,
                Instances.SettingsViewModel.CreditFightSelectFormation,
                Instances.SettingsViewModel.CreditVisitFriends,
                Instances.SettingsViewModel.CreditShopping,
                buyFirst.ToArray(),
                blackList.ToArray(),
                Instances.SettingsViewModel.CreditForceShoppingIfCreditFull,
                Instances.SettingsViewModel.CreditOnlyBuyDiscount,
                Instances.SettingsViewModel.CreditReserveMaxCredit);
        }

        private static bool AppendAward()
        {
            var receiveAward = Instances.SettingsViewModel.ReceiveAward;
            var receiveMail = Instances.SettingsViewModel.ReceiveMail;
            var receiveFreeRecruit = Instances.SettingsViewModel.ReceiveFreeRecruit;
            var receiveOrundum = Instances.SettingsViewModel.ReceiveOrundum;
            var receiveMining = Instances.SettingsViewModel.ReceiveMining;
            var receiveSpecialAccess = Instances.SettingsViewModel.ReceiveSpecialAccess;

            return Instances.AsstProxy.AsstAppendAward(receiveAward, receiveMail, receiveFreeRecruit, receiveOrundum, receiveMining, receiveSpecialAccess);
        }

        private static bool AppendRecruit()
        {
            // for debug
            if (!int.TryParse(Instances.SettingsViewModel.RecruitMaxTimes, out var maxTimes))
            {
                maxTimes = 0;
            }

            var firstList = Instances.SettingsViewModel.AutoRecruitFirstList.Split(';', '；')
                .Select(s => s.Trim());

            var reqList = new List<int>();
            var cfmList = new List<int>();

            if (Instances.SettingsViewModel.ChooseLevel3)
            {
                cfmList.Add(3);
            }

            if (Instances.SettingsViewModel.ChooseLevel4)
            {
                reqList.Add(4);
                cfmList.Add(4);
            }

            // ReSharper disable once InvertIf
            if (Instances.SettingsViewModel.ChooseLevel5)
            {
                reqList.Add(5);
                cfmList.Add(5);
            }

            _ = int.TryParse(Instances.SettingsViewModel.SelectExtraTags, out var selectExtra);

            return Instances.AsstProxy.AsstAppendRecruit(
                maxTimes,
                firstList.ToArray(),
                [.. reqList],
                [.. cfmList],
                Instances.SettingsViewModel.RefreshLevel3,
                Instances.SettingsViewModel.ForceRefresh,
                Instances.SettingsViewModel.UseExpedited,
                selectExtra,
                Instances.SettingsViewModel.NotChooseLevel1,
                Instances.SettingsViewModel.IsLevel3UseShortTime,
                Instances.SettingsViewModel.IsLevel3UseShortTime2);
        }

        private static bool AppendRoguelike()
        {
            _ = int.TryParse(Instances.SettingsViewModel.RoguelikeMode, out var mode);

            return Instances.AsstProxy.AsstAppendRoguelike(
                mode,
                Instances.SettingsViewModel.RoguelikeStartsCount,
                Instances.SettingsViewModel.RoguelikeInvestmentEnabled,
                Instances.SettingsViewModel.RoguelikeInvestmentWithMoreScore,
                Instances.SettingsViewModel.RoguelikeInvestsCount,
                Instances.SettingsViewModel.RoguelikeStopWhenInvestmentFull,
                Instances.SettingsViewModel.RoguelikeSquad,
                Instances.SettingsViewModel.RoguelikeRoles,
                DataHelper.GetCharacterByNameOrAlias(Instances.SettingsViewModel.RoguelikeCoreChar)?.Name ?? Instances.SettingsViewModel.RoguelikeCoreChar,
                Instances.SettingsViewModel.RoguelikeStartWithEliteTwo,
                Instances.SettingsViewModel.RoguelikeOnlyStartWithEliteTwo,
                Instances.SettingsViewModel.Roguelike3FirstFloorFoldartal,
                Instances.SettingsViewModel.Roguelike3StartFloorFoldartal,
                Instances.SettingsViewModel.Roguelike3NewSquad2StartingFoldartal,
                Instances.SettingsViewModel.Roguelike3NewSquad2StartingFoldartals,
                Instances.SettingsViewModel.RoguelikeUseSupportUnit,
                Instances.SettingsViewModel.RoguelikeEnableNonfriendSupport,
                Instances.SettingsViewModel.RoguelikeTheme,
                Instances.SettingsViewModel.RoguelikeRefreshTraderWithDice,
                Instances.SettingsViewModel.RoguelikeSkipTeamCompleteCheck);
        }

        private static bool AppendReclamation()
        {
            return Instances.AsstProxy.AsstAppendReclamation();
        }

        private static bool AppendReclamation2()
        {
            return Instances.AsstProxy.AsstAppendReclamation2(
                Instances.SettingsViewModel.Reclamation2ExEnable ? 1 : 0,
                Instances.SettingsViewModel.Reclamation2ExProduct);
        }

        [DllImport("User32.dll", EntryPoint = "FindWindow")]
        private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("User32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);

        /// <summary>
        /// 一个根据连接配置判断使用关闭模拟器的方式的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorModeSwitcher()
        {
            try
            {
                string emulatorMode = Instances.SettingsViewModel.ConnectConfig;
                Instances.AsstProxy.Connected = false;
                return emulatorMode switch
                {
                    "Nox" => KillEmulatorNox(),
                    "LDPlayer" => KillEmulatorLdPlayer(),
                    "XYAZ" => KillEmulatorXyaz(),
                    "BlueStacks" => KillEmulatorBlueStacks(),
                    "MuMuEmulator12" => KillEmulatorMuMuEmulator12(),
                    _ => KillEmulatorByWindow(),
                };
            }
            catch (Exception e)
            {
                _logger.Error("Failed to close emulator: " + e.Message);
                return false;
            }
        }

        /// <summary>
        /// 一个用于调用 MuMu12 模拟器控制台关闭 MuMu12 的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorMuMuEmulator12()
        {
            string address = Instances.SettingsViewModel.ConnectAddress;
            int emuIndex;
            if (address == "127.0.0.1:16384")
            {
                emuIndex = 0;
            }
            else
            {
                string portStr = address.Split(':')[1];
                int port = int.Parse(portStr);
                emuIndex = (port - 16384) / 32;
            }

            Process[] processes = Process.GetProcessesByName("MuMuPlayer");
            if (processes.Length <= 0)
            {
                return false;
            }

            ProcessModule processModule;
            try
            {
                processModule = processes[0].MainModule;
            }
            catch (Exception e)
            {
                _logger.Error("Error: Failed to get the main module of the emulator process.");
                _logger.Error(e.Message);
                return false;
            }

            if (processModule == null)
            {
                return false;
            }

            string emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation == null)
            {
                return false;
            }

            string consolePath = Path.Combine(emuLocation, "MuMuManager.exe");

            if (File.Exists(consolePath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
                {
                    Arguments = $"api -v {emuIndex} shutdown_player",
                    CreateNoWindow = true,
                    UseShellExecute = false,
                };
                var process = Process.Start(startInfo);
                if (process != null && process.WaitForExit(5000))
                {
                    _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                    return KillEmulator();
                }

                _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                return KillEmulatorByWindow();
            }

            _logger.Error($"Error: `{consolePath}` not found, try to kill emulator by window.");
            return KillEmulatorByWindow();
        }

        /// <summary>
        /// 一个用于调用雷电模拟器控制台关闭雷电模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorLdPlayer()
        {
            string address = Instances.SettingsViewModel.ConnectAddress;
            int emuIndex;
            if (address.Contains(":"))
            {
                string portStr = address.Split(':')[1];
                int port = int.Parse(portStr);
                emuIndex = (port - 5555) / 2;
            }
            else
            {
                string portStr = address.Split('-')[1];
                int port = int.Parse(portStr);
                emuIndex = (port - 5554) / 2;
            }

            Process[] processes = Process.GetProcessesByName("dnplayer");
            if (processes.Length <= 0)
            {
                return false;
            }

            ProcessModule processModule;
            try
            {
                processModule = processes[0].MainModule;
            }
            catch (Exception e)
            {
                _logger.Error("Error: Failed to get the main module of the emulator process.");
                _logger.Error(e.Message);
                return false;
            }

            if (processModule == null)
            {
                return false;
            }

            string emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation == null)
            {
                return false;
            }

            string consolePath = Path.Combine(emuLocation, "ldconsole.exe");

            if (File.Exists(consolePath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
                {
                    Arguments = $"quit --index {emuIndex}",
                    CreateNoWindow = true,
                    UseShellExecute = false,
                };
                var process = Process.Start(startInfo);
                if (process != null && process.WaitForExit(5000))
                {
                    _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                    return KillEmulator();
                }

                _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                return KillEmulatorByWindow();
            }

            _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
            return KillEmulatorByWindow();
        }

        /// <summary>
        /// 一个用于调用夜神模拟器控制台关闭夜神模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorNox()
        {
            string address = Instances.SettingsViewModel.ConnectAddress;
            int emuIndex;
            if (address == "127.0.0.1:62001")
            {
                emuIndex = 0;
            }
            else
            {
                string portStr = address.Split(':')[1];
                int port = int.Parse(portStr);
                emuIndex = port - 62024;
            }

            Process[] processes = Process.GetProcessesByName("Nox");
            if (processes.Length <= 0)
            {
                return false;
            }

            ProcessModule processModule;
            try
            {
                processModule = processes[0].MainModule;
            }
            catch (Exception e)
            {
                _logger.Error("Error: Failed to get the main module of the emulator process.");
                _logger.Error(e.Message);
                return false;
            }

            if (processModule == null)
            {
                return false;
            }

            string emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation == null)
            {
                return false;
            }

            string consolePath = Path.Combine(emuLocation, "NoxConsole.exe");

            if (File.Exists(consolePath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
                {
                    Arguments = $"quit -index:{emuIndex}",
                    CreateNoWindow = true,
                    UseShellExecute = false,
                };
                var process = Process.Start(startInfo);
                if (process != null && process.WaitForExit(5000))
                {
                    _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                    return KillEmulator();
                }

                _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                return KillEmulatorByWindow();
            }

            _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
            return KillEmulatorByWindow();
        }

        /// <summary>
        /// 一个用于调用逍遥模拟器控制台关闭逍遥模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorXyaz()
        {
            string address = Instances.SettingsViewModel.ConnectAddress;
            string portStr = address.Split(':')[1];
            int port = int.Parse(portStr);
            var emuIndex = (port - 21503) / 10;

            Process[] processes = Process.GetProcessesByName("MEmu");
            if (processes.Length <= 0)
            {
                return false;
            }

            ProcessModule processModule;
            try
            {
                processModule = processes[0].MainModule;
            }
            catch (Exception e)
            {
                _logger.Error("Error: Failed to get the main module of the emulator process.");
                _logger.Error(e.Message);
                return false;
            }

            if (processModule == null)
            {
                return false;
            }

            string emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation == null)
            {
                return false;
            }

            string consolePath = Path.Combine(emuLocation, "memuc.exe");

            if (File.Exists(consolePath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(consolePath)
                {
                    Arguments = $"stop -i {emuIndex}",
                    CreateNoWindow = true,
                    UseShellExecute = false,
                };
                var process = Process.Start(startInfo);
                if (process != null && process.WaitForExit(5000))
                {
                    _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                    return KillEmulator();
                }

                _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                return KillEmulatorByWindow();
            }

            _logger.Information($"Error: `{consolePath}` not found, try to kill emulator by window.");
            return KillEmulatorByWindow();
        }

        /// <summary>
        /// 一个用于关闭蓝叠模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        private static bool KillEmulatorBlueStacks()
        {
            Process[] processes = Process.GetProcessesByName("HD-Player");
            if (processes.Length <= 0)
            {
                return false;
            }

            ProcessModule processModule;
            try
            {
                processModule = processes[0].MainModule;
            }
            catch (Exception e)
            {
                _logger.Error("Error: Failed to get the main module of the emulator process.");
                _logger.Error(e.Message);
                return false;
            }

            if (processModule == null)
            {
                return false;
            }

            string emuLocation = processModule.FileName;
            emuLocation = Path.GetDirectoryName(emuLocation);
            if (emuLocation == null)
            {
                return false;
            }

            string consolePath = Path.Combine(emuLocation, "bsconsole.exe");

            if (File.Exists(consolePath))
            {
                _logger.Information($"Info: `{consolePath}` has been found. This may be the BlueStacks China emulator, try to kill the emulator by window.");
                return KillEmulatorByWindow();
            }

            _logger.Information($"Info: `{consolePath}` not found. This may be the BlueStacks International emulator, try to kill the emulator by the port.");
            if (KillEmulator())
            {
                return true;
            }

            _logger.Information("Info: Failed to kill emulator by the port, try to kill emulator process with PID.");

            if (processes.Length > 1)
            {
                _logger.Warning("Warning: The number of elements in processes exceeds one, abort closing the emulator");
                return false;
            }

            try
            {
                processes[0].Kill();
                return processes[0].WaitForExit(20000);
            }
            catch (Exception ex)
            {
                _logger.Error($"Error: Failed to kill emulator process with PID {processes[0].Id}. Exception: {ex.Message}");
            }

            return false;
        }

        /// <summary>
        /// Kills emulator by Window hwnd.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        private static bool KillEmulatorByWindow()
        {
            int pid = 0;
            var windowName = new[]
            {
                "明日方舟",
                "明日方舟 - MuMu模拟器",
                "BlueStacks App Player",
                "BlueStacks",
            };
            foreach (string i in windowName)
            {
                var hwnd = FindWindow(null, i);
                if (hwnd == IntPtr.Zero)
                {
                    continue;
                }

                GetWindowThreadProcessId(hwnd, out pid);
                break;
            }

            if (pid == 0)
            {
                return KillEmulator();
            }

            try
            {
                var emulator = Process.GetProcessById(pid);
                emulator.CloseMainWindow();
                if (!emulator.WaitForExit(5000))
                {
                    emulator.Kill();
                    if (emulator.WaitForExit(5000))
                    {
                        _logger.Information($"Emulator with process ID {pid} killed successfully.");
                        KillEmulator();
                        return true;
                    }

                    _logger.Error($"Failed to kill emulator with process ID {pid}.");
                    return false;
                }

                // 尽管已经成功 CloseMainWindow()，再次尝试 killEmulator()
                // Refer to https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/1878
                KillEmulator();

                // 已经成功 CloseMainWindow()，所以不管 killEmulator() 的结果如何，都返回 true
                return true;
            }
            catch
            {
                _logger.Error("Kill emulator by window error");
            }

            return KillEmulator();
        }

        /// <summary>
        /// Kills emulator.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool KillEmulator()
        {
            int pid = 0;
            string address = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);
            var port = address.StartsWith("127") ? address.Substring(10) : "5555";
            _logger.Information($"address: {address}, port: {port}");

            string portCmd = "netstat -ano|findstr \"" + port + "\"";
            Process checkCmd = new Process
            {
                StartInfo =
                {
                    FileName = "cmd.exe",
                    UseShellExecute = false,
                    RedirectStandardInput = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                },
            };
            try
            {
                checkCmd.Start();
            }
            catch
            {
                _logger.Error("Failed to start cmd.exe");
                checkCmd.Close();
                return false;
            }

            checkCmd.StandardInput.WriteLine(portCmd);
            checkCmd.StandardInput.WriteLine("exit");
            Regex reg = new Regex("\\s+", RegexOptions.Compiled);
            while (true)
            {
                var line = checkCmd.StandardOutput.ReadLine();
                line = line?.Trim();

                if (line == null)
                {
                    break;
                }

                if (!line.StartsWith("TCP", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                line = reg.Replace(line, ",");

                try
                {
                    string[] arr = line.Split(',');
                    if (arr.Length >= 2
                        && Convert.ToBoolean(string.Compare(arr[1], address, StringComparison.Ordinal)))
                    {
                        continue;
                    }

                    pid = int.Parse(arr[4]);
                    break;
                }
                catch (Exception e)
                {
                    _logger.Error("Failed to parse cmd.exe output: " + e.Message);
                }
            }

            if (pid == 0)
            {
                _logger.Error("Failed to get emulator PID");
                return false;
            }

            try
            {
                Process emulator = Process.GetProcessById(pid);
                emulator.Kill();
            }
            catch
            {
                return false;
            }
            finally
            {
                checkCmd.Close();
            }

            return true;
        }

        /// <summary>
        /// The action type.
        /// </summary>
        public enum ActionType
        {
            /// <summary>
            /// Does nothing.
            /// </summary>
            DoNothing,

            /// <summary>
            /// Stops game.
            /// </summary>
            StopGame,

            /// <summary>
            /// Exits MAA.
            /// </summary>
            ExitSelf,

            /// <summary>
            /// Exits emulator.
            /// </summary>
            ExitEmulator,

            /// <summary>
            /// Exits MAA and emulator.
            /// </summary>
            ExitEmulatorAndSelf,

            /// <summary>
            /// Computer suspends.
            /// </summary>
            Suspend,

            /// <summary>
            /// Computer hibernates.
            /// </summary>
            Hibernate,

            /// <summary>
            /// Exits MAA and emulator and computer hibernates.
            /// </summary>
            ExitEmulatorAndSelfAndHibernate,

            /// <summary>
            /// Computer shutdown.
            /// </summary>
            Shutdown,

            /// <summary>
            /// Computer hibernates without Persist.
            /// </summary>
            HibernateWithoutPersist,

            /// <summary>
            /// Exits MAA and emulator and computer hibernates without Persist.
            /// </summary>
            ExitEmulatorAndSelfAndHibernateWithoutPersist,

            /// <summary>
            /// Computer shutdown without Persist.
            /// </summary>
            ShutdownWithoutPersist,

            /// <summary>
            /// Exits MAA and emulator and, if no other processes of MAA are running, computer hibernates.
            /// </summary>
            ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate,

            /// <summary>
            /// Exits MAA and, if no other processes of MAA are running, computer shutdown.
            /// </summary>
            ExitSelfIfOtherMaaElseShutdown,

            /// <summary>
            /// Switch the game to background without killing it.
            /// </summary>
            BackToAndroidHome,
        }

        /// <summary>
        /// Checks after completion.
        /// </summary>
        public async void CheckAfterCompleted()
        {
            await Task.Run(() => Instances.SettingsViewModel.RunScript("EndsWithScript"));

            switch (ActionAfterCompleted)
            {
                case ActionType.DoNothing:
                    break;

                case ActionType.StopGame:
                    if (!Instances.AsstProxy.AsstStartCloseDown())
                    {
                        AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
                    }

                    break;

                case ActionType.ExitSelf:
                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Bootstrapper.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.ExitEmulator:
                    if (!KillEmulatorModeSwitcher())
                    {
                        AddLog(LocalizationHelper.GetString("ExitEmulatorFailed"), UiLogColor.Error);
                    }

                    break;

                case ActionType.ExitEmulatorAndSelf:
                    if (!KillEmulatorModeSwitcher())
                    {
                        AddLog(LocalizationHelper.GetString("ExitEmulatorFailed"), UiLogColor.Error);
                    }

                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Bootstrapper.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.Shutdown:
                case ActionType.ShutdownWithoutPersist:
                    Process.Start("shutdown.exe", "-s -t 60");

                    // 关机询问
                    var shutdownResult = MessageBoxHelper.Show(LocalizationHelper.GetString("AboutToShutdown"), LocalizationHelper.GetString("ShutdownPrompt"), MessageBoxButton.OK, MessageBoxImage.Question, ok: LocalizationHelper.GetString("Cancel"));
                    if (shutdownResult == MessageBoxResult.OK)
                    {
                        Process.Start("shutdown.exe", "-a");
                    }

                    break;

                case ActionType.Suspend:
                    Process.Start("powercfg", "-h off");
                    Process.Start("rundll32.exe", "powrprof.dll,SetSuspendState 0,1,0");
                    Process.Start("powercfg", "-h on");
                    break;

                case ActionType.HibernateWithoutPersist:
                    // 休眠不会导致 MAA 重启，下次执行的还会是休眠
                    // 重新读取结束后动作，并刷新UI
                    _actionAfterCompleted = ConfigurationHelper.GetValue(ConfigurationKeys.ActionAfterCompleted, ActionType.DoNothing.ToString());
                    NotifyOfPropertyChange(nameof(ActionAfterCompleted));
                    goto case ActionType.Hibernate;
                case ActionType.Hibernate:
                    // 休眠提示
                    AddLog(LocalizationHelper.GetString("HibernatePrompt"), UiLogColor.Error);

                    // 休眠不能加时间参数，https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1133
                    Process.Start("shutdown.exe", "-h");
                    break;

                case ActionType.ExitEmulatorAndSelfAndHibernate:
                case ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist:
                    if (!KillEmulatorModeSwitcher())
                    {
                        AddLog(LocalizationHelper.GetString("ExitEmulatorFailed"), UiLogColor.Error);
                    }

                    // 休眠提示
                    AddLog(LocalizationHelper.GetString("HibernatePrompt"), UiLogColor.Error);

                    // 休眠不能加时间参数，https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1133
                    Process.Start("shutdown.exe", "-h");

                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Bootstrapper.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate:
                    if (Process.GetProcessesByName("MAA").Length > 1)
                    {
                        goto case ActionType.ExitEmulatorAndSelf;
                    }
                    else
                    {
                        goto case ActionType.ExitEmulatorAndSelfAndHibernate;
                    }

                case ActionType.ExitSelfIfOtherMaaElseShutdown:
                    if (Process.GetProcessesByName("MAA").Length > 1)
                    {
                        goto case ActionType.ExitSelf;
                    }
                    else
                    {
                        goto case ActionType.Shutdown;
                    }

                case ActionType.BackToAndroidHome:
                    Instances.AsstProxy.AsstBackToHome();
                    break;

                default:
                    Execute.OnUIThread(() =>
                    {
                        using var toast = new ToastNotification(LocalizationHelper.GetString("UnknownActionAfterCompleted"));
                        toast.Show();
                    });

                    _logger.Error($"Unknown ActionAfterCompleted: {ActionAfterCompleted}");
                    break;
            }
        }

        /// <summary>
        /// Gets a value indicating whether it is initialized.
        /// </summary>
        public bool Inited { get; private set; }

        /// <summary>
        /// Sets it initialized.
        /// </summary>
        public void SetInited()
        {
            Inited = true;
            NotifyOfPropertyChange(nameof(Inited));
        }

        private bool _idle;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set
            {
                SetAndNotify(ref _idle, value);
                if (!value)
                {
                    return;
                }

                FightTaskRunning = false;
                InfrastTaskRunning = false;
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
            // ReSharper disable once UnusedMember.Global
            get => _waiting;
            private set => SetAndNotify(ref _waiting, value);
        }

        private bool _fightTaskRunning;

        /// <summary>
        /// Gets or sets a value indicating whether the battle task is running.
        /// </summary>
        public bool FightTaskRunning
        {
            get => _fightTaskRunning;
            set => SetAndNotify(ref _fightTaskRunning, value);
        }

        private bool _infrastTaskRunning;

        public bool InfrastTaskRunning
        {
            get => _infrastTaskRunning;
            set => SetAndNotify(ref _infrastTaskRunning, value);
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

        /// <summary>
        /// Gets or private sets the list of series.
        /// </summary>
        public List<string> SeriesList { get; private set; } = ["1", "2", "3", "4", "5", "6"];

        private ObservableCollection<CombinedData> _stageList = new();

        /// <summary>
        /// Gets or private sets the list of stages.
        /// </summary>
        public ObservableCollection<CombinedData> StageList
        {
            get => _stageList;
            private set => SetAndNotify(ref _stageList, value);
        }

        public ObservableCollection<CombinedData> RemainingSanityStageList { get; private set; } = new();

        public ObservableCollection<CombinedData> AlternateStageList { get; private set; } = new();

        /// <summary>
        /// Gets the stage.
        /// </summary>
        public string Stage
        {
            get
            {
                Stage1 ??= _stage1Fallback;

                if (!Instances.SettingsViewModel.UseAlternateStage)
                {
                    return Stage1;
                }

                if (IsStageOpen(Stage1))
                {
                    return Stage1;
                }

                if (IsStageOpen(Stage2))
                {
                    return Stage2;
                }

                return IsStageOpen(Stage3) ? Stage3 : Stage1;
            }
        }

        private readonly Dictionary<string, string> _stageDictionary = new()
        {
            { "AN", "Annihilation" },
            { "剿灭", "Annihilation" },
            { "CE", "CE-6" },
            { "龙门币", "CE-6" },
            { "LS", "LS-6" },
            { "经验", "LS-6" },
            { "狗粮", "LS-6" },
            { "CA", "CA-5" },
            { "技能", "CA-5" },
            { "AP", "AP-5" },
            { "红票", "AP-5" },
            { "SK", "SK-5" },
            { "碳", "SK-5" },
            { "炭", "SK-5" },
        };

        private string ToUpperAndCheckStage(string value)
        {
            if (string.IsNullOrEmpty(value))
            {
                return value;
            }

            string upperValue = value.ToUpper();
            if (_stageDictionary.TryGetValue(upperValue, out var stage))
            {
                return stage;
            }

            if (StageList == null)
            {
                return value;
            }

            foreach (var item in StageList)
            {
                if (upperValue == item.Value.ToUpper() || upperValue == item.Display.ToUpper())
                {
                    return item.Value;
                }
            }

            return value;
        }

        /// <remarks>Try to fix: issues#5742. 关卡选择为 null 时的一个补丁，可能是 StageList 改变后，wpf binding 延迟更新的问题。</remarks>
        private string _stage1Fallback = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;

        private string _stage1 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;

        /// <summary>
        /// Gets or sets the stage1.
        /// </summary>
        public string Stage1
        {
            get => _stage1;
            set
            {
                if (_stage1 == value)
                {
                    SetAndNotify(ref _stage1, value);
                    return;
                }

                if (CustomStageCode)
                {
                    // 从后往前删
                    if (_stage1?.Length != 3 && value != null)
                    {
                        value = ToUpperAndCheckStage(value);
                    }
                }

                SetAndNotify(ref _stage1, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.Stage1, value);
                UpdateDatePrompt();
            }
        }

        private string _stage2 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage2, string.Empty) ?? string.Empty;

        /// <summary>
        /// Gets or sets the stage2.
        /// </summary>
        public string Stage2
        {
            get => _stage2;
            set
            {
                if (_stage2 == value)
                {
                    SetAndNotify(ref _stage2, value);
                    return;
                }

                if (CustomStageCode)
                {
                    if (_stage2?.Length != 3 && value != null)
                    {
                        value = ToUpperAndCheckStage(value);
                    }
                }

                SetAndNotify(ref _stage2, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.Stage2, value);
                UpdateDatePrompt();
            }
        }

        private string _stage3 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage3, string.Empty) ?? string.Empty;

        /// <summary>
        /// Gets or sets the stage2.
        /// </summary>
        public string Stage3
        {
            get => _stage3;
            set
            {
                if (_stage3 == value)
                {
                    SetAndNotify(ref _stage3, value);
                    return;
                }

                if (CustomStageCode)
                {
                    if (_stage3?.Length != 3 && value != null)
                    {
                        value = ToUpperAndCheckStage(value);
                    }
                }

                SetAndNotify(ref _stage3, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.Stage3, value);
                UpdateDatePrompt();
            }
        }

        private bool _useAlternateStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseAlternateStage, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use alternate stage.
        /// </summary>
        public bool UseAlternateStage
        {
            get => _useAlternateStage;
            set => SetAndNotify(ref _useAlternateStage, value);
        }

        private bool _useRemainingSanityStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, bool.TrueString));

        public bool UseRemainingSanityStage
        {
            get => _useRemainingSanityStage;
            set => SetAndNotify(ref _useRemainingSanityStage, value);
        }

        private bool _customStageCode = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomStageCode, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use custom stage code.
        /// </summary>
        public bool CustomStageCode
        {
            get => _customStageCode;
            set => SetAndNotify(ref _customStageCode, value);
        }

        private string _remainingSanityStage = ConfigurationHelper.GetValue(ConfigurationKeys.RemainingSanityStage, string.Empty) ?? string.Empty;

        public string RemainingSanityStage
        {
            get => _remainingSanityStage;
            set
            {
                if (_remainingSanityStage == value)
                {
                    SetAndNotify(ref _remainingSanityStage, value);
                    return;
                }

                if (CustomStageCode)
                {
                    if (_remainingSanityStage?.Length != 3 && value != null)
                    {
                        value = ToUpperAndCheckStage(value);
                    }
                }

                SetAndNotify(ref _remainingSanityStage, value);
                SetFightRemainingSanityParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.RemainingSanityStage, value);
            }
        }

        private bool _customInfrastEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastEnabled, bool.FalseString));

        public bool CustomInfrastEnabled
        {
            get => _customInfrastEnabled;
            set
            {
                SetAndNotify(ref _customInfrastEnabled, value);
                RefreshCustomInfrastPlan();
            }
        }

        public bool NeedAddCustomInfrastPlanInfo { get; set; } = true;

        private int _customInfrastPlanIndex = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanIndex, "0"));

        public int CustomInfrastPlanIndex
        {
            get => _customInfrastPlanIndex;
            set
            {
                if (value != _customInfrastPlanIndex && NeedAddCustomInfrastPlanInfo)
                {
                    var plan = CustomInfrastPlanInfoList[value];
                    AddLog(plan.Name, UiLogColor.Message);

                    foreach (var period in plan.PeriodList)
                    {
                        AddLog($"[ {period.BeginHour:D2}:{period.BeginMinute:D2} - {period.EndHour:D2}:{period.EndMinute:D2} ]");
                    }

                    if (plan.Description != string.Empty)
                    {
                        AddLog(plan.Description);
                    }
                }

                SetAndNotify(ref _customInfrastPlanIndex, value);
                SetInfrastParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanIndex, value.ToString());
            }
        }

        public ObservableCollection<GenericCombinedData<int>> CustomInfrastPlanList { get; } = new();

        public struct CustomInfrastPlanInfo
        {
            // ReSharper disable InconsistentNaming
            public int Index;

            public string Name;
            public string Description;
            public string DescriptionPost;

            // 有效时间段
            public struct Period
            {
                public int BeginHour;
                public int BeginMinute;
                public int EndHour;
                public int EndMinute;
            }

            public List<Period> PeriodList;

            // ReSharper restore InconsistentNaming
        }

        private List<CustomInfrastPlanInfo> CustomInfrastPlanInfoList { get; } = new();

        private bool _customInfrastPlanHasPeriod;
        private bool _customInfrastInfoOutput;

        public void RefreshCustomInfrastPlan()
        {
            CustomInfrastPlanInfoList.Clear();
            CustomInfrastPlanList.Clear();
            _customInfrastPlanHasPeriod = false;

            if (!CustomInfrastEnabled)
            {
                return;
            }

            if (!File.Exists(Instances.SettingsViewModel.CustomInfrastFile))
            {
                return;
            }

            try
            {
                string jsonStr = File.ReadAllText(Instances.SettingsViewModel.CustomInfrastFile);
                var root = (JObject)JsonConvert.DeserializeObject(jsonStr);

                if (root != null && _customInfrastInfoOutput && root.TryGetValue("title", out var title))
                {
                    AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message);
                    AddLog($"title: {title}", UiLogColor.Info);
                    if (root.TryGetValue("description", out var value))
                    {
                        AddLog($"description: {value}", UiLogColor.Info);
                    }
                }

                var planList = (JArray)root?["plans"];
                if (planList != null)
                {
                    for (int i = 0; i < planList.Count; ++i)
                    {
                        var plan = (JObject)planList[i];
                        string display = plan.TryGetValue("name", out var name) ? name.ToString() : ("Plan " + ((char)('A' + i)));
                        CustomInfrastPlanList.Add(new GenericCombinedData<int> { Display = display, Value = i });
                        string desc = plan.TryGetValue("description", out var description) ? description.ToString() : string.Empty;
                        string descPost = plan.TryGetValue("description_post", out var descriptionPost) ? descriptionPost.ToString() : string.Empty;

                        if (_customInfrastInfoOutput)
                        {
                            AddLog(display, UiLogColor.Message);
                        }

                        var periodList = new List<CustomInfrastPlanInfo.Period>();
                        if (plan.TryGetValue("period", out var token))
                        {
                            var periodArray = (JArray)token;
                            foreach (var periodJson in periodArray)
                            {
                                var period = default(CustomInfrastPlanInfo.Period);
                                string beginTime = periodJson[0]?.ToString();
                                if (beginTime != null)
                                {
                                    var beginSplit = beginTime.Split(':');
                                    period.BeginHour = int.Parse(beginSplit[0]);
                                    period.BeginMinute = int.Parse(beginSplit[1]);
                                }

                                string endTime = periodJson[1]?.ToString();
                                if (endTime != null)
                                {
                                    var endSplit = endTime.Split(':');
                                    period.EndHour = int.Parse(endSplit[0]);
                                    period.EndMinute = int.Parse(endSplit[1]);
                                }

                                periodList.Add(period);
                                if (_customInfrastInfoOutput)
                                {
                                    AddLog($"[ {period.BeginHour:D2}:{period.BeginMinute:D2} - {period.EndHour:D2}:{period.EndMinute:D2} ]");
                                }
                            }

                            if (periodList.Count != 0)
                            {
                                _customInfrastPlanHasPeriod = true;
                            }
                        }

                        if (_customInfrastInfoOutput && desc != string.Empty)
                        {
                            AddLog(desc);
                        }

                        if (_customInfrastInfoOutput && descPost != string.Empty)
                        {
                            AddLog(descPost);
                        }

                        CustomInfrastPlanInfoList.Add(new CustomInfrastPlanInfo
                        {
                            Index = i,
                            Name = display,
                            Description = desc,
                            DescriptionPost = descPost,
                            PeriodList = periodList,
                        });
                    }
                }

                _customInfrastInfoOutput = true;
            }
            catch (Exception)
            {
                _customInfrastInfoOutput = true;
                AddLog(LocalizationHelper.GetString("CustomInfrastFileParseFailed"), UiLogColor.Error);
                return;
            }

            RefreshCustomInfrastPlanIndexByPeriod();
        }

        public void RefreshCustomInfrastPlanIndexByPeriod()
        {
            if (!CustomInfrastEnabled || !_customInfrastPlanHasPeriod || InfrastTaskRunning)
            {
                return;
            }

            var now = DateTime.Now;
            foreach (var plan in CustomInfrastPlanInfoList.Where(
                plan => plan.PeriodList.Any(
                    period => TimeLess(period.BeginHour, period.BeginMinute, now.Hour, now.Minute)
                        && TimeLess(now.Hour, now.Minute, period.EndHour, period.EndMinute))))
            {
                CustomInfrastPlanIndex = plan.Index;
                return;
            }

            if (CustomInfrastPlanIndex >= CustomInfrastPlanList.Count || CustomInfrastPlanList.Count < 0)
            {
                CustomInfrastPlanIndex = 0;
            }

            return;

            static bool TimeLess(int lHour, int lMin, int rHour, int rMin) => (lHour != rHour) ? (lHour < rHour) : (lMin <= rMin);
        }

        public void IncreaseCustomInfrastPlanIndex()
        {
            if (!CustomInfrastEnabled || _customInfrastPlanHasPeriod)
            {
                return;
            }

            AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
            var prePlanPostDesc = CustomInfrastPlanInfoList[CustomInfrastPlanIndex].DescriptionPost;
            if (prePlanPostDesc != string.Empty)
            {
                AddLog(prePlanPostDesc);
            }

            if (CustomInfrastPlanIndex >= CustomInfrastPlanList.Count - 1)
            {
                CustomInfrastPlanIndex = 0;
            }
            else
            {
                ++CustomInfrastPlanIndex;
            }
        }

        /// <summary>
        /// Reset unsaved battle parameters.
        /// </summary>
        public void ResetFightVariables()
        {
            UseStone = false;

            if (UseMedicineWithNull == null)
            {
                UseMedicine = false;
            }

            if (HasTimesLimitedWithNull == null)
            {
                HasTimesLimited = false;
            }

            if (IsSpecifiedDropsWithNull == null)
            {
                IsSpecifiedDrops = false;
            }
        }

        private bool? _useMedicineWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicine, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use medicine with null.
        /// </summary>
        public bool? UseMedicineWithNull
        {
            get => _useMedicineWithNull;
            set
            {
                SetAndNotify(ref _useMedicineWithNull, value);
                if (value == false)
                {
                    UseStone = false;
                }

                SetFightParams();
                value ??= false;
                ConfigurationHelper.SetValue(ConfigurationKeys.UseMedicine, value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use medicine.
        /// </summary>
        private bool UseMedicine
        {
            get => UseMedicineWithNull != false;
            set => UseMedicineWithNull = value;
        }

        private string _medicineNumber = ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicineQuantity, "999");

        /// <summary>
        /// Gets or sets the amount of medicine used.
        /// </summary>
        public string MedicineNumber
        {
            get => _medicineNumber;
            set
            {
                if (_medicineNumber == value)
                {
                    return;
                }

                SetAndNotify(ref _medicineNumber, value);

                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.UseMedicineQuantity, MedicineNumber);
            }
        }

        private bool? _useStoneWithNull = false;

        /// <summary>
        /// Gets or sets a value indicating whether to use originiums with null.
        /// </summary>
        public bool? UseStoneWithNull
        {
            get => _useStoneWithNull;
            set
            {
                SetAndNotify(ref _useStoneWithNull, value);
                if (value != false)
                {
                    MedicineNumber = "999";
                    if (!UseMedicine)
                    {
                        UseMedicineWithNull = null;
                    }
                }

                NotifyOfPropertyChange(nameof(UseStone));

                SetFightParams();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use originiums.
        /// </summary>
        // ReSharper disable once MemberCanBePrivate.Global
        public bool UseStone
        {
            get => UseStoneWithNull != false;
            set => UseStoneWithNull = value;
        }

        private string _stoneNumber = ConfigurationHelper.GetValue(ConfigurationKeys.UseStoneQuantity, "0");

        /// <summary>
        /// Gets or sets the amount of originiums used.
        /// </summary>
        public string StoneNumber
        {
            get => _stoneNumber;
            set
            {
                if (_stoneNumber == value)
                {
                    return;
                }

                SetAndNotify(ref _stoneNumber, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.UseStoneQuantity, StoneNumber);
            }
        }

        private bool? _hasTimesLimitedWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimited, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether the number of times is limited with null.
        /// </summary>
        public bool? HasTimesLimitedWithNull
        {
            get => _hasTimesLimitedWithNull;
            set
            {
                SetAndNotify(ref _hasTimesLimitedWithNull, value);
                SetFightParams();
                value ??= false;
                ConfigurationHelper.SetValue(ConfigurationKeys.TimesLimited, value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the number of times is limited.
        /// </summary>
        private bool HasTimesLimited
        {
            get => HasTimesLimitedWithNull != false;
            set => HasTimesLimitedWithNull = value;
        }

        private string _maxTimes = ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimitedQuantity, "5");

        /// <summary>
        /// Gets or sets the max number of times.
        /// </summary>
        public string MaxTimes
        {
            get => _maxTimes;
            set
            {
                if (MaxTimes == value)
                {
                    return;
                }

                SetAndNotify(ref _maxTimes, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.TimesLimitedQuantity, MaxTimes);
            }
        }

        private string _series = ConfigurationHelper.GetValue(ConfigurationKeys.SeriesQuantity, "1");

        /// <summary>
        /// Gets or sets the max number of times.
        /// </summary>
        // 所以为啥这玩意是 string 呢？改配置的时候把上面那些也都改成 int 吧
        public string Series
        {
            get => _series;
            set
            {
                if (_series == value)
                {
                    return;
                }

                SetAndNotify(ref _series, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.SeriesQuantity, value);
            }
        }

        private bool _hideSeries = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideSeries, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to hide series.
        /// </summary>
        public bool HideSeries
        {
            get => _hideSeries;
            set => SetAndNotify(ref _hideSeries, value);
        }

        #region Drops

        private bool? _isSpecifiedDropsWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DropsEnable, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether the drops are specified.
        /// </summary>
        public bool? IsSpecifiedDropsWithNull
        {
            get => _isSpecifiedDropsWithNull;
            set
            {
                SetAndNotify(ref _isSpecifiedDropsWithNull, value);
                SetFightParams();
                value ??= false;
                ConfigurationHelper.SetValue(ConfigurationKeys.DropsEnable, value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the drops are specified.
        /// </summary>
        private bool IsSpecifiedDrops
        {
            get => IsSpecifiedDropsWithNull != false;
            set => IsSpecifiedDropsWithNull = value;
        }

        /// <summary>
        /// Gets the list of all drops.
        /// </summary>
        private List<CombinedData> AllDrops { get; } = new();

        /// <summary>
        /// 关卡不可掉落的材料
        /// </summary>
        private static readonly HashSet<string> _excludedValues =
        [
            "3213", "3223", "3233", "3243", // 双芯片
            "3253", "3263", "3273", "3283", // 双芯片
            "7001", "7002", "7003", "7004", // 许可
            "4004", "4005",                 // 凭证
            "3105", "3131", "3132", "3233", // 龙骨/加固建材
            "6001",                         // 演习券
            "3141", "4002",                 // 源石
            "32001",                        // 芯片助剂
            "30115",                        // 聚合剂
            "30125",                        // 双极纳米片
            "30135",                        // D32钢
            "30145",                        // 晶体电子单元
            "30155",                        // 烧结核凝晶
        ];

        private void InitDrops()
        {
            foreach (var item in ItemListHelper.ArkItems)
            {
                var val = item.Key;

                // 不是数字的东西都是正常关卡不会掉的（大概吧）
                if (!int.TryParse(val, out _))
                {
                    continue;
                }

                var dis = item.Value.Name;

                if (_excludedValues.Contains(val))
                {
                    continue;
                }

                AllDrops.Add(new CombinedData { Display = dis, Value = val });
            }

            AllDrops.Sort((a, b) => string.Compare(a.Value, b.Value, StringComparison.Ordinal));
            DropsList = new ObservableCollection<CombinedData>(AllDrops);
        }

        /// <summary>
        /// Gets or private sets the list of drops.
        /// </summary>
        public ObservableCollection<CombinedData> DropsList { get; private set; }

        private string _dropsItemId = ConfigurationHelper.GetValue(ConfigurationKeys.DropsItemId, string.Empty);

        /// <summary>
        /// Gets or sets the item ID of drops.
        /// </summary>
        public string DropsItemId
        {
            get => _dropsItemId;
            set
            {
                SetAndNotify(ref _dropsItemId, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.DropsItemId, DropsItemId);
            }
        }

        private string _dropsItemName = ConfigurationHelper.GetValue(ConfigurationKeys.DropsItemName, LocalizationHelper.GetString("NotSelected"));

        /// <summary>
        /// Gets or sets the item Name of drops.
        /// </summary>
        public string DropsItemName
        {
            get => _dropsItemName;
            set
            {
                SetAndNotify(ref _dropsItemName, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.DropsItemName, DropsItemName);
            }
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void DropsListDropDownClosed()
        {
            foreach (var item in DropsList)
            {
                if (DropsItemName != item.Display)
                {
                    continue;
                }

                DropsItemId = item.Value;

                if (DropsItemName != item.Display || DropsItemId != item.Value)
                {
                    DropsItemName = LocalizationHelper.GetString("NotSelected");
                }

                return;
            }

            DropsItemName = LocalizationHelper.GetString("NotSelected");
        }

        private string _dropsQuantity = ConfigurationHelper.GetValue(ConfigurationKeys.DropsQuantity, "5");

        /// <summary>
        /// Gets or sets the quantity of drops.
        /// </summary>
        public string DropsQuantity
        {
            get => _dropsQuantity;
            set
            {
                SetAndNotify(ref _dropsQuantity, value);
                SetFightParams();
                ConfigurationHelper.SetValue(ConfigurationKeys.DropsQuantity, DropsQuantity);
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

        #endregion Drops
    }
}
