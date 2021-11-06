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
    public class InfrastructureConstructionViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public ObservableCollection<ItemViewModel> ItemViewModels { get; set; }

        public InfrastructureConstructionViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "基建换班";
            ItemViewModels = new ObservableCollection<ItemViewModel>();
            InitializeItems();

            facility_key.Add("宿舍", "Dorm");
            facility_key.Add("制造站", "Mfg");
            facility_key.Add("贸易站", "Trade");
            facility_key.Add("发电站", "Power");
            facility_key.Add("会客室", "Reception");
            facility_key.Add("办公室", "Office");

            _dormThresholdLabel = "宿舍入驻心情阈值：" + _dormThreshold + "%";
            if (NotUseDrone)
            {
                uses_of_drones = UsesOfDrones.DronesNotUse;
            }
            else if (DroneForTrade)
            {
                uses_of_drones = UsesOfDrones.DronesTrade;
            }
            else if (DroneForMfg)
            {
                uses_of_drones = UsesOfDrones.DronesMfg;
            }
        }

        public Dictionary<string, string> facility_key = new Dictionary<string, string>();

        public void InitializeItems()
        {
            ItemViewModels.Add(new ItemViewModel("宿舍"));
            ItemViewModels.Add(new ItemViewModel("制造站"));
            ItemViewModels.Add(new ItemViewModel("贸易站"));
            ItemViewModels.Add(new ItemViewModel("发电站"));
            ItemViewModels.Add(new ItemViewModel("会客室"));
            ItemViewModels.Add(new ItemViewModel("办公室"));
            ItemViewModels.Add(new ItemViewModel("宿舍"));
            //ItemViewModels.Add(new ItemViewModel(8, "控制中枢"));
        }

        private bool _notUseDrone = System.Convert.ToBoolean(ViewStatusStorage.Get("Infrast.NotUseDrone", bool.TrueString));

        public bool NotUseDrone
        {
            get { return _notUseDrone; }
            set
            {
                if (value)
                {
                    uses_of_drones = UsesOfDrones.DronesNotUse;
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
                    uses_of_drones = UsesOfDrones.DronesTrade;
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
                    uses_of_drones = UsesOfDrones.DronesMfg;
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

        private UsesOfDrones uses_of_drones = UsesOfDrones.DronesNotUse;

        public async void ChangeShift()
        {
            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatchDefault();
            });
            bool catchd = await task;
            if (!catchd)
            {
                return;
            }
            // 直接遍历ItemViewModels里面的内容，是排序后的
            var orderList = new List<string>();
            foreach (var item in ItemViewModels)
            {
                if (item.IsChecked == false)
                {
                    continue;
                }
                orderList.Add(facility_key[item.Name]);
            }

            asstProxy.AsstStartInfrastShift(orderList.ToArray(), orderList.Count, (int)uses_of_drones, DormThreshold / 100.0);
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
        }
    }

    public class ItemViewModel : PropertyChangedBase
    {
        public ItemViewModel(string name, bool isChecked = true)
        {
            this.Name = name;
            this.IsChecked = IsChecked;
        }

        private string _name;

        public string Name
        {
            get { return _name; }
            set
            {
                SetAndNotify(ref _name, value);
            }
        }

        private bool _isChecked = true;

        public bool IsChecked
        {
            get { return _isChecked; }
            set
            {
                SetAndNotify(ref _isChecked, value);
            }
        }

        // 换成图标的话要这个，暂时没用
        private string _iconPath;

        public string IconPath
        {
            get { return _name; }
            set
            {
                SetAndNotify(ref _iconPath, value);
            }
        }

        private string _token;

        public string Token
        {
            get { return _token; }
            set
            {
                SetAndNotify(ref _token, value);
            }
        }

        private string _runStatus;

        public string RunStatus
        {
            get { return _runStatus; }
            set
            {
                SetAndNotify(ref _runStatus, value);
            }
        }
    }
}
