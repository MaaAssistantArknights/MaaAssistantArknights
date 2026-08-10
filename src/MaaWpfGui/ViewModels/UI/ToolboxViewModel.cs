// <copyright file="ToolboxViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.Buffers;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using HandyControl.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using ObservableCollections;
using Serilog;
using Stylet;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The view model of recruit.
/// </summary>
public class ToolboxViewModel : Screen
{
    private readonly RunningState _runningState;
    private static readonly ILogger _logger = Log.ForContext<ToolboxViewModel>();

    /// <summary>
    /// Initializes a new instance of the <see cref="ToolboxViewModel"/> class.
    /// </summary>
    public ToolboxViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        DisplayName = LocalizationHelper.GetString("Toolbox");
        _runningState = RunningState.Instance;
        _runningState.StateChanged += (__, e) => {
            Idle = e.NewState.Idle;
            Inited = e.NewState.Inited;
            Stopping = e.NewState.Stopping;

            if (e.NewState.Idle)
            {
                PixelPaintParametersLocked = false;
            }

            if (e.NewState.Stopping && Peeping && !IsPeepTransitioning)
            {
                _ = Peep();
            }
        };
        _peepImageTimer.Elapsed += PeepImageTimerElapsed;

        // 本类型由 Stylet IoC 容器管理，全应用生命周期唯一实例，订阅后无需取消订阅
        LocalizationHelper.LanguageChanged += () => {
            DisplayName = LocalizationHelper.GetString("Toolbox");
            RecruitInfo = LocalizationHelper.GetString("RecruitmentRecognitionTip");
            Application.Current.Dispatcher.InvokeAsync(
                () => {
                    LoadDepotDetails();
                    ClearOperBoxRecognitionData();
                    LoadOperBoxDetails();
                },
                DispatcherPriority.Loaded);
        };
        SettingsViewModel.GuiSettings.OperNameLanguageChanged += () => {
            Application.Current.Dispatcher.InvokeAsync(() => {
                ClearOperBoxRecognitionData();
                LoadOperBoxDetails();
            }, DispatcherPriority.Loaded);
        };
        _peepImageTimer.Interval = 1000d / PeepTargetFps;
        _gachaTimer.Tick += RefreshGachaTip;
        LoadDepotDetails();
        LoadOperBoxDetails();
        InitializeDepotRowPresentation();
        InitializeOperBoxRowPresentation();
        OperBoxSelectedIndex = OperBoxNotHaveList.Count > 0 ? 0 : 1;

        UpdateMiniGameTaskList();
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

    private bool _inited;

    public bool Inited
    {
        get => _inited;
        set => SetAndNotify(ref _inited, value);
    }

    private bool _stopping;

    public bool Stopping
    {
        get => _stopping;
        set => SetAndNotify(ref _stopping, value);
    }

    #region Recruit

    /// <summary>
    /// Gets or sets the recruit info.
    /// </summary>
    public string RecruitInfo { get => field; set => SetAndNotify(ref field, value); } = LocalizationHelper.GetString("RecruitmentRecognitionTip");

    public ObservableCollection<Inline> RecruitResultInlines { get => field; set => SetAndNotify(ref field, value); } = [];

    public void UpdateRecruitResult(JArray? resultArray)
    {
        ObservableCollection<Inline> recruitResultInlines = [];

        foreach (var combs in resultArray ?? [])
        {
            int tagLevel = (int)(combs["level"] ?? -1);
            var tagStr = $"{tagLevel}★ Tags:    ";
            tagStr = ((JArray?)combs["tags"] ?? []).Aggregate(tagStr, (current, tag) => current + $"{tag}    ");
            var tagRun = new Run(tagStr);
            tagRun.SetResourceReference(TextElement.ForegroundProperty, UiLogColor.Text);
            tagRun.Tag = UiLogColor.Text;

            recruitResultInlines.Add(tagRun);

            recruitResultInlines.Add(new LineBreak());

            var opersArray = (JArray?)combs["opers"] ?? [];

            var opersWithPotential = opersArray.Select(oper => {
                int operLevel = (int)(oper["level"] ?? -1);
                var operId = oper["id"]?.ToString();

                int pot = -1;
                if (RecruitmentShowPotential && OperBoxPotential != null && operId != null && (tagLevel >= 4 || operLevel == 1))
                {
                    if (OperBoxPotential.TryGetValue(operId, out var potentialValue))
                    {
                        pot = potentialValue;
                    }
                }

                return new { Oper = oper, Potential = pot, OperLevel = operLevel };
            })
            .OrderByDescending(x => x.OperLevel)
            .ThenBy(x => x.Potential)
            .ToList();

            foreach (var x in opersWithPotential)
            {
                var oper = x.Oper;
                int operLevel = x.OperLevel;
                var operId = oper["id"]?.ToString();
                var operName = DataHelper.GetLocalizedCharacterName(oper["name"]?.ToString());

                bool isMaxPot = false;
                string potentialText = string.Empty;

                if (RecruitmentShowPotential && OperBoxPotential != null && operId != null && (tagLevel >= 4 || operLevel == 1))
                {
                    if (OperBoxPotential.TryGetValue(operId, out var pot))
                    {
                        potentialText = $" ( {pot} )";
                        if (pot == 6)
                        {
                            isMaxPot = true;
                            potentialText = " ( MAX )";
                        }
                    }
                    else
                    {
                        potentialText = " ( !!! NEW !!! )";
                    }
                }

                var run = new Run($"{operName}{potentialText}    ");
                var brushKey = GetBrushKeyByStar(operLevel, isMaxPot);
                run.SetResourceReference(TextElement.ForegroundProperty, brushKey);
                run.Tag = brushKey;

                recruitResultInlines.Add(run);
            }

            recruitResultInlines.Add(new LineBreak());
            recruitResultInlines.Add(new LineBreak());
        }

        RecruitResultInlines = recruitResultInlines;
        return;

        string GetBrushKeyByStar(int level, bool isMax)
        {
            return (level, isMax) switch {
                (6, true) => UiLogColor.Star6OperatorPotentialFull,
                (6, false) => UiLogColor.Star6Operator,
                (5, true) => UiLogColor.Star5OperatorPotentialFull,
                (5, false) => UiLogColor.Star5Operator,
                (4, true) => UiLogColor.Star4OperatorPotentialFull,
                (4, false) => UiLogColor.Star4Operator,
                (3, true) => UiLogColor.Star3OperatorPotentialFull,
                (3, false) => UiLogColor.Star3Operator,
                (2, true) => UiLogColor.Star2OperatorPotentialFull,
                (2, false) => UiLogColor.Star2Operator,
                (1, true) => UiLogColor.Star1OperatorPotentialFull,
                (1, false) => UiLogColor.Star1Operator,
                _ => UiLogColor.Text,
            };
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 3.
    /// </summary>
    public bool ChooseLevel3
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3 = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3;

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 4.
    /// </summary>
    public bool ChooseLevel4
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4 = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4;

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 5.
    /// </summary>
    public bool ChooseLevel5
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel5 = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel5;

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 6.
    /// </summary>
    public bool ChooseLevel6
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel6 = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel6;

    [PropertyDependsOn(nameof(ChooseLevel3Time))]
    public int ChooseLevel3Hour
    {
        get => ChooseLevel3Time / 60;
        set => ChooseLevel3Time = (value * 60) + ChooseLevel3Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel3Time))]
    public int ChooseLevel3Min
    {
        get => (ChooseLevel3Time % 60) / 10 * 10;
        set => ChooseLevel3Time = (ChooseLevel3Hour * 60) + value;
    }

