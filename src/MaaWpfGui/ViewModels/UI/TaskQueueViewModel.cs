// <copyright file="TaskQueueViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using StyletIoC;
using Application = System.Windows.Application;
using ComboBox = System.Windows.Controls.ComboBox;
using Screen = Stylet.Screen;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of task queue.
    /// </summary>
    public class TaskQueueViewModel : Screen
    {
        private readonly IContainer _container;
        private StageManager _stageManager;
        private readonly RunningState runningState;

        private static readonly ILogger _logger = Log.ForContext<TaskQueueViewModel>();

        /// <summary>
        /// Gets or sets the view models of task items.
        /// </summary>
        public ObservableCollection<DragItemViewModel> TaskItemViewModels { get; set; }

        /// <summary>
        /// Gets the visibility of task setting views.
        /// </summary>
        public TaskSettingVisibilityInfo TaskSettingVisibilities { get; } = TaskSettingVisibilityInfo.Current;

        public SettingsViewModel TaskSettingDataContext { get => Instances.SettingsViewModel; }

        /// <summary>
        /// 实时更新任务顺序
        /// </summary>
        public void TaskItemSelectionChanged()
        {
            Application.Current.Dispatcher.InvokeAsync(() =>
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
        /// Gets or sets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        private string _actionAfterCompleted = ConfigurationHelper.GetValue(ConfigurationKeys.ActionAfterCompleted, ActionType.DoNothing.ToString());

        /// <summary>
        /// Gets or sets the list of the actions after completion.
        /// </summary>
        public List<GenericCombinedData<ActionType>> ActionAfterCompletedList { get; set; }

        /// <summary>
        /// Gets or sets the action after completion.
        /// </summary>
        public ActionType ActionAfterCompleted
        {
            get
            {
                if (!Enum.TryParse(_actionAfterCompleted, out ActionType action))
                {
                    return ActionType.DoNothing;
                }

                return action;
            }

            set
            {
                string storeValue = value.ToString();
                SetAndNotify(ref _actionAfterCompleted, storeValue);

                if (value == ActionType.HibernateWithoutPersist ||
                    value == ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist ||
                    value == ActionType.ShutdownWithoutPersist)
                {
                    storeValue = ActionType.DoNothing.ToString();
                }

                ConfigurationHelper.SetValue(ConfigurationKeys.ActionAfterCompleted, storeValue);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TaskQueueViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public TaskQueueViewModel(IContainer container)
        {
            _container = container;
            runningState = RunningState.Instance;
            runningState.IdleChanged += RunningState_IdleChanged;
        }

        private void RunningState_IdleChanged(object sender, bool e)
        {
            Idle = e;
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

        private readonly DispatcherTimer _timer = new DispatcherTimer();

        public bool ConfirmExit()
        {
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

            var window = Instances.MainWindowManager.GetWindowIfVisible();
            var result = MessageBoxHelper.ShowNative(window, LocalizationHelper.GetString("ConfirmExitText"), LocalizationHelper.GetString("ConfirmExitTitle"), "MAA", MessageBoxButton.YesNo, MessageBoxImage.Information, MessageBoxResult.No);
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
                var delayTime = new Random().Next(0, 10 * 60 * 1000);
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
                if (Instances.SettingsViewModel.UpdatAutoCheck)
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

            refreshCustomInfrastPlanIndexByPeriod();

            if (!runningState.GetIdle() && !Instances.SettingsViewModel.ForceScheduledStart && !Instances.SettingsViewModel.CustomConfig)
            {
                return;
            }

            var timeToStart = false;
            var timeToChangeConfig = false;
            var configIndex = 0;
            for (int i = 0; i < 8; ++i)
            {
                if (Instances.SettingsViewModel.TimerModels.Timers[i].IsOn)
                {
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

                    if (currentTime == restartDateTime)
                    {
                        timeToChangeConfig = true;
                        configIndex = i;
                        break;
                    }
                    else if (currentTime == startTime)
                    {
                        timeToStart = true;
                        break;
                    }
                }
            }

            if (timeToChangeConfig)
            {
                if (Instances.SettingsViewModel.CustomConfig)
                {
                    // CurrentConfiguration设置后会重启
                    Instances.SettingsViewModel.CurrentConfiguration = Instances.SettingsViewModel.TimerModels.Timers[configIndex].TimerConfig;
                    return;
                }
            }

            if (timeToStart)
            {
                if (Instances.SettingsViewModel.ForceScheduledStart)
                {
                    if (!runningState.GetIdle())
                    {
                        await Stop();
                    }

                    if (!Instances.AsstProxy.AsstAppendCloseDown())
                    {
                        AddLog(LocalizationHelper.GetString("CloseArknightsFailed"), UiLogColor.Error);
                    }

                    ResetFightVariables();
                }

                LinkStart();
            }
        }

        /// <summary>
        /// Initializes items.
        /// </summary>
        private void InitializeItems()
        {
            string[] task_list = new string[]
            {
                "WakeUp",
                "Recruiting",
                "Base",
                "Combat",
                "Mall",
                "Mission",
                "AutoRoguelike",

                // "ReclamationAlgorithm",
            };
            ActionAfterCompletedList = new List<GenericCombinedData<ActionType>>
            {
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("DoNothing"), Value = ActionType.DoNothing },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ExitArknights"), Value = ActionType.StopGame },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ExitEmulator"), Value = ActionType.ExitEmulator },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ExitSelf"), Value = ActionType.ExitSelf },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ExitEmulatorAndSelf"), Value = ActionType.ExitEmulatorAndSelf },

                // new GenericCombData<ActionTypeAfterCompleted>{ Display="待机",Value=ActionTypeAfterCompleted.Suspend },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ExitEmulatorAndSelfAndHibernate"), Value = ActionType.ExitEmulatorAndSelfAndHibernate },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("Hibernate"), Value = ActionType.Hibernate },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("Shutdown"), Value = ActionType.Shutdown },

                // new GenericCombData<ActionType> { Display = Localization.GetString("ExitEmulatorAndSelfAndHibernate") + "*", Value = ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("HibernateWithoutPersist"), Value = ActionType.HibernateWithoutPersist },
                new GenericCombinedData<ActionType> { Display = LocalizationHelper.GetString("ShutdownWithoutPersist"), Value = ActionType.ShutdownWithoutPersist },
            };
            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[task_list.Length]);
            var non_order_list = new List<DragItemViewModel>();
            for (int i = 0; i != task_list.Length; ++i)
            {
                var task = task_list[i];
                bool parsed = int.TryParse(ConfigurationHelper.GetTaskOrder(task, "-1"), out var order);

                var vm = new DragItemViewModel(LocalizationHelper.GetString(task), task, "TaskQueue.");

                if (task == TaskSettingVisibilityInfo.DefaultVisibleTaskSetting)
                {
                    vm.EnableSetting = true;
                }

                if (!parsed || order < 0 || order >= temp_order_list.Count)
                {
                    non_order_list.Add(vm);
                }
                else
                {
                    temp_order_list[order] = vm;
                }
            }

            foreach (var new_vm in non_order_list)
            {
                int i = 0;
                while (temp_order_list[i] != null)
                {
                    ++i;
                }

                temp_order_list[i] = new_vm;
            }

            TaskItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            InitDrops();
            NeedToUpdateDatePrompt();
            UpdateDatePrompt();
            UpdateStageList(true);
            RefreshCustonInfrastPlan();

            if (DateTime.UtcNow.ToYJDate().IsAprilFoolsDay())
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
        /// <param name="forceUpdate">Whether or not to update the stage list for selection forcely</param>
        // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
        // 这个函数被列为public可见，意味着他注入对象前被调用
        public void UpdateStageList(bool forceUpdate)
        {
            var hideUnavailableStage = Instances.SettingsViewModel.HideUnavailableStage;

            // forceUpdate: initializing or settings changing, update stage list forcely
            if (!forceUpdate && !hideUnavailableStage)
            {
                return;
            }

            EnableSetFightParams = false;

            var stage1 = Stage1 ?? string.Empty;
            var stage2 = Stage2 ?? string.Empty;
            var stage3 = Stage3 ?? string.Empty;
            var rss = RemainingSanityStage ?? string.Empty;

            if (hideUnavailableStage)
            {
                StageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList(_curDayOfWeek));
            }
            else
            {
                StageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList());
            }

            AlternateStageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList());

            RemainingSanityStageList = new ObservableCollection<CombinedData>(_stageManager.GetStageList())
            {
                [0] = new CombinedData { Display = LocalizationHelper.GetString("NoUse"), Value = string.Empty },
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

            Stage1 = stage1;
            Stage2 = stage2;
            Stage3 = stage3;
            RemainingSanityStage = rss;

            EnableSetFightParams = true;
        }

        private bool NeedToUpdateDatePrompt()
        {
            var now = DateTime.UtcNow.ToYJDateTime();
            var hour = now.Hour;
            var min = now.Minute;

            // yj历的4/16点
            if (min == 0 && hour == 12)
            {
                return true;
            }
            else
            {
                if (_curDayOfWeek == now.DayOfWeek)
                {
                    return false;
                }
                else
                {
                    _curDayOfWeek = now.DayOfWeek;
                    return true;
                }
            }
        }

        private bool NeedToCheckForUpdates()
        {
            var now = DateTime.UtcNow.ToYJDateTime();
            var hour = now.Hour;
            var min = now.Minute;

            // yj历的4/22点
            if (min == 0 && hour == 18)
            {
                return true;
            }

            return false;
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
                if (stage == null)
                {
                    continue;
                }

                if (_stageManager.GetStageInfo(stage)?.IsActivityClosed() == true)
                {
                    builder.Append(stage).Append(": ").AppendLine(LocalizationHelper.GetString("ClosedStage"));
                    break;
                }
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
        /// Gets or sets the stages of today.
        /// </summary>
        public string StagesOfToday
        {
            get => _stagesOfToday;
            set => SetAndNotify(ref _stagesOfToday, value);
        }

        /// <summary>
        /// Adds log.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLog(string content, string color = UiLogColor.Trace, string weight = "Regular")
        {
            var log = new LogItemViewModel(content, color, weight);
            LogItemViewModels.Add(log);
            _logger.Information(content);
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        public void ClearLog()
        {
            LogItemViewModels.Clear();
            _logger.Information("Main windows log clear.");
            _logger.Information(string.Empty);
        }

        /// <summary>
        /// Selects all.
        /// </summary>
        public void SelectedAll()
        {
            foreach (var item in TaskItemViewModels)
            {
                switch (item.OriginalName)
                {
                    case "AutoRoguelike":
                    case "ReclamationAlgorithm":
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

        private int _inverseSelectedWidth = 90;

        /// <summary>
        /// Gets or sets the width of "Select inversely".
        /// </summary>
        public int InverseSelectedWidth
        {
            get => _inverseSelectedWidth;
            set => SetAndNotify(ref _inverseSelectedWidth, value);
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

        private string _inverseShowText = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString)) ? LocalizationHelper.GetString("Inverse") : LocalizationHelper.GetString("Clear");

        /// <summary>
        /// Gets or sets the text to be displayed for "Select inversely".
        /// </summary>
        public string InverseShowText
        {
            get => _inverseShowText;
            set => SetAndNotify(ref _inverseShowText, value);
        }

        private string _inverseMenuText = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString)) ? LocalizationHelper.GetString("Clear") : LocalizationHelper.GetString("Inverse");

        /// <summary>
        /// Gets or sets the text of inversion menu.
        /// </summary>
        public string InverseMenuText
        {
            get => _inverseMenuText;
            set => SetAndNotify(ref _inverseMenuText, value);
        }

        /// <summary>
        /// Changes inversion mode.
        /// </summary>
        public void ChangeInverseMode()
        {
            InverseMode = !InverseMode;
        }

        /// <summary>
        /// Selects inversely.
        /// </summary>
        public void InverseSelected()
        {
            if (_inverseMode)
            {
                foreach (var item in TaskItemViewModels)
                {
                    switch (item.OriginalName)
                    {
                        case "AutoRoguelike":
                            item.IsChecked = false;
                            continue;
                        case "ReclamationAlgorithm":
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
        /// Starts.
        /// </summary>
        public async void LinkStart()
        {
            if (!runningState.GetIdle())
            {
                return;
            }

            runningState.SetIdle(false);

            // 虽然更改时已经保存过了，不过保险起见还是在点击开始之后再保存一次任务及基建列表
            TaskItemSelectionChanged();
            Instances.SettingsViewModel.InfrastOrderSelectionChanged();

            ClearLog();

            await Task.Run(() => Instances.SettingsViewModel.RunScript("StartsWithScript"));

            AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));
            if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
            {
                AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
            }

            string errMsg = string.Empty;
            bool connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            // 尝试启动模拟器
            if (!connected)
            {
                AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("TryToStartEmulator"));
                await Task.Run(() => Instances.SettingsViewModel.TryToStartEmulator(true));

                if (Stopping)
                {
                    SetStopped();
                    return;
                }

                connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            }

            // 尝试重启adb
            if (!connected)
            {
                AddLog(LocalizationHelper.GetString("ConnectFailed") + "\n" + LocalizationHelper.GetString("RestartADB"));
                await Task.Run(() => Instances.SettingsViewModel.RestartADB());

                if (Stopping)
                {
                    SetStopped();
                    return;
                }

                connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            }

            if (!connected)
            {
                AddLog(errMsg, UiLogColor.Error);
                runningState.SetIdle(true);
                SetStopped();
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
                if (item.OriginalName == "Base")
                {
                    taskRet &= appendInfrast();
                }
                else if (item.OriginalName == "WakeUp")
                {
                    taskRet &= appendStart();
                }
                else if (item.OriginalName == "Combat")
                {
                    taskRet &= appendFight();
                }
                else if (item.OriginalName == "Recruiting")
                {
                    taskRet &= appendRecruit();
                }
                else if (item.OriginalName == "Mall")
                {
                    taskRet &= appendMall();
                }
                else if (item.OriginalName == "Mission")
                {
                    taskRet &= Instances.AsstProxy.AsstAppendAward();
                }
                else if (item.OriginalName == "AutoRoguelike")
                {
                    taskRet &= AppendRoguelike();
                }
                else if (item.OriginalName == "ReclamationAlgorithm")
                {
                    taskRet &= AppendReclamation();
                }
                else
                {
                    --count;

                    // TODO 报错
                }

                if (!taskRet)
                {
                    AddLog(item.OriginalName + "Error", UiLogColor.Error);
                    taskRet = true;
                    --count;
                }
            }

            if (count == 0)
            {
                AddLog(LocalizationHelper.GetString("UnselectedTask"));
                runningState.SetIdle(true);
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
        /// Stops.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task Stop()
        {
            Stopping = true;
            AddLog(LocalizationHelper.GetString("Stopping"));
            await Task.Run(Instances.AsstProxy.AsstStop);

            int count = 0;
            while (Instances.AsstProxy.AsstRunning() && count <= 600)
            {
                await Task.Delay(100);
                count++;
            }
        }

        public void SetStopped()
        {
            if (!runningState.GetIdle() || Stopping)
            {
                AddLog(LocalizationHelper.GetString("Stopped"));
            }

            Stopping = false;
            runningState.SetIdle(true);
        }

        private bool appendStart()
        {
            var mode = Instances.SettingsViewModel.ClientType;
            var enable = mode.Length != 0;
            return Instances.AsstProxy.AsstAppendStartUp(mode, enable);
        }

        private bool appendFight()
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

            int drops_quantity = 0;
            if (IsSpecifiedDrops)
            {
                if (!int.TryParse(DropsQuantity, out drops_quantity))
                {
                    drops_quantity = 0;
                }
            }

            string curStage = Stage;

            bool mainFightRet = Instances.AsstProxy.AsstAppendFight(curStage, medicine, stone, times, DropsItemId, drops_quantity);

            if (mainFightRet && (curStage == "Annihilation") && Instances.SettingsViewModel.UseAlternateStage)
            {
                foreach (var stage in new[] { Stage1, Stage2, Stage3 })
                {
                    if (IsStageOpen(stage) && (stage != curStage))
                    {
                        mainFightRet = Instances.AsstProxy.AsstAppendFight(stage, medicine, 0, int.MaxValue, string.Empty, 0);
                    }
                }
            }

            if (mainFightRet && UseRemainingSanityStage && !string.IsNullOrEmpty(RemainingSanityStage))
            {
                return Instances.AsstProxy.AsstAppendFight(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
            }

            return mainFightRet;
        }

        public bool EnableSetFightParams { get; set; } = true;

        /// <summary>
        /// Sets parameters.
        /// </summary>
        public void SetFightParams()
        {
            if (EnableSetFightParams)
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

                int drops_quantity = 0;
                if (IsSpecifiedDrops)
                {
                    if (!int.TryParse(DropsQuantity, out drops_quantity))
                    {
                        drops_quantity = 0;
                    }
                }

                Instances.AsstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, DropsItemId, drops_quantity);
            }
        }

        public void SetFightRemainingSanityParams()
        {
            Instances.AsstProxy.AsstSetFightTaskParams(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
        }

        public void SetInfrastParams()
        {
            var order = Instances.SettingsViewModel.GetInfrastOrderList();
            Instances.AsstProxy.AsstSetInfrastTaskParams(order.ToArray(), Instances.SettingsViewModel.UsesOfDrones, Instances.SettingsViewModel.DormThreshold / 100.0, Instances.SettingsViewModel.DormFilterNotStationedEnabled, Instances.SettingsViewModel.DormTrustEnabled, Instances.SettingsViewModel.OriginiumShardAutoReplenishment,
                Instances.SettingsViewModel.CustomInfrastEnabled, Instances.SettingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendInfrast()
        {
            if (Instances.SettingsViewModel.CustomInfrastEnabled && !File.Exists(Instances.SettingsViewModel.CustomInfrastFile))
            {
                AddLog(LocalizationHelper.GetString("CustomizeInfrastSelectionEmpty"), UiLogColor.Error);
                return false;
            }

            var order = Instances.SettingsViewModel.GetInfrastOrderList();
            return Instances.AsstProxy.AsstAppendInfrast(order.ToArray(), Instances.SettingsViewModel.UsesOfDrones, Instances.SettingsViewModel.DormThreshold / 100.0, Instances.SettingsViewModel.DormFilterNotStationedEnabled, Instances.SettingsViewModel.DormTrustEnabled, Instances.SettingsViewModel.OriginiumShardAutoReplenishment,
                Instances.SettingsViewModel.CustomInfrastEnabled, Instances.SettingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendMall()
        {
            var buy_first = Instances.SettingsViewModel.CreditFirstList.Split(new char[] { ';', '；' });
            var black_list = Instances.SettingsViewModel.CreditBlackList.Split(new char[] { ';', '；' });
            for (var i = 0; i < buy_first.Length; ++i)
            {
                buy_first[i] = buy_first[i].Trim();
            }

            for (var i = 0; i < black_list.Length; ++i)
            {
                black_list[i] = black_list[i].Trim();
            }

            return Instances.AsstProxy.AsstAppendMall(
                !string.IsNullOrEmpty(this.Stage) && Instances.SettingsViewModel.CreditFightTaskEnabled,
                Instances.SettingsViewModel.CreditShopping,
                buy_first,
                black_list,
                Instances.SettingsViewModel.CreditForceShoppingIfCreditFull);
        }

        private bool appendRecruit()
        {
            // for debug
            if (!int.TryParse(Instances.SettingsViewModel.RecruitMaxTimes, out var max_times))
            {
                max_times = 0;
            }

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

            if (Instances.SettingsViewModel.ChooseLevel5)
            {
                reqList.Add(5);
                cfmList.Add(5);
            }

            return Instances.AsstProxy.AsstAppendRecruit(
                max_times, reqList.ToArray(), cfmList.ToArray(), Instances.SettingsViewModel.RefreshLevel3, Instances.SettingsViewModel.UseExpedited,
                Instances.SettingsViewModel.NotChooseLevel1, Instances.SettingsViewModel.IsLevel3UseShortTime, Instances.SettingsViewModel.IsLevel3UseShortTime2);
        }

        private bool AppendRoguelike()
        {
            int.TryParse(Instances.SettingsViewModel.RoguelikeMode, out var mode);

            return Instances.AsstProxy.AsstAppendRoguelike(
                mode, Instances.SettingsViewModel.RoguelikeStartsCount,
                Instances.SettingsViewModel.RoguelikeInvestmentEnabled, Instances.SettingsViewModel.RoguelikeInvestsCount, Instances.SettingsViewModel.RoguelikeStopWhenInvestmentFull,
                Instances.SettingsViewModel.RoguelikeSquad, Instances.SettingsViewModel.RoguelikeRoles, Instances.SettingsViewModel.RoguelikeCoreChar, Instances.SettingsViewModel.RoguelikeUseSupportUnit,
                Instances.SettingsViewModel.RoguelikeEnableNonfriendSupport, Instances.SettingsViewModel.RoguelikeTheme, Instances.SettingsViewModel.RoguelikeRefreshTraderWithDice);
        }

        private bool AppendReclamation()
        {
            return Instances.AsstProxy.AsstAppendReclamation();
        }

        [DllImport("User32.dll", EntryPoint = "FindWindow")]
        private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("User32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);

        /// <summary>
        /// 一个根据连接配置判断使用关闭模拟器的方式的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorModeSwitcher()
        {
            string emulatorMode = Instances.SettingsViewModel.ConnectConfig;
            Instances.AsstProxy.Connected = false;
            return emulatorMode switch
            {
                "Nox" => KillEmulatorNox(),
                "LDPlayer" => KillEmulatorLDPlayer(),
                "XYAZ" => KillEmulatorXYAZ(),
                "BlueStacks" => KillEmulatorBlueStacks(),
                "MuMuEmulator12" => KillEmulatorMuMuEmulator12(),
                _ => KillEmulatorbyWindow(),
            };
        }

        /// <summary>
        /// 一个用于调用 MuMu12 模拟器控制台关闭 MuMu12 的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorMuMuEmulator12()
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
            if (processes.Length > 0)
            {
                string emuLocation = processes[0].MainModule.FileName;
                emuLocation = Path.GetDirectoryName(emuLocation);
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
                    if (process.WaitForExit(5000))
                    {
                        _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                        return KillEmulator();
                    }
                    else
                    {
                        _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                        return KillEmulatorbyWindow();
                    }
                }
                else
                {
                    _logger.Error($"Error: `{consolePath}` not found, try to kill eumlator by window.");
                    return KillEmulatorbyWindow();
                }
            }

            return false;
        }

        /// <summary>
        /// 一个用于调用雷电模拟器控制台关闭雷电模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorLDPlayer()
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
            if (processes.Length > 0)
            {
                string emuLocation = processes[0].MainModule.FileName;
                emuLocation = Path.GetDirectoryName(emuLocation);
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
                    if (process.WaitForExit(5000))
                    {
                        _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                        return KillEmulator();
                    }
                    else
                    {
                        _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                        return KillEmulatorbyWindow();
                    }
                }
                else
                {
                    _logger.Information($"Error: `{consolePath}` not found, try to kill eumlator by window.");
                    return KillEmulatorbyWindow();
                }
            }

            return false;
        }

        /// <summary>
        /// 一个用于调用夜神模拟器控制台关闭夜神模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorNox()
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
            if (processes.Length > 0)
            {
                string emuLocation = processes[0].MainModule.FileName;
                emuLocation = Path.GetDirectoryName(emuLocation);
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
                    if (process.WaitForExit(5000))
                    {
                        _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                        return KillEmulator();
                    }
                    else
                    {
                        _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                        return KillEmulatorbyWindow();
                    }
                }
                else
                {
                    _logger.Information($"Error: `{consolePath}` not found, try to kill eumlator by window.");
                    return KillEmulatorbyWindow();
                }
            }

            return false;
        }

        /// <summary>
        /// 一个用于调用逍遥模拟器控制台关闭逍遥模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorXYAZ()
        {
            string address = Instances.SettingsViewModel.ConnectAddress;
            int emuIndex;
            string portStr = address.Split(':')[1];
            int port = int.Parse(portStr);
            emuIndex = (port - 21503) / 10;

            Process[] processes = Process.GetProcessesByName("MEmu");
            if (processes.Length > 0)
            {
                string emuLocation = processes[0].MainModule.FileName;
                emuLocation = Path.GetDirectoryName(emuLocation);
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
                    if (process.WaitForExit(5000))
                    {
                        _logger.Information($"Emulator at index {emuIndex} closed through console. Console path: {consolePath}");
                        return KillEmulator();
                    }
                    else
                    {
                        _logger.Warning($"Console process at index {emuIndex} did not exit within the specified timeout. Killing emulator by window. Console path: {consolePath}");
                        return KillEmulatorbyWindow();
                    }
                }
                else
                {
                    _logger.Information($"Error: `{consolePath}` not found, try to kill eumlator by window.");
                    return KillEmulatorbyWindow();
                }
            }

            return false;
        }

        /// <summary>
        /// 一个用于关闭蓝叠模拟器的方法
        /// </summary>
        /// <returns>是否关闭成功</returns>
        public bool KillEmulatorBlueStacks()
        {
            Process[] processes = Process.GetProcessesByName("HD-Player");
            if (processes.Length > 0)
            {
                string emuLocation = processes[0].MainModule.FileName;
                emuLocation = Path.GetDirectoryName(emuLocation);
                string consolePath = Path.Combine(emuLocation, "bsconsole.exe");

                if (File.Exists(consolePath))
                {
                    _logger.Information($"Info: `{consolePath}` has been found. This may be the BlueStacks China emulator, try to kill the emulator by window.");
                    return KillEmulatorbyWindow();
                }
                else
                {
                    _logger.Information($"Info: `{consolePath}` not found. This may be the BlueStacks International emulator, try to kill the emulator by the port.");
                    if (KillEmulator())
                    {
                        return true;
                    }
                    else
                    {
                        try
                        {
                            _logger.Information($"Info: Failed to kill emulator by the port, try to kill emulator process with PID.");
                            processes[0].Kill();
                            return processes[0].WaitForExit(20000);
                        }
                        catch (Exception ex)
                        {
                            _logger.Error($"Error: Failed to kill emulator process with PID {processes[0].Id}. Exception: {ex.Message}");
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Kills emulator by Window hwnd.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool KillEmulatorbyWindow()
        {
            IntPtr hwnd;
            int pid = 0;
            var windowname = new[]
            {
                "明日方舟",
                "明日方舟 - MuMu模拟器",
                "BlueStacks App Player",
                "BlueStacks",
            };
            foreach (string i in windowname)
            {
                hwnd = FindWindow(null, i);
                if (hwnd != IntPtr.Zero)
                {
                    GetWindowThreadProcessId(hwnd, out pid);
                    break;
                }
            }

            do
            {
                if (pid == 0)
                {
                    break;
                }

                Process emulator;
                try
                {
                    emulator = Process.GetProcessById(pid);
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
                        else
                        {
                            _logger.Error($"Failed to kill emulator with process ID {pid}.");
                            return false;
                        }
                    }
                    else
                    {
                        // 尽管已经成功 CloseMainWindow()，再次尝试 killEmulator()
                        // Refer to https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/1878
                        KillEmulator();

                        // 已经成功 CloseMainWindow()，所以不管 killEmulator() 的结果如何，都返回 true
                        return true;
                    }
                }
                catch
                {
                    break;
                }
            }
            while (false);

            return KillEmulator();
        }

        /// <summary>
        /// Kills emulator.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool KillEmulator()
        {
            int pid = 0;
            string port;
            string address = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);
            if (address.StartsWith("127"))
            {
                port = address.Substring(10);
            }
            else
            {
                port = "5555";
            }

            string portCmd = "netstat -ano|findstr \"" + port + "\"";
            Process checkCmd = new Process();
            checkCmd.StartInfo.FileName = "cmd.exe";
            checkCmd.StartInfo.UseShellExecute = false;
            checkCmd.StartInfo.RedirectStandardInput = true;
            checkCmd.StartInfo.RedirectStandardOutput = true;
            checkCmd.StartInfo.RedirectStandardError = true;
            checkCmd.StartInfo.CreateNoWindow = true;
            try
            {
                checkCmd.Start();
            }
            catch
            {
                checkCmd.Close();
                return false;
            }

            checkCmd.StandardInput.WriteLine(portCmd);
            checkCmd.StandardInput.WriteLine("exit");
            Regex reg = new Regex("\\s+", RegexOptions.Compiled);
            string line;
            while (true)
            {
                line = checkCmd.StandardOutput.ReadLine();
                try
                {
                    line = line.Trim();
                }
                catch
                {
                    break;
                }

                if (line.StartsWith("TCP", StringComparison.OrdinalIgnoreCase))
                {
                    line = reg.Replace(line, ",");
                    string[] arr = line.Split(',');
                    if (!Convert.ToBoolean(string.Compare(arr[1], address, StringComparison.Ordinal)) || !Convert.ToBoolean(string.Compare(arr[1], "[::]:" + port, StringComparison.Ordinal)) || !Convert.ToBoolean(string.Compare(arr[1], "0.0.0.0:" + port, StringComparison.Ordinal)))
                    {
                        pid = int.Parse(arr[4]);
                        break;
                    }
                }
            }

            if (pid == 0)
            {
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
        }

        /// <summary>
        /// Checks after completion.
        /// </summary>
        public void CheckAfterCompleted()
        {
            Task.Run(() => Instances.SettingsViewModel.RunScript("EndsWithScript"));

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
                    Application.Current.Shutdown();

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
                    Application.Current.Shutdown();

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
                    ActionAfterCompleted = ActionType.DoNothing;
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
                    Application.Current.Shutdown();

                    // Environment.Exit(0);
                    break;

                default:
                    break;
            }
        }

        /// <summary>
        /// Gets a value indicating whether it is initialized.
        /// </summary>
        public bool Inited { get; private set; } = false;

        /// <summary>
        /// Sets it initialized.
        /// </summary>
        public void SetInited()
        {
            Inited = true;
            NotifyOfPropertyChange("Inited");
        }

        private bool _idle = true;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set
            {
                SetAndNotify(ref _idle, value);
                if (value)
                {
                    FightTaskRunning = false;
                    InfrastTaskRunning = false;
                }
            }
        }

        private bool _stopping = false;

        /// <summary>
        /// Gets or sets a value indicating whether `stop` is awaiting.
        /// </summary>
        public bool Stopping
        {
            get => _stopping;
            set => SetAndNotify(ref _stopping, value);
        }

        private bool _fightTaskRunning = false;

        /// <summary>
        /// Gets or sets a value indicating whether the battle task is running.
        /// </summary>
        public bool FightTaskRunning
        {
            get => _fightTaskRunning;
            set => SetAndNotify(ref _fightTaskRunning, value);
        }

        private bool _infrastTaskRunning = false;

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

        private ObservableCollection<CombinedData> _stageList = new ObservableCollection<CombinedData>();

        /// <summary>
        /// Gets or sets the list of stages.
        /// </summary>
        public ObservableCollection<CombinedData> StageList
        {
            get => _stageList;
            set => SetAndNotify(ref _stageList, value);
        }

        public ObservableCollection<CombinedData> RemainingSanityStageList { get; set; } = new ObservableCollection<CombinedData>();

        public ObservableCollection<CombinedData> AlternateStageList { get; set; } = new ObservableCollection<CombinedData>();

        /// <summary>
        /// Gets the stage.
        /// </summary>
        public string Stage
        {
            get
            {
                if (Instances.SettingsViewModel.UseAlternateStage)
                {
                    if (IsStageOpen(Stage1))
                    {
                        return Stage1;
                    }

                    if (IsStageOpen(Stage2))
                    {
                        return Stage2;
                    }

                    if (IsStageOpen(Stage3))
                    {
                        return Stage3;
                    }
                }

                return Stage1;
            }
        }

        private readonly Dictionary<string, string> stageDictionary = new Dictionary<string, string>
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
            if (stageDictionary.ContainsKey(upperValue))
            {
                return stageDictionary[upperValue];
            }

            if (StageList != null)
            {
                foreach (var item in StageList)
                {
                    if (upperValue == item.Value.ToUpper() || upperValue == item.Display.ToUpper())
                    {
                        return item.Value;
                    }
                }
            }

            return value;
        }

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
            set
            {
                SetAndNotify(ref _customStageCode, value);
            }
        }

        private string _remainingSanityStage = ConfigurationHelper.GetValue(ConfigurationKeys.RemainingSanityStage, string.Empty) ?? string.Empty;

        public string RemainingSanityStage
        {
            get
            {
                return _remainingSanityStage;
            }

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
                RefreshCustonInfrastPlan();
            }
        }

        private int _customInfrastPlanIndex = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanIndex, "0"));

        public int CustomInfrastPlanIndex
        {
            get => _customInfrastPlanIndex;
            set
            {
                if (value != _customInfrastPlanIndex)
                {
                    var plan = CustomInfrastPlanInfoList[value];
                    AddLog(plan.Name, UiLogColor.Message);

                    foreach (var period in plan.PeriodList)
                    {
                        AddLog(string.Format("[ {0:D2}:{1:D2} - {2:D2}:{3:D2} ]",
                            period.BeginHour, period.BeginMinute,
                            period.EndHour, period.EndMinute));
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

        public ObservableCollection<GenericCombinedData<int>> CustomInfrastPlanList { get; set; } = new ObservableCollection<GenericCombinedData<int>>();

        public struct CustomInfrastPlanInfo
        {
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
        }

        public List<CustomInfrastPlanInfo> CustomInfrastPlanInfoList { get; set; } = new List<CustomInfrastPlanInfo>();

        private bool _customInfrastPlanHasPeriod = false;
        private bool _customInfrastInfoOutput = false;

        public void RefreshCustonInfrastPlan()
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

                if (_customInfrastInfoOutput && root.ContainsKey("title"))
                {
                    AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message);
                    AddLog(root["title"].ToString(), UiLogColor.Info);
                    if (root.ContainsKey("description"))
                    {
                        AddLog(root["description"].ToString());
                    }
                }

                var plan_list = (JArray)root["plans"];
                for (int i = 0; i < plan_list.Count; ++i)
                {
                    var plan = (JObject)plan_list[i];
                    string display = plan.ContainsKey("name") ? plan["name"].ToString() : ("Plan " + ((char)('A' + i)).ToString());
                    CustomInfrastPlanList.Add(new GenericCombinedData<int> { Display = display, Value = i });
                    string desc = plan.ContainsKey("description") ? plan["description"].ToString() : string.Empty;
                    string descPost = plan.ContainsKey("description_post") ? plan["description_post"].ToString() : string.Empty;

                    if (_customInfrastInfoOutput)
                    {
                        AddLog(display, UiLogColor.Message);
                    }

                    var periodList = new List<CustomInfrastPlanInfo.Period>();
                    if (plan.ContainsKey("period"))
                    {
                        var periodArray = (JArray)plan["period"];
                        foreach (var periodJson in periodArray)
                        {
                            var period = default(CustomInfrastPlanInfo.Period);
                            string beginTime = periodJson[0].ToString();
                            var beginSplited = beginTime.Split(':');
                            period.BeginHour = int.Parse(beginSplited[0]);
                            period.BeginMinute = int.Parse(beginSplited[1]);

                            string endTime = periodJson[1].ToString();
                            var endSplited = endTime.Split(':');
                            period.EndHour = int.Parse(endSplited[0]);
                            period.EndMinute = int.Parse(endSplited[1]);
                            periodList.Add(period);
                            if (_customInfrastInfoOutput)
                            {
                                AddLog(string.Format("[ {0:D2}:{1:D2} - {2:D2}:{3:D2} ]",
                                    period.BeginHour, period.BeginMinute,
                                    period.EndHour, period.EndMinute));
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

                _customInfrastInfoOutput = true;
            }
            catch (Exception)
            {
                _customInfrastInfoOutput = true;
                AddLog(LocalizationHelper.GetString("CustomInfrastFileParseFailed"), UiLogColor.Error);
                return;
            }

            refreshCustomInfrastPlanIndexByPeriod();
        }

        private void refreshCustomInfrastPlanIndexByPeriod()
        {
            if (!CustomInfrastEnabled || !_customInfrastPlanHasPeriod || InfrastTaskRunning)
            {
                return;
            }

            static bool timeLess(int lHour, int lMin, int rHour, int rMin) => (lHour != rHour) ? (lHour < rHour) : (lMin <= rMin);

            var now = DateTime.Now;
            foreach (var plan in CustomInfrastPlanInfoList)
            {
                foreach (var period in plan.PeriodList)
                {
                    if (timeLess(period.BeginHour, period.BeginMinute, now.Hour, now.Minute) &&
                        timeLess(now.Hour, now.Minute, period.EndHour, period.EndMinute))
                    {
                        CustomInfrastPlanIndex = plan.Index;
                        return;
                    }
                }
            }
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
        public bool UseMedicine
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

                // If the amount of medicine is 0, the stone is not used.
                UseStone = UseStone;
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
                // If the amount of medicine is 0, the stone is not used.
                if (!int.TryParse(MedicineNumber, out int result) || result == 0)
                {
                    value = false;
                }

                SetAndNotify(ref _useStoneWithNull, value);
                if (value != false && !UseMedicine)
                {
                    UseMedicine = true;
                }

                SetFightParams();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use originiums.
        /// </summary>
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
        public bool HasTimesLimited
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
        public bool IsSpecifiedDrops
        {
            get => IsSpecifiedDropsWithNull != false;
            set => IsSpecifiedDropsWithNull = value;
        }

        /// <summary>
        /// Gets or sets the list of all drops.
        /// </summary>
        public List<CombinedData> AllDrops { get; set; } = new List<CombinedData>();

        /// <summary>
        /// 关卡不可掉落的材料
        /// </summary>
        private static readonly HashSet<string> excludedValues = new HashSet<string>()
        {
            "3213", "3223", "3233", "3243", // 双芯片
            "3253", "3263", "3273", "3283",
            "7001", "7002", "7003", "7004", // 许可/凭证
            "4004", "4005",
            "3105", "3131", "3132", "3233", // 龙骨/加固建材
            "6001",                         // 演习券
            "3141", "4002",                 // 源石
            "32001",                        // 芯片助剂
            "30115",                        // 聚合剂
            "30125",                        // 双极纳米片
            "30135",                        // D32钢
            "30145",                        // 晶体电子单元
            "30155",                        // 烧结核凝晶
        };

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

                if (excludedValues.Contains(val))
                {
                    continue;
                }

                AllDrops.Add(new CombinedData { Display = dis, Value = val });
            }

            AllDrops.Sort((a, b) =>
            {
                return string.Compare(a.Value, b.Value, StringComparison.Ordinal);
            });
            DropsList = new ObservableCollection<CombinedData>(AllDrops);
        }

        /// <summary>
        /// Gets or sets the list of drops.
        /// </summary>
        public ObservableCollection<CombinedData> DropsList { get; set; }

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

        public void DropsListDropDownClosed()
        {
            foreach (var item in DropsList)
            {
                if (DropsItemName == item.Display)
                {
                    DropsItemId = item.Value;

                    if (DropsItemName != item.Display || DropsItemId != item.Value)
                    {
                        DropsItemName = LocalizationHelper.GetString("NotSelected");
                    }

                    return;
                }
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
        /// DropsList ComboBox loaded
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event args</param>
        public void DropsList_Loaded(object sender, EventArgs e)
        {
            (sender as ComboBox)?.MakeComboBoxSearchable();
        }

        #endregion Drops
    }
}
