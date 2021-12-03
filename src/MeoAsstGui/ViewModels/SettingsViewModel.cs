// MeoAssistanceGui - A part of the MeoAssistance-Arknight project
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
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class SettingsViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public SettingsViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "设置";

            _listTitle.Add("基建");
            _listTitle.Add("自动公招");
            _listTitle.Add("信用商店");
            _listTitle.Add("企鹅数据");
            //_listTitle.Add("连接");
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
            InfrastItemViewModels = new ObservableCollection<DragItemViewModel>();

            string key = "Infrast.";
            //InfrastItemViewModels.Add(new DragItemViewModel("宿舍", key));
            InfrastItemViewModels.Add(new DragItemViewModel("制造站", key));
            InfrastItemViewModels.Add(new DragItemViewModel("贸易站", key));
            InfrastItemViewModels.Add(new DragItemViewModel("控制中枢", key));
            InfrastItemViewModels.Add(new DragItemViewModel("发电站", key));
            InfrastItemViewModels.Add(new DragItemViewModel("会客室", key));
            InfrastItemViewModels.Add(new DragItemViewModel("办公室", key));
            InfrastItemViewModels.Add(new DragItemViewModel("宿舍", key));

            facilityKey.Add("宿舍", "Dorm");
            facilityKey.Add("制造站", "Mfg");
            facilityKey.Add("贸易站", "Trade");
            facilityKey.Add("发电站", "Power");
            facilityKey.Add("会客室", "Reception");
            facilityKey.Add("办公室", "Office");
            facilityKey.Add("控制中枢", "Control");

            _dormThresholdLabel = "宿舍入驻心情阈值：" + _dormThreshold + "%";

            if (NotUseDrone)
            {
                _usesOfDrones = UsesOfDrones.DronesNotUse;
            }
            else if (DroneForTrade)
            {
                _usesOfDrones = UsesOfDrones.DronesTrade;
            }
            else if (DroneForMfg)
            {
                _usesOfDrones = UsesOfDrones.DronesMfg;
            }
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
        public Dictionary<string, string> facilityKey = new Dictionary<string, string>();
        public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; }

        private bool _notUseDrone = System.Convert.ToBoolean(ViewStatusStorage.Get("Infrast.NotUseDrone", bool.TrueString));

        public bool NotUseDrone
        {
            get { return _notUseDrone; }
            set
            {
                if (value)
                {
                    _usesOfDrones = UsesOfDrones.DronesNotUse;
                }
                SetAndNotify(ref _notUseDrone, value);
                ViewStatusStorage.Set("Infrast.NotUseDrone", value.ToString());
            }
        }

        private bool _droneForTrade = System.Convert.ToBoolean(ViewStatusStorage.Get("Infrast.DroneForTrade", bool.FalseString));

        public bool DroneForTrade
        {
            get { return _droneForTrade; }
            set
            {
                if (value)
                {
                    _usesOfDrones = UsesOfDrones.DronesTrade;
                }
                SetAndNotify(ref _droneForTrade, value);
                ViewStatusStorage.Set("Infrast.DroneForTrade", value.ToString());
            }
        }

        private bool _droneForMfg = System.Convert.ToBoolean(ViewStatusStorage.Get("Infrast.DroneForMfg", bool.FalseString));

        public bool DroneForMfg
        {
            get { return _droneForMfg; }
            set
            {
                if (value)
                {
                    _usesOfDrones = UsesOfDrones.DronesMfg;
                }
                SetAndNotify(ref _droneForMfg, value);
                ViewStatusStorage.Set("Infrast.DroneForMfg", value.ToString());
            }
        }

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

                orderList.Add(facilityKey[item.Name]);
            }
            return orderList;
        }

        private UsesOfDrones _usesOfDrones = UsesOfDrones.DronesNotUse;

        public UsesOfDrones UsesOfDrones
        {
            get { return _usesOfDrones; }
            set
            {
                SetAndNotify(ref _usesOfDrones, value);
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

        private int _selectedIndex = 0;

        public int SelectedIndex
        {
            get { return _selectedIndex; }
            set
            {
                if (_notifySource != 1)
                {
                    if (_notifySource == 0)
                    {
                        _notifySource = 1;
                    }
                    ScrollOffset = (double)value / (ListTitle.Count - 1);
                    SetAndNotify(ref _selectedIndex, value);
                }
                _notifySource = 0;
            }
        }

        private double _scrollOffset = 0;

        public double ScrollOffset
        {
            get { return _scrollOffset; }
            set
            {
                if (_notifySource != 2)
                {
                    if (_notifySource == 0)
                    {
                        _notifySource = 2;
                    }
                    SelectedIndex = (int)(value / ScrollHeight * (ListTitle.Count - 1));
                    SetAndNotify(ref _scrollOffset, value);
                }
                _notifySource = 0;
            }
        }

        private double _scrollHeight = 258; // ScrollViewer.ScrollableHeight, 不知道该咋获取，默认值是258

        public double ScrollHeight
        {
            get { return _scrollHeight; }
            set
            {
                SetAndNotify(ref _scrollHeight, value);
            }
        }

        /* 信用商店设置 */

        private bool _creditShopping = System.Convert.ToBoolean(ViewStatusStorage.Get("Mall.CreditShopping", bool.FalseString));

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
    }
}