    public int ChooseLevel3Time
    {
        get; set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3Time = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3Time;

    [PropertyDependsOn(nameof(ChooseLevel4Time))]
    public int ChooseLevel4Hour
    {
        get => ChooseLevel4Time / 60;
        set => ChooseLevel4Time = (value * 60) + ChooseLevel4Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel4Time))]
    public int ChooseLevel4Min
    {
        get => (ChooseLevel4Time % 60) / 10 * 10;
        set => ChooseLevel4Time = (ChooseLevel4Hour * 60) + value;
    }

    public int ChooseLevel4Time
    {
        get; set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4Time = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4Time;

    /// <summary>
    /// Gets or sets a value indicating whether to set time automatically.
    /// </summary>
    public bool RecruitAutoSetTime
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.AutoSetTime = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.AutoSetTime;

    /// <summary>
    /// Starts calculation.
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task RecruitStartCalc()
    {
        string errMsg = string.Empty;
        RecruitInfo = LocalizationHelper.GetString("ConnectingToEmulator");
        _runningState.SetIdle(false);
        var recruitCaught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (!recruitCaught)
        {
            RecruitInfo = errMsg;
            _runningState.SetIdle(true);
            return;
        }

        RecruitInfo = LocalizationHelper.GetString("Identifying");

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

        var task = new AsstRecruitTask() {
            SelectList = levelList,
            ConfirmList = [-1], // 仅公招识别时将-1加入comfirm_level
            SetRecruitTime = RecruitAutoSetTime,
            ChooseLevel3Time = ChooseLevel3Time,
            ChooseLevel4Time = ChooseLevel4Time,
            ServerType = Instances.SettingsViewModel.ServerType,
        };
        var (type, taskParams) = task.Serialize();
        bool ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.RecruitCalc, type, taskParams);
        ret &= Instances.AsstProxy.AsstStart();
    }

    public bool RecruitmentShowPotential
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.ShowPotential = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.ShowPotential;

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
                    JArray? resultArray = (JArray?)subTaskDetails?["result"];
                    UpdateRecruitResult(resultArray);
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

    /// <summary>
    /// Gets or sets 上次仓库同步时间（UTC 时间）
    /// </summary>
    public DateTimeOffset? LastDepotSyncTime { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets 上次仓库同步时间的显示文本（本地时间）
    /// </summary>
    [PropertyDependsOn(nameof(LastDepotSyncTime))]
    public string LastDepotSyncTimeText
    {
        get {
            if (LastDepotSyncTime == null)
            {
                return string.Empty;
            }

            // 将 UTC 时间转换为本地时间显示
            return LastDepotSyncTime.Value.ToLocalTimeString();
        }
    }

    private const int DepotRowSize = 5;

    private ObservableList<DepotResultDate> _depotResult = [];

    /// <summary>
    /// Gets or sets the depot result.
    /// </summary>
    public ObservableList<DepotResultDate> DepotResult
    {
        get => _depotResult;
        set {
            if (ReferenceEquals(_depotResult, value))
            {
                RefreshDepotRows();
                InvalidateDepotCache();
                return;
            }

            _depotResult.CollectionChanged -= DepotResultCollectionChanged;
            SetAndNotify(ref _depotResult, value);
            _depotResult.CollectionChanged += DepotResultCollectionChanged;
            RefreshDepotRows();
            InvalidateDepotCache();
        }
    }

    private ObservableCollection<ObservableCollection<DepotResultDate>> _depotRows = [];

    public ObservableCollection<ObservableCollection<DepotResultDate>> DepotRows
    {
        get => _depotRows;
        private set => SetAndNotify(ref _depotRows, value);
    }

    public int DepotColumnCount => GetColumnCount(DepotResult.Count, DepotRowSize);

    // 缓存相关字段
    private bool _depotCacheInvalid = true;
    private string? _cachedArkPlannerResult;
    private string? _cachedLoliconResult;
    private readonly HashSet<int> _pendingDepotSyncTimeResetTaskIds = [];

    public void MarkDepotRecognitionSyncTimeForReset(int taskId)
    {
        if (taskId > 0)
        {
            _pendingDepotSyncTimeResetTaskIds.Add(taskId);
        }
    }

    /// <summary>
    /// 标记仓库缓存失效
    /// </summary>
    private void InvalidateDepotCache()
    {
        _depotCacheInvalid = true;
        _cachedArkPlannerResult = null;
        _cachedLoliconResult = null;
    }

    public class DepotResultDate : IComparable<DepotResultDate>
    {
        public string? Name { get; set; }

        public string Id { get; set; } = null!;

        public BitmapSource? Image { get; set; }

        /// <summary>
        /// Gets or sets 物品数量（原始数值）
        /// </summary>
        public int Count { get; set; }

        /// <summary>
        /// Gets 格式化后的显示数量（用于 UI 绑定）
        /// </summary>
        public string? DisplayCount => Count >= 0 ? Count.FormatNumber(false) : null;

        /// <summary>
        /// 创建后懒加载缓存的 sortId，避免每次比较都查字典。
        /// </summary>
        private int? _cachedSortId;

        private int SortId => _cachedSortId ??=
            ItemListHelper.ArkItems != null &&
            ItemListHelper.ArkItems.TryGetValue(Id, out var item)
                ? item.SortId
                : int.MaxValue;

        /// <summary>
        /// 按游戏内置 sortId 排序（值越小越靠前），查不到的按 ID 文本兜底。
        /// </summary>
        /// <param name="other">要比较的另一个 DepotResultDate 对象</param>
        /// <returns>比较结果</returns>
        public int CompareTo(DepotResultDate? other)
        {
            if (other is null)
            {
                return 1;
            }

            if (SortId != other.SortId)
            {
                return SortId.CompareTo(other.SortId);
            }

            return string.Compare(Id, other.Id, StringComparison.OrdinalIgnoreCase);
        }
    }

    private void InitializeDepotRowPresentation()
    {
        _depotResult.CollectionChanged += DepotResultCollectionChanged;
        RefreshDepotRows();
    }

    private void DepotResultCollectionChanged(in NotifyCollectionChangedEventArgs<DepotResultDate> e) => RefreshDepotRows();

    private void RefreshDepotRows()
    {
        DepotRows = BuildRows(DepotResult, DepotRowSize);
        NotifyOfPropertyChange(nameof(DepotColumnCount));
    }

    /// <summary>
    /// 保存仓库详情数据
    /// </summary>
    private void SaveDepotDetails()
    {
        // 构建简化格式：{"itemId": count}
        var details = new JObject {
            ["done"] = true,
            ["data"] = JObject.FromObject(DepotResult.Where(item => item.Count >= 0).ToDictionary(item => item.Id, item => item.Count)),
        };

        // 保存同步时间为 UTC（如果有）
        if (LastDepotSyncTime.HasValue)
        {
            details["syncTime"] = LastDepotSyncTime.Value.ToLocalTime().ToString("o"); // ISO 8601 格式
        }

        JsonDataHelper.Set(JsonDataKey.DepotData, details);
    }

    private void LoadDepotDetails()
    {
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
        catch (Exception ex)
        {
            _logger.Error("parse depot json failed,\n{str}", json, ex);
        }
    }

    /// <summary>
    /// Gets 获取 ArkPlanner 导出格式（带缓存）
    /// </summary>
    public string ArkPlannerResult
    {
        get {
            if (DepotResult.Count == 0)
            {
                return string.Empty;
            }

            // 使用缓存
            if (!_depotCacheInvalid && _cachedArkPlannerResult != null)
            {
                return _cachedArkPlannerResult;
            }

            // 重新计算
            var items = DepotResult
                .Where(item => item.Count >= 0)
                .Select(item => new JObject {
                    ["id"] = item.Id,
                    ["have"] = item.Count,
                    ["name"] = item.Name ?? string.Empty,
                });

            var result = new JObject {
                ["@type"] = "@penguin-statistics/depot",
                ["items"] = new JArray(items),
            };

            _cachedArkPlannerResult = result.ToString(Formatting.None);
            _depotCacheInvalid = false; // 标记缓存已更新
            return _cachedArkPlannerResult;
        }
    }

    /// <summary>
    /// Gets 获取 工具箱 导出格式（带缓存）
    /// </summary>
    public string LoliconResult
    {
        get {
            if (DepotResult.Count == 0)
            {
                return string.Empty;
            }

            // 使用缓存
            if (!_depotCacheInvalid && _cachedLoliconResult != null)
            {
                return _cachedLoliconResult;
            }

            // 重新计算
            var depotData = new JObject();
            foreach (var item in DepotResult)
            {
                if (item.Count >= 0)
                {
                    depotData[item.Id] = item.Count;
                }
            }

            _cachedLoliconResult = depotData.ToString(Formatting.None);
            _depotCacheInvalid = false; // 标记缓存已更新
            return _cachedLoliconResult;
        }
    }

    /// <summary>
    /// 解析仓库识别结果（兼容新旧格式）
    /// </summary>
    /// <param name="details">详细的 JSON 参数</param>
    /// <param name="updateSyncTime">是否更新同步时间为当前时间（从 Core 获取新数据时为 true，从本地加载时为 false）</param>
    /// <param name="taskId">传入对应的任务 ID 以便在收到回调后重置识别状态</param>
    /// <returns>是否成功</returns>
    public bool DepotParse(JObject? details, bool updateSyncTime = false, int taskId = 0)
    {
        if (details == null)
        {
            return false;
        }

        if (_pendingDepotSyncTimeResetTaskIds.Remove(taskId))
        {
            ResetDepotRecognitionState();
        }

        DepotResult.Clear();

        Dictionary<string, int> depotItems = [];

        // 尝试解析新格式
        var dataToken = details["data"];
        if (dataToken is JObject dataObj)
        {
            foreach (var prop in dataObj.Properties())
            {
                if (int.TryParse(prop.Value.ToString(), out var count))
                {
                    depotItems[prop.Name] = count;
                }
            }
        }
        else if (dataToken?.ToString() is string dataStr && !string.IsNullOrEmpty(dataStr))
        { // 旧版格式迁移
            try
            {
                var dataO = JObject.Parse(dataStr);
                foreach (var prop in dataO.Properties())
                {
                    if (int.TryParse(prop.Value.ToString(), out var count))
                    {
                        depotItems[prop.Name] = count;
                    }
                }
            }
            catch (Exception ex)
            {
                _logger.Warning(ex, "Failed to parse depot data format");
            }
        }

        // 如果新格式解析失败，尝试旧格式
        if (depotItems.Count == 0)
        {
            if (depotItems.Count == 0)
            {
                var arkplannerItems = details["arkplanner"]?["object"]?["items"]?.Cast<JObject>() ?? [];

                foreach (var item in arkplannerItems)
                {
                    var id = (string?)item["id"];
                    if (string.IsNullOrEmpty(id))
                    {
                        continue;
                    }

                    if (item["have"] != null && int.TryParse(item["have"]?.ToString(), out var count))
                    {
                        depotItems[id] = count;
                    }
                }
            }
        }

        // 构建结果并检查成就，按游戏内置 sortId 排序
        var results = depotItems.Select(kvp => new DepotResultDate {
            Id = kvp.Key,
            Name = ItemListHelper.GetItemName(kvp.Key),
            Image = ItemListHelper.GetItemImage(kvp.Key),
            Count = kvp.Value,
        }).OrderBy(r => r);

        foreach (var result in results)
        {
            if (result.Count > 0 &&
                result.Count > AchievementTrackerHelper.Instance.GetProgress(AchievementIds.WarehouseMiser))
            {
                AchievementTrackerHelper.Instance.SetProgress(AchievementIds.WarehouseMiser, result.Count);
            }
        }

        DepotResult.AddRange(results);

        // 标记缓存失效
        InvalidateDepotCache();

        bool done = (bool)(details["done"] ?? false);
        if (!done)
        {
            return true;
        }

        if (updateSyncTime)
        {
            // 从 Core 获取新数据，更新为当前 UTC 时间
            AchievementTrackerHelper.Instance.CheckResyncAfterDays(LastDepotSyncTime?.UtcDateTime, 7, AchievementIds.ResumeRecord);
            LastDepotSyncTime = DateTimeOffset.UtcNow;
        }
        else
        {
            // 从本地加载，读取保存的时间
            var syncTimeStr = details["syncTime"]?.ToString(Formatting.None)?.Trim('"');
            if (!string.IsNullOrEmpty(syncTimeStr) && DateTimeOffset.TryParse(syncTimeStr, null, DateTimeStyles.AssumeUniversal, out var lastDepotSyncTime))
            {
                LastDepotSyncTime = lastDepotSyncTime;
            }
        }

        DepotInfo = LocalizationHelper.GetString("IdentificationCompleted");
        SaveDepotDetails();
        Instances.TaskQueueViewModel.UpdateDatePrompt();

        return true;
    }

    /// <summary>
    /// 仓库导出格式
    /// </summary>
    public enum DepotExportFormat
    {
        /// <summary>
        /// https://github.com/penguin-statistics/ArkPlanner
        /// </summary>
        Arkplanner = 0,

        /// <summary>
        /// https://arkntools.app/#/material
        /// </summary>
        Lolicon = 1,

        /// <summary>
        /// Specifies that the content is formatted using Markdown syntax.
        /// </summary>
        Markdown = 2,

        /// <summary>
        /// Specifies that the content is formatted as comma-separated values (CSV).
        /// </summary>
        Csv = 3,
    }

    /// <summary>
    /// 干员BOX导出格式
    /// </summary>
    public enum OperBoxExportFormat
    {
        /// <summary>
        /// Represents the clipboard as a data source or destination.
        /// </summary>
        Clipboard = 0,

        /// <summary>
        /// Specifies that the content type is JSON format.
        /// </summary>
        Json = 1,

        /// <summary>
        /// Specifies that the content is formatted using Markdown syntax.
        /// </summary>
        Markdown = 2,

        /// <summary>
        /// Specifies that the data format is comma-separated values (CSV).
        /// </summary>
        Csv = 3,
    }

    public record struct ExportEntry(string Display, int Value);

    public List<ExportEntry> ExportOptionList { get; } = [
        new(LocalizationHelper.GetString("ExportToArkplanner"), (int)DepotExportFormat.Arkplanner),
        new(LocalizationHelper.GetString("ExportToLolicon"), (int)DepotExportFormat.Lolicon),
        new(LocalizationHelper.GetString("ExportToMarkdown"), (int)DepotExportFormat.Markdown),
        new(LocalizationHelper.GetString("ExportToCsv"), (int)DepotExportFormat.Csv),
    ];

    private int _selectedExportValue;

    public int SelectedExportValue
    {
        get => _selectedExportValue;
        set => SetAndNotify(ref _selectedExportValue, value);
    }

    [UsedImplicitly]
    public void ExecuteSelectedExport()
    {
        switch ((DepotExportFormat)_selectedExportValue)
        {
            case DepotExportFormat.Arkplanner: ExportToArkplanner(); break;
            case DepotExportFormat.Lolicon: ExportToLolicon(); break;
            case DepotExportFormat.Markdown: ExportToMarkdown(); break;
            case DepotExportFormat.Csv: ExportToCsv(); break;
        }
    }

    /// <summary>
    /// Export depot info to ArkPlanner.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToArkplanner()
    {
        Clipboard.Clear();
        Clipboard.SetDataObject(ArkPlannerResult);
        Growl.Info(LocalizationHelper.GetString("CopiedToClipboard"));
    }

    /// <summary>
    /// Export depot info to Lolicon.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToLolicon()
    {
        Clipboard.Clear();
        Clipboard.SetDataObject(LoliconResult);
        Growl.Info(LocalizationHelper.GetString("CopiedToClipboard"));
    }

    /// <summary>
    /// Export depot info to Markdown file.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToMarkdown()
    {
        ExportDepot(BuildMarkdownExportLines, "Markdown files (*.md)|*.md|All files (*.*)|*.*", ".md", "Arknights_Depot_Export.md");
    }

    /// <summary>
    /// Export depot info to CSV file.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToCsv()
    {
        ExportDepot(BuildCsvExportLines, "CSV files (*.csv)|*.csv|All files (*.*)|*.*", ".csv", "Arknights_Depot_Export.csv");
    }

    private void ExportDepot(Func<IReadOnlyList<DepotResultDate>, IEnumerable<string>> lineBuilder, string filter, string defaultExt, string defaultFileName)
    {
        var items = DepotResult.Where(item => item.Count >= 0).ToList();
        if (items.Count == 0)
        {
            return;
        }

        var content = string.Join(Environment.NewLine, lineBuilder(items));

        var dialog = new Microsoft.Win32.SaveFileDialog {
            Filter = filter,
            DefaultExt = defaultExt,
            FileName = defaultFileName,
        };

        if (dialog.ShowDialog() != true)
        {
            return;
        }

        File.WriteAllText(dialog.FileName, content);
        Growl.Info(LocalizationHelper.GetString("ExportedToFile"));
    }

    private static IEnumerable<string> BuildMarkdownExportLines(IReadOnlyList<DepotResultDate> items)
    {
        var lines = new List<string> { "# Arknights Depot Export", string.Empty, "| ID | Name | Count |", "| --- | --- | --- |" };
        foreach (var item in items)
        {
            lines.Add($"| {item.Id} | {item.Name ?? string.Empty} | {item.Count} |");
        }

        return lines;
    }

    private static IEnumerable<string> BuildCsvExportLines(IReadOnlyList<DepotResultDate> items)
    {
        var lines = new List<string> { "ID,Name,Count" };
        foreach (var item in items)
        {
            var name = item.Name ?? string.Empty;
            if (name.Contains(',') || name.Contains('"') || name.Contains('\n'))
            {
                name = "\"" + name.Replace("\"", "\"\"") + "\"";
            }

            lines.Add($"{item.Id},{name},{item.Count}");
        }

        return lines;
    }

    /*
    private void DepotClear()
    {
        DepotResult.Clear();
        InvalidateDepotCache();
    }
    */

    // 需要排除的物品 ID（不统计到仓库）
    private static readonly HashSet<string> ExcludedItemIds =
    [
        "3401", // 家具
        "3112", "3113", "3114", // 碳
        "5001", // 经验
    ];

    /// <summary>
    /// 检查物品 ID 是否应该被排除（不统计到仓库）
    /// </summary>
    /// <param name="itemId">物品 ID</param>
    /// <returns>true 表示应该排除</returns>
    private static bool ShouldExcludeItem(string itemId)
    {
        // 排除特定 ID
        if (ExcludedItemIds.Contains(itemId))
        {
            return true;
        }

        // 排除非纯数字的 ID
        if (!int.TryParse(itemId, out _))
        {
            return true;
        }

        return false;
    }

    /// <summary>
    /// 根据 StageDrops 数据更新仓库
    /// </summary>
    /// <param name="drops">关卡掉落数据列表 (ItemId, ItemName, Total, Add)</param>
    public void UpdateDepotFromDrops(List<(string ItemId, string ItemName, int Total, int Add)> drops)
    {
        if (drops == null || drops.Count == 0)
        {
            return;
        }

        bool hasUpdates = false;

        foreach (var (itemId, _, total, add) in drops)
        {
            if (string.IsNullOrEmpty(itemId) || add <= 0)
            {
                continue;
            }

            // 过滤不需要统计的物品
            if (ShouldExcludeItem(itemId))
            {
                continue;
            }

            // 查找现有仓库项
            var existingItem = DepotResult.FirstOrDefault(x => x.Id == itemId);
            if (existingItem != null)
            {
                // 更新现有物品数量
                if (existingItem.Count >= 0)
                {
                    var newCount = existingItem.Count + add;
                    existingItem.Count = newCount;
                    hasUpdates = true;

                    // 更新成就进度
                    if (newCount > AchievementTrackerHelper.Instance.GetProgress(AchievementIds.WarehouseMiser))
                    {
                        AchievementTrackerHelper.Instance.SetProgress(AchievementIds.WarehouseMiser, newCount);
                    }
                }
            }
            else
            {
                // 添加新物品
                var newItem = new DepotResultDate {
                    Id = itemId,
                    Name = ItemListHelper.GetItemName(itemId),
                    Image = ItemListHelper.GetItemImage(itemId),
                    Count = add,
                };
                DepotResult.Add(newItem);
                hasUpdates = true;
            }
        }

        // 如果有更新，重新排序并保存
        if (hasUpdates)
        {
            // 按游戏内置 sortId 排序
            DepotResult.Sort();

            // 标记缓存失效
            InvalidateDepotCache();

            // 保存更新后的数据
            SaveDepotDetails();
            Instances.TaskQueueViewModel.UpdateDatePrompt();

            _logger.Information("Depot updated from stage drops, {Count} items processed", drops.Count);
        }
    }

    /// <summary>
    /// 重置仓库识别状态。
    /// </summary>
    public void ResetDepotRecognitionState()
    {
        // DepotParse 方法已经处理了数据清除和缓存失效，这里不需要重复调用
        // DepotClear();
        LastDepotSyncTime = null;
    }

    /// <summary>
    /// 追加或启动仓库识别任务。
    /// </summary>
    /// <param name="startImmediately">是否立刻启动。</param>
    /// <returns>是否成功。</returns>
    public bool StartDepotRecognitionTask(bool startImmediately = true)
    {
        bool ret = Instances.AsstProxy.AsstStartDepot(startImmediately);
        if (ret && startImmediately)
        {
            DepotInfo = LocalizationHelper.GetString("Identifying");
        }

        return ret;
    }

    /// <summary>
    /// Starts depot recognition.
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task StartDepot()
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

        ResetDepotRecognitionState();
        StartDepotRecognitionTask();
    }

    #endregion Depot

    #region OperBox

    /// <summary>
    /// Gets or sets 上次干员同步时间
    /// </summary>
    public DateTimeOffset? LastOperBoxSyncTime { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets 上次干员同步时间的显示文本
    /// </summary>
    [PropertyDependsOn(nameof(LastOperBoxSyncTime))]
    public string LastOperBoxSyncTimeText
    {
        get {
            if (LastOperBoxSyncTime == null)
            {
                return string.Empty;
            }

            return LastOperBoxSyncTime.Value.ToLocalTimeString();
        }
    }

    private int _operBoxSelectedIndex = 0;

    public int OperBoxSelectedIndex
    {
        get => _operBoxSelectedIndex;
        set => SetAndNotify(ref _operBoxSelectedIndex, value);
    }

    private string _operBoxInfo = LocalizationHelper.GetString("OperBoxRecognitionTip");

    public string OperBoxInfo
    {
        get => _operBoxInfo;
        set => SetAndNotify(ref _operBoxInfo, value);
    }

    /// <summary>
    /// Gets OperBoxDataArray from OperBoxHaveList for backward compatibility
    /// </summary>
    [Obsolete("Use OperBoxHaveList instead")]
    public List<OperBoxData.OperData> OperBoxDataArray
    {
        get => [.. OperBoxHaveList.Select(op => new OperBoxData.OperData {
            Id = op.Id,
            Name = op.Name,
            Rarity = op.Rarity,
            Elite = op.Elite,
            Level = op.Level,
            Potential = op.Potential,
            Own = true,
        })];
    }

    private Dictionary<string, int>? _operBoxPotential;

    public Dictionary<string, int>? OperBoxPotential
    {
        get {
            if (_operBoxPotential != null)
            {
                return _operBoxPotential;
            }

            _operBoxPotential = OperBoxHaveList.ToDictionary(oper => oper.Id, oper => oper.Potential);
            return _operBoxPotential;
        }
    }

    public class Operator(string id, string name, int rarity, int elite = 0, int level = 0, int potential = 0)
    {
        [JsonProperty("id")]
        public string Id { get; } = id;

        [JsonProperty("name")]
        public string Name { get; } = name;

        [JsonProperty("rarity")]
        public int Rarity { get; } = rarity;

        [JsonProperty("elite")]
        public int Elite { get; } = elite;

        [JsonProperty("level")]
        public int Level { get; } = level;

        [JsonProperty("potential")]
        public int Potential { get; } = potential;

        public int IdNumber { get; } = ExtractIdNumber(id);

        public string RarityStars => (IsPallas && Level > 0) ? LocalizationHelper.GetPallasString(6, 6) : new('★', Rarity);

        /// <summary>
        /// Gets the path to the Elite icon image
        /// </summary>
        public string EliteIconPath => $"/Res/Img/Operator/Elite_{Elite}.png";

        /// <summary>
        /// Gets the path to the Potential icon image
        /// </summary>
        public string PotentialIconPath => Potential > 0 && Potential <= 6
            ? $"/Res/Img/Operator/Potential_{Potential}.png"
            : "/Res/Img/Operator/Potential_1.png";

        /// <summary>
        /// Gets the resource key based on rarity
        /// </summary>
        public string RarityColorResourceKey => (IsPallas && Level > 0) ? "AchievementBrush.Rare.LinearGradientBrush" : $"Star{Rarity}OperatorLogBrush";

        public bool Equals(Operator? other) => other != null && Name == other.Name && Rarity == other.Rarity;

        public override bool Equals(object? obj) => obj is Operator other && Equals(other);

        public override int GetHashCode() => HashCode.Combine(Id, Name, Rarity);

        public override string ToString() => $"{Name} (★{Rarity})";

        public bool IsPallas => Id == "char_485_pallas";

        private static int ExtractIdNumber(string id)
        {
            // Expected format: "char_002_amiya"
            if (string.IsNullOrEmpty(id))
            {
                return 0;
            }

            int firstUnderscore = id.IndexOf('_');
            if (firstUnderscore < 0 || firstUnderscore >= id.Length - 1)
            {
                return 0;
            }

            int secondUnderscore = id.IndexOf('_', firstUnderscore + 1);
            if (secondUnderscore < 0 || secondUnderscore <= firstUnderscore + 1)
            {
                return 0;
            }

            ReadOnlySpan<char> numericSpan = id.AsSpan(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
            if (int.TryParse(numericSpan, out int value))
            {
                return value;
            }

            return 0;
        }
    }

    private const int OperBoxRowSize = 5;

    private ObservableCollection<Operator> _operBoxHaveList = [];

    public ObservableCollection<Operator> OperBoxHaveList
    {
        get => _operBoxHaveList;
        set {
            if (ReferenceEquals(_operBoxHaveList, value))
            {
                RefreshOperBoxHaveRows();
                return;
            }

            _operBoxHaveList.CollectionChanged -= OperBoxHaveListCollectionChanged;
            SetAndNotify(ref _operBoxHaveList, value);
            _operBoxHaveList.CollectionChanged += OperBoxHaveListCollectionChanged;
            RefreshOperBoxHaveRows();
        }
    }

    private ObservableCollection<ObservableCollection<Operator>> _operBoxHaveRows = [];

    public ObservableCollection<ObservableCollection<Operator>> OperBoxHaveRows
    {
        get => _operBoxHaveRows;
        private set => SetAndNotify(ref _operBoxHaveRows, value);
    }

    public int OperBoxHaveColumnCount => GetColumnCount(OperBoxHaveList.Count, OperBoxRowSize);

    private ObservableCollection<Operator> _operBoxNotHaveList = [];

    public ObservableCollection<Operator> OperBoxNotHaveList
    {
        get => _operBoxNotHaveList;
        set {
            if (ReferenceEquals(_operBoxNotHaveList, value))
            {
                RefreshOperBoxNotHaveRows();
                return;
            }

            _operBoxNotHaveList.CollectionChanged -= OperBoxNotHaveListCollectionChanged;
            SetAndNotify(ref _operBoxNotHaveList, value);
            _operBoxNotHaveList.CollectionChanged += OperBoxNotHaveListCollectionChanged;
            RefreshOperBoxNotHaveRows();
        }
    }

    private ObservableCollection<ObservableCollection<Operator>> _operBoxNotHaveRows = [];

    public ObservableCollection<ObservableCollection<Operator>> OperBoxNotHaveRows
    {
        get => _operBoxNotHaveRows;
        private set => SetAndNotify(ref _operBoxNotHaveRows, value);
    }

    public int OperBoxNotHaveColumnCount => GetColumnCount(OperBoxNotHaveList.Count, OperBoxRowSize);

    private void InitializeOperBoxRowPresentation()
    {
        _operBoxHaveList.CollectionChanged += OperBoxHaveListCollectionChanged;
        _operBoxNotHaveList.CollectionChanged += OperBoxNotHaveListCollectionChanged;
        RefreshOperBoxHaveRows();
        RefreshOperBoxNotHaveRows();
    }

    private void OperBoxHaveListCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        RefreshOperBoxHaveRows();
    }

    private void OperBoxNotHaveListCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        RefreshOperBoxNotHaveRows();
    }

    private void RefreshOperBoxHaveRows()
    {
        OperBoxHaveRows = BuildRows(OperBoxHaveList, OperBoxRowSize);
        NotifyOfPropertyChange(nameof(OperBoxHaveColumnCount));
    }

    private void RefreshOperBoxNotHaveRows()
    {
        OperBoxNotHaveRows = BuildRows(OperBoxNotHaveList, OperBoxRowSize);
        NotifyOfPropertyChange(nameof(OperBoxNotHaveColumnCount));
    }

    private static ObservableCollection<ObservableCollection<T>> BuildRows<T>(IEnumerable<T> items, int rowSize)
    {
        ObservableCollection<ObservableCollection<T>> rows = [];

        foreach (var row in items.Chunk(rowSize))
        {
            rows.Add(new ObservableCollection<T>(row));
        }

        return rows;
    }

    private static int GetColumnCount(int count, int rowSize)
    {
        return count <= 0 ? 1 : Math.Min(count, rowSize);
    }

    private void SaveOperBoxDetails(List<OperBoxData.OperData> details)
    {
        var data = new JObject {
            ["done"] = true,
            ["own_opers"] = JArray.FromObject(details),
        };

        if (LastOperBoxSyncTime.HasValue)
        {
            data["syncTime"] = LastOperBoxSyncTime.Value.ToLocalTime().ToString("o");
        }

        JsonDataHelper.Set(JsonDataKey.OperBoxData, data);
    }

    private void SortOperBoxLists()
    {
        OperBoxHaveList = SortOperBoxList(OperBoxHaveList);
        OperBoxNotHaveList = SortOperBoxList(OperBoxNotHaveList);
    }

    private static ObservableCollection<Operator> SortOperBoxList(ObservableCollection<Operator> list)
    {
        if (list == null || list.Count <= 0)
        {
            return list ?? [];
        }

        return [.. list
            .OrderByDescending(x => x.IsPallas && x.Level > 0)
            .ThenByDescending(x => x.Rarity)
            .ThenByDescending(x => x.Elite)
            .ThenByDescending(x => x.Level)
            .ThenByDescending(x => x.Potential)
            .ThenByDescending(x => x.IdNumber)];
    }

    private void LoadOperBoxDetails()
    {
        // TODO: 删除老数据节省 gui.json 的大小，后续版本可以删除
        // var json = ConfigurationHelper.GetValue(ConfigurationKeys.OperBoxData, string.Empty);
        var json = JsonDataHelper.Get(JsonDataKey.OperBoxData, string.Empty);
        if (string.IsNullOrWhiteSpace(json))
        {
            return;
        }

        try
        {
            var token = JToken.Parse(json);
            if (token is JArray oldArray)
            {
                var ownOpers = oldArray
                    .ToObject<List<OperBoxData.OperData>>()?
                    .Where(i => !string.IsNullOrEmpty(i.Id))
                    .ToList();
                if (ownOpers is null)
                {
                    return;
                }
                LoadOperBoxList(ownOpers);
                return;
            }
            else if (token is JObject details)
            {
                OperBoxParse(details, updateSyncTime: false);
            }
        }
        catch
        {
            // 兼容老数据或异常时忽略
        }

        void LoadOperBoxList(List<OperBoxData.OperData> ownOpers)
        {
            var operDataMap = ownOpers.ToDictionary(o => o.Id);
            foreach (var (id, oper) in DataHelper.Operators)
            {
                if (!DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType.ToCustomString()))
                {
                    continue;
                }

                var name = DataHelper.GetLocalizedCharacterName(oper) ?? "???";
                if (operDataMap.TryGetValue(id, out var operData))
                {
                    OperBoxHaveList.Add(new Operator(id, name, oper.Rarity, operData.Elite, operData.Level, operData.Potential));

                    if (id == "char_485_pallas")
                    {
                        AchievementTrackerHelper.Instance.Unlock(AchievementIds.WarehouseKeeper);
                    }
                }
                else
                {
                    OperBoxNotHaveList.Add(new Operator(id, name, oper.Rarity));
                }
            }

            SortOperBoxLists();
        }
    }

    /// <summary>
    /// 每次传进来的都是完整数据, 临时缓存去重
    /// </summary>
    private HashSet<string> _tempOperHaveSet = [];
    private readonly HashSet<int> _pendingOperBoxRecognitionResetTaskIds = [];

    public void MarkOperBoxRecognitionDataForReset(int taskId)
    {
        if (taskId > 0)
        {
            _pendingOperBoxRecognitionResetTaskIds.Add(taskId);
        }
    }

    private void ClearOperBoxRecognitionData()
    {
        OperBoxSelectedIndex = 1;
        _operBoxPotential = null;
        _tempOperHaveSet = [];
        OperBoxHaveList = [];
        OperBoxNotHaveList = [];
        LastOperBoxSyncTime = null;
    }

    /// <summary>
    /// 解析干员识别结果
    /// </summary>
    /// <param name="details">新增的干员数据</param>
    /// <param name="updateSyncTime">是否更新同步时间（从 Core 获取新数据时为 true，从本地加载时为 false）</param>
    /// <param name="taskId">传入对应的任务 ID 以便在收到回调后重置识别状态</param>
    /// <returns>是否成功</returns>
    public bool OperBoxParse(JObject? details, bool updateSyncTime = true, int taskId = 0)
    {
        if (details == null)
        {
            return false;
        }

        if (_pendingOperBoxRecognitionResetTaskIds.Remove(taskId))
        {
            ResetOperBoxRecognitionState();
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
                OperBoxHaveList.Add(new Operator(oper.Id, name, oper.Rarity, oper.Elite, oper.Level, oper.Potential));
                if (oper.Id == "char_485_pallas")
                {
                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.WarehouseKeeper);
                }
            }
        }

        bool done = (bool)(details["done"] ?? false);
        if (!done)
        {
            return true;
        }

        foreach (var (id, oper) in DataHelper.Operators)
        {
            if (!_tempOperHaveSet.Contains(id) && DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType.ToCustomString()))
            {
                var name = DataHelper.GetLocalizedCharacterName(oper) ?? "???";
                OperBoxNotHaveList.Add(new Operator(id, name, oper.Rarity));
            }
        }

        SortOperBoxLists();

        if (OperBoxNotHaveList.Count > 0)
        {
            OperBoxSelectedIndex = 0;
        }

        if (updateSyncTime)
        {
            AchievementTrackerHelper.Instance.CheckResyncAfterDays(LastOperBoxSyncTime?.UtcDateTime, 7, AchievementIds.ResumeRecord);
            LastOperBoxSyncTime = DateTimeOffset.UtcNow;
        }
        else
        {
            var syncTimeStr = details["syncTime"]?.ToString(Formatting.None)?.Trim('"');
            if (!string.IsNullOrEmpty(syncTimeStr) && DateTimeOffset.TryParse(syncTimeStr, null, DateTimeStyles.AssumeUniversal, out var lastOperBoxSyncTime))
            {
                LastOperBoxSyncTime = lastOperBoxSyncTime;
            }
        }

        OperBoxInfo = $"{LocalizationHelper.GetString("IdentificationCompleted")}  {LocalizationHelper.GetString("OperBoxRecognitionTip")}";
        SaveOperBoxDetails(ownOpers);
        _tempOperHaveSet = [];
        return true;
    }

    /// <summary>
    /// 重置干员识别状态。
    /// </summary>
    public void ResetOperBoxRecognitionState()
    {
        ClearOperBoxRecognitionData();
        LastOperBoxSyncTime = null;
    }

    /// <summary>
    /// 追加或启动干员识别任务。
    /// </summary>
    /// <param name="startImmediately">是否立刻启动。</param>
    /// <returns>是否成功。</returns>
    public bool StartOperBoxRecognitionTask(bool startImmediately = true)
    {
        bool ret = Instances.AsstProxy.AsstStartOperBox(startImmediately);
        if (ret && startImmediately)
        {
            OperBoxInfo = LocalizationHelper.GetString("Identifying");
        }

        return ret;
    }

    /// <summary>
    /// 开始识别干员
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task StartOperBox()
    {
        ResetOperBoxRecognitionState();
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

        StartOperBoxRecognitionTask();
    }

    public List<GenericCombinedData<OperBoxExportFormat>> OperBoxExportOptionList { get; } = [
        new(LocalizationHelper.GetString("OperBoxExportToClipboard"), OperBoxExportFormat.Clipboard),
        new(LocalizationHelper.GetString("OperBoxExportToJson"), OperBoxExportFormat.Json),
        new(LocalizationHelper.GetString("ExportToMarkdown"), OperBoxExportFormat.Markdown),
        new(LocalizationHelper.GetString("ExportToCsv"), OperBoxExportFormat.Csv),
    ];

    public OperBoxExportFormat SelectedOperBoxExportValue
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.OperBoxExportFormat = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.OperBoxExportFormat;

    // UI 绑定的方法
    [UsedImplicitly]
    public void ExportOperBox()
    {
        switch (SelectedOperBoxExportValue)
        {
            case OperBoxExportFormat.Clipboard: ExportOperBoxToClipboard(); break;
            case OperBoxExportFormat.Json: ExportOperBoxToJson(); break;
            case OperBoxExportFormat.Markdown: ExportOperBoxToMarkdown(); break;
            case OperBoxExportFormat.Csv: ExportOperBoxToCsv(); break;
        }
    }

    private List<OperBoxData.OperData> BuildOperBoxExportList()
    {
        if (OperBoxHaveList.Count == 0)
        {
            return [];
        }

        var exportList = new List<OperBoxData.OperData>();
        var userOperMap = OperBoxHaveList.ToDictionary(op => op.Id);

        foreach (var (operId, operInfo) in DataHelper.Operators)
        {
            if (!DataHelper.IsCharacterAvailableInClient(operInfo, SettingsViewModel.GameSettings.ClientType.ToCustomString()))
            {
                continue;
            }

            var operName = DataHelper.GetLocalizedCharacterName(operInfo) ?? "???";
            if (userOperMap.TryGetValue(operId, out var value))
            {
                exportList.Add(new OperBoxData.OperData() {
                    Id = value.Id,
                    Name = value.Name,
                    Rarity = value.Rarity,
                    Elite = value.Elite,
                    Level = value.Level,
                    Potential = value.Potential,
                    Own = true,
                });
            }
            else
            {
                exportList.Add(new OperBoxData.OperData() {
                    Id = operId,
                    Name = operName,
                    Rarity = operInfo.Rarity,
                    Own = false,
                });
            }
        }

        return exportList;
    }

    private void ExportOperBoxToClipboard()
    {
        var exportList = BuildOperBoxExportList();
        if (exportList.Count == 0)
        {
            return;
        }

        Clipboard.Clear();
        Clipboard.SetDataObject(JsonConvert.SerializeObject(exportList, Formatting.Indented));
        Growl.Info(LocalizationHelper.GetString("CopiedToClipboard"));
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.OperatorRoster);
    }

    private void ExportOperBoxToFile(Func<IReadOnlyList<OperBoxData.OperData>, string> contentBuilder, string filter, string defaultExt, string defaultFileName)
    {
        var exportList = BuildOperBoxExportList();
        if (exportList.Count == 0)
        {
            return;
        }

        var content = contentBuilder(exportList);

        var dialog = new Microsoft.Win32.SaveFileDialog {
            Filter = filter,
            DefaultExt = defaultExt,
            FileName = defaultFileName,
        };

        if (dialog.ShowDialog() != true)
        {
            return;
        }

        File.WriteAllText(dialog.FileName, content, new UTF8Encoding(true));
        Growl.Info(LocalizationHelper.GetString("ExportedToFile"));
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.OperatorRoster);
    }

    private void ExportOperBoxToJson()
    {
        ExportOperBoxToFile(
            list => JsonConvert.SerializeObject(list, Formatting.Indented),
            "JSON files (*.json)|*.json|All files (*.*)|*.*",
            ".json",
            "Arknights_OperBox_Export.json");
    }

    private void ExportOperBoxToMarkdown()
    {
        ExportOperBoxToFile(
            list => string.Join(Environment.NewLine, BuildOperBoxMarkdownExportLines(list)),
            "Markdown files (*.md)|*.md|All files (*.*)|*.*",
            ".md",
            "Arknights_OperBox_Export.md");
    }

    private void ExportOperBoxToCsv()
    {
        ExportOperBoxToFile(
            list => string.Join(Environment.NewLine, BuildOperBoxCsvExportLines(list)),
            "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
            ".csv",
            "Arknights_OperBox_Export.csv");
    }

    private static IEnumerable<string> BuildOperBoxMarkdownExportLines(IReadOnlyList<OperBoxData.OperData> items)
    {
        var nameHeader = LocalizationHelper.GetString("OperBoxExportHeaderName");
        var idHeader = LocalizationHelper.GetString("OperBoxExportHeaderId");
        var rarityHeader = LocalizationHelper.GetString("OperBoxExportHeaderRarity");
        var eliteHeader = LocalizationHelper.GetString("OperBoxExportHeaderElite");
        var levelHeader = LocalizationHelper.GetString("OperBoxExportHeaderLevel");
        var ownHeader = LocalizationHelper.GetString("OperBoxExportHeaderOwn");
        var potentialHeader = LocalizationHelper.GetString("OperBoxExportHeaderPotential");
        var yes = LocalizationHelper.GetString("OperBoxExportYes");
        var no = LocalizationHelper.GetString("OperBoxExportNo");

        yield return $"| {nameHeader} | {idHeader} | {rarityHeader} | {eliteHeader} | {levelHeader} | {ownHeader} | {potentialHeader} |";
        yield return "| :-- | :-- | :-- | :-- | :-- | :-- | :-- |";
        foreach (var item in items)
        {
            yield return $"| {item.Name} | {item.Id} | {item.Rarity} | {item.Elite} | {item.Level} | {(item.Own ? yes : no)} | {item.Potential} |";
        }
    }

    private static IEnumerable<string> BuildOperBoxCsvExportLines(IReadOnlyList<OperBoxData.OperData> items)
    {
        var nameHeader = LocalizationHelper.GetString("OperBoxExportHeaderName");
        var idHeader = LocalizationHelper.GetString("OperBoxExportHeaderId");
        var rarityHeader = LocalizationHelper.GetString("OperBoxExportHeaderRarity");
        var eliteHeader = LocalizationHelper.GetString("OperBoxExportHeaderElite");
        var levelHeader = LocalizationHelper.GetString("OperBoxExportHeaderLevel");
        var ownHeader = LocalizationHelper.GetString("OperBoxExportHeaderOwn");
        var potentialHeader = LocalizationHelper.GetString("OperBoxExportHeaderPotential");
        var yes = LocalizationHelper.GetString("OperBoxExportYes");
        var no = LocalizationHelper.GetString("OperBoxExportNo");

        yield return $"{nameHeader},{idHeader},{rarityHeader},{eliteHeader},{levelHeader},{ownHeader},{potentialHeader}";
        foreach (var item in items)
        {
            var name = item.Name ?? string.Empty;
            if (name.Contains(',') || name.Contains('"') || name.Contains('\n'))
            {
                name = "\"" + name.Replace("\"", "\"\"") + "\"";
            }

            yield return $"{name},{item.Id},{item.Rarity},{item.Elite},{item.Level},{(item.Own ? yes : no)},{item.Potential}";
        }
    }

    #endregion OperBox

    #region Gacha

    private string _gachaInfo = LocalizationHelper.GetString("GachaInitTip");

    public string GachaInfo
    {
        get => _gachaInfo;
        set => SetAndNotify(ref _gachaInfo, value);
    }

    // UI 绑定的方法
    public async Task GachaOnce()
    {
        await StartGacha();
    }

    // UI 绑定的方法
    public async Task GachaTenTimes()
    {
        await StartGacha(false);
    }

    private bool _isGachaInProgress;

    public bool IsGachaInProgress
    {
        get => _isGachaInProgress;
        set {
            if (!SetAndNotify(ref _isGachaInProgress, value))
            {
                return;
            }

            if (!value)
            {
                _gachaTimer.Stop();
                GachaInfo = LocalizationHelper.GetString("GachaInitTip");
            }
        }
    }

    public async Task StartGacha(bool once = true)
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
        _ = Peep();
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
    private bool _gachaShowDisclaimer = true; // !ConfigurationHelper.GetValue(ConfigurationKeys.ShowDisclaimerNoMore, false);

    public bool GachaShowDisclaimer
    {
        get => _gachaShowDisclaimer;
        set {
            SetAndNotify(ref _gachaShowDisclaimer, value);
        }
    }

    public bool GachaShowDisclaimerNoMore
    {
        get => ConfigFactory.CurrentConfig.Toolbox.GachaShowDisclaimerNoMore;
        set {
            ConfigFactory.CurrentConfig.Toolbox.GachaShowDisclaimerNoMore = value;
            NotifyOfPropertyChange();
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
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
        set {
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
        set {
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

    private WriteableBitmap? _peepImage;

    public WriteableBitmap? PeepImage
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

    public int PeepTargetFps
    {
        get; set {
            value = value switch {
                > 600 => 600,
                < 1 => 1,
                _ => value,
            };

            SetAndNotify(ref field, value);
            _peepImageTimer.Interval = 1000d / field;
            ConfigFactory.CurrentConfig.Toolbox.PeepTargetFps = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.PeepTargetFps;

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

    private async void PeepImageTimerElapsed(object? sender, EventArgs? e)
    {
        try
        {
            await RefreshPeepImageAsync();
        }
        catch
        {
            // ignored
        }
    }

    private readonly WriteableBitmap?[] _peepImageCache = new WriteableBitmap?[PeepImageSemaphoreMaxCount];

    private async Task RefreshPeepImageAsync()
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
                _logger.Information("Screenshot Semaphore Full, increase semaphore count to {PeepImageSemaphoreCurrentCount}", _peepImageSemaphoreCurrentCount);
                return;
            }

            _logger.Warning("Screenshot Semaphore Full, Reduce Target FPS count to {PeepTargetFps}", --PeepTargetFps);
            _ = Execute.OnUIThreadAsync(() => {
                Growl.Clear();
                Growl.Warning($"Screenshot taking too long, reduce Target FPS to {PeepTargetFps}");
            });
            return;
        }

        try
        {
            var count = Interlocked.Increment(ref _peepImageCount);
            var index = count % _peepImageCache.Length;
            var frameData = await Instances.AsstProxy.AsstGetImageBgrDataAsync(forceScreencap: true);
            if (frameData is null || frameData.Length == 0)
            {
                _logger.Warning("Peep image data is null or empty.");
                return;
            }

            // 若不满足条件，提前释放 frameData 避免内存泄露
            if (!Peeping || count <= _peepImageNewestCount)
            {
                _logger.Debug("Peep image count {Count} is not the newest, skip updating image.", count);
                ArrayPool<byte>.Shared.Return(frameData);
                return;
            }

            await Execute.OnUIThreadAsync(() => {
                _peepImageCache[index] = AsstProxy.WriteBgrToBitmap(frameData, _peepImageCache[index]);
            });

            PeepImage = _peepImageCache[index];
            ArrayPool<byte>.Shared.Return(frameData);
            Interlocked.Exchange(ref _peepImageNewestCount, count);

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
    /// <returns>Task</returns>
    public async Task Peep()
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
                Array.Fill(_peepImageCache, null);

                // 由 Peep() 方法启动的 Peep 也需要停止，Block 不会自动停止
                if (IsGachaInProgress || IsPeepInProgress)
                {
                    await Instances.TaskQueueViewModel.Stop();
                    Instances.TaskQueueViewModel.SetStopped();
                }

                IsPeepInProgress = false;
                IsGachaInProgress = false;
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

    public class MiniGameCategoryItem : PropertyChangedBase
    {
        public string Display { get; set; } = string.Empty;

        public string Value { get; set; } = string.Empty;

        public string Category { get; set; } = string.Empty;

        public bool IsSecretFront => Value == "MiniGame@SecretFront";

        public bool IsPixelPaint => Value is "MiniGame@PixelPaint" or "MiniGame@PixelPaint@Begin";
    }

    public ObservableCollection<MiniGameCategoryItem> MiniGameCategoryItems { get; } = [];

    private MiniGameCategoryItem? _selectedMiniGameItem;

    public MiniGameCategoryItem? SelectedMiniGameItem
    {
        get => _selectedMiniGameItem;
        set {
            if (!SetAndNotify(ref _selectedMiniGameItem, value) || value == null)
            {
                return;
            }

            MiniGameTaskName = value.Value;
        }
    }

    public bool IsPixelPaintSelected => SelectedMiniGameItem?.IsPixelPaint == true;

    public static void UpdateMiniGameTaskList()
    {
        var categorizedItems = Instances.StageManager.MiniGameEntries
            .Select(t => {
                var isCurrentEvent = t.UtcStartTime != DateTime.MinValue || t.UtcExpireTime != DateTime.MinValue;
                var category = LocalizationHelper.GetString(isCurrentEvent
                    ? "MiniGameCategoryCurrentEvent"
                    : "MiniGameCategoryPermanent");
                return new MiniGameCategoryItem {
                    Display = string.IsNullOrEmpty(t.DisplayKey)
                        ? t.Display
                        : (LocalizationHelper.TryGetString(t.DisplayKey, out var loc) ? loc : t.Display),
                    Value = t.Value,
                    Category = category,
                };
            })
            .ToList();

        Execute.OnUIThread(() => {
            var toolbox = Instances.ToolboxViewModel;
            if (toolbox == null)
            {
                return;
            }

            var prevSelected = toolbox.SelectedMiniGameItem?.Value;

            toolbox.MiniGameCategoryItems.Clear();
            foreach (var item in categorizedItems)
            {
                toolbox.MiniGameCategoryItems.Add(item);
            }

            toolbox.SelectedMiniGameItem = toolbox.MiniGameCategoryItems
                .FirstOrDefault(i => i.Value == prevSelected)
                ?? toolbox.MiniGameCategoryItems.FirstOrDefault();
        });
    }

    public string MiniGameTaskName
    {
        get; set {
            SetAndNotify(ref field, value);
            MiniGameTip = GetMiniGameTip(value);
        }
    } = "SS@Store@Begin";

    public string GetMiniGameTask()
    {
        return MiniGameTaskName switch {
            "MiniGame@SecretFront" => $"{MiniGameTaskName}@Begin@Ending{SecretFrontEnding}{(string.IsNullOrEmpty(SecretFrontEvent) ? string.Empty : $"@{SecretFrontEvent}")}",
            _ => MiniGameTaskName,
        };
    }

    private string? _miniGameTip;

    public string MiniGameTip
    {
        get {
            _miniGameTip ??= GetMiniGameTip(MiniGameTaskName);
            return _miniGameTip;
        }
        set => SetAndNotify(ref _miniGameTip, value);
    }

    private static string GetMiniGameTip(string name)
    {
        if (string.IsNullOrEmpty(name))
        {
            return LocalizationHelper.GetString("MiniGameNameEmptyTip");
        }

        var entry = Instances.StageManager.MiniGameEntries.FirstOrDefault(e => e.Value == name);
        if (entry == null)
        {
            return LocalizationHelper.GetString("MiniGameNameEmptyTip");
        }

        // 优先使用 TipKey 的本地化
        if (!string.IsNullOrEmpty(entry.TipKey) && LocalizationHelper.TryGetString(entry.TipKey, out var tipFromKey))
        {
            return tipFromKey;
        }

        // 然后使用 API Tip
        if (!string.IsNullOrEmpty(entry.Tip))
        {
            return entry.Tip;
        }

        // 若不存在 Tip，再尝试使用 DisplayKey + "Tip" 的约定键
        if (!string.IsNullOrEmpty(entry.DisplayKey))
        {
            var displayTipKey = entry.DisplayKey + "Tip";
            if (LocalizationHelper.TryGetString(displayTipKey, out var displayTip))
            {
                return displayTip;
            }

            // 最后回退为 Display 的本地化或原始 Display
            if (LocalizationHelper.TryGetString(entry.DisplayKey, out var displayLoc))
            {
                return displayLoc;
            }
        }

        if (!string.IsNullOrEmpty(entry.Display))
        {
            return entry.Display;
        }

        return string.Empty;
    }

    public List<string> SecretFrontEndingList { get; set; } = ["A", "B", "C", "D", "E"];

    public string SecretFrontEnding { get; set => SetAndNotify(ref field, value); } = "A";

    public List<GenericCombinedData<string>> SecretFrontEventList { get; set; } =
    [
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event1"), Value = "支援作战平台" },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event2"), Value = "游侠" },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event3"), Value = "诡影迷踪" },
    ];

    public string SecretFrontEvent { get; set => SetAndNotify(ref field, value); } = string.Empty;

    #region PixelPaint

    private BitmapSource? _pixelPaintSourceImage;

    private BitmapSource? _pixelPaintPreview;

    public BitmapSource? PixelPaintPreview
    {
        get => _pixelPaintPreview;
        private set => SetAndNotify(ref _pixelPaintPreview, value);
    }

    private string _pixelPaintStatusText = string.Empty;

    public string PixelPaintStatusText
    {
        get => _pixelPaintStatusText;
        private set => SetAndNotify(ref _pixelPaintStatusText, value);
    }

    private PixelPaintHelper.PreparedImage? _pixelPaintPrepared;

    private PixelPaintHelper.ConvertResult? _pixelPaintResult;

    private bool _pixelPaintParametersLocked;

    public bool PixelPaintParametersLocked
    {
        get => _pixelPaintParametersLocked;
        private set => SetAndNotify(ref _pixelPaintParametersLocked, value);
    }

    /// <summary>相对去边后内容图的归一化取景（0~1）。</summary>
    private System.Windows.Rect _pixelPaintView = new(0, 0, 1, 1);

    private System.Windows.Point? _pixelPaintDragStart;

    private System.Windows.Rect _pixelPaintDragOriginView;

    public List<GenericCombinedData<string>> PixelPaintFitModeList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@FitCrop"), Value = "Crop" },
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@FitContain"), Value = "Contain" },
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@FitStretch"), Value = "Stretch" },
    ];

    public string PixelPaintFitMode
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    } = "Crop";

    public List<GenericCombinedData<string>> PixelPaintDitherModeList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@DitherNone"), Value = "None" },
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@DitherFS"), Value = "FloydSteinberg" },
        new() { Display = LocalizationHelper.GetString("MiniGame@PixelPaint@DitherAtkinson"), Value = "Atkinson" },
    ];

    public string PixelPaintDitherMode
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    } = "FloydSteinberg";

    public double PixelPaintContrast
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    } = 100;

    public double PixelPaintBrightness
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    } = 100;

    public double PixelPaintSaturation
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    } = 100;

    public bool PixelPaintPaintWhite
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                ReconvertPixelPaint();
            }
        }
    }

    /// <summary>拖动绘制开关：同色同行连续格一次画完（更快，部分触控模式可能丢点）。</summary>
    public bool PixelPaintSwipeEnabled { get; set; } = true;

    /// <summary>逐格点击的额外等待（ms），默认 0（各触控方式自带基础间隔）。</summary>
    public int PixelPaintGridClickDelay { get; set; } = 0;

    public void PixelPaintPickImage()
    {
        if (PixelPaintParametersLocked)
        {
            return;
        }

        var dialog = new Microsoft.Win32.OpenFileDialog {
            Filter = "Image|*.png;*.jpg;*.jpeg;*.bmp;*.webp;*.gif|All|*.*",
            CheckFileExists = true,
            Multiselect = false,
        };
        if (dialog.ShowDialog() != true)
        {
            return;
        }

        LoadPixelPaintImage(dialog.FileName);
    }

    public void PixelPaintDrop(object sender, DragEventArgs e)
    {
        if (PixelPaintParametersLocked || e.Data == null)
        {
            return;
        }

        if (!e.Data.GetDataPresent(DataFormats.FileDrop))
        {
            return;
        }

        if (e.Data.GetData(DataFormats.FileDrop) is not string[] files || files.Length == 0)
        {
            return;
        }

        LoadPixelPaintImage(files[0]);
    }

    public void PixelPaintDragOver(object sender, DragEventArgs e)
    {
        e.Effects = (!PixelPaintParametersLocked && e.Data?.GetDataPresent(DataFormats.FileDrop) == true)
            ? DragDropEffects.Copy
            : DragDropEffects.None;
        e.Handled = true;
    }

    public void PixelPaintPreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (PixelPaintParametersLocked || _pixelPaintSourceImage == null)
        {
            return;
        }

        // 滚轮缩放取景：向上放大（缩小 view），向下缩小（放大 view）
        var factor = e.Delta > 0 ? 0.9 : 1.0 / 0.9;
        var cx = _pixelPaintView.X + (_pixelPaintView.Width / 2);
        var cy = _pixelPaintView.Y + (_pixelPaintView.Height / 2);
        var nw = Math.Clamp(_pixelPaintView.Width * factor, 0.05, 1.0);
        var nh = Math.Clamp(_pixelPaintView.Height * factor, 0.05, 1.0);
        var nx = Math.Clamp(cx - (nw / 2), 0, 1 - nw);
        var ny = Math.Clamp(cy - (nh / 2), 0, 1 - nh);
        _pixelPaintView = new System.Windows.Rect(nx, ny, nw, nh);
        ReconvertPixelPaint();
        e.Handled = true;
    }

    public void PixelPaintPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (PixelPaintParametersLocked || _pixelPaintSourceImage == null)
        {
            return;
        }

        if (sender is not IInputElement el)
        {
            return;
        }

        _pixelPaintDragStart = e.GetPosition(el);
        _pixelPaintDragOriginView = _pixelPaintView;
        el.CaptureMouse();
        e.Handled = true;
    }

    public void PixelPaintPreviewMouseMove(object sender, MouseEventArgs e)
    {
        if (_pixelPaintDragStart is null || PixelPaintParametersLocked)
        {
            return;
        }

        if (sender is not FrameworkElement el)
        {
            return;
        }

        var pos = e.GetPosition(el);
        var dx = (pos.X - _pixelPaintDragStart.Value.X) / Math.Max(1.0, el.ActualWidth);
        var dy = (pos.Y - _pixelPaintDragStart.Value.Y) / Math.Max(1.0, el.ActualHeight);

        // 拖图像：鼠标右移时内容左移（view.X 减小）
        var nx = Math.Clamp(_pixelPaintDragOriginView.X - (dx * _pixelPaintDragOriginView.Width), 0, 1 - _pixelPaintDragOriginView.Width);
        var ny = Math.Clamp(_pixelPaintDragOriginView.Y - (dy * _pixelPaintDragOriginView.Height), 0, 1 - _pixelPaintDragOriginView.Height);
        _pixelPaintView = new System.Windows.Rect(nx, ny, _pixelPaintDragOriginView.Width, _pixelPaintDragOriginView.Height);
        ReconvertPixelPaint();
        e.Handled = true;
    }

    public void PixelPaintPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (sender is IInputElement el && el.IsMouseCaptured)
        {
            el.ReleaseMouseCapture();
        }

        _pixelPaintDragStart = null;
        e.Handled = true;
    }

    public void PixelPaintResetView()
    {
        if (PixelPaintParametersLocked)
        {
            return;
        }

        _pixelPaintView = new System.Windows.Rect(0, 0, 1, 1);
        ReconvertPixelPaint();
    }

    public void PixelPaintResetParameters()
    {
        if (PixelPaintParametersLocked)
        {
            return;
        }

        _pixelPaintView = new System.Windows.Rect(0, 0, 1, 1);
        PixelPaintFitMode = "Crop";
        PixelPaintDitherMode = "FloydSteinberg";
        PixelPaintContrast = 100;
        PixelPaintBrightness = 100;
        PixelPaintSaturation = 100;
        PixelPaintPaintWhite = false;
        ReconvertPixelPaint();
    }

    private void LoadPixelPaintImage(string path)
    {
        try
        {
            var bmp = new BitmapImage();
            bmp.BeginInit();
            bmp.CacheOption = BitmapCacheOption.OnLoad;
            bmp.UriSource = new Uri(path);
            bmp.EndInit();
            bmp.Freeze();

            _pixelPaintSourceImage = bmp;
            _pixelPaintPrepared = PixelPaintHelper.Prepare(bmp, trimEmptyBorder: true);
            _pixelPaintView = new System.Windows.Rect(0, 0, 1, 1);
            ReconvertPixelPaint();
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Load pixel paint image failed: {Path}", path);
            PixelPaintStatusText = LocalizationHelper.GetString("MiniGame@PixelPaint@LoadFailed");
        }
    }

    private void ReconvertPixelPaint()
    {
        if (PixelPaintParametersLocked || _pixelPaintSourceImage == null)
        {
            return;
        }

        try
        {
            var fit = PixelPaintFitMode switch {
                "Contain" => PixelPaintHelper.FitMode.Contain,
                "Stretch" => PixelPaintHelper.FitMode.Stretch,
                _ => PixelPaintHelper.FitMode.Crop,
            };
            var dither = PixelPaintDitherMode switch {
                "None" => PixelPaintHelper.DitherMode.None,
                "Atkinson" => PixelPaintHelper.DitherMode.Atkinson,
                _ => PixelPaintHelper.DitherMode.FloydSteinberg,
            };

            var options = new PixelPaintHelper.ConvertOptions {
                Fit = fit,
                Dither = dither,
                ContrastPercent = PixelPaintContrast,
                BrightnessPercent = PixelPaintBrightness,
                SaturationPercent = PixelPaintSaturation,
                ContentViewNormalized = _pixelPaintView,
                TrimEmptyBorder = true,
            };

            if (_pixelPaintPrepared == null)
            {
                return;
            }

            var result = PixelPaintHelper.Convert(_pixelPaintPrepared, options, skipWhite: !PixelPaintPaintWhite);
            _pixelPaintResult = result;
            PixelPaintPreview = result.Preview;
            PixelPaintStatusText = string.Format(
                LocalizationHelper.GetString("MiniGame@PixelPaint@ReadyStatus"),
                result.PaintedCellCount,
                result.Groups.Count);
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Pixel paint convert failed");
            _pixelPaintResult = null;
            PixelPaintPreview = null;
            PixelPaintStatusText = LocalizationHelper.GetString("MiniGame@PixelPaint@ConvertFailed");
        }
    }

    #endregion

    public void StartMiniGame()
    {
        _ = StartMiniGameAsync();
    }

    private async Task StartMiniGameAsync()
    {
        if (!Idle)
        {
            await Instances.TaskQueueViewModel.Stop();
            return;
        }

        var isPixelPaint = IsPixelPaintSelected;
        if (isPixelPaint && (_pixelPaintResult == null || _pixelPaintResult.Groups.Count == 0))
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MiniGame@PixelPaint@NeedImage"), UiLogColor.Warning);
            return;
        }

        Instances.TaskQueueViewModel.ClearLog();

        _runningState.SetIdle(false);
        if (isPixelPaint)
        {
            PixelPaintParametersLocked = true;
        }

        string errMsg = string.Empty;
        bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (!caught)
        {
            _runningState.SetIdle(true);
            PixelPaintParametersLocked = false;
            return;
        }

        if (_runningState.GetStopping())
        {
            Instances.TaskQueueViewModel.SetStopped();
            PixelPaintParametersLocked = false;
            return;
        }

        if (isPixelPaint)
        {
            var groups = _pixelPaintResult!.Groups;
            caught = Instances.AsstProxy.AsstPixelPaint(groups, PixelPaintSwipeEnabled, PixelPaintGridClickDelay);
            if (caught)
            {
                Instances.TaskQueueViewModel.AddLog(
                    string.Format(
                        LocalizationHelper.GetString("MiniGame@PixelPaint@StartLog"),
                        groups.Sum(g => g.Points.Count),
                        groups.Count),
                    UiLogColor.Info);
            }
        }
        else
        {
            caught = Instances.AsstProxy.AsstMiniGame(GetMiniGameTask());
        }

        if (!caught)
        {
            _runningState.SetIdle(true);
            PixelPaintParametersLocked = false;
        }
        else
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.SlackingOff);
        }
    }

    #endregion
}
