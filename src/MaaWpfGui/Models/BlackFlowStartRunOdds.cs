// <copyright file="BlackFlowStartRunOdds.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Models;

/// <summary>
/// 黑流树海刷开局的单局命中概率估算。
/// 先用观测样本给出「单次培育出现概率」与「每局平均培育数」，相乘得到每局期望只数，
/// 再与本次运行累计的实测样本按 Beta 分布合并，得到后验命中率。
/// </summary>
/// <remarks>
/// <para>模型：单局某类型的期望只数 λ = 每局平均培育数 × 该类型单次出现概率；
/// 把每局产物近似看作泊松过程，则单局至少获得一只的概率为 1 − e^−λ。
/// 多目标按相互独立处理，命中率相乘。</para>
/// <para>数据引用：观测样本取自 MAA 自身的运行日志，采集于 2026-10-30 22:27 至 13:13 的一次连续
/// 14.77 小时黑流树海刷开局（单台设备，共 146 局、96 个培育产物）。逐局用日志中的
/// ｢BlackFlow cultivation harvest recognized count N｣ 与紧随其后的
/// ｢Proceed [{ text: 襁褓… }]｣ 两行还原该局的产物集合。
/// 这是个人单机观测样本，<b>不是官方公布的概率</b>，也不保证对后续游戏版本或他人账号成立。</para>
/// <para>校验：用本模型反推的目标组合命中率与同批实测基本一致
/// （猫 + 羽蛇 3.02% 对实测 2.74%，猫 + 三头犬 0.63% 对 0.68%，羽蛇 + 三头犬 0.199% 对 0/146）。
/// 罕见组合实测样本过少，仍以模型值为主。</para>
/// <para>更新方式：获得新样本时只需替换 <see cref="PriorTrials"/>、
/// <see cref="AverageAnimalsPerRun"/> 与 <see cref="_singleDrawProbability"/>，计算与界面无需改动。</para>
/// </remarks>
public static class BlackFlowStartRunOdds
{
    /// <summary>
    /// 先验样本的局数。
    /// </summary>
    private const int PriorTrials = 146;

    /// <summary>
    /// 每局平均培育出的动物只数（96 个产物 ÷ 146 局）。约一半的局没有产出。
    /// </summary>
    private const double AverageAnimalsPerRun = 0.6575;

    /// <summary>
    /// 单次培育里各类型动物出现的概率，分母为全部培育产物。
    /// </summary>
    private static readonly Dictionary<RoguelikeBlackFlowStartAnimal, double> _singleDrawProbability = new()
    {
        [RoguelikeBlackFlowStartAnimal.Cat] = 0.5625,
        [RoguelikeBlackFlowStartAnimal.Dog] = 0.2500,
        [RoguelikeBlackFlowStartAnimal.FeatheredSerpent] = 0.1562,
        [RoguelikeBlackFlowStartAnimal.Cerberus] = 0.0312,
    };

    /// <summary>
    /// 本次运行累计的实测样本，键为规范化后的目标组合。
    /// </summary>
    private static readonly Dictionary<string, (int Hits, int Trials)> _live = new();

    /// <summary>
    /// 记录一局刷开局的结算结果。
    /// </summary>
    /// <param name="animal1">第一只目标动物。</param>
    /// <param name="animal2">第二只目标动物。</param>
    /// <param name="obtained">本局是否拿到目标组合。</param>
    public static void Record(RoguelikeBlackFlowStartAnimal animal1, RoguelikeBlackFlowStartAnimal animal2, bool obtained)
    {
        var targets = TargetsOf(animal1, animal2);
        if (targets.Count == 0)
        {
            return;
        }

        string key = KeyOf(targets);
        _live.TryGetValue(key, out (int Hits, int Trials) stat);
        _live[key] = (stat.Hits + (obtained ? 1 : 0), stat.Trials + 1);
    }

    /// <summary>
    /// 估算目标组合的单局命中率。
    /// </summary>
    /// <param name="animal1">第一只目标动物。</param>
    /// <param name="animal2">第二只目标动物。</param>
    /// <returns>后验命中率与本次运行的实测局数；两只都是 ｢无｣ 时返回 null。</returns>
    public static (double Probability, int LiveTrials)? Estimate(
        RoguelikeBlackFlowStartAnimal animal1,
        RoguelikeBlackFlowStartAnimal animal2)
    {
        var targets = TargetsOf(animal1, animal2);
        if (targets.Count == 0)
        {
            return null;
        }

        // 先验命中率：多目标按独立处理，命中率相乘。
        double priorProbability = 1.0;
        foreach (RoguelikeBlackFlowStartAnimal target in targets)
        {
            priorProbability *= AtLeastOne(target);
        }

        (int Hits, int Trials) live = _live.TryGetValue(KeyOf(targets), out var stat) ? stat : (0, 0);

        // Beta(α, β) 后验均值：把先验折算成等效命中/未命中局数，再加一平滑。
        double alpha = (priorProbability * PriorTrials) + live.Hits + 1;
        double beta = ((1 - priorProbability) * PriorTrials) + (live.Trials - live.Hits) + 1;
        return (alpha / (alpha + beta), live.Trials);
    }

    /// <summary>
    /// 单局至少获得一只指定类型动物的概率（泊松近似）。
    /// </summary>
    /// <param name="animal">目标动物类型。</param>
    /// <returns>概率，未建模的类型按 0 处理。</returns>
    private static double AtLeastOne(RoguelikeBlackFlowStartAnimal animal)
    {
        double singleDraw = _singleDrawProbability.TryGetValue(animal, out double value) ? value : 0;
        return 1 - Math.Exp(-AverageAnimalsPerRun * singleDraw);
    }

    /// <summary>
    /// 规范化目标组合：去掉 ｢无｣、去重、按枚举值升序。
    /// </summary>
    /// <param name="animal1">第一只目标动物。</param>
    /// <param name="animal2">第二只目标动物。</param>
    /// <returns>排序去重后的目标列表。</returns>
    private static List<RoguelikeBlackFlowStartAnimal> TargetsOf(
        RoguelikeBlackFlowStartAnimal animal1,
        RoguelikeBlackFlowStartAnimal animal2)
    {
        List<RoguelikeBlackFlowStartAnimal> all = [animal1, animal2];
        return all
            .Where(x => x != RoguelikeBlackFlowStartAnimal.None)
            .Distinct()
            .OrderBy(x => (int)x)
            .ToList();
    }

    /// <summary>
    /// 把规范化后的目标列表转成字典键。
    /// </summary>
    /// <param name="targets">目标列表。</param>
    /// <returns>按枚举值升序、以逗号连接的键。</returns>
    private static string KeyOf(List<RoguelikeBlackFlowStartAnimal> targets)
        => string.Join(",", targets.Select(x => x.ToString()));
}
