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
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
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
                if (value != ActionType.Shutdown && value != ActionType.Hibernate)
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

        // public void ShowButton()
        // {
        //    Visible = Visibility.Visible;
        //    Hibernate = true;
        // }

        // private Visibility _visible = Visibility.Collapsed;

        // public Visibility Visible
        // {
        //    get { return _visible; }
        //    set
        //    {
        //        SetAndNotify(ref _visible, value);
        //    }
        // }
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
                UpdateStageList();
            }

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

        /// <summary>
        /// Initializes items.
        /// </summary>
        public void InitializeItems()
        {
            string[] task_list = new string[]
            {
                Localization.GetString("WakeUp"),
                Localization.GetString("Recruiting"),
                Localization.GetString("Base"),
                Localization.GetString("Combat"),
                Localization.GetString("Visiting"),
                Localization.GetString("Mall"),
                Localization.GetString("Mission"),
                Localization.GetString("AutoRoguelike"),
            };
            ActionAfterCompletedList = new List<GenericCombData<ActionType>>
            {
                new GenericCombData<ActionType> { Display = Localization.GetString("DoNothing"), Value = ActionType.DoNothing },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitArknights"), Value = ActionType.StopGame },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitMAA"), Value = ActionType.ExitSelf },
                new GenericCombData<ActionType> { Display = Localization.GetString("CloseEmulator"), Value = ActionType.ExitEmulator },
                new GenericCombData<ActionType> { Display = Localization.GetString("ExitMAAAndCloseEmulator"), Value = ActionType.ExitEmulatorAndSelf },

                // new GenericCombData<ActionTypeAfterCompleted>{ Display="待机",Value=ActionTypeAfterCompleted.Suspend },
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

                var vm = new DragItemViewModel(task, "TaskQueue.");
                if (task == Localization.GetString("AutoRoguelike"))
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

            AllStageList = new List<CombData>
            {
                // 「当前/上次」关卡导航
                new CombData { Display = Localization.GetString("DefaultStage"), Value = string.Empty },

                // SideStory「理想城：长夏狂欢季」活动
                new CombData { Display = "IC-9", Value = "IC-9" },
                new CombData { Display = "IC-8", Value = "IC-8" },
                new CombData { Display = "IC-7", Value = "IC-7" },

                // 主线关卡
                new CombData { Display = "1-7", Value = "1-7" },

                // 资源本
                new CombData { Display = Localization.GetString("CE-6"), Value = "CE-6" },
                new CombData { Display = Localization.GetString("AP-5"), Value = "AP-5" },
                new CombData { Display = Localization.GetString("CA-5"), Value = "CA-5" },
                new CombData { Display = Localization.GetString("LS-6"), Value = "LS-6" },

                // 剿灭模式
                new CombData { Display = Localization.GetString("Annihilation"), Value = "Annihilation" },

                // 芯片本
                new CombData { Display = Localization.GetString("PR-A-1"), Value = "PR-A-1" },
                new CombData { Display = Localization.GetString("PR-A-2"), Value = "PR-A-2" },
                new CombData { Display = Localization.GetString("PR-B-1"), Value = "PR-B-1" },
                new CombData { Display = Localization.GetString("PR-B-2"), Value = "PR-B-2" },
                new CombData { Display = Localization.GetString("PR-C-1"), Value = "PR-C-1" },
                new CombData { Display = Localization.GetString("PR-C-2"), Value = "PR-C-2" },
                new CombData { Display = Localization.GetString("PR-D-1"), Value = "PR-D-1" },
                new CombData { Display = Localization.GetString("PR-D-2"), Value = "PR-D-2" },

                // 老版本「当前/上次」关卡导航
                // new CombData { Display = Localization.GetString("CurrentStage"), Value = string.Empty },
                // new CombData { Display = Localization.GetString("LastBattle"), Value = "LastBattle" },

                // SideStory「绿野幻梦」活动
                // new CombData { Display = "DV-6", Value = "DV-6" },
                // new CombData { Display = "DV-7", Value = "DV-7" },
                // new CombData { Display = "DV-8", Value = "DV-8" },

                // SideStory「尘影余音」活动
                // new CombData { Display = "LE-7", Value = "LE-7" },
                // new CombData { Display = "LE-6", Value = "LE-6" },
                // new CombData { Display = "LE-5", Value = "LE-5" },

                // SideStory「愚人号」活动关卡
                // new CombData { Display = "SN-8", Value = "SN-8" },
                // new CombData { Display = "SN-9", Value = "SN-9" },
                // new CombData { Display = "SN-10", Value = "SN-10" },

                // SideStory「风雪过境」活动关卡
                // new CombData { Display = "BI-7", Value = "BI-7" },
                // new CombData { Display = "BI-8", Value = "BI-8" }
            };

            _stageAvailableInfo = new Dictionary<string, Tuple<List<DayOfWeek>, string>>
            {
                { "CE-6", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, Localization.GetString("CETip")) },
                { "AP-5", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, Localization.GetString("APTip")) },
                { "CA-5", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday }, Localization.GetString("CATip")) },
                { "LS-6", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday, DayOfWeek.Sunday }, Localization.GetString("LSTip")) },

                // 碳本没做导航
                { "SK-5", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday }, Localization.GetString("SKTip")) },
                { "PR-A-1", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }, Localization.GetString("PR-ATip")) },
                { "PR-A-2", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }, string.Empty) },
                { "PR-B-1", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }, Localization.GetString("PR-BTip")) },
                { "PR-B-2", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }, string.Empty) },
                { "PR-C-1", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, Localization.GetString("PR-CTip")) },
                { "PR-C-2", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, string.Empty) },
                { "PR-D-1", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }, Localization.GetString("PR-DTip")) },
                { "PR-D-2", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }, string.Empty) },

                // 下面的不支持导航
                { "Pormpt1", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Monday }, Localization.GetString("Pormpt1")) },
                { "Pormpt2", new Tuple<List<DayOfWeek>, string>(new List<DayOfWeek> { DayOfWeek.Sunday }, Localization.GetString("Pormpt2")) },
            };

            InitDrops();
            CheckAndUpdateDayOfWeek();
            UpdateDatePrompt();
            UpdateStageList();
        }

        private Dictionary<string, Tuple<List<DayOfWeek>, string>> _stageAvailableInfo;
        private DayOfWeek _curDayOfWeek;

        /// <summary>
        /// Updates stage list.
        /// </summary>
        public void UpdateStageList()
        {
            ObservableCollection<CombData> newList;
            var settingsModel = _container.Get<SettingsViewModel>();
            if (settingsModel.HideUnavailableStage)
            {
                newList = new ObservableCollection<CombData>();
                foreach (var item in AllStageList)
                {
                    if (_stageAvailableInfo.ContainsKey(item.Value))
                    {
                        var info = _stageAvailableInfo[item.Value];
                        if (info.Item1.Contains(_curDayOfWeek))
                        {
                            newList.Add(item);
                        }
                    }
                    else
                    {
                        newList.Add(item);
                    }
                }
            }
            else
            {
                newList = new ObservableCollection<CombData>(AllStageList);
            }

            if (StageList == newList)
            {
                return;
            }

            StageList = newList;

            bool hasSavedValue = false;
            foreach (var item in StageList)
            {
                if (item.Value == Stage)
                {
                    hasSavedValue = true;
                    break;
                }
            }

            if (!hasSavedValue)
            {
                Stage = string.Empty;
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
            var prompt = Localization.GetString("TodaysStageTip") + "\n";

            foreach (var item in _stageAvailableInfo)
            {
                if (item.Value.Item1.Contains(_curDayOfWeek) && item.Value.Item2 != string.Empty)
                {
                    prompt += item.Value.Item2 + "\n";
                }
            }

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
            get { return _stagesOfToday; }
            set { SetAndNotify(ref _stagesOfToday, value); }
        }

        /// <summary>
        /// Adds log.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        public void AddLog(string content, string color = LogColor.Trace, string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));

            // LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        /// <summary>
        /// Clears log.
        /// </summary>
        public void ClearLog()
        {
            LogItemViewModels.Clear();
        }

        /// <summary>
        /// Selects all.
        /// </summary>
        public void SelectedAll()
        {
            foreach (var item in TaskItemViewModels)
            {
                if (item.Name == Localization.GetString("AutoRoguelike"))
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
            get
            {
                return _inverseMode;
            }

            set
            {
                SetAndNotify(ref _inverseMode, value);
                InverseShowText = value ? Localization.GetString("Inverse") : Localization.GetString("Clear");
                InverseMenuText = value ? Localization.GetString("Clear") : Localization.GetString("Inverse");
                ViewStatusStorage.Set("MainFunction.InverseMode", value.ToString());
            }
        }

        private int _selectedAllWidth =
            ViewStatusStorage.Get("GUI.InverseClearMode", "Clear") == "ClearInverse" ? SelectedAllWidthWhenBoth : 90;

        /// <summary>
        /// Gets or sets the width of "Select All".
        /// </summary>
        public int SelectedAllWidth
        {
            get
            {
                return _selectedAllWidth;
            }

            set
            {
                SetAndNotify(ref _selectedAllWidth, value);
            }
        }

        /// <summary>
        /// The width of "Select All" when both.
        /// </summary>
        public const int SelectedAllWidthWhenBoth = 85;

        private int _inverseSelectedWidth = 95;

        /// <summary>
        /// Gets or sets the width of "Select inversely".
        /// </summary>
        public int InverseSelectedWidth
        {
            get
            {
                return _inverseSelectedWidth;
            }

            set
            {
                SetAndNotify(ref _inverseSelectedWidth, value);
            }
        }

        private Visibility _inverseShowVisibility =
            ViewStatusStorage.Get("GUI.InverseClearMode", "Clear") == "ClearInverse" ? Visibility.Visible : Visibility.Collapsed;

        /// <summary>
        /// Gets or sets the visibility of "Select inversely".
        /// </summary>
        public Visibility InverseShowVisibility
        {
            get
            {
                return _inverseShowVisibility;
            }

            set
            {
                SetAndNotify(ref _inverseShowVisibility, value);
            }
        }

        private Visibility _notInverseShowVisibility =
            ViewStatusStorage.Get("GUI.InverseClearMode", "Clear") == "ClearInverse" ? Visibility.Collapsed : Visibility.Visible;

        /// <summary>
        /// Gets or sets the visibility of "Not inversion".
        /// </summary>
        public Visibility NotInverseShowVisibility
        {
            get
            {
                return _notInverseShowVisibility;
            }

            set
            {
                SetAndNotify(ref _notInverseShowVisibility, value);
            }
        }

        private string _inverseShowText = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.InverseMode", bool.FalseString)) ? Localization.GetString("Inverse") : Localization.GetString("Clear");

        /// <summary>
        /// Gets or sets the text to be displayed for "Select inversely".
        /// </summary>
        public string InverseShowText
        {
            get
            {
                return _inverseShowText;
            }

            set
            {
                SetAndNotify(ref _inverseShowText, value);
            }
        }

        private string _inverseMenuText = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.InverseMode", bool.FalseString)) ? Localization.GetString("Clear") : Localization.GetString("Inverse");

        /// <summary>
        /// Gets or sets the text of inversion menu.
        /// </summary>
        public string InverseMenuText
        {
            get
            {
                return _inverseMenuText;
            }

            set
            {
                SetAndNotify(ref _inverseMenuText, value);
            }
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
                    if (item.Name == Localization.GetString("AutoRoguelike"))
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

            ClearLog();

            SaveSettingValue();

            AddLog(Localization.GetString("ConnectingToEmulator"));

            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = string.Empty;
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg, true);
            });
            bool caught = await task;
            if (!caught)
            {
                AddLog(errMsg, LogColor.Error);
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
            int index = 0;
            foreach (var item in TaskItemViewModels)
            {
                ViewStatusStorage.Set("TaskQueue.Order." + item.Name, index.ToString());
                ++index;

                if (item.IsChecked == false)
                {
                    continue;
                }

                ++count;
                if (item.Name == Localization.GetString("Base"))
                {
                    ret &= appendInfrast();
                }
                else if (item.Name == Localization.GetString("WakeUp"))
                {
                    ret &= appendStart();
                }
                else if (item.Name == Localization.GetString("Combat"))
                {
                    ret &= appendFight();
                }
                else if (item.Name == Localization.GetString("Recruiting"))
                {
                    ret &= appendRecruit();
                }
                else if (item.Name == Localization.GetString("Visiting"))
                {
                    ret &= asstProxy.AsstAppendVisit();
                }
                else if (item.Name == Localization.GetString("Mall"))
                {
                    ret &= appendMall();
                }
                else if (item.Name == Localization.GetString("Mission"))
                {
                    ret &= asstProxy.AsstAppendAward();
                }
                else if (item.Name == Localization.GetString("AutoRoguelike"))
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
        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            AddLog(Localization.GetString("Stopped"));
            Idle = true;
        }

        /// <summary>
        /// 保存界面设置数值
        /// </summary>
        private void SaveSettingValue()
        {
            // 吃理智药个数
            ViewStatusStorage.Set("MainFunction.UseMedicine.Quantity", MedicineNumber);

            // 吃石头颗数
            ViewStatusStorage.Set("MainFunction.UseStone.Quantity", StoneNumber);

            // 指定刷关次数
            ViewStatusStorage.Set("MainFunction.TimesLimited.Quantity", MaxTimes);

            // 指定掉落材料
            ViewStatusStorage.Set("MainFunction.Drops.ItemId", DropsItemId);

            // 指定掉落材料数量
            ViewStatusStorage.Set("MainFunction.Drops.Quantity", DropsQuantity);
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
            return asstProxy.AsstAppendFight(Stage, medicine, stone, times, DropsItemId, drops_quantity);
        }

        /// <summary>
        /// Sets parameters.
        /// </summary>
        public void SetParams()
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
            bool isSet = asstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, DropsItemId, drops_quantity);
            if (isSet)
            {
                AddLog(Localization.GetString("SetSuccessfully"), LogColor.Message);
            }
            else
            {
                AddLog(Localization.GetString("SetFailed"), LogColor.Error);
            }
        }

        private bool appendInfrast()
        {
            var settings = _container.Get<SettingsViewModel>();
            var order = settings.GetInfrastOrderList();
            settings.SaveInfrastOrderList();
            int orderLen = order.Count;
            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendInfrast(1, order.ToArray(), orderLen,
                settings.UsesOfDrones, settings.DormThreshold / 100.0);
        }

        private bool appendMall()
        {
            var settings = _container.Get<SettingsViewModel>();
            var asstProxy = _container.Get<AsstProxy>();
            var buy_first = settings.CreditFirstList.Split(new char[] { ';', '；' });
            var black_list = settings.CreditBlackList.Split(new char[] { ';', '；' });
            return asstProxy.AsstAppendMall(settings.CreditShopping, buy_first, black_list);
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

            bool need_refresh = settings.RefreshLevel3;
            bool use_expedited = settings.UseExpedited;

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRecruit(
                max_times, reqList.ToArray(), reqList.Count, cfmList.ToArray(), cfmList.Count, need_refresh, use_expedited, settings.NotChooseLevel1);
        }

        private bool appendRoguelike()
        {
            var settings = _container.Get<SettingsViewModel>();
            int mode;
            int.TryParse(settings.RoguelikeMode, out mode);

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRoguelike(
                mode, settings.RoguelikeStartsCount,
                settings.RoguelikeInvestmentEnabled, settings.RoguelikeInvestsCount, settings.RoguelikeStopWhenInvestmentFull,
                settings.RoguelikeSquad, settings.RoguelikeRoles, settings.RoguelikeCoreChar);
        }

        [DllImport("user32.dll", EntryPoint = "FindWindow")]
        private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("User32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowThreadProcessId(IntPtr hwnd, out int id);

        /// <summary>
        /// Kills emulator by Window hwnd.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool killEumlatorbyWindow()
        {
            IntPtr hwnd;
            int pid = 0;
            var windowname = new[] { "BlueStacks App Player", "BlueStacks", "明日方舟 - MuMu模拟器", "夜神模拟器", "逍遥模拟器", "明日方舟" };
            Process emulator;
            foreach (string i in windowname)
            {
                hwnd = FindWindow(null, i);
                if (hwnd != IntPtr.Zero)
                {
                    GetWindowThreadProcessId(hwnd, out pid);
                    break;
                }
            }

            if (pid != 0)
            {
                emulator = Process.GetProcessById(pid);
                emulator.CloseMainWindow();
                if (!emulator.HasExited)
                {
                    try
                    {
                        emulator.Kill();
                    }
                    catch
                    {
                        return killEmulator();
                    }

                    return true;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                return killEmulator();
            }
        }

        /// <summary>
        /// Kills emulator.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool killEmulator()
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
                    if (!killEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("CloseEmulatorFailed"), LogColor.Error);
                    }

                    break;

                case ActionType.ExitEmulatorAndSelf:
                    if (!killEumlatorbyWindow())
                    {
                        AddLog(Localization.GetString("CloseEmulatorFailed"), LogColor.Error);
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

                default:
                    break;
            }
        }

        // public void CheckAndShutdown()
        // {
        //    if (Shutdown)
        //    {
        //        System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

        // var result = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK, MessageBoxImage.Question);
        //        if (result == MessageBoxResult.OK)
        //        {
        //            System.Diagnostics.Process.Start("shutdown.exe", "-a");
        //        }
        //    }
        //    if (Hibernate)
        //    {
        //        System.Diagnostics.Process.Start("shutdown.exe", "-h");
        //    }
        //    if (Suspend)
        //    {
        //        System.Diagnostics.Process.Start("rundll32.exe", "powrprof.dll,SetSuspendState 0,1,0");
        //    }
        // }
        private bool _idle = false;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get
            {
                return _idle;
            }

            set
            {
                SetAndNotify(ref _idle, value);
                var settings = _container.Get<SettingsViewModel>();
                settings.Idle = value;
                if (value)
                {
                    FightTaskRunning = false;
                }
            }
        }

        private bool _fightTaskRunning = false;

        /// <summary>
        /// Gets or sets a value indicating whether the battle task is running.
        /// </summary>
        public bool FightTaskRunning
        {
            get
            {
                return _fightTaskRunning;
            }

            set
            {
                SetAndNotify(ref _fightTaskRunning, value);
            }
        }

        // private bool _shutdown = false;

        // public bool Shutdown
        // {
        //    get { return _shutdown; }
        //    set
        //    {
        //        SetAndNotify(ref _shutdown, value);

        // if (value)
        //        {
        //            Hibernate = false;
        //            Suspend = false;
        //        }
        //    }
        // }

        // private bool _hibernate = false;  // 休眠

        // public bool Hibernate
        // {
        //    get { return _hibernate; }
        //    set
        //    {
        //        SetAndNotify(ref _hibernate, value);

        // if (value)
        //        {
        //            Shutdown = false;
        //            Suspend = false;
        //        }
        //    }
        // }

        // private bool _suspend = false;  // 待机

        // public bool Suspend
        // {
        //    get { return _suspend; }
        //    set
        //    {
        //        SetAndNotify(ref _suspend, value);

        // if (value)
        //        {
        //            Shutdown = false;
        //            Hibernate = false;
        //        }
        //    }
        // }

        /// <summary>
        /// Gets or sets the list of all stages.
        /// </summary>
        public List<CombData> AllStageList { get; set; }

        private ObservableCollection<CombData> _stageList = new ObservableCollection<CombData>();

        /// <summary>
        /// Gets or sets the list of stages.
        /// </summary>
        public ObservableCollection<CombData> StageList
        {
            get
            {
                return _stageList;
            }

            set
            {
                SetAndNotify(ref _stageList, value);
            }
        }

        private string _stage = ViewStatusStorage.Get("MainFunction.Stage", string.Empty);

        /// <summary>
        /// Gets or sets the stage.
        /// </summary>
        public string Stage
        {
            get
            {
                return _stage;
            }

            set
            {
                SetAndNotify(ref _stage, value);
                ViewStatusStorage.Set("MainFunction.Stage", value);
            }
        }

        private bool _useMedicine = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.UseMedicine", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to use medicine.
        /// </summary>
        public bool UseMedicine
        {
            get
            {
                return _useMedicine;
            }

            set
            {
                SetAndNotify(ref _useMedicine, value);
                if (!value)
                {
                    UseStone = false;
                }

                ViewStatusStorage.Set("MainFunction.UseMedicine", value.ToString());
            }
        }

        private string _medicineNumber = ViewStatusStorage.Get("MainFunction.UseMedicine.Quantity", "999");

        /// <summary>
        /// Gets or sets the amount of medicine used.
        /// </summary>
        public string MedicineNumber
        {
            get
            {
                return _medicineNumber;
            }

            set
            {
                SetAndNotify(ref _medicineNumber, value);
            }
        }

        private bool _useStone;

        /// <summary>
        /// Gets or sets a value indicating whether to use originiums.
        /// </summary>
        public bool UseStone
        {
            get
            {
                return _useStone;
            }

            set
            {
                SetAndNotify(ref _useStone, value);
                if (value)
                {
                    UseMedicine = true;
                }
            }
        }

        private string _stoneNumber = ViewStatusStorage.Get("MainFunction.UseStone.Quantity", "0");

        /// <summary>
        /// Gets or sets the amount of originiums used.
        /// </summary>
        public string StoneNumber
        {
            get
            {
                return _stoneNumber;
            }

            set
            {
                SetAndNotify(ref _stoneNumber, value);
            }
        }

        private bool _hasTimesLimited;

        /// <summary>
        /// Gets or sets a value indicating whether the number of times is limited.
        /// </summary>
        public bool HasTimesLimited
        {
            get
            {
                return _hasTimesLimited;
            }

            set
            {
                SetAndNotify(ref _hasTimesLimited, value);
            }
        }

        private string _maxTimes = ViewStatusStorage.Get("MainFunction.TimesLimited.Quantity", "5");

        /// <summary>
        /// Gets or sets the max number of times.
        /// </summary>
        public string MaxTimes
        {
            get
            {
                return _maxTimes;
            }

            set
            {
                SetAndNotify(ref _maxTimes, value);
            }
        }

        private bool _isSpecifiedDrops = Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.Drops.Enable", bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether the drops are specified.
        /// </summary>
        public bool IsSpecifiedDrops
        {
            get
            {
                return _isSpecifiedDrops;
            }

            set
            {
                SetAndNotify(ref _isSpecifiedDrops, value);
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

        private string _dropsItemId = ViewStatusStorage.Get("MainFunction.Drops.ItemId", "0");

        /// <summary>
        /// Gets or sets the item ID of drops.
        /// </summary>
        public string DropsItemId
        {
            get
            {
                return _dropsItemId;
            }

            set
            {
                if (value == null)
                {
                    return;
                }

                SetAndNotify(ref _dropsItemId, value);
            }
        }

        private string _dropsItem = string.Empty;
        private bool _isFirstLoadDropItem = true;
        private long _preSetDropsItemTicks = 0;

        /// <summary>
        /// Gets or sets the item of drops.
        /// </summary>
        public string DropsItem
        {
            get
            {
                return _dropsItem;
            }

            set
            {
                if (_isFirstLoadDropItem)
                {
                    _isFirstLoadDropItem = false;
                }
                else
                {
                    IsDropDown = true;
                }

                if (DateTime.Now.Ticks - _preSetDropsItemTicks < 50)
                {
                    return;
                }

                _preSetDropsItemTicks = DateTime.Now.Ticks;
                DropsList.Clear();
                foreach (CombData drop in AllDrops)
                {
                    var enumStr = drop.Display;
                    if (enumStr.Contains(value))
                    {
                        DropsList.Add(drop);
                    }
                }

                SetAndNotify(ref _dropsItem, value);
            }
        }

        private bool _isDropDown = false;

        /// <summary>
        /// Gets or sets a value indicating whether it is dropdown.
        /// </summary>
        public bool IsDropDown
        {
            get
            {
                return _isDropDown;
            }

            set
            {
                SetAndNotify(ref _isDropDown, value);
            }
        }

        private string _dropsQuantity = ViewStatusStorage.Get("MainFunction.Drops.Quantity", "5");

        /// <summary>
        /// Gets or sets the quantity of drops.
        /// </summary>
        public string DropsQuantity
        {
            get
            {
                return _dropsQuantity;
            }

            set
            {
                SetAndNotify(ref _dropsQuantity, value);
            }
        }
    }
}
