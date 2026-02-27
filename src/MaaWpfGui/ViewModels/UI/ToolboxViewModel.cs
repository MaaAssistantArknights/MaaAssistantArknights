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
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using HandyControl.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using RecastConstants = MaaWpfGui.Constants.RecastConstants;
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
    /// Gets the mini game recast settings model.
    /// 小游戏重新投钱设置 model
    /// </summary>
    public static MiniGameRecastUserControlModel MiniGameRecastSettings { get; } = MiniGameRecastUserControlModel.Instance;

    /// <summary>
    /// Initializes a new instance of the <see cref="ToolboxViewModel"/> class.
    /// </summary>
    public ToolboxViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        DisplayName = LocalizationHelper.GetString("Toolbox");
        _runningState = RunningState.Instance;
        _runningState.StateChanged += (__, e) => {
            Idle = e.Idle;
            Inited = e.Inited;
            Stopping = e.Stopping;

            if (e.Stopping && Peeping && !IsPeepTransitioning)
            {
                _ = Peep();
            }
        };
        _peepImageTimer.Elapsed += PeepImageTimerElapsed;
        _peepImageTimer.Interval = 1000d / PeepTargetFps;
        _gachaTimer.Tick += RefreshGachaTip;
        LoadDepotDetails();
        LoadOperBoxDetails();
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
        get => field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel3, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3, true);

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 4.
    /// </summary>
    public bool ChooseLevel4
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel4, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4, true);

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 5.
    /// </summary>
    public bool ChooseLevel5
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel5, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5, true);

    private bool _chooseLevel6 = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel6, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 6.
    /// </summary>
    public bool ChooseLevel6
    {
        get => _chooseLevel6;
        set {
            SetAndNotify(ref _chooseLevel6, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ChooseLevel6, value.ToString());
        }
    }

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
        get => field;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ToolBoxChooseLevel3Time, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ToolBoxChooseLevel3Time, 540);

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
        get => field;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ToolBoxChooseLevel4Time, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ToolBoxChooseLevel4Time, 540);

    [PropertyDependsOn(nameof(ChooseLevel5Time))]
    public int ChooseLevel5Hour
    {
        get => ChooseLevel5Time / 60;
        set => ChooseLevel5Time = (value * 60) + ChooseLevel5Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel5Time))]
    public int ChooseLevel5Min
    {
        get => (ChooseLevel5Time % 60) / 10 * 10;
        set => ChooseLevel5Time = (ChooseLevel5Hour * 60) + value;
    }

    public int ChooseLevel5Time
    {
        get => field;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ToolBoxChooseLevel5Time, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.ToolBoxChooseLevel5Time, 540);

    private bool _autoSetTime = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoSetTime, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to set time automatically.
    /// </summary>
    public bool RecruitAutoSetTime
    {
        get => _autoSetTime;
        set {
            SetAndNotify(ref _autoSetTime, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AutoSetTime, value.ToString());
        }
    }

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
            ChooseLevel5Time = ChooseLevel5Time,
            ServerType = Instances.SettingsViewModel.ServerType,
        };
        var (type, taskParams) = task.Serialize();
        bool ret = Instances.AsstProxy.AsstAppendTaskWithEncoding(AsstProxy.TaskType.RecruitCalc, type, taskParams);
        ret &= Instances.AsstProxy.AsstStart();
    }

    private bool _recruitmentShowPotential = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RecruitmentShowPotential, bool.TrueString));

    public bool RecruitmentShowPotential
    {
        get => _recruitmentShowPotential;
        set {
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

    private DateTime? _lastDepotSyncTime;

    /// <summary>
    /// Gets or sets 上次仓库同步时间（UTC 时间）
    /// </summary>
    public DateTime? LastDepotSyncTime
    {
        get => _lastDepotSyncTime;
        set => SetAndNotify(ref _lastDepotSyncTime, value);
    }

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
            var localTime = LastDepotSyncTime.Value.ToLocalTime();
            return localTime.ToString();
        }
    }

    private ObservableCollection<DepotResultDate> _depotResult = [];

    /// <summary>
    /// Gets or sets the depot result.
    /// </summary>
    public ObservableCollection<DepotResultDate> DepotResult
    {
        get => _depotResult;
        set {
            SetAndNotify(ref _depotResult, value);
            InvalidateDepotCache();
        }
    }

    // 缓存相关字段
    private bool _depotCacheInvalid = true;
    private string? _cachedArkPlannerResult;
    private string? _cachedLoliconResult;

    /// <summary>
    /// 标记仓库缓存失效
    /// </summary>
    private void InvalidateDepotCache()
    {
        _depotCacheInvalid = true;
        _cachedArkPlannerResult = null;
        _cachedLoliconResult = null;
    }

    public class DepotResultDate
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
        public string DisplayCount => Count >= 0 ? Count.FormatNumber(false) : "-1";
    }

    /// <summary>
    /// 保存仓库详情数据
    /// </summary>
    private void SaveDepotDetails()
    {
        // 构建简化格式：{"itemId": count}
        var depotData = new JObject();
        foreach (var item in DepotResult)
        {
            if (item.Count >= 0)
            {
                depotData[item.Id] = item.Count;
            }
        }

        var details = new JObject {
            ["done"] = true,
            ["data"] = depotData.ToString(Formatting.None),
        };

        // 保存同步时间为 UTC（如果有）
        if (LastDepotSyncTime.HasValue)
        {
            details["syncTime"] = LastDepotSyncTime.Value.ToString("o"); // ISO 8601 格式
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
    /// <returns>是否成功</returns>
    public bool DepotParse(JObject? details, bool updateSyncTime = false)
    {
        if (details == null)
        {
            return false;
        }

        DepotResult.Clear();

        Dictionary<string, int> depotItems = [];

        // 尝试解析新格式
        var dataStr = details["data"]?.ToString();
        if (!string.IsNullOrEmpty(dataStr))
        {
            try
            {
                var dataObj = JObject.Parse(dataStr);
                foreach (var prop in dataObj.Properties())
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
                var arkplannerItems = details["arkplanner"]?["object"]?["items"]
                                          ?.Cast<JObject>()
                                      ?? [];

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

        // 转换为 DepotResult，按 ID 排序
        foreach (var kvp in depotItems.OrderBy(x => x.Key))
        {
            var id = kvp.Key;
            var count = kvp.Value;

            DepotResultDate result = new() {
                Id = id,
                Name = ItemListHelper.GetItemName(id),
                Image = ItemListHelper.GetItemImage(id),
                Count = count,
            };

            if (count > 0 &&
                count > AchievementTrackerHelper.Instance.GetProgress(AchievementIds.WarehouseMiser))
            {
                AchievementTrackerHelper.Instance.SetProgress(AchievementIds.WarehouseMiser, count);
            }

            DepotResult.Add(result);
        }

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
            LastDepotSyncTime = DateTime.UtcNow;
        }
        else
        {
            // 从本地加载，读取保存的时间
            var syncTimeStr = details["syncTime"]?.ToString(Formatting.None)?.Trim('"');
            if (!string.IsNullOrEmpty(syncTimeStr) &&
                DateTime.TryParseExact(syncTimeStr, "O", null, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal,
                out var lastDepotSyncTime))
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
    /// Export depot info to ArkPlanner.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToArkplanner()
    {
        System.Windows.Forms.Clipboard.Clear();
        System.Windows.Forms.Clipboard.SetDataObject(ArkPlannerResult);
        DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
    }

    /// <summary>
    /// Export depot info to Lolicon.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void ExportToLolicon()
    {
        System.Windows.Forms.Clipboard.Clear();
        System.Windows.Forms.Clipboard.SetDataObject(LoliconResult);
        DepotInfo = LocalizationHelper.GetString("CopiedToClipboard");
    }

    private void DepotClear()
    {
        DepotResult.Clear();
        InvalidateDepotCache();
    }

    // 需要排除的物品 ID（不统计到仓库）
    private static readonly HashSet<string> ExcludedItemIds =
    [
        "3401", // 家具
        "3112", "3113", "3114", // 碳
        "4001", // 龙门币
        "4003", // 合成玉
        "4006", // 红票
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
            // 按 ID 排序（与 DepotParse 保持一致）
            var sortedItems = DepotResult.OrderBy(x => x.Id).ToList();
            DepotResult.Clear();
            foreach (var item in sortedItems)
            {
                DepotResult.Add(item);
            }

            // 标记缓存失效
            InvalidateDepotCache();

            // 保存更新后的数据
            SaveDepotDetails();
            Instances.TaskQueueViewModel.UpdateDatePrompt();

            _logger.Information("Depot updated from stage drops, {Count} items processed", drops.Count);
        }
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

        DepotInfo = LocalizationHelper.GetString("Identifying");
        DepotClear();

        Instances.AsstProxy.AsstStartDepot();
    }

    #endregion Depot

    #region OperBox

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

        public string RarityStars => IsPallas ? LocalizationHelper.GetPallasString(6, 6) : new('★', Rarity);

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
        public string RarityColorResourceKey => IsPallas ? "HiddenMedalBrush" : $"Star{Rarity}OperatorLogBrush";

        public bool Equals(Operator? other) => other != null && Name == other.Name && Rarity == other.Rarity;

        public override bool Equals(object? obj) => obj is Operator other && Equals(other);

        public override int GetHashCode() => HashCode.Combine(Id, Name, Rarity);

        public override string ToString() => $"{Name} (★{Rarity})";

        private bool IsPallas => Id == "char_485_pallas";

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
            .OrderByDescending(x => x.Rarity)
            .ThenByDescending(x => x.Elite)
            .ThenByDescending(x => x.Level)
            .ThenByDescending(x => x.Potential)
            .ThenByDescending(x => x.IdNumber)];
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

            var operDataMap = ownOpers.ToDictionary(o => o.Id);
            foreach (var (id, oper) in DataHelper.Operators)
            {
                if (DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType))
                {
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
            }

            SortOperBoxLists();
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
            if (!_tempOperHaveSet.Contains(id) && DataHelper.IsCharacterAvailableInClient(oper, SettingsViewModel.GameSettings.ClientType))
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

        OperBoxInfo = $"{LocalizationHelper.GetString("IdentificationCompleted")}\n{LocalizationHelper.GetString("OperBoxRecognitionTip")}";
        SaveOperBoxDetails(ownOpers);
        _tempOperHaveSet = [];
        return true;
    }

    /// <summary>
    /// 开始识别干员
    /// UI 绑定的方法
    /// </summary>
    /// <returns>Task</returns>
    [UsedImplicitly]
    public async Task StartOperBox()
    {
        OperBoxSelectedIndex = 1;
        _operBoxPotential = null;
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

    // UI 绑定的方法
    [UsedImplicitly]
    public void ExportOperBox()
    {
        if (OperBoxHaveList.Count == 0)
        {
            return;
        }

        var exportList = new List<OperBoxData.OperData>();
        var userOperMap = OperBoxHaveList.ToDictionary(op => op.Id);

        foreach (var (operId, operInfo) in DataHelper.Operators)
        {
            if (!DataHelper.IsCharacterAvailableInClient(operInfo, SettingsViewModel.GameSettings.ClientType))
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

        System.Windows.Forms.Clipboard.Clear();
        System.Windows.Forms.Clipboard.SetDataObject(JsonConvert.SerializeObject(exportList, Formatting.Indented));
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
    private bool _gachaShowDisclaimer = true; // !Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ShowDisclaimerNoMore, bool.FalseString));

    public bool GachaShowDisclaimer
    {
        get => _gachaShowDisclaimer;
        set {
            SetAndNotify(ref _gachaShowDisclaimer, value);
        }
    }

    private bool _gachaShowDisclaimerNoMore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, bool.FalseString));

    public bool GachaShowDisclaimerNoMore
    {
        get => _gachaShowDisclaimerNoMore;
        set {
            SetAndNotify(ref _gachaShowDisclaimerNoMore, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, value.ToString());
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

    private int _peepTargetFps = int.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.PeepTargetFps, "20"));

    public int PeepTargetFps
    {
        get {
            return _peepTargetFps;
        }

        set {
            value = value switch {
                > 600 => 600,
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

    public static ObservableCollection<MiniGameEntry> MiniGameTaskList { get; } = [];

    public static void UpdateMiniGameTaskList()
    {
        MiniGameTaskList.Clear();
        var tasks = Instances.StageManager.MiniGameEntries;
        foreach (var t in tasks)
        {
            MiniGameTaskList.Add(new MiniGameEntry { Display = t.Display, DisplayKey = t.DisplayKey, Value = t.Value, Tip = t.Tip, TipKey = t.TipKey });
        }
    }

    private string _miniGameTaskName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MiniGameTaskName, "SS@Store@Begin");

    public string MiniGameTaskName
    {
        get => _miniGameTaskName;
        set {
            SetAndNotify(ref _miniGameTaskName, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MiniGameTaskName, value);
            MiniGameTip = GetMiniGameTip(value);
        }
    }

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

        // 然后使用 explicit Tip
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

    private string _secretFrontEnding = ConfigurationHelper.GetValue(ConfigurationKeys.MiniGameSecretFrontEnding, "A");

    public string SecretFrontEnding
    {
        get => _secretFrontEnding;
        set {
            SetAndNotify(ref _secretFrontEnding, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.MiniGameSecretFrontEnding, value);
        }
    }

    public List<GenericCombinedData<string>> SecretFrontEventList { get; set; } = [
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event1"), Value = "支援作战平台" },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event2"), Value = "游侠" },
        new GenericCombinedData<string> { Display = LocalizationHelper.GetString("MiniGame@SecretFront@Event3"), Value = "诡影迷踪" },
    ];

    private string _secretFrontEvent = ConfigurationHelper.GetValue(ConfigurationKeys.MiniGameSecretFrontEvent, string.Empty);

    public string SecretFrontEvent
    {
        get => _secretFrontEvent;
        set {
            SetAndNotify(ref _secretFrontEvent, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.MiniGameSecretFrontEvent, value);
        }
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
            return;
        }

        _runningState.SetIdle(false);
        string errMsg = string.Empty;
        bool caught = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
        if (!caught)
        {
            _runningState.SetIdle(true);
        }

        if (_runningState.GetStopping())
        {
            Instances.TaskQueueViewModel.SetStopped();
            return;
        }

        // If it's the recast task, pass the condition parameters
        // 如果是重新投钱任务,传递条件参数
        caught = Instances.AsstProxy.AsstMiniGame(MiniGameTaskName, BuildMiniGameExtraParams());
        if (!caught)
        {
            _runningState.SetIdle(true);
        }
    }

    /// <summary>
    /// Builds extra parameters for the current mini game task.
    /// 构建当前小游戏任务的额外参数
    /// </summary>
    /// <returns>Extra parameters as JObject, or null if none needed.</returns>
    private JObject? BuildMiniGameExtraParams()
    {
        if (MiniGameTaskName != RecastConstants.TaskName)
        {
            return null;
        }

        var conditions = MiniGameRecastSettings.RecastConditions;
        if (conditions == null || conditions.Count == 0)
        {
            return null;
        }

        var conditionsArray = new JArray();
        foreach (var condition in conditions)
        {
            conditionsArray.Add(condition.ToJObject());
        }

        return new JObject { ["conditions"] = conditionsArray };
    }

    #endregion
}
