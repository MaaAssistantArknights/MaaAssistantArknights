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
using System.Windows.Controls;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
{
    /// <summary>
    /// The view model of task queue.
    /// </summary>
    public class TaskQueueViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        private StageManager _stageManager;
        private SettingsViewModel _settingsViewModel;
        private AsstProxy _asstProxy;

        /// <summary>
        /// Gets or sets the view models of task items.
        /// </summary>
        public ObservableCollection<DragItemViewModel> TaskItemViewModels { get; set; }

        /// <summary>
        /// 实时更新任务顺序
        /// </summary>
        public void TaskItemSelectionChanged()
        {
            int index = 0;
            foreach (var item in TaskItemViewModels)
            {
                ViewStatusStorage.Set("TaskQueue.Order." + item.OriginalName, index.ToString());
                ++index;
            }
        }

        /// <summary>
        /// Gets or sets the view models of log items.
        /// </summary>
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        private string _actionAfterCompleted = ViewStatusStorage.Get("MainFunction.ActionAfterCompleted", ActionType.DoNothing.ToString());

        /// <summary>
        /// Gets or sets the list of the actions after completion.
        /// </summary>
        public List<GenericCombData<ActionType>> ActionAfterCompletedList { get; set; }

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

                if (value == ActionType.HibernateWithoutPersist || value == ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist
                    || value == ActionType.ShutdownWithoutPersist)
                {
                    storeValue = ActionType.DoNothing.ToString();
                }

                ViewStatusStorage.Set("MainFunction.ActionAfterCompleted", storeValue);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TaskQueueViewModel"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        /// <param name="windowManager">The window manager.</param>
        public TaskQueueViewModel(IContainer container, IWindowManager windowManager)
        {
            _windowManager = windowManager;
            _container = container;
        }

        protected override void OnInitialActivate()
        {
            base.OnInitialActivate();
            _settingsViewModel = _container.Get<SettingsViewModel>();
            _asstProxy = _container.Get<AsstProxy>();
            _stageManager = _container.Get<StageManager>();

            DisplayName = Localization.GetString("Farming");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            InitializeItems();
            InitTimer();

            if (_settingsViewModel.LoadGUIParameters && _settingsViewModel.SaveGUIParametersOnClosing)
            {
                Application.Current.MainWindow.Closing += _settingsViewModel.SaveGUIParameters;
            }
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

        private readonly System.Windows.Forms.Timer _timer = new System.Windows.Forms.Timer();

        private void InitTimer()
        {
            _timer.Enabled = true;
            _timer.Interval = 1000 * 50;
            _timer.Tick += Timer1_Elapsed;
            _timer.Start();
        }

        private void Timer1_Elapsed(object sender, EventArgs e)
        {
            if (NeedToUpdateDatePrompt())
            {
                Execute.OnUIThread(() =>
                {
                    _stageManager.UpdateStage(true);
                    UpdateDatePrompt();
                    UpdateStageList(false);
                });
            }

            refreshCustomInfrastPlanIndexByPeriod();

            if (!Idle)
            {
                return;
            }

            int intMinute = DateTime.Now.Minute;
            int intHour = DateTime.Now.Hour;
            var timeToStart = false;
            for (int i = 0; i < 8; ++i)
            {
                if (_settingsViewModel.TimerModels.Timers[i].IsOn &&
                    _settingsViewModel.TimerModels.Timers[i].Hour == intHour &&
                    _settingsViewModel.TimerModels.Timers[i].Min == intMinute)
                {
                    timeToStart = true;
                    break;
                }
            }

            if (timeToStart)
            {
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
            ActionAfterCompletedList = new List<GenericCombData<ActionType>>
            {
                new GenericCombData<ActionType> { Display = Localization.GetString("DoNothing"), Value = ActionType.DoNothing },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitArknights"), Value = ActionType.StopGame },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitEmulator"), Value = ActionType.ExitEmulator },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitSelf"), Value = ActionType.ExitSelf },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitEmulatorAndSelf"), Value = ActionType.ExitEmulatorAndSelf },

                // new GenericCombData<ActionTypeAfterCompleted>{ Display="待机",Value=ActionTypeAfterCompleted.Suspend },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitEmulatorAndSelfAndHibernate"), Value = ActionType.ExitEmulatorAndSelfAndHibernate },
                new GenericCombData<ActionType> { Display = Localization.GetString("Hibernate"), Value = ActionType.Hibernate },
                new GenericCombData<ActionType> { Display = Localization.GetString("Shutdown"), Value = ActionType.Shutdown },

                // new GenericCombData<ActionType> { Display = Localization.GetString("ExitEmulatorAndSelfAndHibernate") + "*", Value = ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist },
                new GenericCombData<ActionType> { Display = Localization.GetString("Hibernate") + "*", Value = ActionType.HibernateWithoutPersist },
                new GenericCombData<ActionType> { Display = Localization.GetString("Shutdown") + "*", Value = ActionType.ShutdownWithoutPersist },
            };
            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[task_list.Length]);
            var non_order_list = new List<DragItemViewModel>();
            for (int i = 0; i != task_list.Length; ++i)
            {
                var task = task_list[i];
                bool parsed = int.TryParse(ViewStatusStorage.Get("TaskQueue.Order." + task, "-1"), out var order);

                var vm = new DragItemViewModel(Localization.GetString(task), task, "TaskQueue.");

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

            RemainingSanityStageList = new ObservableCollection<CombData>(_stageManager.GetStageList())
            {
                // It's Cur/Last option
                [0] = new CombData { Display = Localization.GetString("NoUse"), Value = string.Empty },
            };

            InitDrops();
            NeedToUpdateDatePrompt();
            UpdateDatePrompt();
            UpdateStageList(true);
            RefreshCustonInfrastPlan();
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
            if (_settingsViewModel.HideUnavailableStage)
            {
                // update available stage list
                var stage1 = Stage1 ??= string.Empty;
                StageList = new ObservableCollection<CombData>(_stageManager.GetStageList(_curDayOfWeek));

                // reset closed stage1 to "Last/Current"
                if (!CustomStageCode)
                {
                    Stage1 = _stageManager.IsStageOpen(stage1, _curDayOfWeek) ? stage1 : string.Empty;
                }
            }
            else
            {
                // initializing or settings changing, update stage list forcely
                if (forceUpdate)
                {
                    var stage1 = Stage1 ??= string.Empty;
                    var stage2 = Stage2 ??= string.Empty;
                    var stage3 = Stage3 ??= string.Empty;

                    EnableSetFightParams = false;

                    StageList = new ObservableCollection<CombData>(_stageManager.GetStageList());

                    // reset closed stages to "Last/Current"
                    if (!CustomStageCode)
                    {
                        Stage1 = StageList.Any(x => x.Value == stage1) ? stage1 : string.Empty;
                        Stage2 = StageList.Any(x => x.Value == stage2) ? stage2 : string.Empty;
                        Stage3 = StageList.Any(x => x.Value == stage3) ? stage3 : string.Empty;
                    }

                    EnableSetFightParams = true;
                }
            }

            var rss = RemainingSanityStage ??= string.Empty;
            RemainingSanityStageList = new ObservableCollection<CombData>(_stageManager.GetStageList())
            {
                [0] = new CombData { Display = Localization.GetString("NoUse"), Value = string.Empty },
            };
            if (!CustomStageCode)
            {
                RemainingSanityStage = RemainingSanityStageList.Any(x => x.Value == rss) ? rss : string.Empty;
            }
        }

        private bool NeedToUpdateDatePrompt()
        {
            var now = Utils.GetYJTimeNow();
            var hour = now.Hour;
            var min = now.Minute;

            // yj历的16点
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

        /// <summary>
        /// Updates date prompt.
        /// </summary>
        // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
        // 这个函数被列为public可见，意味着他注入对象前被调用
        public void UpdateDatePrompt()
        {
            var builder = new StringBuilder(Localization.GetString("TodaysStageTip") + "\n");

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
                    builder.Append(stage).Append(": ").AppendLine(Localization.GetString("ClosedStage"));
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
        public void AddLog(string content, string color = UILogColor.Trace, string weight = "Regular")
        {
            var log = new LogItemViewModel(content, color, weight);
            LogItemViewModels.Add(log);
            Logger.Info(content);
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        public void ClearLog()
        {
            LogItemViewModels.Clear();
            Logger.Info("\n");
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

        private bool _inverseMode = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.InverseMode", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use inverse mode.
        /// </summary>
        public bool InverseMode
        {
            get => _inverseMode;
            set
            {
                SetAndNotify(ref _inverseMode, value);
                InverseShowText = value ? Localization.GetString("Inverse") : Localization.GetString("Clear");
                InverseMenuText = value ? Localization.GetString("Clear") : Localization.GetString("Inverse");
                ViewStatusStorage.Set("MainFunction.InverseMode", value.ToString());
            }
        }

        /// <summary>
        /// The width of "Select All" when both.
        /// </summary>
        public const int SelectedAllWidthWhenBoth = 80;

        private int _selectedAllWidth =
            ViewStatusStorage.Get("GUI.InverseClearMode", "Clear") == "ClearInverse" ? SelectedAllWidthWhenBoth : 85;

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

        private bool _showInverse = ViewStatusStorage.Get("GUI.InverseClearMode", "Clear") == "ClearInverse";

        /// <summary>
        /// Gets or sets a value indicating whether "Select inversely" is visible.
        /// </summary>
        public bool ShowInverse
        {
            get => _showInverse;
            set => SetAndNotify(ref _showInverse, value);
        }

        private string _inverseShowText = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.InverseMode", bool.FalseString)) ? Localization.GetString("Inverse") : Localization.GetString("Clear");

        /// <summary>
        /// Gets or sets the text to be displayed for "Select inversely".
        /// </summary>
        public string InverseShowText
        {
            get => _inverseShowText;
            set => SetAndNotify(ref _inverseShowText, value);
        }

        private string _inverseMenuText = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.InverseMode", bool.FalseString)) ? Localization.GetString("Clear") : Localization.GetString("Inverse");

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
            if (Idle == false)
            {
                return;
            }

            Idle = false;

            // 虽然更改时已经保存过了，不过保险起见还是在点击开始之后再保存一次任务及基建列表
            TaskItemSelectionChanged();
            _settingsViewModel.InfrastOrderSelectionChanged();

            ClearLog();

            AddLog(Localization.GetString("ConnectingToEmulator"));

            string errMsg = string.Empty;
            var task = Task.Run(() =>
            {
                return _asstProxy.AsstConnect(ref errMsg);
            });
            bool caught = await task;

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            if (!caught)
            {
                AddLog(errMsg, UILogColor.Error);
                AddLog(Localization.GetString("ConnectFailed") + "\n" + Localization.GetString("TryToStartEmulator"));
                var subtask = Task.Run(() =>
                {
                    _settingsViewModel.TryToStartEmulator(true);
                });
                await subtask;
                task = Task.Run(() =>
                {
                    return _asstProxy.AsstConnect(ref errMsg);
                });
                caught = await task;
                if (!caught)
                {
                    AddLog(errMsg, UILogColor.Error);
                    Idle = true;
                    SetStopped();
                    return;
                }
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            bool ret = true;

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
                    ret &= appendInfrast();
                }
                else if (item.OriginalName == "WakeUp")
                {
                    ret &= appendStart();
                }
                else if (item.OriginalName == "Combat")
                {
                    ret &= appendFight();
                }
                else if (item.OriginalName == "Recruiting")
                {
                    ret &= appendRecruit();
                }
                else if (item.OriginalName == "Mall")
                {
                    ret &= appendMall();
                }
                else if (item.OriginalName == "Mission")
                {
                    ret &= _asstProxy.AsstAppendAward();
                }
                else if (item.OriginalName == "AutoRoguelike")
                {
                    ret &= AppendRoguelike();
                }
                else if (item.OriginalName == "ReclamationAlgorithm")
                {
                    ret &= AppendReclamation();
                }
                else
                {
                    --count;

                    // TODO 报错
                }
            }

            if (count == 0)
            {
                AddLog(Localization.GetString("UnselectedTask"));
                Idle = true;
                SetStopped();
                return;
            }

            // 一般是点了“停止”按钮了
            if (Stopping)
            {
                SetStopped();
                return;
            }

            ret &= _asstProxy.AsstStart();

            if (ret)
            {
                AddLog(Localization.GetString("Running"));
                if (!_settingsViewModel.AdbReplaced && !_settingsViewModel.IsAdbTouchMode())
                {
                    AddLog(Localization.GetString("AdbReplacementTips"), UILogColor.Info);
                }
            }
            else
            {
                AddLog(Localization.GetString("UnknownErrorOccurs"));
            }
        }

        /// <summary>
        /// Stops.
        /// </summary>
        public async void Stop()
        {
            Stopping = true;
            AddLog(Localization.GetString("Stopping"));
            var task = Task.Run(_asstProxy.AsstStop);
            await task;
        }

        public void SetStopped()
        {
            if (!Idle || Stopping)
            {
                AddLog(Localization.GetString("Stopped"));
            }

            Stopping = false;
            Idle = true;
        }

        private bool appendStart()
        {
            var mode = _settingsViewModel.ClientType;
            var enable = mode.Length != 0;
            return _asstProxy.AsstAppendStartUp(mode, enable);
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

            bool mainFightRet = _asstProxy.AsstAppendFight(Stage, medicine, stone, times, DropsItemId, drops_quantity);

            if (mainFightRet && (Stage == "Annihilation") && _settingsViewModel.UseAlternateStage)
            {
                foreach (var stage in new[] { Stage1, Stage2, Stage3 })
                {
                    if (IsStageOpen(stage) && (stage != Stage))
                    {
                        mainFightRet = _asstProxy.AsstAppendFight(stage, medicine, 0, int.MaxValue, string.Empty, 0);
                    }
                }
            }

            if (mainFightRet && UseRemainingSanityStage && (RemainingSanityStage != string.Empty))
            {
                return _asstProxy.AsstAppendFight(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
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

                _asstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, DropsItemId, drops_quantity);
            }
        }

        public void SetFightRemainingSanityParams()
        {
            _asstProxy.AsstSetFightTaskParams(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
        }

        public void SetInfrastParams()
        {
            var order = _settingsViewModel.GetInfrastOrderList();
            _asstProxy.AsstSetInfrastTaskParams(order.ToArray(), _settingsViewModel.UsesOfDrones, _settingsViewModel.DormThreshold / 100.0, _settingsViewModel.DormFilterNotStationedEnabled, _settingsViewModel.DormTrustEnabled, _settingsViewModel.OriginiumShardAutoReplenishment,
                _settingsViewModel.CustomInfrastEnabled, _settingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendInfrast()
        {
            var order = _settingsViewModel.GetInfrastOrderList();
            return _asstProxy.AsstAppendInfrast(order.ToArray(), _settingsViewModel.UsesOfDrones, _settingsViewModel.DormThreshold / 100.0, _settingsViewModel.DormFilterNotStationedEnabled, _settingsViewModel.DormTrustEnabled, _settingsViewModel.OriginiumShardAutoReplenishment,
                _settingsViewModel.CustomInfrastEnabled, _settingsViewModel.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendMall()
        {
            var buy_first = _settingsViewModel.CreditFirstList.Split(new char[] { ';', '；' });
            var black_list = _settingsViewModel.CreditBlackList.Split(new char[] { ';', '；' });
            for (var i = 0; i < buy_first.Length; ++i)
            {
                buy_first[i] = buy_first[i].Trim();
            }

            for (var i = 0; i < black_list.Length; ++i)
            {
                black_list[i] = black_list[i].Trim();
            }

            return _asstProxy.AsstAppendMall(
                this.Stage != string.Empty && _settingsViewModel.CreditFightTaskEnabled,
                _settingsViewModel.CreditShopping,
                buy_first,
                black_list,
                _settingsViewModel.CreditForceShoppingIfCreditFull);
        }

        private bool appendRecruit()
        {
            // for debug
            if (!int.TryParse(_settingsViewModel.RecruitMaxTimes, out var max_times))
            {
                max_times = 0;
            }

            var reqList = new List<int>();
            var cfmList = new List<int>();

            if (_settingsViewModel.ChooseLevel3)
            {
                cfmList.Add(3);
            }

            if (_settingsViewModel.ChooseLevel4)
            {
                reqList.Add(4);
                cfmList.Add(4);
            }

            if (_settingsViewModel.ChooseLevel5)
            {
                reqList.Add(5);
                cfmList.Add(5);
            }

            return _asstProxy.AsstAppendRecruit(
                max_times, reqList.ToArray(), cfmList.ToArray(),
                _settingsViewModel.RefreshLevel3, _settingsViewModel.UseExpedited,
                _settingsViewModel.NotChooseLevel1, _settingsViewModel.IsLevel3UseShortTime);
        }

        private bool AppendRoguelike()
        {
            int.TryParse(_settingsViewModel.RoguelikeMode, out var mode);

            return _asstProxy.AsstAppendRoguelike(
                mode, _settingsViewModel.RoguelikeStartsCount,
                _settingsViewModel.RoguelikeInvestmentEnabled, _settingsViewModel.RoguelikeInvestsCount, _settingsViewModel.RoguelikeStopWhenInvestmentFull,
                _settingsViewModel.RoguelikeSquad, _settingsViewModel.RoguelikeRoles, _settingsViewModel.RoguelikeCoreChar, _settingsViewModel.RoguelikeUseSupportUnit,
                _settingsViewModel.RoguelikeEnableNonfriendSupport, _settingsViewModel.RoguelikeTheme);
        }

        private bool AppendReclamation()
        {
            return _asstProxy.AsstAppendReclamation();
        }

        [DllImport("User32.dll", EntryPoint = "FindWindow")]
        private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("User32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);

        /// <summary>
        /// Kills emulator by Window hwnd.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool KillEumlatorbyWindow()
        {
            IntPtr hwnd;
            int pid = 0;
            var windowname = new[]
            {
                "明日方舟",
                "夜神模拟器",
                "逍遥模拟器",
                "雷电模拟器(64)",
                "雷电模拟器",
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
                        emulator.WaitForExit(5000);
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
            string address = ViewStatusStorage.Get("Connect.Address", string.Empty);
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
            switch (ActionAfterCompleted)
            {
                case ActionType.DoNothing:
                    break;

                case ActionType.StopGame:
                    if (!_asstProxy.AsstStartCloseDown())
                    {
                        AddLog(Localization.GetString("CloseArknightsFailed"), UILogColor.Error);
                    }

                    break;

                case ActionType.ExitSelf:
                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Application.Current.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.ExitEmulator:
                    if (!KillEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("ExitEmulatorFailed"), UILogColor.Error);
                    }

                    break;

                case ActionType.ExitEmulatorAndSelf:
                    if (!KillEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("ExitEmulatorFailed"), UILogColor.Error);
                    }

                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Application.Current.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.Shutdown:
                case ActionType.ShutdownWithoutPersist:
                    Process.Start("shutdown.exe", "-s -t 60");

                    // 关机询问
                    var shutdownResult = _windowManager.ShowMessageBox(Localization.GetString("AboutToShutdown"), Localization.GetString("ShutdownPrompt"), MessageBoxButton.OK, MessageBoxImage.Question);
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

                case ActionType.Hibernate:
                case ActionType.HibernateWithoutPersist:
                    // 休眠提示
                    AddLog(Localization.GetString("HibernatePrompt"), UILogColor.Error);

                    // 休眠不能加时间参数，https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1133
                    Process.Start("shutdown.exe", "-h");
                    break;

                case ActionType.ExitEmulatorAndSelfAndHibernate:
                case ActionType.ExitEmulatorAndSelfAndHibernateWithoutPersist:
                    if (!KillEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("ExitEmulatorFailed"), UILogColor.Error);
                    }

                    // 休眠提示
                    AddLog(Localization.GetString("HibernatePrompt"), UILogColor.Error);

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

        private bool _idle = false;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set
            {
                SetAndNotify(ref _idle, value);
                _settingsViewModel.Idle = value;
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

        private ObservableCollection<CombData> _stageList = new ObservableCollection<CombData>();

        /// <summary>
        /// Gets or sets the list of stages.
        /// </summary>
        public ObservableCollection<CombData> StageList
        {
            get => _stageList;
            set => SetAndNotify(ref _stageList, value);
        }

        public ObservableCollection<CombData> RemainingSanityStageList { get; set; } = new ObservableCollection<CombData>();

        /// <summary>
        /// Gets the stage.
        /// </summary>
        public string Stage
        {
            get
            {
                if (CustomStageCode)
                {
                    return Stage1;
                }

                if (_settingsViewModel.UseAlternateStage)
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

        private string ToUpperAndCheckStage(string value)
        {
            if (string.IsNullOrEmpty(value))
            {
                return value;
            }

            value = value.ToUpper();
            if (StageList != null)
            {
                foreach (var item in StageList)
                {
                    if (value == item.Value)
                    {
                        break;
                    }
                    else if (value == item.Display)
                    {
                        value = item.Value;
                        break;
                    }
                }
            }

            return value;
        }

        private string _stage1 = ViewStatusStorage.Get("MainFunction.Stage1", string.Empty) ?? string.Empty;

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
                    return;
                }

                if (CustomStageCode)
                {
                    value = ToUpperAndCheckStage(value);
                }

                SetAndNotify(ref _stage1, value);
                SetFightParams();
                ViewStatusStorage.Set("MainFunction.Stage1", value);
                UpdateDatePrompt();
            }
        }

        private string _stage2 = ViewStatusStorage.Get("MainFunction.Stage2", string.Empty) ?? string.Empty;

        /// <summary>
        /// Gets or sets the stage2.
        /// </summary>
        public string Stage2
        {
            get => _stage2;
            set
            {
                SetAndNotify(ref _stage2, value);
                SetFightParams();
                ViewStatusStorage.Set("MainFunction.Stage2", value);
                UpdateDatePrompt();
            }
        }

        private string _stage3 = ViewStatusStorage.Get("MainFunction.Stage3", string.Empty) ?? string.Empty;

        /// <summary>
        /// Gets or sets the stage2.
        /// </summary>
        public string Stage3
        {
            get => _stage3;
            set
            {
                SetAndNotify(ref _stage3, value);
                SetFightParams();
                ViewStatusStorage.Set("MainFunction.Stage3", value);
                UpdateDatePrompt();
            }
        }

        private bool _useAlternateStage = Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseAlternateStage", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use alternate stage.
        /// </summary>
        public bool UseAlternateStage
        {
            get => _useAlternateStage;
            set => SetAndNotify(ref _useAlternateStage, value);
        }

        private bool _useRemainingSanityStage = Convert.ToBoolean(ViewStatusStorage.Get("Fight.UseRemainingSanityStage", bool.TrueString));

        public bool UseRemainingSanityStage
        {
            get => _useRemainingSanityStage;
            set => SetAndNotify(ref _useRemainingSanityStage, value);
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
            }
        }

        private string _remainingSanityStage = ViewStatusStorage.Get("Fight.RemainingSanityStage", string.Empty) ?? string.Empty;

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
                    return;
                }

                if (CustomStageCode)
                {
                    value = ToUpperAndCheckStage(value);
                }

                SetAndNotify(ref _remainingSanityStage, value);
                SetFightRemainingSanityParams();
                ViewStatusStorage.Set("Fight.RemainingSanityStage", value);
            }
        }

        private bool _customInfrastEnabled = Convert.ToBoolean(ViewStatusStorage.Get("Infrast.CustomInfrastEnabled", bool.FalseString));

        public bool CustomInfrastEnabled
        {
            get => _customInfrastEnabled;
            set
            {
                SetAndNotify(ref _customInfrastEnabled, value);
                RefreshCustonInfrastPlan();
            }
        }

        private int _customInfrastPlanIndex = Convert.ToInt32(ViewStatusStorage.Get("Infrast.CustomInfrastPlanIndex", "0"));

        public int CustomInfrastPlanIndex
        {
            get => _customInfrastPlanIndex;
            set
            {
                if (value != _customInfrastPlanIndex)
                {
                    var plan = CustomInfrastPlanInfoList[value];
                    AddLog(plan.Name, UILogColor.Message);

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
                ViewStatusStorage.Set("Infrast.CustomInfrastPlanIndex", value.ToString());
            }
        }

        public ObservableCollection<GenericCombData<int>> CustomInfrastPlanList { get; set; } = new ObservableCollection<GenericCombData<int>>();

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

            if (!File.Exists(_settingsViewModel.CustomInfrastFile))
            {
                return;
            }

            try
            {
                string jsonStr = File.ReadAllText(_settingsViewModel.CustomInfrastFile);
                var root = (JObject)JsonConvert.DeserializeObject(jsonStr);

                if (_customInfrastInfoOutput && root.ContainsKey("title"))
                {
                    AddLog(Localization.GetString("CustomInfrastTitle"), UILogColor.Message);
                    AddLog(root["title"].ToString(), UILogColor.Info);
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
                    CustomInfrastPlanList.Add(new GenericCombData<int> { Display = display, Value = i });
                    string desc = plan.ContainsKey("description") ? plan["description"].ToString() : string.Empty;
                    string descPost = plan.ContainsKey("description_post") ? plan["description_post"].ToString() : string.Empty;

                    if (_customInfrastInfoOutput)
                    {
                        AddLog(display, UILogColor.Message);
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
                AddLog(Localization.GetString("CustomInfrastFileParseFailed"), UILogColor.Error);
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

            AddLog(Localization.GetString("CustomInfrastPlanIndexAutoSwitch"), UILogColor.Message);
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

        private bool _useMedicine = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.UseMedicine", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use medicine.
        /// </summary>
        public bool UseMedicine
        {
            get => _useMedicine;
            set
            {
                SetAndNotify(ref _useMedicine, value);
                if (!value)
                {
                    UseStone = false;
                }

                SetFightParams();
                ViewStatusStorage.Set("MainFunction.UseMedicine", value.ToString());
            }
        }

        private string _medicineNumber = ViewStatusStorage.Get("MainFunction.UseMedicine.Quantity", "999");

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
                ViewStatusStorage.Set("MainFunction.UseMedicine.Quantity", MedicineNumber);
            }
        }

        private bool _useStone;

        /// <summary>
        /// Gets or sets a value indicating whether to use originiums.
        /// </summary>
        public bool UseStone
        {
            get => _useStone;
            set
            {
                // If the amount of medicine is 0, the stone is not used.
                if (!int.TryParse(MedicineNumber, out int result) || result == 0)
                {
                    value = false;
                }

                SetAndNotify(ref _useStone, value);
                if (value)
                {
                    UseMedicine = true;
                }

                SetFightParams();
            }
        }

        private string _stoneNumber = ViewStatusStorage.Get("MainFunction.UseStone.Quantity", "0");

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
                ViewStatusStorage.Set("MainFunction.UseStone.Quantity", StoneNumber);
            }
        }

        private bool _hasTimesLimited;

        /// <summary>
        /// Gets or sets a value indicating whether the number of times is limited.
        /// </summary>
        public bool HasTimesLimited
        {
            get => _hasTimesLimited;
            set
            {
                SetAndNotify(ref _hasTimesLimited, value);
                SetFightParams();
            }
        }

        private string _maxTimes = ViewStatusStorage.Get("MainFunction.TimesLimited.Quantity", "5");

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
                ViewStatusStorage.Set("MainFunction.TimesLimited.Quantity", MaxTimes);
            }
        }

        #region Drops

        private bool _isSpecifiedDrops = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.Drops.Enable", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether the drops are specified.
        /// </summary>
        public bool IsSpecifiedDrops
        {
            get => _isSpecifiedDrops;
            set
            {
                SetAndNotify(ref _isSpecifiedDrops, value);
                SetFightParams();
                ViewStatusStorage.Set("MainFunction.Drops.Enable", value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets the list of all drops.
        /// </summary>
        public List<CombData> AllDrops { get; set; } = new List<CombData>();

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
            var reader = Utils.GetItemList();
            foreach (var item in reader)
            {
                var val = item.Key;

                // 不是数字的东西都是正常关卡不会掉的（大概吧）
                if (!int.TryParse(val, out _))
                {
                    continue;
                }

                var dis = item.Value["name"].ToString();

                if (excludedValues.Contains(val))
                {
                    continue;
                }

                AllDrops.Add(new CombData { Display = dis, Value = val });
            }

            AllDrops.Sort((a, b) =>
            {
                return string.Compare(a.Value, b.Value, StringComparison.Ordinal);
            });
            DropsList = new ObservableCollection<CombData>(AllDrops);
        }

        /// <summary>
        /// Gets or sets the list of drops.
        /// </summary>
        public ObservableCollection<CombData> DropsList { get; set; }

        private string _dropsItemId = ViewStatusStorage.Get("MainFunction.Drops.ItemId", string.Empty);

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
                ViewStatusStorage.Set("MainFunction.Drops.ItemId", DropsItemId);
            }
        }

        private string _dropsItemName = ViewStatusStorage.Get("MainFunction.Drops.ItemName", Localization.GetString("NotSelected"));

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
                ViewStatusStorage.Set("MainFunction.Drops.ItemName", DropsItemName);
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
                        DropsItemName = Localization.GetString("NotSelected");
                    }

                    return;
                }
            }

            DropsItemName = Localization.GetString("NotSelected");
        }

        private string _dropsQuantity = ViewStatusStorage.Get("MainFunction.Drops.Quantity", "5");

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
                ViewStatusStorage.Set("MainFunction.Drops.Quantity", DropsQuantity);
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
