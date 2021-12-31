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
using System.IO;
using System.Runtime.InteropServices;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class SettingsViewModel : Screen
    {
        private readonly IWindowManager _windowManager;
        private readonly IContainer _container;

        [DllImport("MeoAssistant.dll")] private static extern IntPtr AsstGetVersion();

        private readonly string _versionInfo = "版本号：" + Marshal.PtrToStringAnsi(AsstGetVersion());

        public string VersionInfo
        {
            get { return _versionInfo; }
        }

        public SettingsViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "设置";

            _listTitle.Add("基建设置");
            _listTitle.Add("自动公招");
            _listTitle.Add("信用商店");
            _listTitle.Add("企鹅数据");
            _listTitle.Add("连接设置");
            _listTitle.Add("软件更新");
            //_listTitle.Add("其他");

            InfrastInit();
        }

        private List<string> _listTitle = new List<string>();

        public List<string> ListTitle
        {
            get { return _listTitle; }
            set
            {
                SetAndNotify(ref _listTitle, value);
            }
        }

        private void InfrastInit()
        {
            /* 基建设置 */
            string[] facility_list = new string[] { "制造站", "贸易站", "控制中枢", "发电站", "会客室", "办公室", "宿舍" };

            var temp_order_list = new List<DragItemViewModel>(new DragItemViewModel[facility_list.Length]);
            for (int i = 0; i != facility_list.Length; ++i)
            {
                var facility = facility_list[i];
                int order = -1;
                bool parsed = int.TryParse(ViewStatusStorage.Get("Infrast.Order." + facility, "-1"), out order);

                if (!parsed || order < 0)
                {
                    temp_order_list[i] = new DragItemViewModel(facility, "Infrast.");
                }
                else
                {
                    temp_order_list[order] = new DragItemViewModel(facility, "Infrast.");
                }
            }
            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(temp_order_list);

            FacilityKey.Add("宿舍", "Dorm");
            FacilityKey.Add("制造站", "Mfg");
            FacilityKey.Add("贸易站", "Trade");
            FacilityKey.Add("发电站", "Power");
            FacilityKey.Add("会客室", "Reception");
            FacilityKey.Add("办公室", "Office");
            FacilityKey.Add("控制中枢", "Control");

            UsesOfDronesList = new List<CombData>();
            UsesOfDronesList.Add(new CombData { Display = "不使用无人机", Value = "_NotUse" });
            UsesOfDronesList.Add(new CombData { Display = "贸易站-龙门币", Value = "Money" });
            UsesOfDronesList.Add(new CombData { Display = "贸易站-合成玉", Value = "SyntheticJade" });
            UsesOfDronesList.Add(new CombData { Display = "制造站-经验书", Value = "CombatRecord" });
            UsesOfDronesList.Add(new CombData { Display = "制造站-赤金", Value = "PureGold" });
            UsesOfDronesList.Add(new CombData { Display = "制造站-源石碎片", Value = "OriginStone" });
            UsesOfDronesList.Add(new CombData { Display = "制造站-芯片组", Value = "Chip" });

            _dormThresholdLabel = "宿舍入驻心情阈值：" + _dormThreshold + "%";
        }

        private bool _idle = true;

        public bool Idle
        {
            get { return _idle; }
            set
            {
                SetAndNotify(ref _idle, value);
            }
        }

        /* 基建设置 */
        public Dictionary<string, string> FacilityKey = new Dictionary<string, string>();
        public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; }

        public List<CombData> UsesOfDronesList { get; set; }

        private int _dormThreshold = System.Convert.ToInt32(ViewStatusStorage.Get("Infrast.DormThreshold", "30"));

        public int DormThreshold
        {
            get { return _dormThreshold; }
            set
            {
                DormThresholdLabel = "宿舍入驻心情阈值：" + _dormThreshold + "%";
                SetAndNotify(ref _dormThreshold, value);
                ViewStatusStorage.Set("Infrast.DormThreshold", value.ToString());
            }
        }

        private string _dormThresholdLabel;

        public string DormThresholdLabel
        {
            get { return _dormThresholdLabel; }
            set
            {
                SetAndNotify(ref _dormThresholdLabel, value);
            }
        }

        public List<string> GetInfrastOrderList()
        {
            var orderList = new List<string>();
            foreach (var item in InfrastItemViewModels)
            {
                if (item.IsChecked == false)
                {
                    continue;
                }

                orderList.Add(FacilityKey[item.Name]);
            }
            return orderList;
        }

        public void SaveInfrastOrderList()
        {
            for (int i = 0; i < InfrastItemViewModels.Count; i++)
            {
                ViewStatusStorage.Set("Infrast.Order." + InfrastItemViewModels[i].Name, i.ToString());
            }
        }

        private string _usesOfDrones = ViewStatusStorage.Get("Infrast.UsesOfDrones", "Money");

        public string UsesOfDrones
        {
            get { return _usesOfDrones; }
            set
            {
                SetAndNotify(ref _usesOfDrones, value);
                ViewStatusStorage.Set("Infrast.UsesOfDrones", value);
            }
        }

        private InfrastWorkMode _infrastWorkMode = InfrastWorkMode.Aggressive;

        public InfrastWorkMode InfrastWorkMode
        {
            get { return _infrastWorkMode; }
            set
            {
                SetAndNotify(ref _infrastWorkMode, value);
            }
        }

        // 消息源，0：无；1：SelectedIndex；2：ScrollOffset
        private int _notifySource = 0;

        private void resetNotifySource()
        {
            System.Timers.Timer t = new System.Timers.Timer(20);
            t.Elapsed += new System.Timers.ElapsedEventHandler(delegate (object source, System.Timers.ElapsedEventArgs e)
            {
                _notifySource = 0;
            });
            t.AutoReset = false;
            t.Enabled = true;
            t.Start();
        }

        private int _selectedIndex = 0;

        public int SelectedIndex
        {
            get { return _selectedIndex; }
            set
            {
                if (_notifySource == 0)
                {
                    _notifySource = 1;
                    SetAndNotify(ref _selectedIndex, value);
                    ScrollOffset = (double)value / ListTitle.Count;
                    //_notifySource = 0;
                    resetNotifySource();
                }
                else if (_notifySource == 2)
                {
                    SetAndNotify(ref _selectedIndex, value);
                }
            }
        }

        private double _scrollOffset = 0;

        public double ScrollOffset
        {
            get { return _scrollOffset; }
            set
            {
                if (_notifySource == 0)
                {
                    _notifySource = 2;
                    SetAndNotify(ref _scrollOffset, value);
                    SelectedIndex = (int)(value / ScrollHeight * ListTitle.Count);
                    //_notifySource = 0;
                    resetNotifySource();
                }
                else if (_notifySource == 1)
                {
                    SetAndNotify(ref _scrollOffset, value);
                }
            }
        }

        private double _scrollHeight = 660; // ScrollViewer.ScrollableHeight, 不知道该咋获取，当前的默认值是这个

        public double ScrollHeight
        {
            get { return _scrollHeight; }
            set
            {
                SetAndNotify(ref _scrollHeight, value);
            }
        }

        /* 信用商店设置 */

        private bool _creditShopping = System.Convert.ToBoolean(ViewStatusStorage.Get("Mall.CreditShopping", bool.TrueString));

        public bool CreditShopping
        {
            get { return _creditShopping; }
            set
            {
                SetAndNotify(ref _creditShopping, value);
                ViewStatusStorage.Set("Mall.CreditShopping", value.ToString());
            }
        }

        /* 企鹅数据设置 */

        private string _penguinId = ViewStatusStorage.Get("Penguin.Id", "");

        public string PenguinId
        {
            get { return _penguinId; }
            set
            {
                SetAndNotify(ref _penguinId, value);
                ViewStatusStorage.Set("Penguin.Id", value);
            }
        }

        /* 自动公招设置 */
        private string _recruitMaxTimes = ViewStatusStorage.Get("AutoRecruit.MaxTimes", "3");

        public string RecruitMaxTimes
        {
            get { return _recruitMaxTimes; }
            set
            {
                SetAndNotify(ref _recruitMaxTimes, value);
                ViewStatusStorage.Set("AutoRecruit.MaxTimes", value);
            }
        }

        private bool _refreshLevel3 = System.Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.RefreshLevel3", bool.TrueString));

        public bool RefreshLevel3
        {
            get { return _refreshLevel3; }
            set
            {
                SetAndNotify(ref _refreshLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.RefreshLevel3", value.ToString());
            }
        }

        private bool _chooseLevel3 = System.Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel3", bool.TrueString));

        public bool ChooseLevel3
        {
            get { return _chooseLevel3; }
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel3", value.ToString());
            }
        }

        private bool _chooseLevel4 = System.Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel4", bool.TrueString));

        public bool ChooseLevel4
        {
            get { return _chooseLevel4; }
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel4", value.ToString());
            }
        }

        private bool _chooseLevel5 = System.Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel5", bool.FalseString));

        public bool ChooseLevel5
        {
            get { return _chooseLevel5; }
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel5", value.ToString());
            }
        }

        /* 软件更新设置 */
        private bool _updateBeta = System.Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UpdateBeta", bool.FalseString));

        public bool UpdateBeta
        {
            get { return _updateBeta; }
            set
            {
                SetAndNotify(ref _updateBeta, value);
                ViewStatusStorage.Set("VersionUpdate.UpdateBeta", value.ToString());
            }
        }

        private string _proxy = ViewStatusStorage.Get("VersionUpdate.Proxy", "");

        public string Proxy
        {
            get { return _proxy; }
            set
            {
                SetAndNotify(ref _proxy, value);
                ViewStatusStorage.Set("VersionUpdate.Proxy", value);
            }
        }

        /* 连接设置 */

        private string _connectAddress = ViewStatusStorage.Get("Connect.Address", string.Empty);

        public string ConnectAddress
        {
            get { return _connectAddress; }
            set
            {
                SetAndNotify(ref _connectAddress, value);
                ViewStatusStorage.Set("Connect.Address", value);
            }
        }

        private string _bluestacksConfPath = ViewStatusStorage.Get("Connect.BluestacksConfPath", string.Empty);

        public string BluestacksConfPath
        {
            get { return _bluestacksConfPath; }
            set
            {
                SetAndNotify(ref _bluestacksConfPath, value);
                ViewStatusStorage.Set("Connect.BluestacksConfPath", value);
            }
        }

        public void TryToSetBlueStacksHyperVAddress()
        {
            if (BluestacksConfPath.Length == 0)
            {
                return;
            }
            var all_lines = File.ReadAllLines(BluestacksConfPath);
            foreach (var line in all_lines)
            {
                if (line.StartsWith("bst.instance.Nougat64.status.adb_port"))
                {
                    var sp = line.Split('"');
                    ConnectAddress = "127.0.0.1:" + sp[1];
                }
            }
        }
    }
}
