// <copyright file="RoguelikeSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;
using Mode = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeMode;
using RoguelikeBoskySubNodeType = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeBoskySubNodeType;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeTheme;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class RoguelikeSettingsUserControlModel : TaskSettingsViewModel, RoguelikeSettingsUserControlModel.ISerialize
{
    static RoguelikeSettingsUserControlModel()
    {
        Instance = new();
    }

    public static RoguelikeSettingsUserControlModel Instance { get; }

    public void InitRoguelike()
    {
        GenerateRoguelikeThemeList();
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
    }

    private void GenerateRoguelikeThemeList()
    {
        RoguelikeThemeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeThemePhantom"), Value = Theme.Phantom });
        RoguelikeThemeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeThemeMizuki"), Value = Theme.Mizuki });
        RoguelikeThemeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeThemeSami"), Value = Theme.Sami });
        RoguelikeThemeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeThemeSarkaz"), Value = Theme.Sarkaz });
        RoguelikeThemeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeThemeJieGarden"), Value = Theme.JieGarden });
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
                -1 => LocalizationHelper.GetString("NotSwitch") + " (-1)",
                int.MaxValue => $"MAX ({maxThemeDifficulty})",
                0 => "MIN (0)",
                _ => value.ToString(),
            };
            RoguelikeDifficultyList.Add(new() { Display = display, Value = value });
        }

        // 验证当前选中的难度是否在新列表中
        RoguelikeDifficulty = RoguelikeDifficultyList.Any(item => item.Value == RoguelikeDifficulty) ? difficulty : -1;
    }

    private static int GetMaxDifficultyForTheme(Theme theme) => theme switch {
        Theme.Phantom => 15,
        Theme.Mizuki => 18,
        Theme.Sami => 15,
        Theme.Sarkaz => 18,
        Theme.JieGarden => 18,
        _ => 20,
    };

    private void UpdateRoguelikeModeList()
    {
        var roguelikeMode = RoguelikeMode;

        switch (RoguelikeTheme)
        {
            case Theme.JieGarden:
                RoguelikeModeList = [
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyExp"), Value = Mode.Exp },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyGold"), Value = Mode.Investment },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyLastReward"), Value = Mode.Collectible },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyFindPlaytime"), Value = Mode.FindPlaytime },
                ];
                break;

            default:
                RoguelikeModeList =
                [
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyExp"), Value = Mode.Exp },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyGold"), Value = Mode.Investment },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyLastReward"), Value = Mode.Collectible },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyMonthlySquad"), Value = Mode.Squad },
                    new() { Display = LocalizationHelper.GetString("RoguelikeStrategyDeepExploration"), Value = Mode.Exploration },
                ];

                if (RoguelikeTheme == Theme.Sami)
                {
                    RoguelikeModeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStrategyCollapse"), Value = Mode.CLP_PDS });
                }

                break;
        }

        RoguelikeMode = RoguelikeModeList.Any(x => x.Value == roguelikeMode) ? roguelikeMode : RoguelikeModeList.First().Value;
    }

    private void UpdateRoguelikeRolesList()
    {
        var roguelikeRoles = RoguelikeRoles;
        RoguelikeRolesList =
        [
            new() { Display = LocalizationHelper.GetString("FirstMoveAdvantage"), Value = "先手必胜" },
            new() { Display = LocalizationHelper.GetString("SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
            new() { Display = LocalizationHelper.GetString("OvercomingYourWeaknesses"), Value = "取长补短" },
        ];

        switch (RoguelikeTheme)
        {
            case Theme.JieGarden:
                RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("FlexibleDeployment"), Value = "灵活部署" });
                RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("Unbreakable"), Value = "坚不可摧" });
                break;
        }

        RoguelikeRolesList.Add(new() { Display = LocalizationHelper.GetString("AsYourHeartDesires"), Value = "随心所欲" });

        RoguelikeRoles = RoguelikeRolesList.Any(x => x.Value == roguelikeRoles) ? roguelikeRoles : "稳扎稳打";
    }

    private readonly Dictionary<string, List<(string Key, string Value)>> _squadDictionary = new() {
        ["Phantom_Default"] =
        [
            ("GatheringSquad", "集群分队"),
            ("SpearheadSquad", "矛头分队"),
            ("ResearchSquad", "研究分队"),
        ],
        ["Mizuki_Default"] =
        [
            ("GatheringSquad", "集群分队"),
            ("SpearheadSquad", "矛头分队"),
            ("IS2NewSquad1", "心胜于物分队"),
            ("IS2NewSquad2", "物尽其用分队"),
            ("IS2NewSquad3", "以人为本分队"),
            ("ResearchSquad", "研究分队"),
        ],
        ["Sami_Default"] =
        [
            ("GatheringSquad", "集群分队"),
            ("SpearheadSquad", "矛头分队"),
            ("IS3NewSquad1", "永恒狩猎分队"),
            ("IS3NewSquad2", "生活至上分队"),
            ("IS3NewSquad3", "科学主义分队"),
            ("IS3NewSquad4", "特训分队"),
        ],
        ["Sarkaz_1"] =
        [
            ("GatheringSquad", "集群分队"),
            ("SpearheadSquad", "矛头分队"),
            ("IS4NewSquad2", "博闻广记分队"),
            ("IS4NewSquad3", "蓝图测绘分队"),
            ("IS4NewSquad6", "点刺成锭分队"),
            ("IS4NewSquad7", "拟态学者分队"),
        ],
        ["Sarkaz_Default"] =
        [
            ("GatheringSquad", "集群分队"),
            ("SpearheadSquad", "矛头分队"),
            ("IS4NewSquad1", "魂灵护送分队"),
            ("IS4NewSquad2", "博闻广记分队"),
            ("IS4NewSquad3", "蓝图测绘分队"),
            ("IS4NewSquad4", "因地制宜分队"),
            ("IS4NewSquad5", "异想天开分队"),
            ("IS4NewSquad6", "点刺成锭分队"),
            ("IS4NewSquad7", "拟态学者分队"),
            ("IS4NewSquad8", "专业人士分队"),
        ],
        ["JieGarden_Default"] =
        [
            ("SpecialForceSquad", "特勤分队"),
            ("IS5NewSquad1", "高台突破分队"),
            ("IS5NewSquad2", "地面突破分队"),
            ("IS5NewSquad3", "游客分队"),
            ("IS5NewSquad4", "司岁台分队"),
            ("IS5NewSquad5", "天师府分队"),
            ("IS5NewSquad6", "花团锦簇分队"),
            ("IS5NewSquad7", "棋行险着分队"),
            ("IS5NewSquad8", "岁影回音分队"),
            ("IS5NewSquad9", "代理人分队"),
            ("IS5NewSquad10", "知学分队"),
            ("IS5NewSquad11", "商贾分队"),
        ],
    };

    // 通用分队
    private readonly List<(string Key, string Value)> _commonSquads =
    [
        ("LeaderSquad", "指挥分队"),
        ("SupportSquad", "后勤分队"),
        ("TacticalAssaultOperative", "突击战术分队"),
        ("TacticalFortificationOperative", "堡垒战术分队"),
        ("TacticalRangedOperative", "远程战术分队"),
        ("TacticalDestructionOperative", "破坏战术分队"),
        ("First-ClassSquad", "高规格分队"),
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
            RoguelikeSquadList.Add(new() { Display = LocalizationHelper.GetString(key), Value = value });
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

                    if (!DataHelper.IsCharacterAvailableInClient(name, SettingsViewModel.GameSettings.ClientType))
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
    public List<GenericCombinedData<Theme>> RoguelikeThemeList { get; } = [];

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
    /// Gets or sets the roguelike core character.
    /// </summary>
    public string RoguelikeCoreChar
    {
        get => GetTaskConfig<RoguelikeTask>().CoreChar;
        set {
            if (!SetTaskConfig<RoguelikeTask>(t => t.CoreChar == value, t => t.CoreChar = value))
            {
                return;
            }

            Instances.TaskQueueViewModel.AddLog("Core Char:" + value);
        }
    }

    private ObservableCollection<string> _roguelikeCoreCharList = [];

    /// <summary>
    /// Gets the roguelike core character.
    /// </summary>
    public ObservableCollection<string> RoguelikeCoreCharList
    {
        get => _roguelikeCoreCharList;
        private set {
            if (!string.IsNullOrEmpty(RoguelikeCoreChar) && !value.Contains(RoguelikeCoreChar))
            {
                value.Add(RoguelikeCoreChar);
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
                Theme.Phantom => "RoguelikeThemeTipPhantom",
                Theme.Mizuki => "RoguelikeThemeTipMizuki",
                Theme.Sami => "RoguelikeThemeTipSami",
                Theme.Sarkaz => "RoguelikeThemeTipSarkaz",
                Theme.JieGarden => "RoguelikeThemeTipJieGarden",
                _ => "RoguelikeThemeTipPhantom",
            };

            var tip = LocalizationHelper.GetString(key);
            return string.IsNullOrWhiteSpace(tip) ? LocalizationHelper.GetString("RoguelikeTheme") ?? string.Empty : tip;
        }
    }

    private void UpdateRoguelikeStartWithAllDict()
    {
        var config = GetTaskConfig<RoguelikeTask>().CollectibleStartAwards;
        var list = new List<GenericCombinedData<RoguelikeCollectibleAward>>()
        {
           new() { Display = LocalizationHelper.GetString("RoguelikeStartWithKettle"), Value = RoguelikeCollectibleAward.HotWater },
           new() { Display = LocalizationHelper.GetString("RoguelikeStartWithShield"), Value = RoguelikeCollectibleAward.Shield },
           new() { Display = LocalizationHelper.GetString("RoguelikeStartWithIngot"), Value = RoguelikeCollectibleAward.Ingot },
           new() { Display = LocalizationHelper.GetString("RoguelikeStartWithHope"), Value = RoguelikeCollectibleAward.Hope },
           new() { Display = LocalizationHelper.GetString("RoguelikeStartWithRandomReward"), Value = RoguelikeCollectibleAward.Random },
        };

        switch (RoguelikeTheme)
        {
            case Theme.Mizuki:
                list.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStartWithKey"), Value = RoguelikeCollectibleAward.Key });
                list.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStartWithDice"), Value = RoguelikeCollectibleAward.Dice });
                break;

            case Theme.Sarkaz:
                list.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStartWithIdea"), Value = RoguelikeCollectibleAward.Idea });
                break;

            case Theme.JieGarden:
                list.Remove(list.First(i => i.Value == RoguelikeCollectibleAward.Hope));
                list.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStartWithTicket"), Value = RoguelikeCollectibleAward.Ticket });
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
    /// Gets or sets a value indicating whether to use support unit.
    /// </summary>
    public bool RoguelikeUseSupportUnit
    {
        get => GetTaskConfig<RoguelikeTask>().UseSupport;
        set {
            if (value && RoguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional)
            {
                RoguelikeStartWithEliteTwo = false;
            }

            SetTaskConfig<RoguelikeTask>(t => t.UseSupport == value, t => t.UseSupport = value);
        }
    }

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
    public bool RoguelikeInvestmentWithMoreScore => GetTaskConfig<RoguelikeTask>().InvestWithMoreScore && RoguelikeMode == Mode.Investment;

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
    public ObservableCollection<GenericCombinedData<RoguelikeBoskySubNodeType>> RoguelikeFindPlaytimeTargetList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("RoguelikePlaytimeLing"), Value = RoguelikeBoskySubNodeType.Ling },
        new() { Display = LocalizationHelper.GetString("RoguelikePlaytimeShu"), Value = RoguelikeBoskySubNodeType.Shu },
        new() { Display = LocalizationHelper.GetString("RoguelikePlaytimeNian"), Value = RoguelikeBoskySubNodeType.Nian },
    ];

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

    private bool _roguelikeDelayAbortUntilCombatComplete = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether delay abort until battle complete
    /// </summary>
    public bool RoguelikeDelayAbortUntilCombatComplete
    {
        get => _roguelikeDelayAbortUntilCombatComplete;
        set {
            SetAndNotify(ref _roguelikeDelayAbortUntilCombatComplete, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, value.ToString());
        }
    }

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

    public override void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
        if (msg != AsstMsg.SubTaskExtraInfo)
        {
            return;
        }

        var subTaskDetails = details["details"];
        switch (details["what"]?.ToString() ?? string.Empty)
        {
            case "RoguelikeInvestmentReachFull":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeInvestmentReachFull"), UiLogColor.Info);
                AchievementTrackerHelper.Instance.SetProgress(AchievementIds.RoguelikeGoldMax, 999);
                break;

            case "RoguelikeInvestmentReachLimit":
                Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeInvestmentReachLimit"), subTaskDetails!["limit"]), UiLogColor.Info);
                break;

            case "RoguelikeInvestment":
                Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeInvestment"), subTaskDetails!["count"], subTaskDetails["total"], subTaskDetails["deposit"]), UiLogColor.Info);
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

                    var roguelikeInfo = string.Format(
                        LocalizationHelper.GetString("RoguelikeSettlement"),
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
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("RoguelikeEvent") + $" {subTaskDetails!["name"]}", UiLogColor.EventIS);
                break;

            case "RoguelikeEncounterOptions":
                var options = (subTaskDetails!["options"]! as JArray) ?? [];
                var logLines = new List<string>
                {
                    string.Format(LocalizationHelper.GetString("RoguelikeEncounterOptions"), options.Count, UiLogColor.EventIS),
                };

                foreach (var option in options)
                {
                    string messageKey = option["enabled"]!.Value<bool>() ? "RoguelikeEncounterEnabledOption" : "RoguelikeEncounterDisabledOption";
                    var text = option["text"]!.ToString();
                    logLines.Add(string.Format(LocalizationHelper.GetString(messageKey), text));
                }

                Instances.TaskQueueViewModel.AddLog(string.Join("\n", logLines), UiLogColor.EventIS, updateCardImage: true);
                break;

            case "BoskyPassageNode":
                {
                    var nodeType = subTaskDetails!["node_type"]?.ToString();
                    var (localizedNodeType, logColor) = nodeType switch {
                        "Omissions" => (LocalizationHelper.GetString("BoskyOmissions"), UiLogColor.BoskyOmissionsIS),
                        "Legend" => (LocalizationHelper.GetString("BoskyLegend"), UiLogColor.BoskyLegendIS),
                        "OldShop" => (LocalizationHelper.GetString("BoskyOldShop"), UiLogColor.BoskyOldShopIS),
                        "YiTrader" => (LocalizationHelper.GetString("BoskyYiTrader"), UiLogColor.TraderIS),
                        "Scheme" => (LocalizationHelper.GetString("BoskyScheme"), UiLogColor.BoskySchemeIS),
                        "Playtime" => (LocalizationHelper.GetString("BoskyPlaytime"), UiLogColor.BoskyPlaytimeIS),
                        "Doubts" => (LocalizationHelper.GetString("BoskyDoubts"), UiLogColor.BoskyDoubtsIS),
                        "Disaster" => (LocalizationHelper.GetString("BoskyDisaster"), UiLogColor.EmergencyIS),
                        _ => (nodeType ?? "Unknown", UiLogColor.EventIS),
                    };

                    Instances.TaskQueueViewModel.AddLog(localizedNodeType, logColor);
                    break;
                }

            case "RoguelikeCoppersRecognitionError":
                {
                    var recognizedName = subTaskDetails!["recognized_name"]?.ToString() ?? "Unknown";
                    var message = string.Format(LocalizationHelper.GetString("RoguelikeCoppersRecognitionError"), recognizedName);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.Error);
                    break;
                }

            case "RoguelikeCoppersExchangeInfo":
                {
                    var toDiscard = subTaskDetails!["to_discard"]?.ToString() ?? "Unknown";
                    var toPickup = subTaskDetails["to_pickup"]?.ToString() ?? "Unknown";
                    var message = string.Format(LocalizationHelper.GetString("RoguelikeCoppersExchange"), toDiscard, toPickup);
                    Instances.TaskQueueViewModel.AddLog(message, UiLogColor.EventIS);
                    break;
                }

            case "EncounterOcrError":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("EncounterOcrError"), UiLogColor.Error);
                break;

            case "RoguelikeJieGardenTargetFound":
                {
                    var targetSubtype = subTaskDetails!["target_subtype"]?.ToString();
                    var localizedTarget = targetSubtype switch {
                        "Ling" => LocalizationHelper.GetString("RoguelikePlaytimeLing"),
                        "Shu" => LocalizationHelper.GetString("RoguelikePlaytimeShu"),
                        "Nian" => LocalizationHelper.GetString("RoguelikePlaytimeNian"),
                        _ => targetSubtype ?? "Unknown",
                    };
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeJieGardenTargetFound"), localizedTarget), UiLogColor.Success);
                    break;
                }

            case "FoldartalGainOcrNextLevel":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("FoldartalGainOcrNextLevel") + $" {subTaskDetails!["foldartal"]}");
                break;

            case "MonthlySquadCompleted":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MonthlySquadCompleted"), UiLogColor.RareOperator);
                break;

            case "DeepExplorationCompleted":
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DeepExplorationCompleted"), UiLogColor.RareOperator);
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
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeGainParadigm"), cur), UiLogColor.Info);
                }
                else if (deepen_or_weaken == 1 && prev != string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeDeepenParadigm"), cur, prev), UiLogColor.Info);
                }
                else if (deepen_or_weaken == -1 && cur == string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeLoseParadigm"), string.Empty, prev), UiLogColor.Info);
                }
                else if (deepen_or_weaken == -1 && cur != string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(string.Format(LocalizationHelper.GetString("RoguelikeWeakenParadigm"), cur, prev), UiLogColor.Info);
                }

                break;
        }
    }

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize)?.Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        bool? ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not RoguelikeTask roguelike)
            {
                return null;
            }

            bool roguelikeSquadIsProfessional = roguelike.Mode == Mode.Collectible && roguelike.Theme != Theme.Phantom && roguelike.Squad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";
            bool roguelikeSquadIsFoldartal = roguelike.Mode == Mode.Collectible && roguelike.Theme == Theme.Sami && roguelike.Squad == "生活至上分队";
            var task = new AsstRoguelikeTask() {
                Theme = roguelike.Theme,
                Mode = roguelike.Mode,
                Starts = roguelike.StartCount,
                Difficulty = roguelike.Difficulty,
                Squad = roguelike.Squad,
                Roles = roguelike.Roles,
                CoreChar = DataHelper.GetCharacterByNameOrAlias(roguelike.CoreChar)?.Name ?? roguelike.CoreChar,
                UseSupport = roguelike.UseSupport,
                UseSupportNonFriend = roguelike.UseSupportNonFriend,

                InvestmentEnabled = roguelike.Investment,
                InvestmentCount = roguelike.InvestCount,
                InvestmentStopWhenFull = roguelike.StopWhenDepositFull,
                InvestmentWithMoreScore = roguelike.InvestWithMoreScore && roguelike.Mode == Mode.Investment,
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
                int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
                null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Roguelike, task),
                _ => null,
            };
        }
    }
}
