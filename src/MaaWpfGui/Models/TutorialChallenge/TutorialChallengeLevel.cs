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
using MaaWpfGui.Configuration.Single.MaaTask;

namespace MaaWpfGui.Models.TutorialChallenge;

/// <summary>
/// 教程关卡操作场右侧设置面板的种类，决定关卡窗口展示哪套沙盒面板。
/// </summary>
public enum TutorialChallengePanelKind
{
    /// <summary>
    /// 无设置面板，仅任务队列操作。
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
}

/// <summary>
/// 教程关卡定义。判定为终态检测，不追踪操作路径，沙盒队列呈现目标形态即达成。
/// </summary>
public class TutorialChallengeLevel
{
    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeLevel"/> class.
    /// </summary>
    /// <param name="id">关卡标识，用于推导本地化 key（TutorialChallenge.Level.{id}.Title / .Goal）与进度存档的通关键。</param>
    /// <param name="panelKind">操作场右侧设置面板的种类。</param>
    /// <param name="sandboxFactory">初始沙盒任务列表工厂，每次进入关卡重新调用。</param>
    /// <param name="goalPredicate">终态判定谓词，输入为当前沙盒任务列表。</param>
    /// <param name="smartPredicate">可选的更优解判定；达成 <paramref name="goalPredicate"/> 但不满足此项时，提示更优解法。</param>
    /// <param name="addableTaskTypes">｢+｣ 添加任务菜单列出的任务类型；null 表示与本关面板相关的默认全集（None: Fight；Fight: StartUp+Fight；DepotMaintain: StartUp+DepotMaintain+Fight）。</param>
    public TutorialChallengeLevel(
        string id,
        TutorialChallengePanelKind panelKind,
        Func<IList<BaseTask>> sandboxFactory,
        Func<IReadOnlyList<BaseTask>, bool> goalPredicate,
        Func<IReadOnlyList<BaseTask>, bool>? smartPredicate = null,
        IReadOnlyList<Type>? addableTaskTypes = null)
    {
        Id = id;
        PanelKind = panelKind;
        SandboxFactory = sandboxFactory;
        GoalPredicate = goalPredicate;
        SmartPredicate = smartPredicate;
        AddableTaskTypes = addableTaskTypes ?? panelKind switch
        {
            TutorialChallengePanelKind.Fight => [typeof(StartUpTask), typeof(FightTask)],
            TutorialChallengePanelKind.DepotMaintain => [typeof(StartUpTask), typeof(DepotMaintainTask), typeof(FightTask)],
            _ => [typeof(FightTask)],
        };
    }

    /// <summary>
    /// Gets 关卡标识。
    /// </summary>
    public string Id { get; }

    /// <summary>
    /// Gets 操作场右侧设置面板的种类。
    /// </summary>
    public TutorialChallengePanelKind PanelKind { get; }

    /// <summary>
    /// Gets 初始沙盒任务列表工厂。
    /// </summary>
    public Func<IList<BaseTask>> SandboxFactory { get; }

    /// <summary>
    /// Gets 终态判定谓词。
    /// </summary>
    public Func<IReadOnlyList<BaseTask>, bool> GoalPredicate { get; }

    /// <summary>
    /// Gets 可选的更优解判定谓词。
    /// </summary>
    public Func<IReadOnlyList<BaseTask>, bool>? SmartPredicate { get; }

    /// <summary>
    /// Gets ｢+｣ 添加任务菜单列出的任务类型，按关卡过滤无关任务。
    /// </summary>
    public IReadOnlyList<Type> AddableTaskTypes { get; }

    /// <summary>
    /// Gets 关卡名本地化 key。
    /// </summary>
    public string TitleKey => $"TutorialChallenge.Level.{Id}.Title";

    /// <summary>
    /// Gets 关卡目标描述本地化 key。
    /// </summary>
    public string GoalKey => $"TutorialChallenge.Level.{Id}.Goal";
}
