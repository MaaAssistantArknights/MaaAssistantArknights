// <copyright file="RoguelikeSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static Microsoft.WindowsAPICodePack.Shell.PropertySystem.SystemProperties.System;
using Mode = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeMode;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.RoguelikeTheme;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;
public class RoguelikeSettingsUserControlModel : TaskViewModel
{
    static RoguelikeSettingsUserControlModel()
    {
        Instance = new();
    }

    public static RoguelikeSettingsUserControlModel Instance { get; }

    public void InitRoguelike()
    {
        UpdateRoguelikeDifficultyList();
        UpdateRoguelikeModeList();
        UpdateRoguelikeSquadList();

        UpdateRoguelikeCoreCharList();
    }

    private void UpdateRoguelikeDifficultyList()
    {
        int maxThemeDifficulty = GetMaxDifficultyForTheme(RoguelikeTheme);

        if (RoguelikeDifficultyList.Count == 0)
        {
            RoguelikeDifficultyList =
            [
                new() { Display = "MAX", Value = int.MaxValue }
            ];
            for (int i = 20; i >= -1; --i)
            {
                RoguelikeDifficultyList.Add(new() { Display = i.ToString(), Value = i });
            }
        }

        var sortedItems = RoguelikeDifficultyList
            .Select(item => item)
            .OrderBy(item => item.Value switch
            {
                -1 => 0,
                int.MaxValue => 1,
                _ when item.Value <= maxThemeDifficulty => 2 + (maxThemeDifficulty - item.Value),
                _ => 2 + maxThemeDifficulty + 1 + (20 - item.Value),
            })
            .ToList();

        for (int newIndex = 0; newIndex < sortedItems.Count; newIndex++)
        {
            int currentIndex = RoguelikeDifficultyList.IndexOf(sortedItems[newIndex]);
            if (currentIndex != newIndex)
            {
                RoguelikeDifficultyList.Move(currentIndex, newIndex);
            }

            int value = RoguelikeDifficultyList[newIndex].Value;
            RoguelikeDifficultyList[newIndex].Display = value switch
            {
                -1 => LocalizationHelper.GetString("Current"),
                int.MaxValue => "MAX",
                0 => "MIN",
                _ => value > maxThemeDifficulty ? $"{value} (NONSUPPORT)" : value.ToString(),
            };
        }
    }

    private static int GetMaxDifficultyForTheme(Theme theme)
    {
        return theme switch
        {
            Theme.Phantom => 0,
            Theme.Mizuki => 18,
            Theme.Sami => 15,
            Theme.Sarkaz => 18,
            _ => 20,
        };
    }

    private void UpdateRoguelikeModeList()
    {
        var roguelikeMode = RoguelikeMode;

        RoguelikeModeList =
        [
            new() { Display = LocalizationHelper.GetString("RoguelikeStrategyExp"), Value = Mode.Exp },
            new() { Display = LocalizationHelper.GetString("RoguelikeStrategyGold"), Value = Mode.Investment },

            // new CombData { Display = "两者兼顾，投资过后退出", Value = "2" } // 弃用
            // new CombData { Display = Localization.GetString("3"), Value = "3" },  // 开发中
            new() { Display = LocalizationHelper.GetString("RoguelikeStrategyLastReward"), Value = Mode.Collectible },
            new() { Display = LocalizationHelper.GetString("RoguelikeStrategyMonthlySquad"), Value = Mode.Squad },
            new() { Display = LocalizationHelper.GetString("RoguelikeStrategyDeepExploration"), Value = Mode.Exploration },
        ];

        switch (RoguelikeTheme)
        {
            case Theme.Sami:
                RoguelikeModeList.Add(new() { Display = LocalizationHelper.GetString("RoguelikeStrategyCollapse"), Value = Mode.CLP_PDS });

                break;
        }

        RoguelikeMode = RoguelikeModeList.Any(x => x.Value == roguelikeMode) ? roguelikeMode : 0;
    }

