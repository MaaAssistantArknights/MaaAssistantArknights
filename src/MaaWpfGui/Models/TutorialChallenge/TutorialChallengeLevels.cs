// <copyright file="TutorialChallengeLevels.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;

namespace MaaWpfGui.Models.TutorialChallenge;

/// <summary>
/// 教程关卡静态注册表。新增关卡时在此追加定义与对应本地化 key，UI 与判定框架无需改动。
/// </summary>
public static class TutorialChallengeLevels
{
    // 关卡用到的关卡 code 与物品 id（游戏侧固定事实）
    private const string StageAp5 = "AP-5"; // 龙门币外的采购凭证（红票）本
    private const string StageCe6 = "CE-6"; // 龙门币本
    private const string Stage17 = "1-7"; // 固源岩产地
    private const string ItemRock = "30012"; // 固源岩

    /// <summary>
    /// Gets 全部关卡，顺序即清单页建议学习路径，关卡间无解锁依赖。
    /// </summary>
    public static IReadOnlyList<TutorialChallengeLevel> All =>
    [
        new(
            id: "AddFight",
            panelKind: TutorialChallengePanelKind.None,
            sandboxFactory: () => [new StartUpTask()],
            goalPredicate: tasks => tasks.OfType<FightTask>().Any()),

        new(
            id: "StagePriority",
            panelKind: TutorialChallengePanelKind.Fight,
            sandboxFactory: () => [new StartUpTask(), new FightTask { StagePlan = [Stage17] }],
            goalPredicate: tasks => tasks.OfType<FightTask>().Any(fight =>
                fight.UseOptionalStage
                && fight.StagePlan.Count >= 3
                && fight.StagePlan[0] == StageAp5
                && fight.StagePlan[1] == StageCe6
                && fight.StagePlan[2] == Stage17)),

        new(
            id: "TargetDrop",
            panelKind: TutorialChallengePanelKind.Fight,
            sandboxFactory: () => [new StartUpTask(), new FightTask { StagePlan = [Stage17], DropCount = 5 }],
            goalPredicate: tasks => tasks.OfType<FightTask>().Any(fight =>
                fight.StagePlan is [var first, ..] && first == Stage17
                && fight.EnableTargetDrop == true
                && fight.DropId == ItemRock
                && fight.DropCount == 10)),

        new(
            id: "InventoryTarget",
            panelKind: TutorialChallengePanelKind.Fight,
            sandboxFactory: () => [new StartUpTask(), new FightTask { StagePlan = [Stage17], DropCount = 50 }],
            goalPredicate: tasks => tasks.OfType<FightTask>().Any(fight =>
                fight.StagePlan is [var first, ..] && first == Stage17
                && fight.EnableTargetDrop == true
                && fight.DropId == ItemRock
                && fight.DropCount == 100
                && fight.IsInventoryTarget)),

        new(
            id: "ChipStock",
            panelKind: TutorialChallengePanelKind.DepotMaintain,
            sandboxFactory: () => [new StartUpTask()],
            goalPredicate: tasks => IsChipStockByPresets(tasks) || IsChipStockByFightTasks(tasks),
            smartPredicate: IsChipStockByPresets),
    ];

    /// <summary>
    /// 芯片全套库存的更优解判定：存在库存保持任务，其计划覆盖 芯片（Chip1）与芯片组（Chip2）两个预设展开的全部目标。
    /// </summary>
    /// <param name="tasks">沙盒任务列表。</param>
    /// <returns>覆盖全部芯片目标时返回 true。</returns>
    private static bool IsChipStockByPresets(IReadOnlyList<BaseTask> tasks)
    {
        var expected = ExpandChipPresets();
        return tasks.OfType<DepotMaintainTask>().Any(task => expected.All(expectedPlan
            => task.PlanList.Any(plan => plan.Stage == expectedPlan.Stage
                && plan.DropId == expectedPlan.DropId
                && plan.DropCount == expectedPlan.DropCount)));
    }

    /// <summary>
    /// 芯片全套库存的朴素解判定：逐材料配置的理智作战任务覆盖全部芯片目标（按库存目标模式）。
    /// </summary>
    /// <param name="tasks">沙盒任务列表。</param>
    /// <returns>覆盖全部芯片目标时返回 true。</returns>
    private static bool IsChipStockByFightTasks(IReadOnlyList<BaseTask> tasks)
    {
        var expected = ExpandChipPresets();
        return expected.All(expectedPlan => tasks.OfType<FightTask>().Any(fight =>
            fight.EnableTargetDrop == true
            && fight.IsInventoryTarget
            && fight.DropId == expectedPlan.DropId
            && fight.DropCount == expectedPlan.DropCount));
    }

    /// <summary>
    /// 将 Chip1 与 Chip2 预设展开为 (关卡, 物品, 目标数量) 计划清单，期望值与 DepotMaintain 预设数据同源。
    /// </summary>
    /// <returns>全部芯片与芯片组目标计划。</returns>
    private static List<(string Stage, string DropId, int DropCount)> ExpandChipPresets()
    {
        return [.. DepotMaintainTaskUserControlModel.PresetData["Chip1"]
            .Concat(DepotMaintainTaskUserControlModel.PresetData["Chip2"])
            .SelectMany(preset => preset.Drops.Select(dropId => (preset.Stage, DropId: dropId, DropCount: preset.DefaultCount)))];
    }
}
