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

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class TaskQueueViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public ObservableCollection<DragItemViewModel> TaskItemViewModels { get; set; }
        public ObservableCollection<LogItemViewModel> LogItemViewModels { get; set; }

        public TaskQueueViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "一键长草";
            LogItemViewModels = new ObservableCollection<LogItemViewModel>();
            InitializeItems();
        }

        public void InitializeItems()
        {
            string[] task_list = new string[] { "开始唤醒", "刷理智", "自动公招", "基建换班", "访问好友", "收取信用及购物", "领取日常奖励" };

            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[task_list.Length]);
            int order_offset = 0;
            for (int i = 0; i != task_list.Length; ++i)
            {
                var task = task_list[i];
                int order = -1;
                bool parsed = int.TryParse(ViewStatusStorage.Get("TaskQueue.Order." + task, "-1"), out order);

                if (!parsed || order < 0)
                {
                    temp_order_list[i] = new DragItemViewModel(task, "TaskQueue.");
                    ++order_offset;
                }
                else
                {
                    temp_order_list[order + order_offset] = new DragItemViewModel(task, "TaskQueue.");
                }
            }
            TaskItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            StageList = new List<CombData>();
            StageList.Add(new CombData { Display = "当前关卡", Value = "" });
            StageList.Add(new CombData { Display = "上次作战", Value = "LastBattle" });
            StageList.Add(new CombData { Display = "剿灭作战", Value = "Annihilation" });
            StageList.Add(new CombData { Display = "龙门币-5", Value = "CE-5" });
            StageList.Add(new CombData { Display = "红票-5", Value = "AP-5" });
            StageList.Add(new CombData { Display = "经验-5", Value = "LS-5" });
            StageList.Add(new CombData { Display = "技能-5", Value = "CA-5" });
            StageList.Add(new CombData { Display = "BI-7", Value = "BI-7" });
            StageList.Add(new CombData { Display = "BI-8", Value = "BI-8" });
        }

        public void AddLog(string content, string color = "Gray", string weight = "Regular")
        {
            LogItemViewModels.Add(new LogItemViewModel(content, color, weight));
            //LogItemViewModels.Insert(0, new LogItemViewModel(time + content, color, weight));
        }

        public void ClearLog()
        {
            LogItemViewModels.Clear();
        }

        public async void LinkStart()
        {
            ClearLog();

            AddLog("正在捕获模拟器窗口……");

            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatch();
            });
            bool catchd = await task;
            if (!catchd)
            {
                AddLog("捕获模拟器窗口失败，若是第一次运行，请尝试使用管理员权限", "darkred");
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
                    ret &= asstProxy.AsstAppendStartUp();
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
                else
                {
                    --count;
                    // TODO 报错
                }
            }
            if (count == 0)
            {
                AddLog("未选择任务");
                return;
            }
            setPenguinId();
            ret &= asstProxy.AsstStart();

            if (ret)
            {
                AddLog("正在运行中……");
            }
            else
            {
                AddLog("出现未知错误");
            }
            Idle = !ret;
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            AddLog("已停止");
            Idle = true;
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

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendFight(Stage, medicine, stone, times);
        }

        private bool appendInfrast()
        {
            var settings = _container.Get<SettingsViewModel>();
            var order = settings.GetInfrastOrderList();
            settings.SaveInfrastOrderList();
            int orderLen = order.Count;
            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendInfrast((int)settings.InfrastWorkMode, order.ToArray(), orderLen,
                settings.UsesOfDrones, settings.DormThreshold / 100.0);
        }

        private bool appendMall()
        {
            var settings = _container.Get<SettingsViewModel>();
            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendMall(settings.CreditShopping);
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

            var asstProxy = _container.Get<AsstProxy>();
            return asstProxy.AsstAppendRecruit(max_times, reqList.ToArray(), reqList.Count, cfmList.ToArray(), cfmList.Count, need_refresh);
        }

        private void setPenguinId()
        {
            var settings = _container.Get<SettingsViewModel>();
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstSetPenguinId(settings.PenguinId);
        }

        public void CheckAndShutdown()
        {
            if (Shutdown == true)
            {
                System.Diagnostics.Process.Start("shutdown.exe", "-s -t 60");

                var result = _windowManager.ShowMessageBox("已刷完，即将关机，是否取消？", "提示", MessageBoxButton.OK);
                if (result == MessageBoxResult.OK)
                {
                    System.Diagnostics.Process.Start("shutdown.exe", "-a");
                }
            }
        }

        private bool _idle = false;

        public bool Idle
        {
            get { return _idle; }
            set
            {
                SetAndNotify(ref _idle, value);
                var settings = _container.Get<SettingsViewModel>();
                settings.Idle = value;
            }
        }

        private bool _shutdown = false;

        public bool Shutdown
        {
            get { return _shutdown; }
            set
            {
                SetAndNotify(ref _shutdown, value);
            }
        }

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

        private string _medicineNumber = "999";

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
            }
        }

        private string _stoneNumber = "0";

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

        private string _maxTimes = "5";

        public string MaxTimes
        {
            get { return _maxTimes; }
            set
            {
                SetAndNotify(ref _maxTimes, value);
            }
        }
    }
}