    private readonly Dictionary<string, List<(string Key, string Value)>> _squadDictionary = new()
    {
        ["Phantom_Default"] =
        [
            ("ResearchSquad", "研究分队"),
        ],
        ["Mizuki_Default"] =
        [
            ("IS2NewSquad1", "心胜于物分队"),
            ("IS2NewSquad2", "物尽其用分队"),
            ("IS2NewSquad3", "以人为本分队"),
            ("ResearchSquad", "研究分队"),
        ],
        ["Sami_Default"] =
        [
            ("IS3NewSquad1", "永恒狩猎分队"),
            ("IS3NewSquad2", "生活至上分队"),
            ("IS3NewSquad3", "科学主义分队"),
            ("IS3NewSquad4", "特训分队"),
        ],
        ["Sarkaz_1"] =
        [
            ("IS4NewSquad2", "博闻广记分队"),
            ("IS4NewSquad3", "蓝图测绘分队"),
            ("IS4NewSquad6", "点刺成锭分队"),
            ("IS4NewSquad7", "拟态学者分队"),
        ],
        ["Sarkaz_Default"] =
        [
            ("IS4NewSquad1", "魂灵护送分队"),
            ("IS4NewSquad2", "博闻广记分队"),
            ("IS4NewSquad3", "蓝图测绘分队"),
            ("IS4NewSquad4", "因地制宜分队"),
            ("IS4NewSquad5", "异想天开分队"),
            ("IS4NewSquad6", "点刺成锭分队"),
            ("IS4NewSquad7", "拟态学者分队"),
            ("IS4NewSquad8", "专业人士分队"),
        ],
    };

    // 通用分队
    private readonly List<(string Key, string Value)> _commonSquads =
    [
        ("LeaderSquad", "指挥分队"),
        ("GatheringSquad", "集群分队"),
        ("SupportSquad", "后勤分队"),
        ("SpearheadSquad", "矛头分队"),
        ("TacticalAssaultOperative", "突击战术分队"),
        ("TacticalFortificationOperative", "堡垒战术分队"),
        ("TacticalRangedOperative", "远程战术分队"),
        ("TacticalDestructionOperative", "破坏战术分队"),
        ("First-ClassSquad", "高规格分队"),
    ];

