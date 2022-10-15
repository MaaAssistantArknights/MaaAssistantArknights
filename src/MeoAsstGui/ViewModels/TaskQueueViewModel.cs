// <copyright file="TaskQueueViewModel.cs" company="MaaAssistantArknights">
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
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using MeoAsstGui.Helper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of task queue.
    /// </summary>
    public class TaskQueueViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

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
                if (Enum.TryParse(_actionAfterCompleted, out ActionType action))
                {
                    return action;
                }

                return ActionType.DoNothing;
            }

            set
            {
                SetAndNotify(ref _actionAfterCompleted, value.ToString());
                string storeValue = ActionType.DoNothing.ToString();
                if (value != ActionType.Shutdown && value != ActionType.Hibernate && value != ActionType.ExitEmulatorAndSelfAndHibernate)
                {
                    storeValue = value.ToString();
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
            _container = container;
            _windowManager = windowManager;
            DisplayName = Localization.GetString("Farming");
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            InitializeItems();
            InitTimer();
            var trayIcon = _container.Get<TrayIcon>();
            trayIcon.SetTaskQueueViewModel(this);
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
            if (CheckAndUpdateDayOfWeek())
            {
                UpdateDatePrompt();
                UpdateStageList(false);
            }

            refreshCustomInfrastPlanIndexByPeriod();

            int intMinute = DateTime.Now.Minute;

            if (intMinute != 0 || Idle == false)
            {
                return;
            }

            int intHour = DateTime.Now.Hour;
            var settings = _container.Get<SettingsViewModel>();
            if ((settings.Timer1 && settings.Timer1Hour == intHour) ||
                (settings.Timer2 && settings.Timer2Hour == intHour) ||
                (settings.Timer3 && settings.Timer3Hour == intHour) ||
                (settings.Timer4 && settings.Timer4Hour == intHour) ||
                (settings.Timer5 && settings.Timer5Hour == intHour) ||
                (settings.Timer6 && settings.Timer6Hour == intHour) ||
                (settings.Timer7 && settings.Timer7Hour == intHour) ||
                (settings.Timer8 && settings.Timer8Hour == intHour))
            {
                LinkStart();
            }
        }

        // TODO: Delete this.

        /// <summary>
        /// Delete old configurations.
        /// Note that this feature ought to be removed in later versions.
        /// </summary>
        public void DeleteOldConfig()
        {
            string[] saved_list_name_1 = new string[]
            {
                "WakeUp", "Recruiting", "Base", "Combat", "Visiting", "Mall", "Mission", "AutoRoguelike",
            };
            foreach (var name in saved_list_name_1)
            {
                string local_name = Localization.GetString(name);
                string check = ViewStatusStorage.Get("TaskQueue." + local_name + ".IsChecked", string.Empty);
                string order = ViewStatusStorage.Get("TaskQueue.Order." + local_name, string.Empty);
                if (check != string.Empty)
                {
                    ViewStatusStorage.Set("TaskQueue." + name + ".IsChecked", check);
                }

                if (order != string.Empty)
                {
                    ViewStatusStorage.Set("TaskQueue.Order." + name, order);
                }
            }

            string[] saved_list_name_2 = new string[]
            {
                "Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm",
            };
            foreach (var name in saved_list_name_2)
            {
                string local_name = Localization.GetString(name);
                string check = ViewStatusStorage.Get("Infrast." + local_name + ".IsChecked", string.Empty);
                string order = ViewStatusStorage.Get("Infrast.Order." + local_name, string.Empty);
                if (check != string.Empty)
                {
                    ViewStatusStorage.Set("Infrast." + name + ".IsChecked", check);
                }

                if (order != string.Empty)
                {
                    ViewStatusStorage.Set("Infrast.Order." + name, order);
                }
            }

            string[] old_list_name = new string[]
            {
                "开始唤醒", "自动公招", "基建换班", "刷理智", "访问好友", "收取信用及购物", "领取日常奖励", "自动肉鸽",
                "宿舍", "制造站", "贸易站", "发电站", "会客室", "办公室", "控制中枢",
                "開始喚醒", "自動公招", "基建換班", "刷理智", "訪問好友", "收取信用及購物", "領取日常獎勵", "自動肉鴿",
                "宿舍", "製造站", "貿易站", "發電站", "會客室", "辦公室", "控制中樞",
                "ウェイクアップ", "公開求人", "基地仕事", "作戦", "戦友訪問", "FP交換", "報酬受取", "自動ローグ",
                "宿舎", "製造所", "貿易所", "発電所", "応接室", "事務所", "制御中枢",
                "웨이크업", "공개모집", "기반시설 교대", "작전", "친구 방문", "상점", "일일 퀘스트 보상을 수집", "통합전략",
                "숙소", "제조소", "무역소", "발전소", "응접실", "사무실", "제어 센터",
                "🍸💃💃", "🍸🍺🍻", "🍺🍸🍺", "🍸🍷", "🍺🍸🍷", "🍻🍺🍸🍻", "🍺🍸🕺🍸", "🍺🍸🍸",
                "🍻💃", "🕺🍺", "🍺🍺", "🍺🍸", "🍺🍻", "🕺🍸", "🍻🍸🍻",
                "Login", "Recruit", "Visit Friends", "Credit Store", "Collect mission rewards", "Auto I.S.",
                "Manufacturing Station", "Trade Post", "Power Station", "Reception Room", "Control Center",
            };
            foreach (var name in old_list_name)
            {
                ViewStatusStorage.Delete("TaskQueue." + name + ".IsChecked");
                ViewStatusStorage.Delete("TaskQueue.Order." + name);
                ViewStatusStorage.Delete("Infrast." + name + ".IsChecked");
                ViewStatusStorage.Delete("Infrast.Order." + name);
            }
        }

        /// <summary>
        /// Initializes items.
        /// </summary>
        public void InitializeItems()
        {
            DeleteOldConfig();

            string[] task_list = new string[]
            {
                "WakeUp",
                "Recruiting",
                "Base",
                "Combat",
                "Visiting",
                "Mall",
                "Mission",
                "AutoRoguelike",
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
            };
            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[task_list.Length]);
            var non_order_list = new List<DragItemViewModel>();
            for (int i = 0; i != task_list.Length; ++i)
            {
                var task = task_list[i];
                int order;
                bool parsed = int.TryParse(ViewStatusStorage.Get("TaskQueue.Order." + task, "-1"), out order);

                var vm = new DragItemViewModel(Localization.GetString(task), task, "TaskQueue.");
                if (task == "AutoRoguelike")
                {
                    vm.IsChecked = false;
                }

                if (!parsed || order < 0)
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

            _stageManager = new StageManager();
            RemainingSanityStageList = new ObservableCollection<CombData>(_stageManager.GetStageList());

            // It's Cur/Last option
            RemainingSanityStageList[0] = new CombData { Display = Localization.GetString("NoUse"), Value = string.Empty };

            InitDrops();
            CheckAndUpdateDayOfWeek();
            UpdateDatePrompt();
            UpdateStageList(true);
            RefreshCustonInfrastPlan();
        }

        private StageManager _stageManager;
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
        public void UpdateStageList(bool forceUpdate)
        {
            var settingsModel = _container.Get<SettingsViewModel>();
            if (settingsModel.HideUnavailableStage)
            {
                // update available stage list
                var stage1 = Stage1;
                StageList = new ObservableCollection<CombData>(_stageManager.GetStageList(_curDayOfWeek));

                // reset closed stage1 to "Last/Current"
                if (!CustomStageCode &&
                    (stage1 == null || !_stageManager.IsStageOpen(stage1, _curDayOfWeek)))
                {
                    Stage1 = string.Empty;
                }
            }
            else
            {
                // initializing or settings changing, update stage list forcely
                if (forceUpdate)
                {
                    var stage1 = Stage1;
                    var stage2 = Stage2;
                    var stage3 = Stage3;

                    StageList = new ObservableCollection<CombData>(_stageManager.GetStageList());

                    // reset closed stages to "Last/Current"
                    if (!CustomStageCode && (stage1 == null))
                    {
                        Stage1 = string.Empty;
                    }

                    if (stage2 == null)
                    {
                        Stage2 = string.Empty;
                    }

                    if (stage3 == null)
                    {
                        Stage3 = string.Empty;
                    }
                }
                else
                {
                    // do nothing
                }
            }
        }

        private bool CheckAndUpdateDayOfWeek()
        {
            var now = DateTime.UtcNow.AddHours(8);
            var hour = now.Hour;
            if (hour >= 0 && hour < 4)
            {
                now = now.AddDays(-1);
            }

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

        /// <summary>
        /// Updates date prompt.
        /// </summary>
        public void UpdateDatePrompt()
        {
            var builder = new StringBuilder(Localization.GetString("TodaysStageTip") + "\n");

            // Closed activity stages
            var stages = new[] { Stage1, Stage2, Stage3 };
            foreach (var stage in stages)
            {
                if (_stageManager.GetStageInfo(stage)?.IsActivityClosed() == true)
                {
                    builder.Append(stage).Append(": ").AppendLine(Localization.GetString("ClosedStage"));
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
        public void AddLog(string content, string color = LogColor.Trace, string weight = "Regular")
        {
            var log = new LogItemViewModel(content, color, weight);
            LogItemViewModels.Add(log);
            File.AppendAllText("gui.log", log.Time + ' ' + log.Content + "\n");
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        public void ClearLog()
        {
            LogItemViewModels.Clear();
            File.AppendAllText("gui.log", "\n");
        }

        /// <summary>
        /// Selects all.
        /// </summary>
        public void SelectedAll()
        {
            foreach (var item in TaskItemViewModels)
            {
                if (item.OriginalName == "AutoRoguelike")
                {
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
                    if (item.OriginalName == "AutoRoguelike")
                    {
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
            _container.Get<SettingsViewModel>().InfrastOrderSelectionChanged();

            ClearLog();

            AddLog(Localization.GetString("ConnectingToEmulator"));

            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = string.Empty;
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg);
            });
            bool caught = await task;

            // 一般是点了“停止”按钮了
            if (Idle)
            {
                return;
            }

            if (!caught)
            {
                AddLog(errMsg, LogColor.Error);
                AddLog(Localization.GetString("ConnectFailed") + "\n" + Localization.GetString("TryToStartEmulator"));
                var settingsModel = _container.Get<SettingsViewModel>();
                var subtask = Task.Run(() =>
                {
                    settingsModel.TryToStartEmulator(true);
                });
                await subtask;
                task = Task.Run(() =>
                {
                    return asstProxy.AsstConnect(ref errMsg);
                });
                caught = await task;
                if (!caught)
                {
                    AddLog(errMsg, LogColor.Error);
                    Idle = true;
                    return;
                }
            }

            // 一般是点了“停止”按钮了
            if (Idle)
            {
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
                else if (item.OriginalName == "Visiting")
                {
                    ret &= asstProxy.AsstAppendVisit();
                }
                else if (item.OriginalName == "Mall")
                {
                    ret &= appendMall();
                }
                else if (item.OriginalName == "Mission")
                {
                    ret &= asstProxy.AsstAppendAward();
                }
                else if (item.OriginalName == "AutoRoguelike")
                {
                    ret &= appendRoguelike();
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
                return;
            }

            // 一般是点了“停止”按钮了
            if (Idle)
            {
                return;
            }

            ret &= asstProxy.AsstStart();

            if (ret)
            {
                AddLog(Localization.GetString("Running"));
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
            Idle = true; // 提前将 Idle 置为 true
            Stopping = true;
            AddLog(Localization.GetString("Stopping"));
            var task = Task.Run(() =>
            {
                return _container.Get<AsstProxy>().AsstStop();
            });
            await task;
            AddLog(Localization.GetString("Stopped"));
            Stopping = false;
        }

        private bool appendStart()
        {
            var settings = _container.Get<SettingsViewModel>();
            var asstProxy = _container.Get<AsstProxy>();
            var mode = settings.ClientType;
            var enable = mode.Length != 0;
            return asstProxy.AsstAppendStartUp(mode, enable);
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

            var asstProxy = _container.Get<AsstProxy>();
            bool mainFightRet = asstProxy.AsstAppendFight(Stage, medicine, stone, times, DropsItemId, drops_quantity);
            if (!mainFightRet || RemainingSanityStage == string.Empty)
            {
                return mainFightRet;
            }

            return asstProxy.AsstAppendFight(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
        }

        /// <summary>
        /// Sets parameters.
        /// </summary>
        public void SetFightParams()
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

            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, DropsItemId, drops_quantity);
        }

        public void SetFightRemainingSanityParams()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstSetFightTaskParams(RemainingSanityStage, 0, 0, int.MaxValue, string.Empty, 0, false);
        }

        public void SetInfrastParams()
        {
            var settings = _container.Get<SettingsViewModel>();
            var order = settings.GetInfrastOrderList();
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstSetInfrastTaskParams(order.ToArray(), settings.UsesOfDrones, settings.DormThreshold / 100.0, settings.DormFilterNotStationedEnabled, settings.DormTrustEnabled, settings.OriginiumShardAutoReplenishment,
                settings.CustomInfrastEnabled, settings.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendInfrast()
        {
            var settings = _container.Get<SettingsViewModel>();
            var order = settings.GetInfrastOrderList();
            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendInfrast(order.ToArray(), settings.UsesOfDrones, settings.DormThreshold / 100.0, settings.DormFilterNotStationedEnabled, settings.DormTrustEnabled, settings.OriginiumShardAutoReplenishment,
                settings.CustomInfrastEnabled, settings.CustomInfrastFile, CustomInfrastPlanIndex);
        }

        private bool appendMall()
        {
            var settings = _container.Get<SettingsViewModel>();
            var asstProxy = _container.Get<AsstProxy>();
            var buy_first = settings.CreditFirstList.Split(new char[] { ';', '；' });
            var black_list = settings.CreditBlackList.Split(new char[] { ';', '；' });
            for (var i = 0; i < buy_first.Length; ++i)
            {
                buy_first[i] = buy_first[i].Trim();
            }

            for (var i = 0; i < black_list.Length; ++i)
            {
                black_list[i] = black_list[i].Trim();
            }

            return asstProxy.AsstAppendMall(settings.CreditShopping, buy_first, black_list, settings.CreditForceShoppingIfCreditFull);
        }

        private bool appendRecruit()
        {
            // for debug
            var settings = _container.Get<SettingsViewModel>();

            int max_times;
            if (!int.TryParse(settings.RecruitMaxTimes, out max_times))
            {
                max_times = 0;
            }

            var reqList = new List<int>();
            var cfmList = new List<int>();

            if (settings.ChooseLevel3)
            {
                cfmList.Add(3);
            }

            if (settings.ChooseLevel4)
            {
                reqList.Add(4);
                cfmList.Add(4);
            }

            if (settings.ChooseLevel5)
            {
                reqList.Add(5);
                cfmList.Add(5);
            }

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRecruit(
                max_times, reqList.ToArray(), cfmList.ToArray(),
                settings.RefreshLevel3, settings.UseExpedited,
                settings.NotChooseLevel1, settings.IsLevel3UseShortTime);
        }

        private bool appendRoguelike()
        {
            var settings = _container.Get<SettingsViewModel>();
            int.TryParse(settings.RoguelikeMode, out var mode);

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRoguelike(
                mode, settings.RoguelikeStartsCount,
                settings.RoguelikeInvestmentEnabled, settings.RoguelikeInvestsCount, settings.RoguelikeStopWhenInvestmentFull,
                settings.RoguelikeSquad, settings.RoguelikeRoles, settings.RoguelikeCoreChar, settings.RoguelikeTheme);
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
                }
                catch
                {
                    break;
                }

                if (emulator.HasExited)
                {
                    break;
                }

                try
                {
                    emulator.Kill();
                }
                catch
                {
                    break;
                }

                // 尽管已经成功 CloseMainWindow()，再次尝试 killEmulator()
                // Refer to https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/1878
                KillEmulator();

                // 已经成功 CloseMainWindow()，所以不管 killEmulator() 的结果如何，都返回 true
                return true;
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
            checkCmd.Start();
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

            Process emulator = Process.GetProcessById(pid);
            try
            {
                emulator.Kill();
            }
            catch
            {
                return false;
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
                    var asstProxy = _container.Get<AsstProxy>();
                    if (!asstProxy.AsstStartCloseDown())
                    {
                        AddLog(Localization.GetString("CloseArknightsFailed"), LogColor.Error);
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
                        AddLog(Localization.GetString("ExitEmulatorFailed"), LogColor.Error);
                    }

                    break;

                case ActionType.ExitEmulatorAndSelf:
                    if (!KillEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("ExitEmulatorFailed"), LogColor.Error);
                    }

                    // Shutdown 会调用 OnExit 但 Exit 不会
                    Application.Current.Shutdown();

                    // Environment.Exit(0);
                    break;

                case ActionType.Shutdown:
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
                    // 休眠提示
                    AddLog(Localization.GetString("HibernatePrompt"), LogColor.Error);

                    // 休眠不能加时间参数，https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1133
                    Process.Start("shutdown.exe", "-h");
                    break;

                case ActionType.ExitEmulatorAndSelfAndHibernate:
                    if (!KillEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("ExitEmulatorFailed"), LogColor.Error);
                    }

                    // 休眠提示
                    AddLog(Localization.GetString("HibernatePrompt"), LogColor.Error);

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

        /*
        public void CheckAndShutdown()
        {
            if (Shutdown)
            {
                System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

                var result = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK, MessageBoxImage.Question);
                if (result == MessageBoxResult.OK)
                {
                    System.Diagnostics.Process.Start("shutdown.exe", "-a");
                }
            }
            if (Hibernate)
            {
                System.Diagnostics.Process.Start("shutdown.exe", "-h");
            }
            if (Suspend)
            {
                System.Diagnostics.Process.Start("rundll32.exe", "powrprof.dll,SetSuspendState 0,1,0");
            }
        }
        */

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
                var settings = _container.Get<SettingsViewModel>();
                settings.Idle = value;
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
                var settingsModel = _container.Get<SettingsViewModel>();
                if (CustomStageCode)
                {
                    return Stage1;
                }

                if (settingsModel.UseAlternateStage)
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

        private string _stage1 = ViewStatusStorage.Get("MainFunction.Stage1", string.Empty);

        /// <summary>
        /// Gets or sets the stage1.
        /// </summary>
        public string Stage1
        {
            get => _stage1;
            set
            {
                SetAndNotify(ref _stage1, value);
                SetFightParams();
                ViewStatusStorage.Set("MainFunction.Stage1", value);
                UpdateDatePrompt();
            }
        }

        private string _stage2 = ViewStatusStorage.Get("MainFunction.Stage2", string.Empty);

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

        private string _stage3 = ViewStatusStorage.Get("MainFunction.Stage3", string.Empty);

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

        private bool _alternateStageDisplay = !Convert.ToBoolean(ViewStatusStorage.Get("GUI.CustomStageCode", bool.FalseString))
            && Convert.ToBoolean(ViewStatusStorage.Get("GUI.UseAlternateStage", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use alternate stage.
        /// </summary>
        public bool AlternateStageDisplay
        {
            get => _alternateStageDisplay;
            set => SetAndNotify(ref _alternateStageDisplay, value);
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
                var settingsModel = _container.Get<SettingsViewModel>();
                AlternateStageDisplay = !value && settingsModel.UseAlternateStage;
            }
        }

        private string _remainingSanityStage = ViewStatusStorage.Get("Fight.RemainingSanityStage", string.Empty);

        public string RemainingSanityStage
        {
            get => _remainingSanityStage;
            set
            {
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
                    AddLog(plan.Name, LogColor.Message);

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

        public void RefreshCustonInfrastPlan()
        {
            CustomInfrastPlanInfoList.Clear();
            CustomInfrastPlanList.Clear();
            _customInfrastPlanHasPeriod = false;

            if (!CustomInfrastEnabled)
            {
                return;
            }

            var settingsModel = _container.Get<SettingsViewModel>();
            if (!File.Exists(settingsModel.CustomInfrastFile))
            {
                return;
            }

            try
            {
                string jsonStr = File.ReadAllText(settingsModel.CustomInfrastFile);
                var root = (JObject)JsonConvert.DeserializeObject(jsonStr);

                if (root.ContainsKey("title"))
                {
                    AddLog(Localization.GetString("CustomInfrastTitle"), LogColor.Message);
                    AddLog(root["title"].ToString(), LogColor.Info);
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

                    AddLog(display, LogColor.Message);

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
                            AddLog(string.Format("[ {0:D2}:{1:D2} - {2:D2}:{3:D2} ]",
                                period.BeginHour, period.BeginMinute,
                                period.EndHour, period.EndMinute));
                        }

                        if (periodList.Count != 0)
                        {
                            _customInfrastPlanHasPeriod = true;
                        }
                    }

                    if (desc != string.Empty)
                    {
                        AddLog(desc);
                    }

                    CustomInfrastPlanInfoList.Add(new CustomInfrastPlanInfo
                    {
                        Index = i,
                        Name = display,
                        Description = desc,
                        PeriodList = periodList,
                    });
                }
            }
            catch (Exception)
            {
                AddLog(Localization.GetString("CustomInfrastFileParseFailed"), LogColor.Error);
                return;
            }

            refreshCustomInfrastPlanIndexByPeriod();
        }

        private void refreshCustomInfrastPlanIndexByPeriod()
        {
            if (!CustomInfrastEnabled || !_customInfrastPlanHasPeriod)
            {
                return;
            }

            bool timeLess(int lHour, int lMin, int rHour, int rMin)
            {
                return (lHour != rHour) ? (lHour < rHour) : (lMin <= rMin);
            }

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

            AddLog(Localization.GetString("CustomInfrastPlanIndexAutoSwitch"), LogColor.Message);
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
                SetAndNotify(ref _medicineNumber, value);
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

        private static readonly string _DropsFilename = Environment.CurrentDirectory + "\\resource\\item_index.json";

        /// <summary>
        /// Gets or sets the list of all drops.
        /// </summary>
        public List<CombData> AllDrops { get; set; } = new List<CombData>();

        private void InitDrops()
        {
            string jsonStr = File.ReadAllText(_DropsFilename);
            var reader = (JObject)JsonConvert.DeserializeObject(jsonStr);
            foreach (var item in reader)
            {
                var val = item.Key;

                // 不是数字的东西都是正常关卡不会掉的（大概吧）
                if (!int.TryParse(val, out _))
                {
                    continue;
                }

                var dis = item.Value["name"].ToString();
                if (dis.EndsWith("双芯片") || dis.EndsWith("寻访凭证") || dis.EndsWith("加固建材")
                    || dis.EndsWith("许可") || dis == "资质凭证" || dis == "高级凭证" || dis == "演习券"
                    || dis.Contains("源石") || dis == "D32钢" || dis == "双极纳米片" || dis == "聚合剂"
                    || dis == "晶体电子单元" || dis == "龙骨" || dis == "芯片助剂")
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

        private string _dropsItemName = ViewStatusStorage.Get("MainFunction.Drops.ItemName", string.Empty);

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
                if (item.Value == DropsItemId)
                {
                    if (DropsItemName != item.Display)
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
