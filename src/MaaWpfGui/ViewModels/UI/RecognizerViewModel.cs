// <copyright file="RecognizerViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>
#nullable enable
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using HandyControl.Controls;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of recruit.
    /// </summary>
    public class RecognizerViewModel : Screen
    {
        private readonly RunningState _runningState;
        private static readonly ILogger _logger = Log.ForContext<RecognizerViewModel>();

        /// <summary>
        /// Initializes a new instance of the <see cref="RecognizerViewModel"/> class.
        /// </summary>
        public RecognizerViewModel()
        {
            DisplayName = LocalizationHelper.GetString("Toolbox");
            _runningState = RunningState.Instance;
            _runningState.IdleChanged += RunningState_IdleChanged;
            _peepImageTimer.Elapsed += RefreshPeepImageAsync;
            _peepImageTimer.Interval = 1000d / PeepTargetFps;
            _gachaTimer.Tick += RefreshGachaTip;
            LoadDepotDetails();
            LoadOperBoxDetails();
        }

        private void RunningState_IdleChanged(object? sender, bool e)
        {
            Idle = e;
            if (!Idle)
            {
                return;
            }

            Peeping = false;
            IsPeepInProgress = false;
            IsGachaInProgress = false;
        }

        private bool _idle;

        /// <summary>
        /// Gets or sets a value indicating whether it is idle.
        /// </summary>
        public bool Idle
        {
            get => _idle;
            set => SetAndNotify(ref _idle, value);
        }

        #region Recruit

        private string _recruitInfo = LocalizationHelper.GetString("RecruitmentRecognitionTip");

        /// <summary>
        /// Gets or sets the recruit info.
        /// </summary>
        public string RecruitInfo
        {
            get => _recruitInfo;
            set => SetAndNotify(ref _recruitInfo, value);
        }

        private string _recruitResult = string.Empty;

        /// <summary>
        /// Gets or sets the recruit result.
        /// </summary>
        public string RecruitResult
        {
            get => _recruitResult;
            set => SetAndNotify(ref _recruitResult, value);
        }

        private bool _chooseLevel3 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 3.
        /// </summary>
        public bool ChooseLevel3
        {
            get => _chooseLevel3;
            set
            {
                SetAndNotify(ref _chooseLevel3, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel3, value.ToString());
            }
        }

        private bool _chooseLevel4 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 4.
        /// </summary>
        public bool ChooseLevel4
        {
            get => _chooseLevel4;
            set
            {
                SetAndNotify(ref _chooseLevel4, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel4, value.ToString());
            }
        }

        private bool _chooseLevel5 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 5.
        /// </summary>
        public bool ChooseLevel5
        {
            get => _chooseLevel5;
            set
            {
                SetAndNotify(ref _chooseLevel5, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel5, value.ToString());
            }
        }

        private bool _chooseLevel6 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel6, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to choose level 6.
        /// </summary>
        public bool ChooseLevel6
        {
            get => _chooseLevel6;
            set
            {
                SetAndNotify(ref _chooseLevel6, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel6, value.ToString());
            }
        }

        private bool _autoSetTime = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoSetTime, bool.TrueString));

        /// <summary>
        /// Gets or sets a value indicating whether to set time automatically.
        /// </summary>
        public bool RecruitAutoSetTime
        {
            get => _autoSetTime;
            set
            {
                SetAndNotify(ref _autoSetTime, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AutoSetTime, value.ToString());
            }
        }

        private bool _recruitCaught;

        /// <summary>
        /// Starts calculation.
        /// </summary>
        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public async void RecruitStartCalc()
        {
            string errMsg = string.Empty;
            RecruitInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            _recruitCaught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!_recruitCaught)
            {
                RecruitInfo = errMsg;
                return;
            }

            RecruitInfo = LocalizationHelper.GetString("Identifying");
            RecruitResult = string.Empty;

            var levelList = new List<int>();

            if (ChooseLevel3)
            {
                levelList.Add(3);
            }

            if (ChooseLevel4)
            {
                levelList.Add(4);
            }

            if (ChooseLevel5)
            {
                levelList.Add(5);
            }

            if (ChooseLevel6)
            {
                levelList.Add(6);
            }

            var task = new AsstRecruitTask()
            {
                SelectList = levelList,
                ConfirmList = [-1], // 仅公招识别时将-1加入comfirm_level
                SetRecruitTime = RecruitAutoSetTime,
                ChooseLevel3Time = TaskQueueViewModel.RecruitTask.ChooseLevel3Time,
                ChooseLevel4Time = TaskQueueViewModel.RecruitTask.ChooseLevel4Time,
                ChooseLevel5Time = TaskQueueViewModel.RecruitTask.ChooseLevel5Time,
                ServerType = Instances.SettingsViewModel.ServerType,
            };
            var (type, taskParams) = task.Serialize();
            bool ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.RecruitCalc, type, taskParams);
            ret &= Instances.AsstProxy.AsstStart();
        }

        private bool _recruitmentShowPotential = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitmentShowPotential, bool.TrueString));

        public bool RecruitmentShowPotential
        {
            get => _recruitmentShowPotential;
            set
            {
                SetAndNotify(ref _recruitmentShowPotential, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.RecruitmentShowPotential, value.ToString());
            }
        }

        public void ProcRecruitMsg(JObject details)
        {
            string? what = details["what"]?.ToString();
            var subTaskDetails = details["details"];

            switch (what)
            {
                case "RecruitTagsDetected":
                    {
                        JArray? tags = (JArray?)subTaskDetails?["tags"];
                        string infoContent = LocalizationHelper.GetString("RecruitTagsDetected");
                        tags ??= [];
                        infoContent = tags.Select(tagName => tagName.ToString()).Aggregate(infoContent, (current, tagStr) => current + (tagStr + "    "));

                        RecruitInfo = infoContent;
                    }

                    break;

                case "RecruitResult":
                    {
                        string resultContent = string.Empty;
                        JArray? resultArray = (JArray?)subTaskDetails?["result"];
                        /* int level = (int)subTaskDetails["level"]; */
                        foreach (var combs in resultArray ?? [])
                        {
                            int tagLevel = (int)(combs["level"] ?? -1);
                            resultContent += tagLevel + "★ Tags:    ";
                            resultContent = (((JArray?)combs["tags"]) ?? []).Aggregate(resultContent, (current, tag) => current + (tag + "    "));

                            resultContent += "\n\t";
                            foreach (var oper in (JArray?)combs["opers"] ?? [])
                            {
                                int operLevel = (int)(oper["level"] ?? -1);
                                var operId = oper["id"]?.ToString();
                                var operName = DataHelper.GetLocalizedCharacterName(oper["name"]?.ToString());

                                string potential = string.Empty;

                                if (RecruitmentShowPotential && OperBoxPotential != null && operId != null
                                    && (tagLevel >= 4 || operLevel == 1))
                                {
                                    if (OperBoxPotential.ContainsKey(operId))
                                    {
                                        potential = " ( " + OperBoxPotential[operId] + " )";
                                    }
                                    else
                                    {
                                        potential = " ( !!! NEW !!! )";
                                    }
                                }

                                resultContent += operLevel + "★ " + operName + potential + "    ";
                            }

                            resultContent += "\n\n";
                        }

                        RecruitResult = resultContent;
                    }

                    break;
            }
        }

        #endregion Recruit

        #region Depot

        private string _depotInfo = string.Empty;

        /// <summary>
        /// Gets or sets the depot info.
        /// </summary>
        public string DepotInfo
        {
            get => _depotInfo;
            set => SetAndNotify(ref _depotInfo, value);
        }

        private ObservableCollection<DepotResultDate> _depotResult = [];

        /// <summary>
        /// Gets or sets the depot result.
        /// </summary>
        public ObservableCollection<DepotResultDate> DepotResult
        {
            get => _depotResult;
            set => SetAndNotify(ref _depotResult, value);
        }

        public class DepotResultDate
        {
            public string? Name { get; set; }

            public string Id { get; set; } = null!;

            public BitmapImage? Image { get; set; }

            public string? Count { get; set; }
        }

        private void SaveDepotDetails(JObject details)
        {
            // var json = details.ToString(Formatting.None);
            // ConfigurationHelper.SetValue(ConfigurationKeys.DepotResult, json);
            JsonDataHelper.Set(JsonDataKey.DepotData, details);
        }

        private void LoadDepotDetails()
        {
            // TODO: 删除老数据节省 gui.json 的大小，后续版本可以删除
            // var json = ConfigurationHelper.GetValue(ConfigurationKeys.DepotResult, string.Empty);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.DepotResult);
            var json = JsonDataHelper.Get(JsonDataKey.DepotData, string.Empty);
            if (string.IsNullOrWhiteSpace(json))
            {
                return;
            }

            try
            {
                var details = JObject.Parse(json);
                DepotParse(details);
            }
            catch
            {
                // 兼容老数据或异常时忽略
            }
        }

        /// <summary>
        /// Gets or sets the ArkPlanner result.
        /// </summary>
        public string ArkPlannerResult { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the Lolicon result.
        /// </summary>
        public string LoliconResult { get; set; } = string.Empty;

        /// <summary>
        /// gets or sets the depot image.
        /// </summary>
        private static readonly Dictionary<string, BitmapImage?> _imageCache = new();

        /// <summary>
        /// parse of depot recognition result
        /// </summary>
        /// <param name="details">detailed json-style parameters</param>
        /// <returns>Success or not</returns>
        public bool DepotParse(JObject? details)
        {
            if (details == null)
            {
                return false;
            }

            DepotResult.Clear();

            var sortedItems = details["arkplanner"]?["object"]?["items"]
                                  ?.Cast<JObject>()
                                  .OrderBy(item => (string?)item["id"])
                              ?? Enumerable.Empty<JObject>();

            foreach (var item in sortedItems)
            {
                var id = (string?)item["id"];
                if (id == null)
                {
                    continue;
                }

                if (!_imageCache.TryGetValue(id, out var image))
                {
                    image = ItemListHelper.GetItemImage(id);
                    _imageCache[id] = image;
                }

                DepotResultDate result = new()
                {
                    Id = id,
                    Name = ItemListHelper.GetItemName(id),
                    Image = image,
                    Count = item["have"] != null && int.TryParse(item["have"]?.ToString() ?? "-1", out int haveValue)
                        ? (haveValue > 10000
                            ? $"{haveValue / 10000.0:F1}w"
                            : haveValue.ToString())
                        : "-1",
                };

                DepotResult.Add(result);
            }

            ArkPlannerResult = details["arkplanner"]?["data"]?.ToString() ?? string.Empty;
            LoliconResult = details["lolicon"]?["data"]?.ToString() ?? string.Empty;

            bool done = (bool)(details["done"] ?? false);
            if (!done)
            {
                return true;
            }

            DepotInfo = LocalizationHelper.GetString("IdentificationCompleted");
            SaveDepotDetails(details);

            return true;
        }

        /// <summary>
        /// Export depot info to ArkPlanner.
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportToArkplanner()
        {
            Clipboard.SetDataObject(ArkPlannerResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        /// <summary>
        /// Export depot info to Lolicon.
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportToLolicon()
        {
            Clipboard.SetDataObject(LoliconResult);
            DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        private void DepotClear()
        {
            DepotResult.Clear();
            ArkPlannerResult = string.Empty;
            LoliconResult = string.Empty;
        }

        /// <summary>
        /// Starts depot recognition.
        /// </summary>
        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public async void StartDepot()
        {
            _runningState.SetIdle(false);
            string errMsg = string.Empty;
            DepotInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                DepotInfo = errMsg;
                _runningState.SetIdle(true);
                return;
            }

            DepotInfo = LocalizationHelper.GetString("Identifying");
            DepotClear();

            Instances.AsstProxy.AsstStartDepot();
        }

        #endregion Depot

        #region OperBox

        private string _operBoxInfo = LocalizationHelper.GetString("OperBoxRecognitionTip");

        public string OperBoxInfo
        {
            get => _operBoxInfo;
            set => SetAndNotify(ref _operBoxInfo, value);
        }

        private List<OperBoxData.OperData> _operBoxDataArray = [];

        public List<OperBoxData.OperData> OperBoxDataArray
        {
            get => _operBoxDataArray;
            set
            {
                SetAndNotify(ref _operBoxDataArray, value);
                _operBoxPotential = null; // reset
            }
        }

        private Dictionary<string, int>? _operBoxPotential;

        public Dictionary<string, int>? OperBoxPotential
        {
            get
            {
                if (_operBoxPotential != null)
                {
                    return _operBoxPotential;
                }

                _operBoxPotential = OperBoxDataArray.ToDictionary(oper => oper.Id, oper => oper.Potential);
                return _operBoxPotential;
            }
        }

        public class Operator(string id, string name, int rarity)
        {
            public string Id { get; } = id;

            public string Name { get; } = name;

            public int Rarity { get; } = rarity;

            public bool Equals(Operator? other) => other != null && Name == other.Name && Rarity == other.Rarity;

            public override bool Equals(object? obj) => obj is Operator other && Equals(other);

            public override int GetHashCode() => HashCode.Combine(Id, Name, Rarity);

            public override string ToString() => $"{Name} (★{Rarity})";
        }

        private ObservableCollection<Operator> _operBoxHaveList = [];

        public ObservableCollection<Operator> OperBoxHaveList
        {
            get => _operBoxHaveList;
            set => SetAndNotify(ref _operBoxHaveList, value);
        }

        private ObservableCollection<Operator> _operBoxNotHaveList = [];

        public ObservableCollection<Operator> OperBoxNotHaveList
        {
            get => _operBoxNotHaveList;
            set => SetAndNotify(ref _operBoxNotHaveList, value);
        }

        private static void SaveOperBoxDetails(List<OperBoxData.OperData> details)
        {
            // var json = details.ToString(Formatting.None);
            // ConfigurationHelper.SetValue(ConfigurationKeys.OperBoxData, json);
            JsonDataHelper.Set(JsonDataKey.OperBoxData, JArray.FromObject(details));
        }

        private void LoadOperBoxDetails()
        {
            // TODO: 删除老数据节省 gui.json 的大小，后续版本可以删除
            // var json = ConfigurationHelper.GetValue(ConfigurationKeys.OperBoxData, string.Empty);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.OperBoxData);
            var json = JsonDataHelper.Get(JsonDataKey.OperBoxData, string.Empty);
            if (string.IsNullOrWhiteSpace(json))
            {
                return;
            }

            try
            {
                var ownOpers = JsonConvert.DeserializeObject<List<OperBoxData.OperData>>(json)?.Where(i => !string.IsNullOrEmpty(i.Id)).ToList();
                if (ownOpers is null)
                {
                    return;
                }

                OperBoxDataArray = ownOpers;
                var ids = ownOpers.Select(o => o.Id).ToHashSet();
                foreach (var (id, oper) in DataHelper.Operators)
                {
                    if (DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType))
                    {
                        var name = DataHelper.GetLocalizedCharacterName(oper) ?? "???";
                        if (ids.Contains(id))
                        {
                            OperBoxHaveList.Add(new Operator(id, name, oper.Rarity));
                        }
                        else
                        {
                            OperBoxNotHaveList.Add(new Operator(id, name, oper.Rarity));
                        }
                    }
                }
            }
            catch
            {
                // 兼容老数据或异常时忽略
            }
        }

        /// <summary>
        /// 每次传进来的都是完整数据, 临时缓存去重
        /// </summary>
        private HashSet<string> _tempOperHaveSet = [];

        public bool OperBoxParse(JObject? details)
        {
            if (details == null)
            {
                return false;
            }

            var ownOpers = (details["own_opers"] as JArray)?.ToObject<List<OperBoxData.OperData>>()?.Where(o => !string.IsNullOrEmpty(o.Id)).ToList();
            if (ownOpers is null)
            {
                return false;
            }

            foreach (var oper in ownOpers)
            {
                if (_tempOperHaveSet.Add(oper.Id))
                {
                    var name = DataHelper.GetLocalizedCharacterName(DataHelper.Operators.FirstOrDefault(i => i.Key == oper.Id).Value) ?? "???";
                    OperBoxHaveList.Add(new Operator(oper.Id, name, oper.Rarity));
                }
            }

            bool done = (bool)(details["done"] ?? false);
            if (!done)
            {
                return true;
            }

            foreach (var (id, oper) in DataHelper.Operators)
            {
                if (!_tempOperHaveSet.Contains(id) && DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType))
                {
                    var name = DataHelper.GetLocalizedCharacterName(oper) ?? "???";
                    OperBoxNotHaveList.Add(new Operator(id, name, oper.Rarity));
                }
            }

            OperBoxInfo = $"{LocalizationHelper.GetString("IdentificationCompleted")}\n{LocalizationHelper.GetString("OperBoxRecognitionTip")}";
            OperBoxDataArray = ownOpers;
            SaveOperBoxDetails(ownOpers);
            _tempOperHaveSet = [];
            return true;
        }

        /// <summary>
        /// 开始识别干员
        /// </summary>
        /// xaml 中用到了
        /// ReSharper disable once UnusedMember.Global
        public async void StartOperBox()
        {
            _tempOperHaveSet = [];
            OperBoxHaveList = [];
            OperBoxNotHaveList = [];
            _runningState.SetIdle(false);

            string errMsg = string.Empty;
            OperBoxInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
            if (!caught)
            {
                OperBoxInfo = errMsg;
                _runningState.SetIdle(true);
                return;
            }

            if (Instances.AsstProxy.AsstStartOperBox())
            {
                OperBoxInfo = LocalizationHelper.GetString("Identifying");
            }
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void ExportOperBox()
        {
            if (OperBoxDataArray.Count == 0)
            {
                return;
            }

            Clipboard.SetDataObject(JsonConvert.SerializeObject(OperBoxHaveList.Concat(OperBoxNotHaveList), Formatting.Indented));
            OperBoxInfo = LocalizationHelper.GetString("CopiedToClipboard");
        }

        #endregion OperBox

        #region Gacha

        private string _gachaInfo = LocalizationHelper.GetString("GachaInitTip");

        public string GachaInfo
        {
            get => _gachaInfo;
            set => SetAndNotify(ref _gachaInfo, value);
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void GachaOnce()
        {
            StartGacha();
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void GachaTenTimes()
        {
            StartGacha(false);
        }

        private bool _isGachaInProgress;

        public bool IsGachaInProgress
        {
            get => _isGachaInProgress;
            set
            {
                if (!SetAndNotify(ref _isGachaInProgress, value))
                {
                    return;
                }

                if (!value)
                {
                    _gachaTimer.Stop();
                }
            }
        }

        public async void StartGacha(bool once = true)
        {
            _runningState.SetIdle(false);

            string errMsg = string.Empty;
            GachaInfo = LocalizationHelper.GetString("ConnectingToEmulator");
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg) && Instances.AsstProxy.AsstStartGacha(once));
            if (!caught)
            {
                GachaInfo = errMsg;
                _runningState.SetIdle(true);
                return;
            }

            _gachaTimer.Interval = TimeSpan.FromSeconds(5);
            _gachaTimer.Start();

            RefreshGachaTip(null, null);
            IsGachaInProgress = true;
            Peep();
        }

        private void RefreshGachaTip(object? sender, EventArgs? e)
        {
            var rd = new Random();
            GachaInfo = LocalizationHelper.GetString("GachaTip" + rd.Next(1, 18));
        }

        // DO NOT CHANGE
        // 请勿更改
        // 請勿更改
        // このコードを変更しないでください
        // 변경하지 마십시오
        private bool _gachaShowDisclaimer = true; // !Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ShowDisclaimerNoMore, bool.FalseString));

        public bool GachaShowDisclaimer
        {
            get => _gachaShowDisclaimer;
            set
            {
                SetAndNotify(ref _gachaShowDisclaimer, value);
            }
        }

        private bool _gachaShowDisclaimerNoMore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, bool.FalseString));

        public bool GachaShowDisclaimerNoMore
        {
            get => _gachaShowDisclaimerNoMore;
            set
            {
                SetAndNotify(ref _gachaShowDisclaimerNoMore, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, value.ToString());
            }
        }

        // xaml 中用到了
        // ReSharper disable once UnusedMember.Global
        public void GachaAgreeDisclaimer()
        {
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetString("GachaWarning"),
                LocalizationHelper.GetString("Warning"),
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning,
                no: LocalizationHelper.GetString("Confirm"),
                yes: LocalizationHelper.GetString("Cancel"),
                iconBrushKey: "DangerBrush");
            if (result != MessageBoxResult.No)
            {
                return;
            }

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.RealGacha);

            GachaShowDisclaimer = false;
        }

        #endregion Gacha

        #region Peep

        private bool _peeping;

        public bool Peeping
        {
            get => _peeping;
            set
            {
                if (!SetAndNotify(ref _peeping, value))
                {
                    return;
                }

                if (!value)
                {
                    _peepImageTimer.Stop();
                }
            }
        }

        private bool _isPeepInProgress;

        /// <summary>
        /// Gets or sets a value indicating whether由 Peep 方法启动的 Peep
        /// </summary>
        public bool IsPeepInProgress
        {
            get => _isPeepInProgress;
            set
            {
                if (!SetAndNotify(ref _isPeepInProgress, value))
                {
                    return;
                }

                if (!value)
                {
                    _peepImageTimer.Stop();
                }
            }
        }

        private BitmapImage? _peepImage;

        public BitmapImage? PeepImage
        {
            get => _peepImage;
            set => SetAndNotify(ref _peepImage, value);
        }

        private double _peepScreenFpf;

        public double PeepScreenFpf
        {
            get => _peepScreenFpf;
            set => SetAndNotify(ref _peepScreenFpf, value);
        }

        private int _peepTargetFps = int.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.PeepTargetFps, "20"));

        public int PeepTargetFps
        {
            get
            {
                return _peepTargetFps;
            }

            set
            {
                value = value switch
                {
                    > 60 => 60,
                    < 1 => 1,
                    _ => value,
                };

                SetAndNotify(ref _peepTargetFps, value);
                _peepImageTimer.Interval = 1000d / _peepTargetFps;
                ConfigurationHelper.SetValue(ConfigurationKeys.PeepTargetFps, value.ToString());
            }
        }

        private DateTime _lastFpsUpdateTime = DateTime.MinValue;
        private int _frameCount;

        private readonly Timer _peepImageTimer = new();
        private readonly DispatcherTimer _gachaTimer = new() { Interval = TimeSpan.FromSeconds(5) };

        private int _peepImageCount;
        private int _peepImageNewestCount;

        private static int _peepImageSemaphoreCurrentCount = 2;
        private const int PeepImageSemaphoreMaxCount = 5;
        private static int _peepImageSemaphoreFailCount = 0;
        private static readonly SemaphoreSlim _peepImageSemaphore = new(_peepImageSemaphoreCurrentCount, PeepImageSemaphoreMaxCount);

        private async void RefreshPeepImageAsync(object? sender, EventArgs? e)
        {
            if (!await _peepImageSemaphore.WaitAsync(0))
            {
                // 一秒内连续三次未能获取信号量，降低 FPS
                if (++_peepImageSemaphoreFailCount < 3)
                {
                    return;
                }

                _peepImageSemaphoreFailCount = 0;

                if (_peepImageSemaphoreCurrentCount < PeepImageSemaphoreMaxCount)
                {
                    _peepImageSemaphoreCurrentCount++;
                    _peepImageSemaphore.Release();
                    return;
                }

                _logger.Warning($"Gacha Semaphore Full, Reduce fps count to {--PeepTargetFps}");
                _ = Execute.OnUIThreadAsync(() =>
                {
                    Growl.Clear();
                    Growl.Warning($"Screenshot taking too long, reduce FPS to {PeepTargetFps}");
                });
                return;
            }

            try
            {
                var count = Interlocked.Increment(ref _peepImageCount);
                var cacheImage = await Instances.AsstProxy.AsstGetFreshImageAsync();
                if (!Peeping || count <= _peepImageNewestCount || cacheImage is null)
                {
                    return;
                }

                Interlocked.Exchange(ref _peepImageNewestCount, count);
                PeepImage = cacheImage;

                var now = DateTime.Now;
                Interlocked.Increment(ref _frameCount);
                var totalSeconds = (now - _lastFpsUpdateTime).TotalSeconds;
                if (totalSeconds < 1)
                {
                    return;
                }

                var frameCount = Interlocked.Exchange(ref _frameCount, 0);
                _lastFpsUpdateTime = now;
                PeepScreenFpf = frameCount / totalSeconds;
                _peepImageSemaphoreFailCount = 0;
            }
            finally
            {
                _peepImageSemaphore.Release();
            }
        }

        private bool _isPeepTransitioning;

        public bool IsPeepTransitioning
        {
            get => _isPeepTransitioning;
            set => SetAndNotify(ref _isPeepTransitioning, value);
        }

        /// <summary>
        /// 获取或停止获取实时截图，在抽卡时额外停止抽卡
        /// </summary>
        public async void Peep()
        {
            if (IsPeepTransitioning)
            {
                return;
            }

            IsPeepTransitioning = true;

            try
            {
                // 正在 Peep 时，点击按钮停止 Peep
                if (Peeping)
                {
                    Peeping = false;
                    _peepImageTimer.Stop();

                    // 由 Peep() 方法启动的 Peep 也需要停止，Block 不会自动停止
                    if (IsGachaInProgress || IsPeepInProgress)
                    {
                        await Instances.TaskQueueViewModel.Stop();
                        Instances.TaskQueueViewModel.SetStopped();
                    }

                    IsPeepInProgress = false;
                    return;
                }

                // 点击按钮开始 Peep
                Peeping = true;

                AchievementTrackerHelper.Instance.Unlock(AchievementIds.PeekScreen);

                // 如果没任务在运行，需要先连接，并标记是由 Peep() 方法启动的 Peep
                if (Idle)
                {
                    _runningState.SetIdle(false);
                    string errMsg = string.Empty;
                    bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
                    if (!caught)
                    {
                        GachaInfo = errMsg;
                        _runningState.SetIdle(true);
                        return;
                    }

                    IsPeepInProgress = true;
                }

                PeepScreenFpf = 0;
                _peepImageCount = 0;
                _peepImageNewestCount = 0;
                _peepImageTimer.Start();
            }
            finally
            {
                IsPeepTransitioning = false;
            }
        }

        #endregion

        #region MiniGame

        public static ObservableCollection<CombinedData> MiniGameTaskList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("NotSelected"), Value = "Stop" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameGreenGrass"), Value = "GreenGrass@DuelChannel@Begin" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameGreenTicketStore"), Value = "GreenTicket@Store@Begin" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameYellowTicketStore"), Value = "YellowTicket@Store@Begin" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameSsStore"), Value = "SS@Store@Begin" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameRAStore"), Value = "RA@Store@Begin" },
        ];

        private string _miniGameTaskName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MiniGameTaskName, "Stop");

        public string MiniGameTaskName
        {
            get => _miniGameTaskName;
            set
            {
                SetAndNotify(ref _miniGameTaskName, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MiniGameTaskName, value);
                MiniGameTip = GetMiniGameTip(value);
            }
        }

        private string? _miniGameTip;

        public string MiniGameTip
        {
            get
            {
                _miniGameTip ??= GetMiniGameTip(MiniGameTaskName);
                return _miniGameTip;
            }
            set => SetAndNotify(ref _miniGameTip, value);
        }

        private static string GetMiniGameTip(string name)
        {
            return name switch
            {
                "GreenGrass@DuelChannel@Begin" => LocalizationHelper.GetString("MiniGameNameGreenGrassTip"),
                "GreenTicket@Store@Begin" => LocalizationHelper.GetString("MiniGameNameGreenTicketStoreTip"),
                "YellowTicket@Store@Begin" => LocalizationHelper.GetString("MiniGameNameYellowTicketStoreTip"),
                "SS@Store@Begin" => LocalizationHelper.GetString("MiniGameNameSsStoreTip"),
                "RA@Store@Begin" => LocalizationHelper.GetString("MiniGameNameRAStoreTip"),
                _ => string.Empty,
            };
        }

        public void StartMiniGame()
        {
            _ = StartMiniGameAsync();
        }

        private async Task StartMiniGameAsync()
        {
            if (!Idle)
            {
                await Instances.TaskQueueViewModel.Stop();
                Instances.TaskQueueViewModel.SetStopped();
                return;
            }

            _runningState.SetIdle(false);
            string errMsg = string.Empty;
            bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg) && Instances.AsstProxy.AsstMiniGame(MiniGameTaskName));
            if (!caught)
            {
                _runningState.SetIdle(true);
            }
        }

        #endregion
    }
}
