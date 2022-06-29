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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Toolkit.Uwp.Notifications;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class TaskQueueViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        public ObservableCollection<DragItemViewModel> TaskItemViewModels { get; set; }
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        private string _actionAfterComplated = ViewStatusStorage.Get("MainFunction.ActionAfterComplated", ActionType.DoNothing.ToString());
        public List<GenericCombData<ActionType>> ActionAfterCompletedList { get; set; }

        public ActionType ActionAfterCompleted
        {
            get
            {
                if (ActionType.TryParse(_actionAfterComplated, out ActionType action))
                {
                    return action;
                }
                return ActionType.DoNothing;
            }
            set
            {
                SetAndNotify(ref _actionAfterComplated, value.ToString());
                string storeValue = ActionType.DoNothing.ToString();
                if (value != ActionType.Shutdown && value != ActionType.Hibernate)
                {
                    storeValue = value.ToString();
                }
                ViewStatusStorage.Set("MainFunction.ActionAfterComplated", storeValue);
            }
        }

        public TaskQueueViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "一键长草";
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            InitializeItems();
            InitTimer();
        }

        //public void ShowButton()
        //{
        //    Visible = Visibility.Visible;
        //    Hibernate = true;
        //}

        //private Visibility _visible = Visibility.Collapsed;

        //public Visibility Visible
        //{
        //    get { return _visible; }
        //    set
        //    {
        //        SetAndNotify(ref _visible, value);
        //    }
        //}

        private System.Windows.Forms.Timer _timer = new System.Windows.Forms.Timer();

        private void InitTimer()
        {
            _timer.Enabled = true;
            _timer.Interval = 1000 * 60;
            _timer.Tick += new EventHandler(Timer1_Elapsed);
            _timer.Start();
        }

        private void Timer1_Elapsed(object sender, EventArgs e)
        {
            int intMinute = DateTime.Now.Minute;

            if (intMinute != 0 || Idle == false)
            {
                return;
            }
            UpdateDatePrompt();

            int intHour = DateTime.Now.Hour;
            var settings = _container.Get<SettingsViewModel>();
            if (settings.Timer1 && settings.Timer1Hour == intHour ||
                settings.Timer2 && settings.Timer2Hour == intHour ||
                settings.Timer3 && settings.Timer3Hour == intHour ||
                settings.Timer4 && settings.Timer4Hour == intHour ||
                settings.Timer5 && settings.Timer5Hour == intHour ||
                settings.Timer6 && settings.Timer6Hour == intHour ||
                settings.Timer7 && settings.Timer7Hour == intHour ||
                settings.Timer8 && settings.Timer8Hour == intHour)
            {
                LinkStart();
            }
        }

        public void InitializeItems()
        {
            string[] task_list = new string[] { "开始唤醒", "刷理智", "自动公招", "基建换班", "访问好友", "收取信用及购物", "领取日常奖励", "无限刷肉鸽" };
            ActionAfterCompletedList = new List<GenericCombData<ActionType>>
            {
                new GenericCombData<ActionType>{ Display="无动作",Value=ActionType.DoNothing },
                new GenericCombData<ActionType>{ Display="退出 明日方舟",Value=ActionType.StopGame },
                new GenericCombData<ActionType>{ Display="退出 MAA",Value=ActionType.ExitSelf },
                new GenericCombData<ActionType>{ Display="关闭模拟器",Value=ActionType.ExitEmulator },
                new GenericCombData<ActionType>{ Display="退出并关闭模拟器",Value=ActionType.ExitEmulatorAndSelf },
                //new GenericCombData<ActionTypeAfterCompleted>{ Display="待机",Value=ActionTypeAfterCompleted.Suspend },
                new GenericCombData<ActionType>{ Display="休眠",Value=ActionType.Hibernate },
                new GenericCombData<ActionType>{ Display="关机",Value=ActionType.Shutdown },
            };
            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[task_list.Length]);
            int order_offset = 0;
            for (int i = 0; i != task_list.Length; ++i)
            {
                var task = task_list[i];
                int order = -1;
                bool parsed = int.TryParse(ViewStatusStorage.Get("TaskQueue.Order." + task, "-1"), out order);

                var vm = new DragItemViewModel(task, "TaskQueue.");
                if (task == "无限刷肉鸽")
                {
                    vm.IsChecked = false;
                }
                if (!parsed || order < 0)
                {
                    temp_order_list[i] = vm;
                    ++order_offset;
                }
                else
                {
                    temp_order_list[order + order_offset] = vm;
                }
            }
            TaskItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            StageList = new List<CombData>
            {
                new CombData { Display = "当前关卡", Value = string.Empty },
                new CombData { Display = "上次作战", Value = "LastBattle" },

                new CombData { Display = "1-7", Value = "1-7" },
                new CombData { Display = "龙门币-6/5", Value = "CE-6" },
                new CombData { Display = "经验-6/5", Value = "LS-6" },
                new CombData { Display = "红票-5", Value = "AP-5" },
                new CombData { Display = "技能-5", Value = "CA-5" },

                // SideStory「尘影余音」活动
                // new CombData { Display = "LE-7", Value = "LE-7" },
                // new CombData { Display = "LE-6", Value = "LE-6" },
                // new CombData { Display = "LE-5", Value = "LE-5" },

                // “愚人号” 活动关卡
                //new CombData { Display = "SN-8", Value = "SN-8" },
                //new CombData { Display = "SN-9", Value = "SN-9" },
                //new CombData { Display = "SN-10", Value = "SN-10" },

                //// “风雪过境” 活动关卡
                //new CombData { Display = "BI-7", Value = "BI-7" },
                //new CombData { Display = "BI-8", Value = "BI-8" }
            };

            InitDrops();

            UpdateDatePrompt();
        }

        public void UpdateDatePrompt()
        {
            var now = DateTime.Now;
            var hour = now.Hour;
            if (hour >= 0 && hour < 4)
            {
                now = now.AddDays(-1);
            }

            var stage_dict = new Dictionary<string, List<DayOfWeek>>
            {
                { "货物运送（龙门币）", new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday } },
                { "粉碎防御（红票）", new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday } },
                { "空中威胁（技能）", new List<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday } },
                { "资源保障（碳）", new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday } },
                { "战术演习（经验）", new List<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday, DayOfWeek.Sunday } },
                { "周一了，可以打剿灭了~", new List<DayOfWeek> { DayOfWeek.Monday } },
                { "周日了，记得打剿灭哦~", new List<DayOfWeek> { DayOfWeek.Sunday } }
            };

            StagesOfToday = "今日关卡小提示：\n";

            foreach (var item in stage_dict)
            {
                if (item.Value.Contains(now.DayOfWeek))
                {
                    StagesOfToday += item.Key + "\n";
                }
            }
        }

        public string StagesOfToday { get; set; }

        public void AddLog(string content, string color = "Gray", string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));
            //LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        public void ClearLog()
        {
            LogItemViewModels.Clear();
        }

        public void SelectedAll()
        {
            foreach (var item in TaskItemViewModels)
            {
                if (item.Name == "无限刷肉鸽")
                    continue;
                item.IsChecked = true;
            }
        }

        public void InverseSelected()
        {
            foreach (var item in TaskItemViewModels)
            {
                if (item.Name == "无限刷肉鸽")
                    continue;
                item.IsChecked = false;
            }
        }

        public async void LinkStart()
        {
            if (Idle == false)
            {
                return;
            }
            Idle = false;

            ClearLog();

            SaveSettingValue();

            AddLog("正在连接模拟器……");

            var asstProxy = _container.Get<AsstProxy>();
            string errMsg = "";
            var task = Task.Run(() =>
            {
                return asstProxy.AsstConnect(ref errMsg, true);
            });
            bool catchd = await task;
            if (!catchd)
            {
                AddLog(errMsg, "darkred");
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
                catchd = await task;
                if (!catchd)
                {
                    AddLog(errMsg, "darkred");
                    Idle = true;
                    return;
                }
            }

            if (Idle)   // 一般是点了“停止”按钮了
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
                if (item.Name == "基建换班")
                {
                    ret &= appendInfrast();
                }
                else if (item.Name == "开始唤醒")
                {
                    ret &= appendStart();
                }
                else if (item.Name == "刷理智")
                {
                    ret &= appendFight();
                }
                else if (item.Name == "自动公招")
                {
                    ret &= appendRecruit();
                }
                else if (item.Name == "访问好友")
                {
                    ret &= asstProxy.AsstAppendVisit();
                }
                else if (item.Name == "收取信用及购物")
                {
                    ret &= appendMall();
                }
                else if (item.Name == "领取日常奖励")
                {
                    ret &= asstProxy.AsstAppendAward();
                }
                else if (item.Name == "无限刷肉鸽")
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
                AddLog("未选择任务");
                Idle = true;
                return;
            }

            if (Idle)   // 一般是点了“停止”按钮了
            {
                return;
            }

            ret &= asstProxy.AsstStart();

            if (ret)
            {
                AddLog("正在运行中……");
            }
            else
            {
                AddLog("出现未知错误");
            }
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            AddLog("已停止");
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
            bool setted = asstProxy.AsstSetFightTaskParams(Stage, medicine, stone, times, DropsItemId, drops_quantity);
            if (setted)
            {
                AddLog("设置成功", "Black");
            }
            else
            {
                AddLog("设置失败", "Black");
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
            var buy_first = settings.CreditFirstList.Split(' ');
            var black_list = settings.CreditBlackList.Split(' ');
            return asstProxy.AsstAppendMall(settings.CreditShopping, buy_first, black_list);
        }

        private bool appendRecruit()
        {
            // for debug
            var settings = _container.Get<SettingsViewModel>();

            int max_times = 0;
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
            int mode = 0;
            int.TryParse(settings.RoguelikeMode, out mode);

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRoguelike(mode);
        }

        public bool killemulator()
        {
            int pid = 0;
            string port;
            string address = ViewStatusStorage.Get("Connect.Address", "");
            if (address.StartsWith("127"))
                port = address.Substring(10);
            else port = "5555";
            string portcmd = "netstat -ano|findstr \"" + port + "\"";
            Process checkcmd = new Process();
            checkcmd.StartInfo.FileName = "cmd.exe";
            checkcmd.StartInfo.UseShellExecute = false;
            checkcmd.StartInfo.RedirectStandardInput = true;
            checkcmd.StartInfo.RedirectStandardOutput = true;
            checkcmd.StartInfo.RedirectStandardError = true;
            checkcmd.StartInfo.CreateNoWindow = true;
            checkcmd.Start();
            checkcmd.StandardInput.WriteLine(portcmd);
            checkcmd.StandardInput.WriteLine("exit");
            Regex reg = new Regex("\\s+", RegexOptions.Compiled);
            string line;
            while (true)
            {
                line = checkcmd.StandardOutput.ReadLine();
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
                    if (!Convert.ToBoolean(arr[1].CompareTo(address)) || !Convert.ToBoolean(arr[1].CompareTo("[::]:" + port)) || !Convert.ToBoolean(arr[1].CompareTo("0.0.0.0:" + port)))
                    {
                        pid = Int32.Parse(arr[4]);
                        break;
                    }
                }
            }
            if (pid == 0)
                return false;
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

        public enum ActionType
        {
            DoNothing,
            StopGame,
            ExitSelf,
            ExitEmulator,
            ExitEmulatorAndSelf,
            Suspend,
            Hibernate,
            Shutdown
        }

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
                        AddLog("关闭游戏失败", "DarkRed");
                    }
                    break;

                case ActionType.ExitSelf:
                    if (new ToastNotification().CheckToastSystem())
                    {
                        ToastNotificationManagerCompat.History.Clear(); //exit似乎不会走bootstapper，单独清一下通知
                    }
                    Environment.Exit(0);
                    break;

                case ActionType.ExitEmulator:
                    if (!killemulator())
                    {
                        AddLog("模拟器关闭失败", "DarkRed");
                    }
                    break;

                case ActionType.ExitEmulatorAndSelf:
                    if (!killemulator())
                    {
                        AddLog("模拟器关闭失败", "DarkRed");
                    }
                    if (new ToastNotification().CheckToastSystem())
                    {
                        ToastNotificationManagerCompat.History.Clear(); //exit似乎不会走bootstapper，单独清一下通知
                    }
                    Environment.Exit(0);
                    break;

                case ActionType.Shutdown:
                    System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");
                    var shutdownResult = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK, MessageBoxImage.Question);
                    if (shutdownResult == MessageBoxResult.OK)
                    {
                        System.Diagnostics.Process.Start("shutdown.exe", "-a");
                    }
                    break;

                case ActionType.Suspend:
                    System.Diagnostics.Process.Start("powercfg", "-h off");
                    System.Diagnostics.Process.Start("rundll32.exe", "powrprof.dll,SetSuspendState 0,1,0");
                    System.Diagnostics.Process.Start("powercfg", "-h on");
                    break;

                case ActionType.Hibernate:
                    System.Diagnostics.Process.Start("shutdown.exe", "-h -t 60");
                    var hibernateResult = _windowManager.ShowMessageBox("已刷完，即将休眠，是否取消？", "提示", MessageBoxButton.OK, MessageBoxImage.Question);
                    if (hibernateResult == MessageBoxResult.OK)
                    {
                        System.Diagnostics.Process.Start("shutdown.exe", "-a");
                    }
                    break;

                default:
                    break;
            }
        }

        //public void CheckAndShutdown()
        //{
        //    if (Shutdown)
        //    {
        //        System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

        //        var result = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK, MessageBoxImage.Question);
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
        //}

        private bool _idle = false;

        public bool Idle
        {
            get { return _idle; }
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

        public bool FightTaskRunning
        {
            get { return _fightTaskRunning; }
            set
            {
                SetAndNotify(ref _fightTaskRunning, value);
            }
        }

        //private bool _shutdown = false;

        //public bool Shutdown
        //{
        //    get { return _shutdown; }
        //    set
        //    {
        //        SetAndNotify(ref _shutdown, value);

        //        if (value)
        //        {
        //            Hibernate = false;
        //            Suspend = false;
        //        }
        //    }
        //}

        //private bool _hibernate = false;  // 休眠

        //public bool Hibernate
        //{
        //    get { return _hibernate; }
        //    set
        //    {
        //        SetAndNotify(ref _hibernate, value);

        //        if (value)
        //        {
        //            Shutdown = false;
        //            Suspend = false;
        //        }
        //    }
        //}

        //private bool _suspend = false;  // 待机

        //public bool Suspend
        //{
        //    get { return _suspend; }
        //    set
        //    {
        //        SetAndNotify(ref _suspend, value);

        //        if (value)
        //        {
        //            Shutdown = false;
        //            Hibernate = false;
        //        }
        //    }
        //}

        public List<CombData> StageList { get; set; }

        private string _stage = ViewStatusStorage.Get("MainFunction.Stage", "LastBattle");

        public string Stage
        {
            get { return _stage; }
            set
            {
                SetAndNotify(ref _stage, value);
                ViewStatusStorage.Set("MainFunction.Stage", value);
            }
        }

        private bool _useMedicine = System.Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.UseMedicine", bool.FalseString));

        public bool UseMedicine
        {
            get { return _useMedicine; }
            set
            {
                SetAndNotify(ref _useMedicine, value);
                ViewStatusStorage.Set("MainFunction.UseMedicine", value.ToString());
            }
        }

        private string _medicineNumber = ViewStatusStorage.Get("MainFunction.UseMedicine.Quantity", "999");

        public string MedicineNumber
        {
            get { return _medicineNumber; }
            set
            {
                SetAndNotify(ref _medicineNumber, value);
            }
        }

        private bool _useStone;

        public bool UseStone
        {
            get { return _useStone; }
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

        public string StoneNumber
        {
            get { return _stoneNumber; }
            set
            {
                SetAndNotify(ref _stoneNumber, value);
            }
        }

        private bool _hasTimesLimited;

        public bool HasTimesLimited
        {
            get { return _hasTimesLimited; }
            set
            {
                SetAndNotify(ref _hasTimesLimited, value);
            }
        }

        private string _maxTimes = ViewStatusStorage.Get("MainFunction.TimesLimited.Quantity", "5");

        public string MaxTimes
        {
            get { return _maxTimes; }
            set
            {
                SetAndNotify(ref _maxTimes, value);
            }
        }

        private bool _isSpecifiedDrops = System.Convert.ToBoolean(ViewStatusStorage.Get("MainFunction.Drops.Enable", bool.FalseString));

        public bool IsSpecifiedDrops
        {
            get { return _isSpecifiedDrops; }
            set
            {
                SetAndNotify(ref _isSpecifiedDrops, value);
                ViewStatusStorage.Set("MainFunction.Drops.Enable", value.ToString());
            }
        }

        private static readonly string _DropsFilename = System.Environment.CurrentDirectory + "\\resource\\item_index.json";

        public List<CombData> AllDrops { get; set; } = new List<CombData>();

        private void InitDrops()
        {
            string jsonStr = File.ReadAllText(_DropsFilename);
            var reader = (JObject)JsonConvert.DeserializeObject(jsonStr);
            foreach (var item in reader)
            {
                var val = item.Key;
                if (!int.TryParse(val, out _))  // 不是数字的东西都是正常关卡不会掉的（大概吧）
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
                return a.Value.CompareTo(b.Value);
            });
            DropsList = new ObservableCollection<CombData>(AllDrops);
        }

        public ObservableCollection<CombData> DropsList { get; set; }

        private string _dropsItemId = ViewStatusStorage.Get("MainFunction.Drops.ItemId", "0");

        public string DropsItemId
        {
            get { return _dropsItemId; }
            set
            {
                SetAndNotify(ref _dropsItemId, value);
            }
        }

        private string _dropsItem = "";
        private bool _isFirstLoadDropItem = true;
        private long _preSetDropsItemTicks = 0;

        public string DropsItem
        {
            get { return _dropsItem; }
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

                // 这里有个很严重的死循环性能问题，不知道怎么解决，简单做个消抖好了_(:з」∠)_
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

        public bool IsDropDown
        {
            get { return _isDropDown; }
            set
            {
                SetAndNotify(ref _isDropDown, value);
            }
        }

        private string _dropsQuantity = ViewStatusStorage.Get("MainFunction.Drops.Quantity", "5");

        public string DropsQuantity
        {
            get { return _dropsQuantity; }
            set
            {
                SetAndNotify(ref _dropsQuantity, value);
            }
        }
    }
}
