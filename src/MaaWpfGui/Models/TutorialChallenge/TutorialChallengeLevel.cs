// <copyright file="TutorialChallengeLevel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Models.TutorialChallenge;

/// <summary>
/// 教程关卡操作场可交互场景的种类，决定关卡窗口向学习者开放哪些沙盒面板。
/// 任务队列和设置可以在同一关中分别作为页签显示。
/// </summary>
public enum TutorialChallengePanelKind
{
    /// <summary>
    /// 无附加场景，仅任务队列操作。
    /// </summary>
    None,

    /// <summary>
    /// 理智作战沙盒面板（关卡选择、备选关卡、指定掉落）。
    /// </summary>
    Fight,

    /// <summary>
    /// 库存保持沙盒面板（预设导入与计划列表）。
    /// </summary>
    DepotMaintain,

    /// <summary>
    /// 公招沙盒面板（保留指定词条等）。
    /// </summary>
    Recruit,

    /// <summary>
    /// 设置页骨架场景（导航、搜索、分区折叠；具体内容随关卡投放）。
    /// </summary>
    Settings,

    /// <summary>
    /// 拖拽投放区场景（拖入更新包识别类别，供 ｢拖拽更新｣ 关卡使用）。
    /// </summary>
    DropZone,
}

/// <summary>
/// 教程关卡难度档位，枚举值即通关可获得的牛牛币数。
/// </summary>
public enum TutorialChallengeDifficulty
{
    /// <summary>
    /// 低难度，通关得 1 牛牛币。
    /// </summary>
    Low = 1,

    /// <summary>
    /// 中难度，通关得 2 牛牛币。
    /// </summary>
    Medium = 2,

    /// <summary>
    /// 高难度，通关得 3 牛牛币。
    /// </summary>
    High = 3,
}

/// <summary>
/// 教程关卡定义。判定为终态检测，不追踪操作路径，沙盒队列呈现目标形态即达成。
/// </summary>
public class TutorialChallengeLevel
{
    private readonly Func<object[]>? _goalArgs;

    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeLevel"/> class.
    /// </summary>
    /// <param name="id">关卡标识，用于推导本地化 key（Toolbox.TutorialChallenge.Level.{id}.Title / .Goal）与进度存档的通关键。</param>
    /// <param name="sandboxFactory">初始沙盒任务列表工厂，每次进入关卡重新调用。</param>
    /// <param name="goalPredicate">终态判定谓词，输入为当前沙盒任务列表与关卡视图模型（可读取窗口级沙盒状态，如定时设置）。</param>
    /// <param name="panelKinds">本关开放的沙盒场景。</param>
    /// <param name="smartSolution">可选的更优解判定与提示文案 key；达成 <paramref name="goalPredicate"/> 但不满足该判定时，弹出该提示。</param>
    /// <param name="goalArgs">目标文案的格式化参数，按当前数据实时取值；null 表示目标文案不含占位符。</param>
    /// <param name="addableTaskTypes">｢+｣ 菜单可添加的任务类型。省略时按本关开放的任务面板选择默认列表。</param>
    /// <param name="settingSectionKinds">本关设置页投放的分区种类白名单；null 表示不构建设置分区（配合 <see cref="TutorialChallengePanelKind.Settings"/> 使用，无关分区不出现）。</param>
    /// <param name="sandboxTimerSlots">本关设置页定时分区的初始定时槽；null 表示默认 1 空槽。</param>
    /// <param name="difficulty">难度档位，枚举值即通关可获得的牛牛币数。</param>
    public TutorialChallengeLevel(
        string id,
        Func<IList<BaseTask>> sandboxFactory,
        Func<IReadOnlyList<BaseTask>, TutorialChallengeLevelViewModel, bool> goalPredicate,
        IReadOnlyList<TutorialChallengePanelKind> panelKinds,
        (Func<IReadOnlyList<BaseTask>, TutorialChallengeLevelViewModel, bool> Predicate, string HintKey)? smartSolution = null,
        Func<object[]>? goalArgs = null,
        IReadOnlyList<Type>? addableTaskTypes = null,
        IReadOnlyList<TutorialChallengeSettingSectionKind>? settingSectionKinds = null,
        IReadOnlyList<(bool? IsEnabled, int Hour, int Minute)>? sandboxTimerSlots = null,
        TutorialChallengeDifficulty difficulty = TutorialChallengeDifficulty.Low)
    {
        Id = id;
        PanelKinds = panelKinds;
        SandboxFactory = sandboxFactory;
        GoalPredicate = goalPredicate;
        SmartSolution = smartSolution;
        _goalArgs = goalArgs;
        AddableTaskTypes = addableTaskTypes ?? DefaultAddableTaskTypes(panelKinds);
        SettingSectionKinds = settingSectionKinds;
        SandboxTimerSlots = sandboxTimerSlots;
        Difficulty = difficulty;
    }

