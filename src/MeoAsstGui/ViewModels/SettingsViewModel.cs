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
using System.Linq;
using System.Runtime.InteropServices;
using Notification.Wpf.Constants;
using Notification.Wpf.Controls;
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
            _listTitle.Add("肉鸽设置");
            _listTitle.Add("自动公招");
            _listTitle.Add("信用商店");
            _listTitle.Add("企鹅数据");
            _listTitle.Add("连接设置");
            _listTitle.Add("通知显示");
            _listTitle.Add("软件更新");
            //_listTitle.Add("其他");

            InfrastInit();
            ToastPositionInit();
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

            UsesOfDronesList = new List<CombData>
            {
                new CombData { Display = "不使用无人机", Value = "_NotUse" },
                new CombData { Display = "贸易站-龙门币", Value = "Money" },
                new CombData { Display = "贸易站-合成玉", Value = "SyntheticJade" },
                new CombData { Display = "制造站-经验书", Value = "CombatRecord" },
                new CombData { Display = "制造站-赤金", Value = "PureGold" },
                new CombData { Display = "制造站-源石碎片", Value = "OriginStone" },
                new CombData { Display = "制造站-芯片组", Value = "Chip" }
            };

            _dormThresholdLabel = "宿舍入驻心情阈值：" + _dormThreshold + "%";

            RoguelikeModeList = new List<CombData>
            {
                new CombData { Display = "尽可能往后打", Value = "0" },
                new CombData { Display = "刷源石锭投资，第一层商店后直接退出", Value = "1" },
                new CombData { Display = "刷源石锭投资，投资过后退出", Value = "2" }
            };
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
        public List<CombData> RoguelikeModeList { get; set; }

        private int _dormThreshold = Convert.ToInt32(ViewStatusStorage.Get("Infrast.DormThreshold", "30"));

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

        #region 设置页面列表和滚动视图联动绑定

        private enum NotifyType
        {
            None,
            SelectedIndex,
            ScrollOffset
        }

        private NotifyType _notifySource = NotifyType.None;

        private System.Timers.Timer _resetNotifyTimer;

        private void ResetNotifySource()
        {
            if (_resetNotifyTimer != null)
            {
                _resetNotifyTimer.Stop();
                _resetNotifyTimer.Close();
            }
            _resetNotifyTimer = new System.Timers.Timer(20);
            _resetNotifyTimer.Elapsed += new System.Timers.ElapsedEventHandler(delegate (object source, System.Timers.ElapsedEventArgs e)
            {
                _notifySource = NotifyType.None;
            });
            _resetNotifyTimer.AutoReset = false;
            _resetNotifyTimer.Enabled = true;
            _resetNotifyTimer.Start();
        }

        public double ScrollViewportHeight { get; set; }

        public double ScrollExtentHeight { get; set; }

        public List<double> RectangleVerticalOffsetList { get; set; }

        private int _selectedIndex = 0;

        public int SelectedIndex
        {
            get { return _selectedIndex; }
            set
            {
                switch (_notifySource)
                {
                    case NotifyType.None:
                        _notifySource = NotifyType.SelectedIndex;
                        SetAndNotify(ref _selectedIndex, value);

                        var isInRange = RectangleVerticalOffsetList != null
                            && RectangleVerticalOffsetList.Count > 0
                            && value < RectangleVerticalOffsetList.Count;

                        if (isInRange)
                            ScrollOffset = RectangleVerticalOffsetList[value];

                        ResetNotifySource();
                        break;

                    case NotifyType.ScrollOffset:
                        SetAndNotify(ref _selectedIndex, value);
                        break;
                }
            }
        }

        private double _scrollOffset = 0;

        public double ScrollOffset
        {
            get { return _scrollOffset; }
            set
            {
                switch (_notifySource)
                {
                    case NotifyType.None:
                        _notifySource = NotifyType.ScrollOffset;
                        SetAndNotify(ref _scrollOffset, value);

                        // 设置 ListBox SelectedIndex 为当前 ScrollOffset 索引
                        var isInRange = RectangleVerticalOffsetList != null && RectangleVerticalOffsetList.Count > 0;

                        if (isInRange)
                        {
                            // 滚动条滚动到底部，返回最后一个 Rectangle 索引
                            if (value + ScrollViewportHeight >= ScrollExtentHeight)
                            {
                                SelectedIndex = RectangleVerticalOffsetList.Count - 1;
                                ResetNotifySource();
                                break;
                            }

                            // 根据出当前 ScrollOffset 选出最后一个在可视范围的 Rectangle 索引
                            var rectangleSelect = RectangleVerticalOffsetList.Select((n, i) => (
                            rectangleAppeared: value >= n,
                            index: i
                            ));

                            var index = rectangleSelect.LastOrDefault(n => n.rectangleAppeared).index;
                            SelectedIndex = index;
                        }

                        ResetNotifySource();
                        break;

                    case NotifyType.SelectedIndex:
                        SetAndNotify(ref _scrollOffset, value);
                        break;
                }
            }
        }

        #endregion 设置页面列表和滚动视图联动绑定

        /* 肉鸽设置 */

        private string _roguelikeMode = ViewStatusStorage.Get("Roguelike.Mode", "0");

        public string RoguelikeMode
        {
            get { return _roguelikeMode; }
            set
            {
                SetAndNotify(ref _roguelikeMode, value);
                ViewStatusStorage.Set("Roguelike.Mode", value);
            }
        }

        /* 信用商店设置 */

        private bool _creditShopping = Convert.ToBoolean(ViewStatusStorage.Get("Mall.CreditShopping", bool.TrueString));

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

        private string _penguinId = ViewStatusStorage.Get("Penguin.Id", string.Empty);

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

        private bool _refreshLevel3 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.RefreshLevel3", bool.TrueString));

        public bool RefreshLevel3
        {
            get { return _refreshLevel3; }
            set
            {
                SetAndNotify(ref _refreshLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.RefreshLevel3", value.ToString());
            }
        }

        private bool _useExpedited = false;

        public bool UseExpedited
        {
            get { return _useExpedited; }
            set { SetAndNotify(ref _useExpedited, value); }
        }

        private bool _chooseLevel3 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel3", bool.TrueString));

        public bool ChooseLevel3
        {
            get { return _chooseLevel3; }
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel3", value.ToString());
            }
        }

        private bool _chooseLevel4 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel4", bool.TrueString));

        public bool ChooseLevel4
        {
            get { return _chooseLevel4; }
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel4", value.ToString());
            }
        }

        private bool _chooseLevel5 = Convert.ToBoolean(ViewStatusStorage.Get("AutoRecruit.ChooseLevel5", bool.FalseString));

        public bool ChooseLevel5
        {
            get { return _chooseLevel5; }
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ViewStatusStorage.Set("AutoRecruit.ChooseLevel5", value.ToString());
            }
        }

        /* 通知显示设置 */

        #region 通知显示

        // 左上
        private bool _toastPositionTopLeft = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.TopLeft.ToString();

        public bool ToastPositionTopLeft
        {
            get { return _toastPositionTopLeft; }
            set
            {
                SetAndNotify(ref _toastPositionTopLeft, value);

                if (value)
                {
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.TopLeft;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.TopLeft.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 上
        private bool _toastPositionTopCenter = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.TopCenter.ToString();

        public bool ToastPositionTopCenter
        {
            get { return _toastPositionTopCenter; }
            set
            {
                SetAndNotify(ref _toastPositionTopCenter, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.TopCenter;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.TopCenter.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 右上
        private bool _toastPositionTopRight = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.TopRight.ToString();

        public bool ToastPositionTopRight
        {
            get { return _toastPositionTopRight; }
            set
            {
                SetAndNotify(ref _toastPositionTopRight, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.TopRight;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.TopRight.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 左
        private bool _toastPositionCenterLeft = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.CenterLeft.ToString();

        public bool ToastPositionCenterLeft
        {
            get { return _toastPositionCenterLeft; }
            set
            {
                SetAndNotify(ref _toastPositionCenterLeft, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.CenterLeft;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.CenterLeft.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 右
        private bool _toastPositionCenterRight = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.CenterRight.ToString();

        public bool ToastPositionCenterRight
        {
            get { return _toastPositionCenterRight; }
            set
            {
                SetAndNotify(ref _toastPositionCenterRight, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.CenterRight;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.CenterRight.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 左下
        private bool _toastPositionBottomLeft = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.BottomLeft.ToString();

        public bool ToastPositionBottomLeft
        {
            get { return _toastPositionBottomLeft; }
            set
            {
                SetAndNotify(ref _toastPositionBottomLeft, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomCenter =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.BottomLeft;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.BottomLeft.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 下
        private bool _toastPositionBottomCenter = ViewStatusStorage.Get("Toast.Position", string.Empty) == NotificationPosition.BottomCenter.ToString();

        public bool ToastPositionBottomCenter
        {
            get { return _toastPositionBottomCenter; }
            set
            {
                SetAndNotify(ref _toastPositionBottomCenter, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomRight = false;
                    NotificationConstants.MessagePosition = NotificationPosition.BottomCenter;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.BottomCenter.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 右下
        private bool _toastPositionBottomRight =
            ViewStatusStorage.Get("Toast.Position", NotificationPosition.BottomRight.ToString()) == NotificationPosition.BottomRight.ToString();

        public bool ToastPositionBottomRight
        {
            get { return _toastPositionBottomRight; }
            set
            {
                SetAndNotify(ref _toastPositionBottomRight, value);

                if (value)
                {
                    ToastPositionTopLeft =
                    ToastPositionTopCenter =
                    ToastPositionTopRight =
                    ToastPositionCenterLeft =
                    ToastPositionCenterRight =
                    ToastPositionBottomLeft =
                    ToastPositionBottomCenter = false;
                    NotificationConstants.MessagePosition = NotificationPosition.BottomRight;
                    ViewStatusStorage.Set("Toast.Position", NotificationPosition.BottomRight.ToString());
                }
                else
                {
                    ToastPositionOnlySound();
                }
            }
        }

        // 设置通知只有通知声音
        private void ToastPositionOnlySound()
        {
            if (!ToastPositionTopLeft
                && !ToastPositionTopCenter
                && !ToastPositionTopRight
                && !ToastPositionCenterLeft
                && !ToastPositionCenterRight
                && !ToastPositionBottomLeft
                && !ToastPositionBottomCenter
                && !ToastPositionBottomRight)
            {
                ViewStatusStorage.Set("Toast.Position", string.Empty);
            }
        }

        // 通知测试
        public void ToastPositionTest()
        {
            Execute.OnUIThread(() =>
            {
                using (var toast = new ToastNotification("通知显示位置测试"))
                {
                    toast.AppendContentText("如果选择了新的位置")
                        .AppendContentText("请先点掉这个通知再测试").Show(lifeTime: 5, row: 2);
                }
            });
        }

        // 通知位置初始化
        private void ToastPositionInit()
        {
            var position = ViewStatusStorage.Get("Toast.Position", NotificationPosition.BottomRight.ToString());
            if (string.IsNullOrWhiteSpace(position))
                return;

            NotificationConstants.MessagePosition = (NotificationPosition)Enum.Parse(typeof(NotificationPosition), position);
        }

        #endregion 通知显示

        /* 软件更新设置 */
        private bool _updateBeta = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UpdateBeta", bool.FalseString));

        public bool UpdateBeta
        {
            get { return _updateBeta; }
            set
            {
                SetAndNotify(ref _updateBeta, value);
                ViewStatusStorage.Set("VersionUpdate.UpdateBeta", value.ToString());
            }
        }

        private string _proxy = ViewStatusStorage.Get("VersionUpdate.Proxy", string.Empty);

        public string Proxy
        {
            get { return _proxy; }
            set
            {
                SetAndNotify(ref _proxy, value);
                ViewStatusStorage.Set("VersionUpdate.Proxy", value);
            }
        }

        private bool _useAria2 = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.UseAria2", bool.TrueString));

        public bool UseAria2
        {
            get { return _useAria2; }
            set
            {
                SetAndNotify(ref _useAria2, value);
                ViewStatusStorage.Set("VersionUpdate.UseAria2", value.ToString());
            }
        }

        private bool _autoDownloadUpdatePackage = Convert.ToBoolean(ViewStatusStorage.Get("VersionUpdate.AutoDownloadUpdatePackage", bool.TrueString));

        public bool AutoDownloadUpdatePackage
        {
            get { return _autoDownloadUpdatePackage; }
            set
            {
                SetAndNotify(ref _autoDownloadUpdatePackage, value);
                ViewStatusStorage.Set("VersionUpdate.AutoDownloadUpdatePackage", value.ToString());
            }
        }

        /* 调试设置 */

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

        private string _adbPath = ViewStatusStorage.Get("Connect.AdbPath", string.Empty);

        public string AdbPath
        {
            get { return _adbPath; }
            set
            {
                SetAndNotify(ref _adbPath, value);
                ViewStatusStorage.Set("Connect.AdbPath", value);
            }
        }

        private string _connectConfig = ViewStatusStorage.Get("Connect.ConnectConfig", "General");

        public string ConnectConfig
        {
            get { return _connectConfig; }
            set
            {
                SetAndNotify(ref _connectConfig, value);
                ViewStatusStorage.Set("Connect.ConnectConfig", value);
            }
        }

        //public void TryToSetBlueStacksHyperVAddress()
        //{
        //    if (AdbPath.Length == 0 || !File.Exists(AdbPath))
        //    {
        //        return;
        //    }
        //    var all_lines = File.ReadAllLines(AdbPath);
        //    foreach (var line in all_lines)
        //    {
        //        if (line.StartsWith("bst.instance.Nougat64.status.adb_port"))
        //        {
        //            var sp = line.Split('"');
        //            ConnectAddress = "127.0.0.1:" + sp[1];
        //        }
        //    }
        //}
    }
}