    private void UpdateRoguelikeSquadList()
    {
        var roguelikeSquad = RoguelikeSquad;
        RoguelikeSquadList =
        [
            new() { Display = LocalizationHelper.GetString("DefaultSquad"), Value = string.Empty }
        ];

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
        RoguelikeSquad = RoguelikeSquadList.Any(x => x.Value == roguelikeSquad) ? roguelikeSquad : string.Empty;
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

    private ObservableCollection<GenericCombinedData<RoguelikeMode>> _roguelikeModeList = [];

    /// <summary>
    /// Gets or sets the list of roguelike modes.
    /// </summary>
    public ObservableCollection<GenericCombinedData<RoguelikeMode>> RoguelikeModeList
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

    /// <summary>
    /// Gets the list of roguelike roles.
    /// </summary>
    public List<CombinedData> RoguelikeRolesList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("DefaultRoles"), Value = string.Empty },
            new() { Display = LocalizationHelper.GetString("FirstMoveAdvantage"), Value = "先手必胜" },
            new() { Display = LocalizationHelper.GetString("SlowAndSteadyWinsTheRace"), Value = "稳扎稳打" },
            new() { Display = LocalizationHelper.GetString("OvercomingYourWeaknesses"), Value = "取长补短" },
            new() { Display = LocalizationHelper.GetString("AsYourHeartDesires"), Value = "随心所欲" },
        ];

    /// <summary>
    /// Gets the list of roguelike lists.
    /// </summary>
    public List<GenericCombinedData<Theme>> RoguelikeThemeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("RoguelikeThemePhantom"), Value = Theme.Phantom },
            new() { Display = LocalizationHelper.GetString("RoguelikeThemeMizuki"), Value = Theme.Mizuki },
            new() { Display = LocalizationHelper.GetString("RoguelikeThemeSami"), Value = Theme.Sami },
            new() { Display = LocalizationHelper.GetString("RoguelikeThemeSarkaz"), Value = Theme.Sarkaz },
        ];

    /// <summary>
    /// Gets or sets the Roguelike theme.
    /// </summary>
    public Theme RoguelikeTheme
    {
        get => GetTaskConfig<RoguelikeTask>()?.Theme ?? default;
        set
        {
            SetTaskConfig<RoguelikeTask>(t => t.Theme == value, t => t.Theme = value);
            UpdateRoguelikeDifficultyList();
            UpdateRoguelikeModeList();
            UpdateRoguelikeSquadList();
            UpdateRoguelikeCoreCharList();
        }
    }

    public int RoguelikeDifficulty
    {
        get => GetTaskConfig<RoguelikeTask>()?.Difficulty ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.Difficulty == value, t => t.Difficulty = value);
    }

    /// <summary>
    /// Gets or sets 策略，往后打 / 刷一层就退 / 烧热水
    /// </summary>
    public Mode RoguelikeMode
    {
        get => GetTaskConfig<RoguelikeTask>()?.Mode ?? default;
        set
        {
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
        get => GetTaskConfig<RoguelikeTask>()?.Squad ?? string.Empty;
        set => SetTaskConfig<RoguelikeTask>(t => t.Squad == value, t => t.Squad = value);
    }

    /// <summary>
    /// Gets or sets the roguelike squad using for last reward mode.
    /// </summary>
    public string RoguelikeCollectibleModeSquad
    {
        get => GetTaskConfig<RoguelikeTask>()?.SquadCollectible ?? string.Empty;
        set => SetTaskConfig<RoguelikeTask>(t => t.SquadCollectible == value, t => t.SquadCollectible = value);
    }

    public bool RoguelikeSquadIsProfessional => RoguelikeMode == Mode.Collectible && RoguelikeTheme != Theme.Phantom && RoguelikeSquad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";

    public bool RoguelikeSquadIsFoldartal => RoguelikeMode == Mode.Collectible && RoguelikeTheme == Theme.Sami && RoguelikeSquad == "生活至上分队";

    /// <summary>
    /// Gets or sets the roguelike roles.
    /// </summary>
    public string RoguelikeRoles
    {
        get => GetTaskConfig<RoguelikeTask>()?.Roles ?? string.Empty;
        set => SetTaskConfig<RoguelikeTask>(t => t.Roles == value, t => t.Roles = value);
    }

    /// <summary>
    /// Gets or sets the roguelike core character.
    /// </summary>
    public string RoguelikeCoreChar
    {
        get => GetTaskConfig<RoguelikeTask>()?.CoreChar ?? string.Empty;
        set
        {
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
        private set
        {
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
        get => GetTaskConfig<RoguelikeTask>()?.StartWithEliteTwo ?? default;
        set
        {
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
        get => GetTaskConfig<RoguelikeTask>()?.StartWithEliteTwoOnly ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.StartWithEliteTwoOnly == value, t => t.StartWithEliteTwoOnly = value);
    }

    /// <summary>
    /// Gets a value indicating whether only need with elite two's core char.
    /// </summary>
    public bool RoguelikeOnlyStartWithEliteTwo => RoguelikeOnlyStartWithEliteTwo && RoguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional;

    public static Dictionary<string, string> RoguelikeStartWithAllDict { get; } = new()
    {
        { "Roguelike@LastReward", LocalizationHelper.GetString("RoguelikeStartWithKettle") },
        { "Roguelike@LastReward2", LocalizationHelper.GetString("RoguelikeStartWithShield") },
        { "Roguelike@LastReward3", LocalizationHelper.GetString("RoguelikeStartWithIngot") },
        { "Roguelike@LastReward4", LocalizationHelper.GetString("RoguelikeStartWithHope") },
        { "Roguelike@LastRewardRand", LocalizationHelper.GetString("RoguelikeStartWithRandomReward") },
        { "Mizuki@Roguelike@LastReward5", LocalizationHelper.GetString("RoguelikeStartWithKey") },
        { "Mizuki@Roguelike@LastReward6", LocalizationHelper.GetString("RoguelikeStartWithDice") },
        { "Sarkaz@Roguelike@LastReward5", LocalizationHelper.GetString("RoguelikeStartWithIdea") },
    };

    private object[] _roguelikeStartWithSelectListRaw = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.RoguelikeStartWithSelectList, "Roguelike@LastReward Roguelike@LastReward4 Sarkaz@Roguelike@LastReward5")
        .Split(' ')
        .Where(s => RoguelikeStartWithAllDict.ContainsKey(s.ToString()))
        .Select(s => (object)new KeyValuePair<string, string>(s, RoguelikeStartWithAllDict[s]))
        .ToArray();

    public object[] RoguelikeStartWithSelectListRaw
    {
        get => _roguelikeStartWithSelectListRaw;
        set
        {
            SetAndNotify(ref _roguelikeStartWithSelectListRaw, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
            var config = string.Join(' ', _roguelikeStartWithSelectListRaw.Cast<KeyValuePair<string, string>>().Select(pair => pair.Key).ToList());
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.RoguelikeStartWithSelectList, config);
        }
    }

    public List<string> RoguelikeStartWithSelectList => _roguelikeStartWithSelectListRaw.Cast<KeyValuePair<string, string>>().Select(pair => pair.Key).ToList();

    /// <summary>
    /// Gets or sets a value indicating whether core char need start with elite two.
    /// </summary>
    public bool Roguelike3FirstFloorFoldartal
    {
        get => GetTaskConfig<RoguelikeTask>()?.SamiFirstFloorFoldartal ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.SamiFirstFloorFoldartal == value, t => t.SamiFirstFloorFoldartal = value);
    }

    public string Roguelike3FirstFloorFoldartals
    {
        get => GetTaskConfig<RoguelikeTask>()?.SamiFirstFloorFoldartals ?? string.Empty;
        set
        {
            value = value.Trim();
            SetTaskConfig<RoguelikeTask>(t => t.SamiFirstFloorFoldartals == value, t => t.SamiFirstFloorFoldartals = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether core char need start with elite two.
    /// </summary>
    public bool Roguelike3NewSquad2StartingFoldartal
    {
        get => GetTaskConfig<RoguelikeTask>()?.SamiNewSquad2StartingFoldartal ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.SamiNewSquad2StartingFoldartal == value, t => t.SamiNewSquad2StartingFoldartal = value);
    }

    public string Roguelike3NewSquad2StartingFoldartals
    {
        get => GetTaskConfig<RoguelikeTask>()?.SamiNewSquad2StartingFoldartals ?? string.Empty;
        set
        {
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
        get => GetTaskConfig<RoguelikeTask>()?.ExpectedCollapsalParadigms ?? string.Empty;
        set
        {
            value = value.Replace('；', ';').Trim();
            SetTaskConfig<RoguelikeTask>(t => t.ExpectedCollapsalParadigms == value, t => t.SamiNewSquad2StartingFoldartals = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use support unit.
    /// </summary>
    public bool RoguelikeUseSupportUnit
    {
        get => GetTaskConfig<RoguelikeTask>()?.UseSupport ?? default;
        set
        {
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
        get => GetTaskConfig<RoguelikeTask>()?.UseSupportNonFriend ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.UseSupportNonFriend == value, t => t.UseSupportNonFriend = value);
    }

    /// <summary>
    /// Gets or sets the start count of roguelike.
    /// </summary>
    public int RoguelikeStartsCount
    {
        get => GetTaskConfig<RoguelikeTask>()?.StartCount ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.StartCount == value, t => t.StartCount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether investment is enabled.
    /// </summary>
    public bool RoguelikeInvestmentEnabled
    {
        get => GetTaskConfig<RoguelikeTask>()?.Investment ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.Investment == value, t => t.Investment = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether investment is enabled.
    /// </summary>
    public bool RoguelikeInvestmentWithMoreScoreRaw
    {
        get => GetTaskConfig<RoguelikeTask>()?.InvestWithMoreScore ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.InvestWithMoreScore == value, t => t.InvestWithMoreScore = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether shopping is enabled in LastReward Mode.
    /// </summary>
    public bool RoguelikeCollectibleModeShopping
    {
        get => GetTaskConfig<RoguelikeTask>()?.CollectibleShopping ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.CollectibleShopping == value, t => t.CollectibleShopping = value);
    }

    public bool RoguelikeRefreshTraderWithDiceRaw
    {
        get => GetTaskConfig<RoguelikeTask>()?.RefreshTraderWithDice ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.RefreshTraderWithDice == value, t => t.RefreshTraderWithDice = value);
    }

    /// <summary>
    /// Gets or sets the invests count of roguelike.
    /// </summary>
    public int RoguelikeInvestsCount
    {
        get => GetTaskConfig<RoguelikeTask>()?.InvestCount ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.InvestCount == value, t => t.InvestCount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to stop when investment is full.
    /// </summary>
    public bool RoguelikeStopWhenInvestmentFull
    {
        get => GetTaskConfig<RoguelikeTask>()?.StopWhenDepositFull ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopWhenDepositFull == value, t => t.StopWhenDepositFull = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to stop when arriving at final boss.
    /// </summary>
    public bool RoguelikeStopAtFinalBoss
    {
        get => GetTaskConfig<RoguelikeTask>()?.StopAtFinalBoss ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopAtFinalBoss == value, t => t.StopAtFinalBoss = value);
    }

    private bool _roguelikeMonthlySquadAutoIterate = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMonthlySquadAutoIterate, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to automatically iterate the monthly squad.
    /// </summary>
    public bool RoguelikeMonthlySquadAutoIterate
    {
        get => GetTaskConfig<RoguelikeTask>()?.MonthlySquadAutoIterate ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.MonthlySquadAutoIterate == value, t => t.MonthlySquadAutoIterate = value);
    }

    public bool RoguelikeMonthlySquadCheckComms
    {
        get => GetTaskConfig<RoguelikeTask>()?.MonthlySquadCheckComms ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.MonthlySquadCheckComms == value, t => t.MonthlySquadCheckComms = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to automatically iterate the deep exploration mode.
    /// </summary>
    public bool RoguelikeDeepExplorationAutoIterate
    {
        get => GetTaskConfig<RoguelikeTask>()?.DeepExplorationAutoIterate ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.DeepExplorationAutoIterate == value, t => t.DeepExplorationAutoIterate = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to stop when max level has been achieved.
    /// </summary>
    public bool RoguelikeStopAtMaxLevel
    {
        get => GetTaskConfig<RoguelikeTask>()?.StopWhenLevelMax ?? default;
        set => SetTaskConfig<RoguelikeTask>(t => t.StopWhenLevelMax == value, t => t.StopWhenLevelMax = value);
    }

    private bool _roguelikeStartWithSeed = false;

    /// <summary>
    /// Gets or sets a value indicating whether start with seed when investing in Sarkaz.
    /// </summary>
    public bool RoguelikeStartWithSeed
    {
        get => _roguelikeStartWithSeed;
        set
        {
            SetAndNotify(ref _roguelikeStartWithSeed, value);
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstRoguelikeTask()
        {
            Theme = RoguelikeTheme,
            Mode = RoguelikeMode,
            Starts = RoguelikeStartsCount,
            Difficulty = RoguelikeDifficulty,
            Squad = RoguelikeSquad,
            Roles = RoguelikeRoles,
            CoreChar = DataHelper.GetCharacterByNameOrAlias(RoguelikeCoreChar)?.Name ?? RoguelikeCoreChar,
            UseSupport = RoguelikeUseSupportUnit,
            UseSupportNonFriend = RoguelikeEnableNonfriendSupport,

            InvestmentEnabled = RoguelikeInvestmentEnabled,
            InvestmentCount = RoguelikeInvestsCount,
            InvestmentStopWhenFull = RoguelikeStopWhenInvestmentFull,
            InvestmentWithMoreScore = RoguelikeInvestmentWithMoreScoreRaw && RoguelikeMode == Mode.Investment,
            RefreshTraderWithDice = RoguelikeTheme == Theme.Mizuki && RoguelikeRefreshTraderWithDiceRaw,

            StopAtFinalBoss = RoguelikeStopAtFinalBoss,
            StopAtMaxLevel = RoguelikeStopAtMaxLevel,

            // 刷开局
            CollectibleModeSquad = RoguelikeCollectibleModeSquad,
            CollectibleModeShopping = RoguelikeCollectibleModeShopping,
            StartWithEliteTwo = RoguelikeStartWithEliteTwo && RoguelikeSquadIsProfessional,
            StartWithEliteTwoNonBattle = RoguelikeOnlyStartWithEliteTwo,

            // 月度小队
            MonthlySquadAutoIterate = RoguelikeMonthlySquadAutoIterate,
            MonthlySquadCheckComms = RoguelikeMonthlySquadCheckComms,

            // 深入探索
            DeepExplorationAutoIterate = RoguelikeDeepExplorationAutoIterate,

            SamiFirstFloorFoldartal = RoguelikeTheme == Theme.Sami && RoguelikeMode == Mode.Collectible && Roguelike3FirstFloorFoldartal,
            SamiStartFloorFoldartal = Roguelike3FirstFloorFoldartals,
            SamiNewSquad2StartingFoldartal = Roguelike3NewSquad2StartingFoldartal && RoguelikeSquadIsFoldartal,
            SamiNewSquad2StartingFoldartals = Roguelike3NewSquad2StartingFoldartals.Split(';').Where(i => !string.IsNullOrEmpty(i)).Take(3).ToList(),

            ExpectedCollapsalParadigms = RoguelikeExpectedCollapsalParadigms.Split(';').Where(i => !string.IsNullOrEmpty(i)).ToList(),
            StartWithSeed = RoguelikeStartWithSeed && RoguelikeTheme == Theme.Sarkaz && RoguelikeMode == Mode.Investment && RoguelikeSquad is "点刺成锭分队" or "后勤分队",
        };

        if (RoguelikeMode == Mode.Collectible && !RoguelikeOnlyStartWithEliteTwo)
        {
            var rewardKeys = new Dictionary<string, string>
            {
                { "Roguelike@LastReward", "hot_water" },
                { "Roguelike@LastReward2", "shield" },
                { "Roguelike@LastReward3", "ingot" },
                { "Roguelike@LastReward4", "hope" },
                { "Roguelike@LastRewardRand", "random" },
                { "Mizuki@Roguelike@LastReward5", "key" },
                { "Mizuki@Roguelike@LastReward6", "dice" },
                { "Sarkaz@Roguelike@LastReward5", "ideas" },
            };
            var startWithSelect = new JObject();
            foreach (var select in RoguelikeStartWithSelectList)
            {
                if (rewardKeys.TryGetValue(select, out var paramKey))
                {
                    task.CollectibleModeStartRewards[paramKey] = true;
                }
            }
        }

        return task.Serialize();
    }
}