    private static IReadOnlyList<Type> DefaultAddableTaskTypes(IReadOnlyList<TutorialChallengePanelKind> panelKinds)
    {
        if (panelKinds.Contains(TutorialChallengePanelKind.Recruit))
        {
            return [typeof(StartUpTask), typeof(RecruitTask), typeof(FightTask)];
        }

        if (panelKinds.Contains(TutorialChallengePanelKind.DepotMaintain))
        {
            return [typeof(StartUpTask), typeof(DepotMaintainTask), typeof(FightTask)];
        }

        if (panelKinds.Contains(TutorialChallengePanelKind.Fight))
        {
            return [typeof(StartUpTask), typeof(FightTask)];
        }

        return [typeof(FightTask)];
    }

    /// <summary>
    /// Gets 关卡标识。
    /// </summary>
    public string Id { get; }

    /// <summary>
    /// Gets 本关开放的沙盒场景列表。
    /// </summary>
    public IReadOnlyList<TutorialChallengePanelKind> PanelKinds { get; }

    /// <summary>
    /// Gets 初始沙盒任务列表工厂。
    /// </summary>
    public Func<IList<BaseTask>> SandboxFactory { get; }

    /// <summary>
    /// Gets 终态判定谓词。
    /// </summary>
    public Func<IReadOnlyList<BaseTask>, TutorialChallengeLevelViewModel, bool> GoalPredicate { get; }

    /// <summary>
    /// Gets 可选的更优解判定谓词与提示文案 key。
    /// </summary>
    public (Func<IReadOnlyList<BaseTask>, TutorialChallengeLevelViewModel, bool> Predicate, string HintKey)? SmartSolution { get; }

    /// <summary>
    /// Gets ｢+｣ 添加任务菜单列出的任务类型，按关卡过滤无关任务。
    /// </summary>
    public IReadOnlyList<Type> AddableTaskTypes { get; }

    /// <summary>
    /// Gets 本关设置页投放的分区种类白名单；null 表示不构建设置分区。
    /// </summary>
    public IReadOnlyList<TutorialChallengeSettingSectionKind>? SettingSectionKinds { get; }

    /// <summary>
    /// Gets 本关设置页定时分区的初始定时槽；null 表示默认 1 空槽。
    /// </summary>
    public IReadOnlyList<(bool? IsEnabled, int Hour, int Minute)>? SandboxTimerSlots { get; }

    /// <summary>
    /// Gets 难度档位，枚举值即通关可获得的牛牛币数。
    /// </summary>
    public TutorialChallengeDifficulty Difficulty { get; }

    /// <summary>
    /// Gets 关卡名本地化 key。
    /// </summary>
    public string TitleKey => $"Toolbox.TutorialChallenge.Level.{Id}.Title";

    /// <summary>
    /// Gets 关卡目标描述（按当前语言取文案，含占位符时代入格式化参数）。
    /// </summary>
    public string Goal => _goalArgs is { } args
        ? LocalizationHelper.GetStringFormat(GoalKey, args())
        : LocalizationHelper.GetString(GoalKey);

    private string GoalKey => $"Toolbox.TutorialChallenge.Level.{Id}.Goal";

    /// <summary>
    /// 本关是否开放指定场景。
    /// </summary>
    /// <param name="kind">场景种类。</param>
    /// <returns>已声明该场景时返回 true。</returns>
    public bool HasPanel(TutorialChallengePanelKind kind) => PanelKinds.Contains(kind);
}
