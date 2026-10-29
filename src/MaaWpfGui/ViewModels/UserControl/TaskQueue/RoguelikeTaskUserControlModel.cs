// <copyright file="RoguelikeTaskUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;
using AsstRoguelikeCoreChar = MaaWpfGui.Models.AsstTasks.AsstRoguelikeTask.AsstRoguelikeCoreChar;
using Mode = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeMode;
using RoguelikeBoskySubNodeType = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeBoskySubNodeType;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeTheme;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class RoguelikeTaskUserControlModel : TaskSettingsViewModel, RoguelikeTaskUserControlModel.ISerialize
{
    static RoguelikeTaskUserControlModel()
    {
        Instance = new();
        Instances.AsstProxy.AsstSubTaskMsgEvent += Instance.ProcSubTaskMsg;
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public static RoguelikeTaskUserControlModel Instance { get; }

    public void InitRoguelike()
    {
        UpdateRoguelikeParams();
    }

    private void UpdateRoguelikeParams()
    {
        // 确保在更新列表之前先更新相关属性
        UpdateRoguelikeDifficultyList();
        UpdateRoguelikeModeList();
        UpdateRoguelikeRolesList();
        UpdateRoguelikeSquadList();
        UpdateRoguelikeStartWithAllDict();
        UpdateRoguelikeCoreCharList();
        UpdateBlackFlowSerpentOptions();
        UpdateBlackFlowStartRewardPriorityItems();
    }

    private void UpdateRoguelikeDifficultyList()
    {
        int maxThemeDifficulty = GetMaxDifficultyForTheme(RoguelikeTheme);
        var difficulty = RoguelikeDifficulty;

        // 0 = min, -1 = current, int.MaxValue = max
        var list = Enumerable.Range(0, maxThemeDifficulty + 1).OrderDescending().ToList();
        list.Insert(0, -1);
        list.Insert(1, int.MaxValue);
        RoguelikeDifficultyList.Clear();
        foreach (var i in list)
        {
            int value = i;
            var display = value switch {
                -1 => LocalizationHelper.GetString("TaskQueue.NotSwitch") + " (-1)",
                int.MaxValue => $"MAX ({maxThemeDifficulty})",
                0 => "MIN (0)",
                _ => value.ToString(),
            };
            RoguelikeDifficultyList.Add(new() { Display = display, Value = value });
        }

        // 验证当前选中的难度是否在新列表中
        RoguelikeDifficulty = RoguelikeDifficultyList.Any(item => item.Value == difficulty) ? difficulty : -1;
        NotifyOfPropertyChange(nameof(RoguelikeDifficulty));
    }

    private static int GetMaxDifficultyForTheme(Theme theme) => theme switch {
        Theme.Phantom => 15,
        Theme.Mizuki => 18,
        Theme.Sami => 15,
        Theme.Sarkaz => 18,
        Theme.JieGarden => 18,
        Theme.BlackFlow => 16,
        _ => 20,
    };

    private void UpdateRoguelikeModeList()
    {
        var roguelikeMode = RoguelikeMode;

        List<GenericCombinedData<Mode>> baseList;
        if (RoguelikeTheme == Theme.BlackFlow)
        {
            baseList =
            [
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.Exp"), Value = Mode.Exp },
                new() { Display = LocalizationHelper.GetString("Roguelike.BlackFlow.Strategy.Investment"), Value = Mode.Investment },
                new() { Display = LocalizationHelper.GetString("Roguelike.BlackFlow.Strategy.StartRun"), Value = Mode.BlackFlowStartRun },
            ];
        }
        else
        {
            baseList =
            [
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.Exp"), Value = Mode.Exp },
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.Gold"), Value = Mode.Investment },
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.LastReward"), Value = Mode.Collectible },
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.MonthlySquad"), Value = Mode.Squad },
                new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.DeepExploration"), Value = Mode.Exploration },
            ];
        }

        switch (RoguelikeTheme)
        {
            case Theme.Sami:
                baseList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.Collapse"), Value = Mode.CLP_PDS });
                break;

            case Theme.JieGarden:
                baseList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Strategy.FindPlaytime"), Value = Mode.FindPlaytime });
                break;
        }

        RoguelikeModeList = new ObservableCollection<GenericCombinedData<Mode>>(baseList);
        RoguelikeMode = RoguelikeModeList.Any(x => x.Value == roguelikeMode) ? roguelikeMode : RoguelikeModeList.First().Value;
    }

    private void UpdateRoguelikeRolesList()
    {
        var roguelikeRoles = RoguelikeRoles;
        RoguelikeRolesList =
        [
            new() { Display = LocalizationHelper.GetString("Roguelike.Roles.FirstMoveAdvantage"), Value = "先手必胜" },
            new() { Display = LocalizationHelper.GetString("Roguelike.Roles.SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
            new() { Display = LocalizationHelper.GetString("Roguelike.Roles.OvercomingYourWeaknesses"), Value = "取长补短" },
        ];

        switch (RoguelikeTheme)
        {
            case Theme.JieGarden:
            case Theme.BlackFlow:
                RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Roles.FlexibleDeployment"), Value = "灵活部署" });
                RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Roles.Unbreakable"), Value = "坚不可摧" });
                break;
        }

        RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Roles.AsYourHeartDesires"), Value = "随心所欲" });

        RoguelikeRoles = RoguelikeRolesList.Any(x => x.Value == roguelikeRoles) ? roguelikeRoles : "稳扎稳打";
    }

    private readonly Dictionary<string, List<(string Key, string Value)>> _squadDictionary = new() {
        ["Phantom_Default"] =
        [
            ("Roguelike.Squad.Gathering", "集群分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.Research", "研究分队"),
        ],
        ["Mizuki_Default"] =
        [
            ("Roguelike.Squad.Gathering", "集群分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.IS2NewSquad1", "心胜于物分队"),
            ("Roguelike.Squad.IS2NewSquad2", "物尽其用分队"),
            ("Roguelike.Squad.IS2NewSquad3", "以人为本分队"),
            ("Roguelike.Squad.Research", "研究分队"),
        ],
        ["Sami_Default"] =
        [
            ("Roguelike.Squad.Gathering", "集群分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.IS3NewSquad1", "永恒狩猎分队"),
            ("Roguelike.Squad.IS3NewSquad2", "生活至上分队"),
            ("Roguelike.Squad.IS3NewSquad3", "科学主义分队"),
            ("Roguelike.Squad.IS3NewSquad4", "特训分队"),
        ],
        ["Sarkaz_1"] =
        [
            ("Roguelike.Squad.Gathering", "集群分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.IS4NewSquad2", "博闻广记分队"),
            ("Roguelike.Squad.IS4NewSquad3", "蓝图测绘分队"),
            ("Roguelike.Squad.IS4NewSquad6", "点刺成锭分队"),
            ("Roguelike.Squad.IS4NewSquad7", "拟态学者分队"),
        ],
        ["Sarkaz_Default"] =
        [
            ("Roguelike.Squad.Gathering", "集群分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.IS4NewSquad1", "魂灵护送分队"),
            ("Roguelike.Squad.IS4NewSquad2", "博闻广记分队"),
            ("Roguelike.Squad.IS4NewSquad3", "蓝图测绘分队"),
            ("Roguelike.Squad.IS4NewSquad4", "因地制宜分队"),
            ("Roguelike.Squad.IS4NewSquad5", "异想天开分队"),
            ("Roguelike.Squad.IS4NewSquad6", "点刺成锭分队"),
            ("Roguelike.Squad.IS4NewSquad7", "拟态学者分队"),
            ("Roguelike.Squad.IS4NewSquad8", "专业人士分队"),
        ],
        ["JieGarden_Default"] =
        [
            ("Roguelike.Squad.SpecialForce", "特勤分队"),
            ("Roguelike.Squad.IS5NewSquad1", "高台突破分队"),
            ("Roguelike.Squad.IS5NewSquad2", "地面突破分队"),
            ("Roguelike.Squad.IS5NewSquad3", "游客分队"),
            ("Roguelike.Squad.IS5NewSquad4", "司岁台分队"),
            ("Roguelike.Squad.IS5NewSquad5", "天师府分队"),
            ("Roguelike.Squad.IS5NewSquad6", "花团锦簇分队"),
            ("Roguelike.Squad.IS5NewSquad7", "棋行险着分队"),
            ("Roguelike.Squad.IS5NewSquad8", "岁影回音分队"),
            ("Roguelike.Squad.IS5NewSquad9", "代理人分队"),
            ("Roguelike.Squad.IS5NewSquad10", "知学分队"),
            ("Roguelike.Squad.IS5NewSquad11", "商贾分队"),
        ],
        ["BlackFlow_Default"] =
        [
            ("Roguelike.Squad.SpecialForce", "特勤分队"),
            ("Roguelike.Squad.Spearhead", "矛头分队"),
            ("Roguelike.Squad.IS5NewSquad1", "高台突破分队"),
            ("Roguelike.Squad.IS5NewSquad2", "地面突破分队"),
            ("Roguelike.Squad.IS6NewSquad1", "本源研修分队"),
            ("Roguelike.Squad.IS6NewSquad2", "文明开化分队"),
            ("Roguelike.Squad.IS6NewSquad3", "开拓者分队"),
            ("Roguelike.Squad.IS6NewSquad4", "多边贸易分队"),
            ("Roguelike.Squad.IS6NewSquad5", "地质调查分队"),
            ("Roguelike.Squad.IS6NewSquad6", "团结阵线分队"),
            ("Roguelike.Squad.IS6NewSquad7", "踏足遍历分队"),
        ],
    };

    // 通用分队
    private readonly List<(string Key, string Value)> _commonSquads =
    [
        ("Roguelike.Squad.Leader", "指挥分队"),
        ("Roguelike.Squad.Support", "后勤分队"),
        ("Roguelike.Squad.TacticalAssault", "突击战术分队"),
        ("Roguelike.Squad.TacticalFortification", "堡垒战术分队"),
        ("Roguelike.Squad.TacticalRanged", "远程战术分队"),
        ("Roguelike.Squad.TacticalDestruction", "破坏战术分队"),
    ];

    private void UpdateRoguelikeSquadList()
    {
        var roguelikeSquad = RoguelikeSquad;
        var roguelikeSquadCollectible = RoguelikeCollectibleModeSquad;
        RoguelikeSquadList = [];

        // 优先匹配 Theme_Mode，其次匹配 Theme_Default
        string themeKey = $"{RoguelikeTheme}_{RoguelikeMode}";
        if (!_squadDictionary.ContainsKey(themeKey))
        {
            themeKey = $"{RoguelikeTheme}_Default";
        }

        // 添加主题分队
        if (_squadDictionary.TryGetValue(themeKey, out var squads))
        {
            foreach (var (key, value) in squads)
            {
                RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString(key), Value = value });
            }
        }

        // 添加通用分队
        foreach (var (key, value) in _commonSquads)
        {
            if (RoguelikeTheme == Theme.BlackFlow && key == "Roguelike.Squad.FirstClass")
            {
                continue;
            }
            RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString(key), Value = value });
        }

        if (RoguelikeTheme is Theme.Phantom or Theme.Mizuki or Theme.Sami or Theme.Sarkaz or Theme.JieGarden)
        {
            RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString("Roguelike.Squad.FirstClass"), Value = "高规格分队" });
        }

        // 选择当前分队
        RoguelikeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquad) ? roguelikeSquad : "指挥分队";
        RoguelikeCollectibleModeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquadCollectible) ? roguelikeSquadCollectible : RoguelikeSquad;
    }

    private void UpdateRoguelikeCoreCharList()
    {
        var filePath = $"resource/roguelike/{RoguelikeTheme}/recruitment.json";
        if (!File.Exists(filePath))
        {
            RoguelikeCoreCharList.Clear();
            return;
        }

        var jsonStr = File.ReadAllText(filePath);
        var json = (JObject?)JsonConvert.DeserializeObject(jsonStr);

        var roguelikeCoreCharList = new ObservableCollection<string>();

        if (json?["priority"] is JArray priorityArray)
        {
            foreach (var priorityItem in priorityArray)
            {
                if (priorityItem?["opers"] is not JArray opersArray)
                {
                    continue;
                }

                foreach (var operItem in opersArray)
                {
                    var isStart = (bool?)operItem["is_start"] ?? false;
                    if (!isStart)
                    {
                        continue;
                    }

                    var name = (string?)operItem["name"];
                    if (string.IsNullOrEmpty(name))
                    {
                        continue;
                    }

                    if (!DataHelper.IsCharacterAvailableInClient(name, SettingsViewModel.GameSettings.ClientType.ToCustomString()))
                    {
                        continue;
                    }

                    var localizedName = DataHelper.GetLocalizedCharacterName(name, SettingsViewModel.GuiSettings.OperNameLocalization);
                    if (!string.IsNullOrEmpty(localizedName))
                    {
                        roguelikeCoreCharList.Add(localizedName);
                    }
                }
            }
        }

        RoguelikeCoreCharList = roguelikeCoreCharList;
    }

    private ObservableCollection<GenericCombinedData<int>> _roguelikeDifficultyList = [];

    public ObservableCollection<GenericCombinedData<int>> RoguelikeDifficultyList
    {
        get => _roguelikeDifficultyList;
        set => SetAndNotify(ref _roguelikeDifficultyList, value);
    }

    private ObservableCollection<GenericCombinedData<Mode>> _roguelikeModeList = [];

    /// <summary>
    /// Gets or sets the list of roguelike modes.
    /// </summary>
    public ObservableCollection<GenericCombinedData<Mode>> RoguelikeModeList
    {
        get => _roguelikeModeList;
        set => SetAndNotify(ref _roguelikeModeList, value);
    }

    private ObservableCollection<CombinedData> _roguelikeSquadList = [];

    /// <summary>
    /// Gets or sets the list of roguelike squad.
    /// </summary>
    public ObservableCollection<CombinedData> RoguelikeSquadList
    {
        get => _roguelikeSquadList;
        set => SetAndNotify(ref _roguelikeSquadList, value);
    }

    private ObservableCollection<CombinedData> _roguelikeRolesList = [];

    /// <summary>
    /// Gets or sets the list of roguelike roles.
    /// </summary>
    public ObservableCollection<CombinedData> RoguelikeRolesList
    {
        get => _roguelikeRolesList;
        set => SetAndNotify(ref _roguelikeRolesList, value);
    }

    /// <summary>
    /// Gets the list of roguelike lists.
    /// </summary>
    public LocalizedObservableList<Theme> RoguelikeThemeList { get; } = new(
        (Theme.Phantom, "Roguelike.Theme.Phantom"),
        (Theme.Mizuki, "Roguelike.Theme.Mizuki"),
        (Theme.Sami, "Roguelike.Theme.Sami"),
        (Theme.Sarkaz, "Roguelike.Theme.Sarkaz"),
        (Theme.JieGarden, "Roguelike.Theme.JieGarden"),
        (Theme.BlackFlow, "Roguelike.Theme.BlackFlow")
            .WithSecondaryKey("Common.NotAdapted", () => SettingsViewModel.GameSettings.ClientType is not (ClientType.Official or ClientType.Bilibili)));

    /// <summary>
    /// Gets or sets the Roguelike theme.
    /// </summary>
    public Theme RoguelikeTheme
    {
        get => GetTaskConfig<RoguelikeTask>().Theme;
        set {
            SetTaskConfig<RoguelikeTask>(t => t.Theme == value, t => t.Theme = value);

            // Check and adjust difficulty if current value is not supported by new theme
            int maxDifficulty = GetMaxDifficultyForTheme(value);
            if (RoguelikeDifficulty != -1 && RoguelikeDifficulty != int.MaxValue && RoguelikeDifficulty > maxDifficulty)
            {
                RoguelikeDifficulty = -1; // Set to "Current" if not supported
            }

            UpdateRoguelikeParams();

            // 强制刷新难度显示
            OnPropertyChanged(nameof(RoguelikeDifficulty));

            // 更新主题提示
            OnPropertyChanged(nameof(RoguelikeThemeTip));
        }
    }

    public int RoguelikeDifficulty
    {
        get => GetTaskConfig<RoguelikeTask>().Difficulty;
        set => SetTaskConfig<RoguelikeTask>(t => t.Difficulty == value, t => t.Difficulty = value);
    }

    /// <summary>
    /// Gets or sets 策略，往后打 / 刷一层就退 / 烧热水
    /// </summary>
    public Mode RoguelikeMode
    {
        get => GetTaskConfig<RoguelikeTask>().Mode;
        set {
            if (value == Mode.Investment)
            {
                RoguelikeInvestmentEnabled = true;
            }

            SetTaskConfig<RoguelikeTask>(t => t.Mode == value, t => t.Mode = value);
            UpdateRoguelikeSquadList();
        }
    }

    /// <summary>
    /// Gets or sets the roguelike squad.
    /// </summary>
    public string RoguelikeSquad
    {
        get => GetTaskConfig<RoguelikeTask>().Squad;
        set => SetTaskConfig<RoguelikeTask>(t => t.Squad == value, t => t.Squad = value);
    }

    /// <summary>
    /// Gets or sets the roguelike squad using for last reward mode.
    /// </summary>
    public string RoguelikeCollectibleModeSquad
    {
        get => GetTaskConfig<RoguelikeTask>().SquadCollectible;
        set => SetTaskConfig<RoguelikeTask>(t => t.SquadCollectible == value, t => t.SquadCollectible = value);
    }

    [PropertyDependsOn(nameof(RoguelikeMode), nameof(RoguelikeTheme), nameof(RoguelikeSquad))]
    public bool RoguelikeSquadIsProfessional => RoguelikeMode == Mode.Collectible && RoguelikeTheme != Theme.Phantom && RoguelikeSquad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";

    [PropertyDependsOn(nameof(RoguelikeMode), nameof(RoguelikeTheme), nameof(RoguelikeSquad))]
    public bool RoguelikeSquadIsFoldartal => RoguelikeMode == Mode.Collectible && RoguelikeTheme == Theme.Sami && RoguelikeSquad == "生活至上分队";

    /// <summary>
    /// Gets or sets the roguelike roles.
    /// </summary>
    public string RoguelikeRoles
    {
        get => GetTaskConfig<RoguelikeTask>().Roles;
        set => SetTaskConfig<RoguelikeTask>(t => t.Roles == value, t => t.Roles = value);
    }

    /// <summary>
    /// 读取指定顺位的开局干员配置项，顺位尚未填写时返回 null。
    /// </summary>
    private static RoguelikeTask.RoguelikeStartingOper? GetStartingOper(RoguelikeTask task, int index)
        => task.StartingOpers.Count > index ? task.StartingOpers[index] : null;

    /// <summary>
    /// 确保指定顺位的开局干员配置项存在（不足则补齐），返回该项用于写入。
    /// </summary>
    private static RoguelikeTask.RoguelikeStartingOper EnsureStartingOper(RoguelikeTask task, int index)
    {
        while (task.StartingOpers.Count <= index)
        {
            task.StartingOpers.Add(new());
        }

        return task.StartingOpers[index];
    }

    private static string GetStartingOperName(RoguelikeTask task, int index)
        => GetStartingOper(task, index)?.Name ?? string.Empty;

    /// <summary>
    /// Gets or sets the roguelike core character of the 1st starting position.
    /// </summary>
    public string RoguelikeCoreChar
    {
        get => GetStartingOperName(GetTaskConfig<RoguelikeTask>(), 0);
        set {
            if (!SetTaskConfig<RoguelikeTask>(t => GetStartingOperName(t, 0) == value, t => EnsureStartingOper(t, 0).Name = value))
            {
                return;
            }

            Instances.TaskQueueViewModel.AddLog(string.IsNullOrEmpty(value)
                ? LocalizationHelper.GetString("Roguelike.StartingCoreChar.Cleared")
                : LocalizationHelper.GetStringFormat("Roguelike.StartingCoreChar.Set", value));
        }
    }

    /// <summary>
    /// Gets or sets the roguelike core character of the 2nd starting position.
    /// </summary>
    public string RoguelikeCoreChar2
    {
        get => GetStartingOperName(GetTaskConfig<RoguelikeTask>(), 1);
        set => SetTaskConfig<RoguelikeTask>(t => GetStartingOperName(t, 1) == value, t => EnsureStartingOper(t, 1).Name = value);
    }

    /// <summary>
    /// Gets or sets the roguelike core character of the 3rd starting position.
    /// </summary>
    public string RoguelikeCoreChar3
    {
        get => GetStartingOperName(GetTaskConfig<RoguelikeTask>(), 2);
        set => SetTaskConfig<RoguelikeTask>(t => GetStartingOperName(t, 2) == value, t => EnsureStartingOper(t, 2).Name = value);
    }

    [PropertyDependsOn(nameof(RoguelikeTheme))]
    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    public string StartingCoreCharTip => LocalizationHelper.GetString("Roguelike.StartingCoreChar.Tip") + "\n\n" + RoguelikeThemeTip;

    private ObservableCollection<string> _roguelikeCoreCharList = [];

    /// <summary>
    /// Gets the roguelike core character candidates shared by all starting positions.
    /// </summary>
    public ObservableCollection<string> RoguelikeCoreCharList
    {
        get => _roguelikeCoreCharList;
        private set {
            // 各顺位当前值不在候选列表时追加，保证已保存的干员名（含别名/手输名）仍可选
            foreach (var coreChar in new[] { RoguelikeCoreChar, RoguelikeCoreChar2, RoguelikeCoreChar3 })
            {
                if (!string.IsNullOrEmpty(coreChar) && !value.Contains(coreChar))
                {
                    value.Add(coreChar);
                }
            }

            SetAndNotify(ref _roguelikeCoreCharList, value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether core char need start with elite two.
    /// </summary>
    public bool RoguelikeStartWithEliteTwo
    {
        get => GetTaskConfig<RoguelikeTask>().StartWithEliteTwo;
        set {
            switch (value)
            {
                case true when RoguelikeUseSupportUnit:
                    RoguelikeUseSupportUnit = false;
                    break;

                case false when RoguelikeOnlyStartWithEliteTwoRaw:
                    RoguelikeOnlyStartWithEliteTwoRaw = false;
                    break;
            }
            SetTaskConfig<RoguelikeTask>(t => t.StartWithEliteTwo == value, t => t.StartWithEliteTwo = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether only need with elite two's core char.
    /// </summary>
    public bool RoguelikeOnlyStartWithEliteTwoRaw
    {
        get => GetTaskConfig<RoguelikeTask>().StartWithEliteTwoOnly;
        set => SetTaskConfig<RoguelikeTask>(t => t.StartWithEliteTwoOnly == value, t => t.StartWithEliteTwoOnly = value);
    }

    /// <summary>
    /// Gets a value indicating whether only need with elite two's core char.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeOnlyStartWithEliteTwoRaw), nameof(RoguelikeStartWithEliteTwo), nameof(RoguelikeSquadIsProfessional))]
    public bool RoguelikeOnlyStartWithEliteTwo => RoguelikeOnlyStartWithEliteTwoRaw && RoguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional;

    /// <summary>
    /// Gets the available start with rewards dictionary based on current theme.
    /// </summary>
    public List<GenericCombinedData<RoguelikeCollectibleAward>> RoguelikeStartAwards { get; private set; } = [];

    /// <summary>
    /// Gets a tip text for current Roguelike theme.
    /// </summary>
    public string RoguelikeThemeTip
    {
        get {
            var key = RoguelikeTheme switch {
                Theme.Phantom => "Roguelike.Theme.Phantom.Tip",
                Theme.Mizuki => "Roguelike.Theme.Mizuki.Tip",
                Theme.Sami => "Roguelike.Theme.Sami.Tip",
                Theme.Sarkaz => "Roguelike.Theme.Sarkaz.Tip",
                Theme.JieGarden => "Roguelike.Theme.JieGarden.Tip",
                Theme.BlackFlow => "Roguelike.Theme.BlackFlow.Tip",
                _ => "Roguelike.Theme.Phantom.Tip",
            };

            var tip = LocalizationHelper.GetString(key);
            return string.IsNullOrWhiteSpace(tip) ? LocalizationHelper.GetString("Roguelike.Theme") ?? string.Empty : tip;
        }
    }

    private void UpdateRoguelikeStartWithAllDict()
    {
        var config = GetTaskConfig<RoguelikeTask>().CollectibleStartAwards;
        var list = new List<GenericCombinedData<RoguelikeCollectibleAward>>()
        {
           new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Kettle"), Value = RoguelikeCollectibleAward.HotWater },
           new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Shield"), Value = RoguelikeCollectibleAward.Shield },
           new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Ingot"), Value = RoguelikeCollectibleAward.Ingot },
           new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Hope"), Value = RoguelikeCollectibleAward.Hope },
           new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.RandomReward"), Value = RoguelikeCollectibleAward.Random },
        };

        switch (RoguelikeTheme)
        {
            case Theme.Mizuki:
                list.Add(new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Key"), Value = RoguelikeCollectibleAward.Key });
                list.Add(new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Dice"), Value = RoguelikeCollectibleAward.Dice });
                break;

            case Theme.Sarkaz:
                list.Add(new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Idea"), Value = RoguelikeCollectibleAward.Idea });
                break;

            case Theme.JieGarden:
                list.Remove(list.First(i => i.Value == RoguelikeCollectibleAward.Hope));
                list.Add(new() { Display = LocalizationHelper.GetString("Roguelike.StartWith.Ticket"), Value = RoguelikeCollectibleAward.Ticket });
                break;
        }

        RoguelikeStartAwards = list;
        OnPropertyChanged(nameof(RoguelikeStartAwards));
        RoguelikeStartWithSelectList = RoguelikeStartAwards.Where(i => config.HasFlag(i.Value)).ToArray();
    }

    public object[] RoguelikeStartWithSelectList
    {
        get {
            var value = GetTaskConfig<RoguelikeTask>().CollectibleStartAwards;
            return RoguelikeStartAwards.Where(v => value.HasFlag(v.Value)).ToArray();
        }

        set {
            var v = value.Cast<GenericCombinedData<RoguelikeCollectibleAward>>().Select(k => k.Value).DefaultIfEmpty((RoguelikeCollectibleAward)0).Aggregate((a, b) => a | b);
            SetTaskConfig<RoguelikeTask>(t => t.CollectibleStartAwards == v, t => t.CollectibleStartAwards = v);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether core char need start with elite two.
    /// </summary>
    public bool Roguelike3FirstFloorFoldartal
    {
        get => GetTaskConfig<RoguelikeTask>().SamiFirstFloorFoldartal;
        set => SetTaskConfig<RoguelikeTask>(t => t.SamiFirstFloorFoldartal == value, t => t.SamiFirstFloorFoldartal = value);
    }

    public string Roguelike3FirstFloorFoldartals
    {
        get => GetTaskConfig<RoguelikeTask>().SamiFirstFloorFoldartals;
        set {
            value = value.Trim();
            SetTaskConfig<RoguelikeTask>(t => t.SamiFirstFloorFoldartals == value, t => t.SamiFirstFloorFoldartals = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether core char need start with elite two.
    /// </summary>
    public bool Roguelike3NewSquad2StartingFoldartal
    {
        get => GetTaskConfig<RoguelikeTask>().SamiNewSquad2StartingFoldartal;
        set => SetTaskConfig<RoguelikeTask>(t => t.SamiNewSquad2StartingFoldartal == value, t => t.SamiNewSquad2StartingFoldartal = value);
    }

    public string Roguelike3NewSquad2StartingFoldartals
    {
        get => GetTaskConfig<RoguelikeTask>().SamiNewSquad2StartingFoldartals;
        set {
            value = value.Replace('；', ';').Trim();
            SetTaskConfig<RoguelikeTask>(t => t.SamiNewSquad2StartingFoldartals == value, t => t.SamiNewSquad2StartingFoldartals = value);
        }
    }

    /// <summary>
    /// Gets or sets the expected collapsal paradigms.
    /// 需要刷的坍缩列表，分号分隔
    /// </summary>
    public string RoguelikeExpectedCollapsalParadigms
    {
        get => GetTaskConfig<RoguelikeTask>().ExpectedCollapsalParadigms;
        set {
            value = value.Replace('；', ';').Trim();
            SetTaskConfig<RoguelikeTask>(t => t.ExpectedCollapsalParadigms == value, t => t.ExpectedCollapsalParadigms = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use support unit for the 1st starting oper.
    /// 勾选时关闭开局凹直升（两者互斥）。
    /// </summary>
    public bool RoguelikeUseSupportUnit
    {
        get => GetStartingOper(GetTaskConfig<RoguelikeTask>(), 0)?.UseSupport ?? false;
        set {
            if (value && RoguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional)
            {
                RoguelikeStartWithEliteTwo = false;
            }

            SetTaskConfig<RoguelikeTask>(t => (GetStartingOper(t, 0)?.UseSupport ?? false) == value, t => EnsureStartingOper(t, 0).UseSupport = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use support unit for the 2nd starting oper.
    /// </summary>
    public bool RoguelikeCoreChar2UseSupport
    {
        get => GetStartingOper(GetTaskConfig<RoguelikeTask>(), 1)?.UseSupport ?? false;
        set => SetTaskConfig<RoguelikeTask>(t => (GetStartingOper(t, 1)?.UseSupport ?? false) == value, t => EnsureStartingOper(t, 1).UseSupport = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use support unit for the 3rd starting oper.
    /// </summary>
    public bool RoguelikeCoreChar3UseSupport
    {
        get => GetStartingOper(GetTaskConfig<RoguelikeTask>(), 2)?.UseSupport ?? false;
        set => SetTaskConfig<RoguelikeTask>(t => (GetStartingOper(t, 2)?.UseSupport ?? false) == value, t => EnsureStartingOper(t, 2).UseSupport = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the 2nd and 3rd starting core characters are enabled.
    /// </summary>
    public bool RoguelikeEnableAdditionalCoreChars
    {
        get => GetTaskConfig<RoguelikeTask>().UseAdditionalStartingOpers;
        set => SetTaskConfig<RoguelikeTask>(t => t.UseAdditionalStartingOpers == value, t => t.UseAdditionalStartingOpers = value);
    }

    /// <summary>
    /// Gets the display text of the 1st starting oper's support unit checkbox.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeCoreChar))]
    public string RoguelikeUseSupportUnitText => GetUseSupportText(RoguelikeCoreChar, 1);

    /// <summary>
    /// Gets the display text of the 2nd starting oper's support unit checkbox.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeCoreChar2))]
    public string RoguelikeCoreChar2UseSupportText => GetUseSupportText(RoguelikeCoreChar2, 2);

    /// <summary>
    /// Gets the display text of the 3rd starting oper's support unit checkbox.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeCoreChar3))]
    public string RoguelikeCoreChar3UseSupportText => GetUseSupportText(RoguelikeCoreChar3, 3);

    private static string GetUseSupportText(string operName, int position)
        => LocalizationHelper.GetStringFormat(
            "Roguelike.UseSupportUnit.Format",
            string.IsNullOrEmpty(operName) ? LocalizationHelper.GetString($"Roguelike.CoreCharPosition{position}") : operName);

    /// <summary>
    /// Gets or sets a value indicating whether can roguelike support unit belong to nonfriend.
    /// </summary>
    public bool RoguelikeEnableNonfriendSupport
    {
        get => GetTaskConfig<RoguelikeTask>().UseSupportNonFriend;
        set => SetTaskConfig<RoguelikeTask>(t => t.UseSupportNonFriend == value, t => t.UseSupportNonFriend = value);
    }

    /// <summary>
    /// Gets or sets the start count of roguelike.
    /// </summary>
    public int RoguelikeStartsCount
    {
        get => GetTaskConfig<RoguelikeTask>().StartCount;
        set => SetTaskConfig<RoguelikeTask>(t => t.StartCount == value, t => t.StartCount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether investment is enabled.
    /// </summary>
    public bool RoguelikeInvestmentEnabled
    {
        get => GetTaskConfig<RoguelikeTask>().Investment;
        set => SetTaskConfig<RoguelikeTask>(t => t.Investment == value, t => t.Investment = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether investment is enabled.
    /// </summary>
    public bool RoguelikeInvestmentWithMoreScoreRaw
    {
        get => GetTaskConfig<RoguelikeTask>().InvestWithMoreScore;
        set => SetTaskConfig<RoguelikeTask>(t => t.InvestWithMoreScore == value, t => t.InvestWithMoreScore = value);
    }

    /// <summary>
    /// Gets a value indicating whether investment is enabled.
    /// </summary>
    public bool RoguelikeInvestmentWithMoreScore => GetTaskConfig<RoguelikeTask>().InvestWithMoreScore && RoguelikeMode == Mode.Investment && RoguelikeTheme != Theme.BlackFlow;

    /// <summary>
    /// Gets or sets a value indicating whether shopping is enabled in LastReward Mode.
    /// </summary>
    public bool RoguelikeCollectibleModeShopping
    {
        get => GetTaskConfig<RoguelikeTask>().CollectibleShopping;
        set => SetTaskConfig<RoguelikeTask>(t => t.CollectibleShopping == value, t => t.CollectibleShopping = value);
    }

    public bool RoguelikeRefreshTraderWithDiceRaw
    {
        get => GetTaskConfig<RoguelikeTask>().RefreshTraderWithDice;
        set => SetTaskConfig<RoguelikeTask>(t => t.RefreshTraderWithDice == value, t => t.RefreshTraderWithDice = value);
    }

    /// <summary>
    /// Gets or sets the invests count of roguelike.
    /// </summary>
    public int RoguelikeInvestsCount
    {
        get => GetTaskConfig<RoguelikeTask>().InvestCount;
        set => SetTaskConfig<RoguelikeTask>(t => t.InvestCount == value, t => t.InvestCount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to stop when investment is full.
    /// </summary>
    public bool RoguelikeStopWhenInvestmentFull
    {
        get => GetTaskConfig<RoguelikeTask>().StopWhenDepositFull;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopWhenDepositFull == value, t => t.StopWhenDepositFull = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to stop when arriving at final boss.
    /// </summary>
    public bool RoguelikeStopAtFinalBoss
    {
        get => GetTaskConfig<RoguelikeTask>().StopAtFinalBoss;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopAtFinalBoss == value, t => t.StopAtFinalBoss = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to automatically iterate the monthly squad.
    /// </summary>
    public bool RoguelikeMonthlySquadAutoIterate
    {
        get => GetTaskConfig<RoguelikeTask>().MonthlySquadAutoIterate;
        set => SetTaskConfig<RoguelikeTask>(t => t.MonthlySquadAutoIterate == value, t => t.MonthlySquadAutoIterate = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to automatically iterate the deep exploration mode.
    /// </summary>
    public bool RoguelikeMonthlySquadCheckComms
    {
        get => GetTaskConfig<RoguelikeTask>().MonthlySquadCheckComms;
        set => SetTaskConfig<RoguelikeTask>(t => t.MonthlySquadCheckComms == value, t => t.MonthlySquadCheckComms = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to automatically iterate the deep exploration mode.
    /// </summary>
    public bool RoguelikeDeepExplorationAutoIterate
    {
        get => GetTaskConfig<RoguelikeTask>().DeepExplorationAutoIterate;
        set => SetTaskConfig<RoguelikeTask>(t => t.DeepExplorationAutoIterate == value, t => t.DeepExplorationAutoIterate = value);
    }

    /// <summary>
    /// Gets or sets the target playtime subnode type for FindPlaytime mode.
    /// </summary>
    public RoguelikeBoskySubNodeType RoguelikeFindPlaytimeTarget
    {
        get => GetTaskConfig<RoguelikeTask>().FindPlaytimeTarget;
        set => SetTaskConfig<RoguelikeTask>(t => t.FindPlaytimeTarget == value, t => t.FindPlaytimeTarget = value);
    }

    /// <summary>
    /// Gets the list of available playtime target options for FindPlaytime mode.
    /// </summary>
    public LocalizedObservableList<RoguelikeBoskySubNodeType> RoguelikeFindPlaytimeTargetList { get; } = new(
        (RoguelikeBoskySubNodeType.Ling, "Roguelike.Playtime.Ling"),
        (RoguelikeBoskySubNodeType.Shu, "Roguelike.Playtime.Shu"),
        (RoguelikeBoskySubNodeType.Nian, "Roguelike.Playtime.Nian"));

    /// <summary>
    /// Gets or sets a value indicating whether the BlackFlow exp mode flies to the third floor with the Mechanic.
    /// </summary>
    public bool RoguelikeBlackFlowFastFlight
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowFastFlight;
        set => SetTaskConfig<RoguelikeTask>(t => t.BlackFlowFastFlight == value, t => t.BlackFlowFastFlight = value);
    }

    /// <summary>
    /// Gets or sets the target ending of the BlackFlow exp mode.
    /// </summary>
    public RoguelikeBlackFlowEnding RoguelikeBlackFlowEnding
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowEnding;
        set => SetTaskConfig<RoguelikeTask>(t => t.BlackFlowEnding == value, t => t.BlackFlowEnding = value);
    }

    /// <summary>
    /// Gets the available BlackFlow endings.
    /// 二、三结局暂不开放，选项不出现。
    /// </summary>
    public LocalizedObservableList<RoguelikeBlackFlowEnding> RoguelikeBlackFlowEndingList { get; } = new(
        (RoguelikeBlackFlowEnding.Ending1, "Roguelike.BlackFlow.Ending1"));

    /// <summary>
    /// 刷开局动物下拉框的选项。羽蛇最多一只，另一格已选羽蛇时本格的羽蛇不可选。
    /// </summary>
    private static LocalizedObservableList<RoguelikeBlackFlowStartAnimal> CreateBlackFlowStartAnimalOptions() => new(
        (RoguelikeBlackFlowStartAnimal.None, "Roguelike.BlackFlow.StartAnimalNone"),
        (RoguelikeBlackFlowStartAnimal.Cat, "Roguelike.BlackFlow.CultivationTargetCat")
            .WithTip("Roguelike.BlackFlow.CultivationTargetCat.Tip"),
        (RoguelikeBlackFlowStartAnimal.Dog, "Roguelike.BlackFlow.CultivationTargetDog")
            .WithTip("Roguelike.BlackFlow.CultivationTargetDog.Tip"),
        (RoguelikeBlackFlowStartAnimal.Cerberus, "Roguelike.BlackFlow.CultivationTargetCerberus")
            .WithTip("Roguelike.BlackFlow.CultivationTargetCerberus.Tip"),
        (RoguelikeBlackFlowStartAnimal.FeatheredSerpent, "Roguelike.BlackFlow.CultivationTargetFeatheredSerpent")
            .WithTip("Roguelike.BlackFlow.CultivationTargetFeatheredSerpent.Tip"));

    public LocalizedObservableList<RoguelikeBlackFlowStartAnimal> RoguelikeBlackFlowStartAnimal1List { get; } = CreateBlackFlowStartAnimalOptions();

    public LocalizedObservableList<RoguelikeBlackFlowStartAnimal> RoguelikeBlackFlowStartAnimal2List { get; } = CreateBlackFlowStartAnimalOptions();

    private void UpdateBlackFlowSerpentOptions()
    {
        RoguelikeBlackFlowStartAnimal1List.Items.Single(i => i.Value == RoguelikeBlackFlowStartAnimal.FeatheredSerpent).IsEnabled =
            RoguelikeBlackFlowStartAnimal2 != RoguelikeBlackFlowStartAnimal.FeatheredSerpent;
        RoguelikeBlackFlowStartAnimal2List.Items.Single(i => i.Value == RoguelikeBlackFlowStartAnimal.FeatheredSerpent).IsEnabled =
            RoguelikeBlackFlowStartAnimal1 != RoguelikeBlackFlowStartAnimal.FeatheredSerpent;
    }

    public RoguelikeBlackFlowStartAnimal RoguelikeBlackFlowStartAnimal1
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowStartAnimal1;
        set {
            SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartAnimal1 == value, t => t.BlackFlowStartAnimal1 = value);
            UpdateBlackFlowSerpentOptions();
            OnPropertyChanged(nameof(RoguelikeBlackFlowHasSerpent));
        }
    }

    public RoguelikeBlackFlowStartAnimal RoguelikeBlackFlowStartAnimal2
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowStartAnimal2;
        set {
            SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartAnimal2 == value, t => t.BlackFlowStartAnimal2 = value);
            UpdateBlackFlowSerpentOptions();
            OnPropertyChanged(nameof(RoguelikeBlackFlowHasSerpent));
        }
    }

    /// <summary>
    /// Gets a value indicating whether the start requirement includes a feathered serpent, which enables the serpent rewards.
    /// </summary>
    public bool RoguelikeBlackFlowHasSerpent =>
        RoguelikeBlackFlowStartAnimal1 == RoguelikeBlackFlowStartAnimal.FeatheredSerpent ||
        RoguelikeBlackFlowStartAnimal2 == RoguelikeBlackFlowStartAnimal.FeatheredSerpent;

    /// <summary>
    /// 刷开局奖励下拉框的选项。框内标签只显示名字，效果放在下拉项的悬浮提示里。
    /// </summary>
    public LocalizedObservableList<RoguelikeBlackFlowStartReward> RoguelikeBlackFlowStartRewardList { get; } = new(
        (RoguelikeBlackFlowStartReward.UnnumberedItem, "Roguelike.BlackFlow.RewardUnnumberedItem").WithTip("Roguelike.BlackFlow.RewardUnnumberedItem.Tip"),
        (RoguelikeBlackFlowStartReward.SurveyAdvance, "Roguelike.BlackFlow.RewardSurveyAdvance").WithTip("Roguelike.BlackFlow.RewardSurveyAdvance.Tip"),
        (RoguelikeBlackFlowStartReward.SpaceLease, "Roguelike.BlackFlow.RewardSpaceLease").WithTip("Roguelike.BlackFlow.RewardSpaceLease.Tip"),
        (RoguelikeBlackFlowStartReward.RegressionCompensation, "Roguelike.BlackFlow.RewardRegressionCompensation").WithTip("Roguelike.BlackFlow.RewardRegressionCompensation.Tip"),
        (RoguelikeBlackFlowStartReward.ForestRide, "Roguelike.BlackFlow.RewardForestRide").WithTip("Roguelike.BlackFlow.RewardForestRide.Tip"),
        (RoguelikeBlackFlowStartReward.NestParasitism, "Roguelike.BlackFlow.RewardNestParasitism").WithTip("Roguelike.BlackFlow.RewardNestParasitism.Tip"));

    public LocalizedObservableList<RoguelikeBlackFlowStartReward> RoguelikeBlackFlowSerpentRewardList { get; } = new(
        (RoguelikeBlackFlowStartReward.Pegasus, "Roguelike.BlackFlow.RewardPegasus").WithTip("Roguelike.BlackFlow.RewardPegasus.Tip"),
        (RoguelikeBlackFlowStartReward.Hippogriff, "Roguelike.BlackFlow.RewardHippogriff").WithTip("Roguelike.BlackFlow.RewardHippogriff.Tip"),
        (RoguelikeBlackFlowStartReward.Dragon, "Roguelike.BlackFlow.RewardDragon").WithTip("Roguelike.BlackFlow.RewardDragon.Tip"),
        (RoguelikeBlackFlowStartReward.Hydra, "Roguelike.BlackFlow.RewardHydra").WithTip("Roguelike.BlackFlow.RewardHydra.Tip"),
        (RoguelikeBlackFlowStartReward.BaiZe, "Roguelike.BlackFlow.RewardBaiZe").WithTip("Roguelike.BlackFlow.RewardBaiZe.Tip"),
        (RoguelikeBlackFlowStartReward.GoldenCrow, "Roguelike.BlackFlow.RewardGoldenCrow").WithTip("Roguelike.BlackFlow.RewardGoldenCrow.Tip"));

    private object[] SelectBlackFlowRewards(LocalizedObservableList<RoguelikeBlackFlowStartReward> options)
    {
        var value = GetTaskConfig<RoguelikeTask>().BlackFlowStartRewards;
        return options.Where(v => value.HasFlag(v.Value)).ToArray();
    }

    // 两个框各管自己那几位，另一框的勾选原样保留。羽蛇框禁用时勾选也保留，只是不生效。
    private void StoreBlackFlowRewards(object[] selected, RoguelikeBlackFlowStartReward mask)
    {
        var picked = selected.Cast<GenericCombinedData<RoguelikeBlackFlowStartReward>>().Aggregate(RoguelikeBlackFlowStartReward.None, (a, b) => a | b.Value);
        var value = (GetTaskConfig<RoguelikeTask>().BlackFlowStartRewards & ~mask) | (picked & mask);
        SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartRewards == value, t => t.BlackFlowStartRewards = value);
    }

    public object[] RoguelikeBlackFlowStartRewardSelectList
    {
        get => SelectBlackFlowRewards(RoguelikeBlackFlowStartRewardList);
        set => StoreBlackFlowRewards(value, ~RoguelikeBlackFlowStartReward.SerpentRewards);
    }

    public object[] RoguelikeBlackFlowSerpentRewardSelectList
    {
        get => SelectBlackFlowRewards(RoguelikeBlackFlowSerpentRewardList);
        set => StoreBlackFlowRewards(value, RoguelikeBlackFlowStartReward.SerpentRewards);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the BlackFlow start-run target runs keep only a first floor with Utopia.
    /// </summary>
    public bool RoguelikeBlackFlowStartUtopia
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowStartUtopia;
        set => SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartUtopia == value, t => t.BlackFlowStartUtopia = value);
    }

    // 实托邦从难度 6 开始出现；难度不切换时按游戏里当前的难度来，不做检查
    private static bool IsBlackFlowStartUtopiaDifficulty(int difficulty) => difficulty is -1 or >= 6;

    /// <summary>
    /// Gets a value indicating whether the selected difficulty allows the Utopia check.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeDifficulty))]
    public bool RoguelikeBlackFlowStartUtopiaAvailable => IsBlackFlowStartUtopiaDifficulty(RoguelikeDifficulty);

    /// <summary>
    /// Gets 凹第一层实托邦时目标局的支援选取顺序，十二种奖励全列，拖动改序。和开局动物、期望奖励的勾选无关。
    /// </summary>
    public ObservableCollection<BlackFlowStartRewardPriorityItemViewModel> RoguelikeBlackFlowStartRewardPriorityItems { get; private set => SetAndNotify(ref field, value); } = [];

    private void UpdateBlackFlowStartRewardPriorityItems()
    {
        var rewards = RoguelikeBlackFlowStartRewardList.Concat(RoguelikeBlackFlowSerpentRewardList).ToList();
        RoguelikeBlackFlowStartRewardPriorityItems.CollectionChanged -= BlackFlowStartRewardPriorityItems_CollectionChanged;
        RoguelikeBlackFlowStartRewardPriorityItems = [.. GetTaskConfig<RoguelikeTask>().BlackFlowStartRewardPriority.Select((reward, index) => new BlackFlowStartRewardPriorityItemViewModel(index, rewards.First(i => i.Value == reward)))];
        RoguelikeBlackFlowStartRewardPriorityItems.CollectionChanged += BlackFlowStartRewardPriorityItems_CollectionChanged;
    }

    private void BlackFlowStartRewardPriorityItems_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        ReindexBlackFlowStartRewardPriorityItems();
        StoreBlackFlowStartRewardPriority();
    }

    /// <summary>
    /// 拖动排序后重新计算并写入各项的列表序号
    /// </summary>
    private void ReindexBlackFlowStartRewardPriorityItems()
    {
        for (int index = 0; index < RoguelikeBlackFlowStartRewardPriorityItems.Count; ++index)
        {
            RoguelikeBlackFlowStartRewardPriorityItems[index].Index = index;
        }
    }

    private void StoreBlackFlowStartRewardPriority()
    {
        var order = RoguelikeBlackFlowStartRewardPriorityItems.Select(i => i.Reward.Value).ToList();
        SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartRewardPriority.SequenceEqual(order), t => t.BlackFlowStartRewardPriority = order);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the BlackFlow start-run preparation runs fly to the third floor with the Mechanic.
    /// </summary>
    public bool RoguelikeBlackFlowStartRunFastFlight
    {
        get => GetTaskConfig<RoguelikeTask>().BlackFlowStartRunFastFlight;
        set => SetTaskConfig<RoguelikeTask>(t => t.BlackFlowStartRunFastFlight == value, t => t.BlackFlowStartRunFastFlight = value);
    }

    /// <summary>
    /// Gets a value indicating whether the preparation runs use the fixed Mechanic start.
    /// 生效时刷开局使用分队显示替换后的值且不可修改，用户原来的设置保留，取消勾选后恢复；常规设置不受影响。
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeTheme), nameof(RoguelikeMode), nameof(RoguelikeBlackFlowStartRunFastFlight))]
    public bool RoguelikeBlackFlowStartRunFlightActive =>
        RoguelikeTheme == Theme.BlackFlow && RoguelikeMode == Mode.BlackFlowStartRun && RoguelikeBlackFlowStartRunFastFlight;

    /// <summary>
    /// Gets a value indicating whether the BlackFlow exp mode uses the fast flight start.
    /// 生效时开始任务传参固定为特勤分队、稳扎稳打与机械师开局，界面常规设置置灰保留，取消勾选后照常生效。
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeTheme), nameof(RoguelikeMode), nameof(RoguelikeBlackFlowFastFlight))]
    public bool RoguelikeBlackFlowExpFlightActive =>
        RoguelikeTheme == Theme.BlackFlow && RoguelikeMode == Mode.Exp && RoguelikeBlackFlowFastFlight;

    /// <summary>
    /// Gets or sets the preparation squad shown in the start-run settings.
    /// </summary>
    [PropertyDependsOn(nameof(RoguelikeTheme), nameof(RoguelikeMode), nameof(RoguelikeBlackFlowStartRunFastFlight), nameof(RoguelikeCollectibleModeSquad))]
    public string RoguelikeBlackFlowPrepSquadView
    {
        get => RoguelikeBlackFlowStartRunFlightActive ? AsstRoguelikeTask.BlackFlowFlightSquad : RoguelikeCollectibleModeSquad;
        set {
            if (!RoguelikeBlackFlowStartRunFlightActive)
            {
                RoguelikeCollectibleModeSquad = value;
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether the FindPlaytime target selection should be visible.
    /// </summary>
    public bool RoguelikeFindPlaytimeTargetVisible => RoguelikeMode == Mode.FindPlaytime && RoguelikeTheme == Theme.JieGarden;

    /// <summary>
    /// Gets or sets a value indicating whether to stop when max level has been achieved.
    /// </summary>
    public bool RoguelikeStopAtMaxLevel
    {
        get => GetTaskConfig<RoguelikeTask>().StopWhenLevelMax;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopWhenLevelMax == value, t => t.StopWhenLevelMax = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether delay abort until battle complete
    /// </summary>
    public bool RoguelikeDelayAbortUntilCombatComplete
    {
        get; set {
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.RoguelikeDelayAbortUntilCombatComplete = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.RoguelikeDelayAbortUntilCombatComplete;

    /// <summary>
    /// Gets or sets a value indicating whether start with seed
    /// </summary>
    public bool RoguelikeStartWithSeed
    {
        get => GetTaskConfig<RoguelikeTask>().StartWithSeed;
        set => SetTaskConfig<RoguelikeTask>(t => t.StartWithSeed == value, t => t.StartWithSeed = value);
    }

    /// <summary>
    /// Gets or sets the seed value for the roguelike task.
    /// </summary>
    public string RoguelikeSeed
    {
        get => GetTaskConfig<RoguelikeTask>().Seed;
        set => SetTaskConfig<RoguelikeTask>(t => t.Seed == value, t => t.Seed = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is RoguelikeTask)
        {
            UpdateRoguelikeParams();
            Refresh();
        }
    }

    private static string LocalizeBlackFlowProfile(string? profile) => profile switch {
        "ending1" => LocalizationHelper.GetString("Roguelike.BlackFlow.Ending1"),
        "ending2" => LocalizationHelper.GetString("Roguelike.BlackFlow.Ending2"),
        "ending3" => LocalizationHelper.GetString("Roguelike.BlackFlow.Ending3"),
        "investment" => LocalizationHelper.GetString("Roguelike.BlackFlow.Strategy.Investment"),
        "burn" or "burn_with_investment" => LocalizationHelper.GetString("Roguelike.BlackFlow.Strategy.Exp"),
        "baby_animal" or "baby_animal_floor3" or "start_run_animals" or "start_run_rewards" or "start_run_rewards_with_investment" => LocalizationHelper.GetString("Roguelike.BlackFlow.Strategy.StartRun"),
        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.StrategyUnknown"),
    };

    private static string LocalizeBlackFlowIdentifier(string prefix, string? identifier, string fallbackKey)
    {
        if (string.IsNullOrWhiteSpace(identifier))
        {
            return LocalizationHelper.GetString(fallbackKey);
        }

        var suffix = string.Concat(
            identifier.Split('_', StringSplitOptions.RemoveEmptyEntries)
                .Select(part => $"{char.ToUpperInvariant(part[0])}{part[1..]}"));
        return LocalizationHelper.TryGetString(prefix + suffix, out var localized)
            ? localized
            : LocalizationHelper.GetString(fallbackKey);
    }

    private static string LocalizeBlackFlowNodeType(string? nodeType) => nodeType switch {
        "empty" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeEmpty"),
        "battle_normal" or "combat" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeCombat"),
        "battle_elite" or "emergency_combat" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeEmergencyCombat"),
        "battle_boss" or "boss" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeBoss"),
        "shop" or "battle_shop" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeBattleShop"),
        "scrap_shop" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeScrapShop"),
        "incident" or "encounter" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeEncounter"),
        "hide_invisible" or "mysterious_presage" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeMysteriousPresage"),
        "hide_battle" or "ferocious_presage" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeFerociousPresage"),
        "expedition" or "scout" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeScout"),
        "battle_savage" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeResidentStronghold"),
        "duel" or "face_off" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeDuel"),
        "employ" or "emergency_aid" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeEmergencyAid"),
        "rest" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeRest"),
        "light" or "feather_point" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeFeatherPoint"),
        "door" or "winding_passage" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeWindingPassage"),
        "sacrifice" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeSacrifice"),
        "wish" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeWish"),
        "portal" or "bosky_passage" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeBoskyPassage"),
        "resident_stronghold" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeResidentStronghold"),
        "final" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeFinal"),
        "fate" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeFate"),
        "evacuate" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeEvacuate"),
        "teleporter" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeTeleporter"),
        "unclassified" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeUnknown"),
        "other" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeOther"),
        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.NodeUnknown"),
    };

    private static string LocalizeBlackFlowMilestoneStatus(string? status) => status switch {
        "available" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MilestoneStatusAvailable"),
        "satisfied" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MilestoneStatusSatisfied"),
        "missed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MilestoneStatusMissed"),
        "impossible" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MilestoneStatusImpossible"),
        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MilestoneStatusUnknown"),
    };

    private static string LocalizeBlackFlowStrategyOutcome(string? outcome) => outcome switch {
        "start_run_prepared" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.StartRunPrepared"),
        "start_run_utopia_missed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.StartRunUtopiaMissed"),
        "investment_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeInvestmentCompleted"),
        "investment_missed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeInvestmentMissed"),
        "burn_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeFloor3RouteCompleted"),
        "baby_cultivation_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeBabyCultivationCompleted"),
        "baby_cultivation_target_missed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeBabyCultivationTargetMissed"),
        "ending_prerequisite_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeEndingPrerequisiteFailed"),
        "strategy_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeStrategyCompleted"),
        "page_recovery_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomePageRecoveryFailed"),
        "ending2_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeEnding2Completed"),
        "ending3_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeEnding3Completed"),
        "ending2_prerequisite_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeEnding2PrerequisiteFailed"),
        "ending3_prerequisite_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeEnding3PrerequisiteFailed"),
        "baby_cultivation_unfinished" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeBabyCultivationUnfinished"),
        "task_event_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeTaskEventFailed"),
        "perception_port_missing" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomePerceptionPortMissing"),
        "map_rebuild_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMapRebuildFailed"),
        "planning_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomePlanningFailed"),
        "transaction_proposal_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeTransactionProposalFailed"),
        "move_preview_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMovePreviewFailed"),
        "move_preview_rejected" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMovePreviewRejected"),
        "move_confirmation_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMoveConfirmationFailed"),
        "post_move_validation_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomePostMoveValidationFailed"),
        "planning_retry_exhausted" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomePlanningRetryExhausted"),
        "state_machine_dead_end" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeStateMachineDeadEnd"),
        "map_recovery_exhausted" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMapRecoveryExhausted"),
        "floor_recognition_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeFloorRecognitionFailed"),
        "movement_inventory_observation_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMovementInventoryFailed"),
        "movement_selection_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeMovementSelectionFailed"),
        "node_dispatch_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeNodeDispatchFailed"),
        "node_result_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeNodeResultFailed"),
        "internal_failure" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeInternalFailure"),
        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.OutcomeUnknown"),
    };

    private static string LocalizeBlackFlowTerminationReason(string? reason) => reason switch {
        "start_run_prepared" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.StartRunPreparedReason"),
        "first_floor_without_utopia" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.StartRunUtopiaMissedReason"),
        "investment_finished" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationInvestmentFinished"),
        "investment_shop_window_closed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationInvestmentShopWindowClosed"),
        "third_floor_reached" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationFloor3Reached"),
        "cultivation_result_reported" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationCultivationReported"),
        "cultivation_target_obtained" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationCultivationTargetObtained"),
        "cultivation_target_not_obtained" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationCultivationTargetNotObtained"),
        "floor1_shop_has_no_seed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationFloor1ShopNoSeed"),
        "mandatory_prerequisite_missed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationMandatoryPrerequisiteMissed"),
        "strategy_terminal_reached" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationStrategyTerminalReached"),
        "node_page_could_not_return_to_map" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationNodePageRecoveryFailed"),
        "ending2_terminal_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationEnding2Completed"),
        "ending3_terminal_completed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationEnding3Completed"),
        "fifth_floor_reached_without_valid_sandtable_payment" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationEnding2PrerequisiteMissing"),
        "fifth_floor_reached_without_special_device" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationEnding3RelicMissing"),
        "third_floor_has_no_portal" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationNoBoskyPassage"),
        "third_floor_action_points_exhausted" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationActionPointsExhaustedBeforeCultivation"),
        "scrap_shop_never_reached" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationScrapShopNeverReached"),
        "map recovery port is unavailable" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationRecoveryPortUnavailable"),
        "BlackFlow perception and task port is not attached" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationPerceptionPortUnavailable"),
        "map rebuild failed twice" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationMapRebuildFailedTwice"),
        "preview replanning exceeded the finite candidate limit" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationPlanningRetryExhausted"),
        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.TerminationUnknown"),
    };

    public void ProcSubTaskMsg(AsstMsg msg, AsstSubTaskMsg? details)
    {
        if (msg != AsstMsg.SubTaskExtraInfo)
        {
            return;
        }

        var subTaskDetails = details?.Details;
        switch (details?.What ?? string.Empty)
        {
            case "RoguelikeInvestmentReachFull":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.InvestmentReachFull"), UiLogColor.Info);
                AchievementTrackerHelper.Instance.SetProgress(AchievementIds.RoguelikeGoldMax, 999);
                break;

            case "RoguelikeInvestmentReachLimit":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.InvestmentReachLimit", subTaskDetails!["limit"]), UiLogColor.Info);
                break;

            case "RoguelikeInvestment":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.Investment", subTaskDetails!["count"], subTaskDetails["total"], subTaskDetails["deposit"]), UiLogColor.Info);
                AchievementTrackerHelper.Instance.SetProgress(AchievementIds.RoguelikeGoldMax, (int)subTaskDetails["deposit"]!);
                break;

            // 肉鸽结算
            case "RoguelikeSettlement":
                {
                    var report = subTaskDetails;
                    var pass = (bool)report!["game_pass"]!;
                    var difficulty = report["difficulty"]?.ToObject<int>() ?? -1;
                    if (difficulty < 0 || difficulty > GetMaxDifficultyForTheme(RoguelikeTheme))
                    {
                        // 最后一位是 0 可能是识别错了，打个补丁
                        if (difficulty % 10 == 0)
                        {
                            difficulty /= 10;
                        }
                        else
                        {
                            difficulty = -1;
                        }
                    }

                    var roguelikeInfo = LocalizationHelper.GetStringFormat(
                        "Asst.Roguelike.Settlement",
                        pass ? "✓" : "✗",
                        report["floor"],
                        report["step"],
                        report["combat"],
                        report["emergency"],
                        report["boss"],
                        report["recruit"],
                        report["collection"],
                        difficulty,
                        report["score"],
                        report["exp"],
                        report["skill"]);

                    if (pass)
                    {
                        if (difficulty >= 4)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.RoguelikeN04);
                        }

                        if (difficulty >= 8)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.RoguelikeN08);
                        }

                        if (difficulty >= 12)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.RoguelikeN12);
                        }

                        if (difficulty >= 15)
                        {
                            AchievementTrackerHelper.Instance.Unlock(AchievementIds.RoguelikeN15);
                        }
                    }

                    Instances.TaskQueueViewModel.AddLog(roguelikeInfo, UiLogColor.Message, updateCardImage: true);
                    break;
                }

            case "RoguelikeCombatEnd":
                // 肉鸽战斗结束，无论成功与否
                Instances.TaskQueueViewModel.RoguelikeInCombatAndShowWait = false;
                break;

            case "RoguelikeEvent":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.Event") + $" {subTaskDetails!["name"]}", UiLogColor.EventIS);
                break;

            case "RoguelikeEncounterOptions":
                var options = (subTaskDetails!["options"]! as JArray) ?? [];
                var logLines = new List<string>
                {
                    LocalizationHelper.GetStringFormat("Asst.Roguelike.EncounterOptions", options.Count),
                };

                foreach (var option in options)
                {
                    string messageKey = option["enabled"]!.Value<bool>() ? "Asst.Roguelike.EncounterEnabledOption" : "Asst.Roguelike.EncounterDisabledOption";
                    var text = option["text"]!.ToString();
                    logLines.Add(LocalizationHelper.GetStringFormat(messageKey, text));
                }

                Instances.TaskQueueViewModel.AddLog(string.Join("\n", logLines), UiLogColor.EventIS, updateCardImage: true);
                break;

            case "RoguelikeEventSelected":
                {
                    var optionText = subTaskDetails?["option_text"]?.ToString() ?? string.Empty;
                    var usedFallback = subTaskDetails?["used_fallback"]?.Value<bool>() ?? false;

                    // 理由取自策略文件里命中规则的 description，随策略配置，兜底选择不显示。
                    var reason = subTaskDetails?["rule_description"]?.ToString();
                    var message = usedFallback
                        ? LocalizationHelper.GetStringFormat("Asst.Roguelike.EventSelectedFallback", optionText)
                        : string.IsNullOrWhiteSpace(reason)
                            ? LocalizationHelper.GetStringFormat("Asst.Roguelike.EventSelected", optionText)
                            : LocalizationHelper.GetStringFormat("Asst.Roguelike.EventSelectedWithReason", optionText, reason);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.EventIS);
                    break;
                }

            case "BlackFlowRoutingDecision":
                {
                    var floor = subTaskDetails?["floor"]?.Value<int>() ?? 0;
                    var before = subTaskDetails?["action_points_before"]?.Value<int>() ?? 0;
                    var after = subTaskDetails?["action_points_after"]?.Value<int>() ?? 0;
                    var movement = subTaskDetails?["movement"]?.ToString() == "walk"
                        ? LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MovementWalk")
                        : LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.MovementProcessing");
                    var nodeName = subTaskDetails?["node_name"]?.ToString();
                    if (string.IsNullOrWhiteSpace(nodeName))
                    {
                        nodeName = LocalizeBlackFlowNodeType(subTaskDetails?["node_type"]?.ToString());
                    }
                    var margin = subTaskDetails?["safety_margin"]?.Value<int>() ?? 0;
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetStringFormat(
                            "Asst.Roguelike.BlackFlow.RoutingDecision",
                            floor,
                            before,
                            after,
                            movement,
                            nodeName,
                            margin),
                        UiLogColor.EventIS);
                    break;
                }

            case "BlackFlowRoutingWarning":
                {
                    var warning = subTaskDetails?["code"]?.ToString() switch {
                        "map_rebuild_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningMapRebuildFailed"),
                        "page_recovery_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningPageRecoveryFailed"),
                        "preview_cost_changed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningPreviewCostChanged"),
                        "route_blocked" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningRouteBlocked"),
                        "insufficient_action_points" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningInsufficientActionPoints"),
                        "target_state_changed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningTargetStateChanged"),
                        "target_unreachable" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningTargetUnreachable"),
                        "inferred_edge_selected" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningInferredEdge"),
                        "post_move_mismatch" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningPostMoveMismatch"),
                        "identity_conflict" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningIdentityConflict"),
                        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.WarningUnknown"),
                    };
                    Instances.TaskQueueViewModel.AddLog(warning, UiLogColor.Warning);
                    break;
                }

            case "BlackFlowMilestoneChanged":
                {
                    var statusCode = subTaskDetails?["status"]?.ToString();
                    if (statusCode == "inactive")
                    {
                        break;
                    }
                    var milestone = LocalizeBlackFlowIdentifier(
                        "Asst.Roguelike.BlackFlow.Milestone",
                        subTaskDetails?["milestone_id"]?.ToString(),
                        "Asst.Roguelike.BlackFlow.MilestoneUnknown");
                    var status = LocalizeBlackFlowMilestoneStatus(statusCode);
                    var color = statusCode switch {
                        "satisfied" => UiLogColor.Success,
                        "missed" or "impossible" => UiLogColor.Warning,
                        _ => UiLogColor.Info,
                    };
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.MilestoneChanged", milestone, status),
                        color);
                    break;
                }

            case "BlackFlowInventoryCleanup":
                {
                    var discardedName = subTaskDetails?["name"]?.ToString() ?? string.Empty;
                    var (message, color) = subTaskDetails?["status"]?.ToString() switch {
                        "started" =>
                            (LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.InventoryCleanupStarted"), UiLogColor.Warning),
                        "discarded" =>
                            (LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.InventoryCleanupDiscarded", discardedName), UiLogColor.Info),
                        "completed" =>
                            (LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.InventoryCleanupCompleted"), UiLogColor.Success),
                        "failed" =>
                            (LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.InventoryCleanupFailed"), UiLogColor.Error),
                        _ => (string.Empty, UiLogColor.Trace),
                    };
                    if (!string.IsNullOrEmpty(message))
                    {
                        Instances.TaskQueueViewModel.AddLog(message, color);
                    }

                    break;
                }

            case "BlackFlowScrapTradeAborted":
                {
                    var item = subTaskDetails?["item"]?.ToString() ?? string.Empty;
                    var message = subTaskDetails?["reason"]?.ToString() switch {
                        "market_unavailable" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.ScrapTradeAbortedMarketUnavailable"),
                        "inventory_incomplete" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.ScrapTradeAbortedInventoryIncomplete"),
                        "page_switch_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.ScrapTradeAbortedPageSwitchFailed"),
                        "sale_search_failed" => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.ScrapTradeAbortedSaleSearchFailed"),
                        "sell_back_search_failed" => LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.ScrapTradeAbortedSellBackSearchFailed", item),
                        "sale_failed" => LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.ScrapTradeAbortedSaleFailed", item),
                        _ => LocalizationHelper.GetString("Asst.Roguelike.BlackFlow.ScrapTradeAbortedUnknown"),
                    };
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.Warning);
                    break;
                }

            case "BlackFlowStartRun":
                {
                    var status = subTaskDetails?["status"]?.ToString();
                    var (key, color) = status switch {
                        "preparing" => ("Asst.Roguelike.BlackFlow.StartRunPreparing", UiLogColor.Info),
                        "target_run" => ("Asst.Roguelike.BlackFlow.StartRunTargetRun", UiLogColor.Info),
                        "reward_missed" => ("Asst.Roguelike.BlackFlow.StartRunRewardMissed", UiLogColor.Info),
                        "reward_picked" => ("Asst.Roguelike.BlackFlow.StartRunRewardPicked", UiLogColor.Info),
                        "completed" => ("Asst.Roguelike.BlackFlow.StartRunCompleted", UiLogColor.Success),
                        "reward_recognition_failed" => ("Asst.Roguelike.BlackFlow.StartRunRewardRecognitionFailed", UiLogColor.Error),
                        "reward_page_missing" => ("Asst.Roguelike.BlackFlow.StartRunRewardPageMissing", UiLogColor.Error),
                        "reward_unrecognized" => ("Asst.Roguelike.BlackFlow.StartRunRewardUnrecognized", UiLogColor.Error),
                        _ => (string.Empty, UiLogColor.Info),
                    };
                    if (key.Length > 0)
                    {
                        // 支援页认出的奖励名随状态一起显示，方便看出有没有认错
                        var message = LocalizationHelper.GetString(key);
                        var rewards = subTaskDetails?["rewards"]?.Values<string>().ToList();
                        if (rewards is { Count: > 0 })
                        {
                            message += "\n" + LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.StartRunRewardsSeen", string.Join("、", rewards));
                        }

                        Instances.TaskQueueViewModel.AddLog(message, color);
                    }

                    break;
                }

            case "BlackFlowStrategyStarted":
                {
                    var profile = LocalizeBlackFlowProfile(subTaskDetails?["profile"]?.ToString());
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.StrategyStarted", profile),
                        UiLogColor.Info);
                    break;
                }

            case "BlackFlowStrategyResult":
                {
                    var outcome = LocalizeBlackFlowStrategyOutcome(subTaskDetails?["outcome"]?.ToString());
                    var reason = LocalizeBlackFlowTerminationReason(subTaskDetails?["termination_reason"]?.ToString());
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetStringFormat("Asst.Roguelike.BlackFlow.StrategyResult", outcome, reason),
                        subTaskDetails?["succeeded"]?.Value<bool>() == true ? UiLogColor.Info : UiLogColor.Warning);
                    break;
                }
            case "BoskyPassageNode":
                {
                    var nodeType = subTaskDetails!["node_type"]?.ToString();
                    var (localizedNodeType, logColor) = nodeType switch {
                        "Omissions" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Omissions"), UiLogColor.BoskyOmissionsIS),
                        "Legend" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Legend"), UiLogColor.BoskyLegendIS),
                        "OldShop" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.OldShop"), UiLogColor.BoskyOldShopIS),
                        "YiTrader" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.YiTrader"), UiLogColor.TraderIS),
                        "Scheme" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Scheme"), UiLogColor.BoskySchemeIS),
                        "Playtime" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Playtime"), UiLogColor.BoskyPlaytimeIS),
                        "Doubts" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Doubts"), UiLogColor.BoskyDoubtsIS),
                        "Disaster" => (LocalizationHelper.GetString("Asst.Roguelike.Bosky.Disaster"), UiLogColor.EmergencyIS),
                        _ => (nodeType ?? "Unknown", UiLogColor.EventIS),
                    };

                    Instances.TaskQueueViewModel.AddLog(localizedNodeType, logColor);
                    break;
                }

            case "RoguelikeCoppersRecognitionError":
                {
                    var recognizedName = subTaskDetails!["recognized_name"]?.ToString() ?? "Unknown";
                    var message = LocalizationHelper.GetStringFormat("Asst.Roguelike.CoppersRecognitionError", recognizedName);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.Error);
                    break;
                }

            case "RoguelikeCoppersExchangeInfo":
                {
                    var toDiscard = subTaskDetails!["to_discard"]?.ToString() ?? "Unknown";
                    var toPickup = subTaskDetails["to_pickup"]?.ToString() ?? "Unknown";
                    var message = LocalizationHelper.GetStringFormat("Asst.Roguelike.CoppersExchange", toDiscard, toPickup);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.EventIS);
                    break;
                }

            case "EncounterOcrError":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.EncounterOcrError"), UiLogColor.Error);
                break;

            case "RoguelikeJieGardenTargetFound":
                {
                    var targetSubtype = subTaskDetails!["target_subtype"]?.ToString();
                    var localizedTarget = targetSubtype switch {
                        "Ling" => LocalizationHelper.GetString("Roguelike.Playtime.Ling"),
                        "Shu" => LocalizationHelper.GetString("Roguelike.Playtime.Shu"),
                        "Nian" => LocalizationHelper.GetString("Roguelike.Playtime.Nian"),
                        _ => targetSubtype ?? "Unknown",
                    };
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.JieGarden.TargetFound", localizedTarget), UiLogColor.Success);
                    break;
                }

            case "FoldartalGainOcrNextLevel":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.FoldartalGainOcrNextLevel") + $" {subTaskDetails!["foldartal"]}");
                break;

            case "MonthlySquadCompleted":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.MonthlySquadCompleted"), UiLogColor.RareOperator);
                break;

            case "DeepExplorationCompleted":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Asst.Roguelike.DeepExplorationCompleted"), UiLogColor.RareOperator);
                break;

            case "RoguelikeCollapsalParadigms":
                string deepen_or_weaken_str = subTaskDetails!["deepen_or_weaken"]?.ToString() ?? "Unknown";
                if (!int.TryParse(deepen_or_weaken_str, out int deepen_or_weaken))
                {
                    break;
                }

                string cur = subTaskDetails["cur"]?.ToString() ?? "UnKnown";
                string prev = subTaskDetails["prev"]?.ToString() ?? "UnKnown";
                if (deepen_or_weaken == 1 && prev == string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.GainParadigm", cur), UiLogColor.Info);
                }
                else if (deepen_or_weaken == 1 && prev != string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.DeepenParadigm", cur, prev), UiLogColor.Info);
                }
                else if (deepen_or_weaken == -1 && cur == string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.LoseParadigm", string.Empty, prev), UiLogColor.Info);
                }
                else if (deepen_or_weaken == -1 && cur != string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("Asst.Roguelike.WeakenParadigm", cur, prev), UiLogColor.Info);
                }

                break;
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not RoguelikeTask roguelike)
            {
                return (null, []);
            }

            // 仅在此处过滤，StartingOpers 配置本体不动，重开开关/回填断点即恢复：
            // 开关关闭时只保留第 1 顺位；顺位链条在首个空名处截断，其后残留值不传参
            var startingOpers = (roguelike.UseAdditionalStartingOpers
                    ? roguelike.StartingOpers
                    : roguelike.StartingOpers.Take(1))
                .TakeWhile(i => !string.IsNullOrEmpty(i.Name))
                .ToList();

            bool isPallasStarter = startingOpers.Any(i =>
                string.Equals(
                    DataHelper.GetCharacterByNameOrAlias(i.Name)?.CodeName,
                    "pallas",
                    StringComparison.OrdinalIgnoreCase));
            bool roguelikeSquadIsProfessional = roguelike.Mode == Mode.Collectible && roguelike.Theme != Theme.Phantom && roguelike.Squad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";
            bool roguelikeSquadIsFoldartal = roguelike.Mode == Mode.Collectible && roguelike.Theme == Theme.Sami && roguelike.Squad == "生活至上分队";
            var task = new AsstRoguelikeTask() {
                Theme = roguelike.Theme,
                Mode = roguelike.Mode,
                Starts = roguelike.StartCount,
                Difficulty = roguelike.Difficulty,
                Squad = roguelike.Squad,
                Roles = roguelike.Roles,

                CoreCharList = [.. startingOpers.Select(i => new AsstRoguelikeCoreChar
                    {
                        Name = DataHelper.GetCharacterByNameOrAlias(i.Name)?.Name ?? i.Name,
                        UseSupport = i.UseSupport,
                    })],
                UseSupportNonFriend = roguelike.UseSupportNonFriend,

                InvestmentEnabled = roguelike.Investment,
                InvestmentCount = roguelike.Mode == Mode.Investment ? roguelike.InvestCount : int.MaxValue,
                InvestmentStopWhenFull = roguelike.StopWhenDepositFull && roguelike.Mode == Mode.Investment,
                InvestmentWithMoreScore = roguelike.Theme != Theme.BlackFlow && roguelike.InvestWithMoreScore && roguelike.Mode == Mode.Investment,
                RefreshTraderWithDice = roguelike.Theme == Theme.Mizuki && roguelike.RefreshTraderWithDice,

                StopAtFinalBoss = roguelike.StopAtFinalBoss,
                StopAtMaxLevel = roguelike.StopWhenLevelMax,

                // 刷开局
                CollectibleModeSquad = roguelike.SquadCollectible,
                CollectibleModeShopping = roguelike.CollectibleShopping,
                StartWithEliteTwo = roguelike.StartWithEliteTwo && roguelikeSquadIsProfessional && roguelike.Theme is Theme.Mizuki or Theme.Sami,
                StartWithEliteTwoNonBattle = roguelike.StartWithEliteTwoOnly && roguelike.Theme is Theme.Mizuki or Theme.Sami,

                // 月度小队
                MonthlySquadAutoIterate = roguelike.MonthlySquadAutoIterate,
                MonthlySquadCheckComms = roguelike.MonthlySquadCheckComms,

                // 深入探索
                DeepExplorationAutoIterate = roguelike.DeepExplorationAutoIterate,

                // 刷常乐节点
                FindPlaytimeTarget = roguelike.FindPlaytimeTarget, // 等待添加到 RoguelikeTask

                BlackFlowCultivationTarget = roguelike.BlackFlowCultivationTarget,
                BlackFlowEnding = roguelike.BlackFlowEnding,
                BlackFlowFastFlight = roguelike.BlackFlowFastFlight,
                BlackFlowStartAnimal1 = roguelike.BlackFlowStartAnimal1,
                BlackFlowStartAnimal2 = roguelike.BlackFlowStartAnimal2,
                BlackFlowStartRewards = roguelike.BlackFlowStartRewards,
                BlackFlowStartRunFastFlight = roguelike.BlackFlowStartRunFastFlight,
                BlackFlowStartUtopia = roguelike.BlackFlowStartUtopia && IsBlackFlowStartUtopiaDifficulty(roguelike.Difficulty),
                BlackFlowStartRewardPriority = roguelike.BlackFlowStartRewardPriority,

                SamiFirstFloorFoldartal = roguelike.Theme == Theme.Sami && roguelike.Mode == Mode.Collectible && roguelike.SamiFirstFloorFoldartal,
                SamiStartFloorFoldartal = roguelike.SamiFirstFloorFoldartals,
                SamiNewSquad2StartingFoldartal = roguelike.SamiNewSquad2StartingFoldartal && roguelikeSquadIsFoldartal,
                SamiNewSquad2StartingFoldartals = [.. roguelike.SamiNewSquad2StartingFoldartals.Split(';').Where(i => !string.IsNullOrEmpty(i)).Take(3)],

                ExpectedCollapsalParadigms = [.. roguelike.ExpectedCollapsalParadigms.Split(';').Where(i => !string.IsNullOrEmpty(i))],

                StartWithSeed = roguelike.StartWithSeed ? roguelike.Seed : null,
            };

            bool squadIsProfessional = roguelike.Mode == Mode.Collectible && roguelike.Theme != Theme.Phantom && roguelike.Squad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";
            bool roguelikeOnlyStartWithEliteTwo = roguelike.StartWithEliteTwoOnly && roguelike.StartWithEliteTwo && squadIsProfessional;

            if (roguelike.Mode == Mode.Collectible && !roguelikeOnlyStartWithEliteTwo)
            {
                var rewardKeys = new Dictionary<RoguelikeCollectibleAward, string>
                {
                    { RoguelikeCollectibleAward.HotWater, "hot_water" },
                    { RoguelikeCollectibleAward.Shield, "shield" },
                    { RoguelikeCollectibleAward.Ingot, "ingot" },
                    { RoguelikeCollectibleAward.Hope, "hope" },
                    { RoguelikeCollectibleAward.Random, "random" },
                    { RoguelikeCollectibleAward.Key, "key" },
                    { RoguelikeCollectibleAward.Dice, "dice" },
                    { RoguelikeCollectibleAward.Idea, "ideas" },
                    { RoguelikeCollectibleAward.Ticket, "ticket" },
                };

                foreach (var reward in rewardKeys.Keys)
                {
                    task.CollectibleModeStartRewards[rewardKeys[reward]] = roguelike.CollectibleStartAwards.HasFlag(reward);
                }
            }

            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => UnlockPallasStarterIfNeeded(FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Roguelike, task))),
                _ => (null, []),
            };

            (bool? IsSuccess, IEnumerable<int> TaskId) UnlockPallasStarterIfNeeded((bool? IsSuccess, IEnumerable<int> TaskId) result)
            {
                if (result.IsSuccess == true && isPallasStarter)
                {
                    AchievementTrackerHelper.Instance.Unlock(AchievementIds.PallasStarter);
                }

                return result;
            }
        }
    }

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        RoguelikeThemeList.RefreshLocalization();
        RoguelikeFindPlaytimeTargetList.RefreshLocalization();
        RoguelikeBlackFlowEndingList.RefreshLocalization();
        RoguelikeBlackFlowStartAnimal1List.RefreshLocalization();
        RoguelikeBlackFlowStartAnimal2List.RefreshLocalization();
        RoguelikeBlackFlowStartRewardList.RefreshLocalization();
        RoguelikeBlackFlowSerpentRewardList.RefreshLocalization();
        UpdateRoguelikeDifficultyList();
        UpdateRoguelikeModeList();
        UpdateRoguelikeRolesList();
        UpdateRoguelikeSquadList();
        OnPropertyChanged(nameof(RoguelikeUseSupportUnitText));
        OnPropertyChanged(nameof(RoguelikeCoreChar2UseSupportText));
        OnPropertyChanged(nameof(RoguelikeCoreChar3UseSupportText));
    }
}
