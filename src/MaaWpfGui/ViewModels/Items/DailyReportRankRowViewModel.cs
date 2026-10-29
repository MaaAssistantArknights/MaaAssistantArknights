// <copyright file="DailyReportRankRowViewModel.cs" company="MaaAssistantArknights">
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
using System.Windows;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 单件理智排名中的一个关卡：某种物品在该关卡平均每获得一个消耗的理智。
/// </summary>
public class DailyReportRankRowViewModel
{
    /// <summary>
    /// 掉落数低于此值时单件理智（理智 ÷ 掉落数）分母过小、比值偏斜，无论误差多小都淡化显示。
    /// </summary>
    public const int MinReliableCount = 10;

    /// <summary>
    /// 95% 置信区间相对半宽超过此值时淡化显示。
    /// </summary>
    public const double MaxReliableError = 0.3;

    /// <summary>
    /// 记录数不少于此值时按记录间的实际波动估算误差，否则退回泊松近似。
    /// </summary>
    public const int MinRecordsForVariance = 5;

    // 双侧 95% 的 t 分布临界值，下标为自由度（1 ~ 30）；自由度更大时取正态分布的 1.96
    private static readonly double[] _tCritical =
    [
        double.NaN, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
        2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
        2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042,
    ];

    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportRankRowViewModel"/> class.
    /// </summary>
    /// <param name="stage">关卡。</param>
    /// <param name="stats">该物品在该关卡的逐条记录累计量。</param>
    /// <param name="sanity">该关卡的理智消耗。</param>
    public DailyReportRankRowViewModel(string stage, DropStats stats, int sanity)
    {
        Stage = stage;
        Count = (int)stats.Count;
        SanityPerItem = sanity / stats.Count;
        RelativeError = EstimateRelativeError(stats);
        ValueText = DailyReportFormat.Rate(SanityPerItem);
        ErrorText = RelativeError < 0.0001
            ? "<" + 0.0001.ToString("P2", LocalizationHelper.CustomCultureInfo)
            : "±" + RelativeError.ToString("P2", LocalizationHelper.CustomCultureInfo);
        ToolTip = LocalizationHelper.GetStringFormat(
            "TaskQueue.DailyReport.Rank.Tip",
            DailyReportFormat.Rate(SanityPerItem * Math.Max(0, 1 - RelativeError)),
            DailyReportFormat.Rate(SanityPerItem * (1 + RelativeError)),
            Count.ToString("N0", LocalizationHelper.CustomCultureInfo),
            ((int)stats.Times).ToString("N0", LocalizationHelper.CustomCultureInfo));
    }

    /// <summary>
    /// 某种物品在某关卡的逐条记录累计量：记录数、场次 t、掉落数 c 及 Σt²、Σc²、Σc·t，
    /// 足以算出掉率 r = Σc / Σt 与记录间偏差平方和 Σ(c − r·t)²。
    /// </summary>
    public struct DropStats
    {
        /// <summary>
        /// 记录数。
        /// </summary>
        public int Records;

        /// <summary>
        /// Σt。
        /// </summary>
        public double Times;

        /// <summary>
        /// Σc。
        /// </summary>
        public double Count;

        /// <summary>
        /// Σt²。
        /// </summary>
        public double TimesSquared;

        /// <summary>
        /// Σc²。
        /// </summary>
        public double CountSquared;

        /// <summary>
        /// Σc·t。
        /// </summary>
        public double CountTimes;

        /// <summary>
        /// 累加一条记录。
        /// </summary>
        /// <param name="times">该记录的场次。</param>
        /// <param name="count">该记录中该物品的掉落数（未掉落为 0，同样计入）。</param>
        public void Add(int times, int count)
        {
            Records++;
            Times += times;
            Count += count;
            TimesSquared += (double)times * times;
            CountSquared += (double)count * count;
            CountTimes += (double)count * times;
        }
    }

    /// <summary>
    /// 估算单件理智 95% 置信区间的相对半宽（与掉率的相对误差相同）。
    /// 记录足够时把每条记录当作一个样本，按记录间的实际偏差估算掉率方差：
    /// 不假设记录内各场独立，连战的保底平滑、每场多掉、固定掉落等都体现在实际偏差里，系数按 t 分布取。
    /// 记录不足时退回泊松近似，样本数取掉落数与场次的较小者，避免每场大量掉落（如龙门币）被当作大量独立样本而显示虚假的精确。
    /// </summary>
    private static double EstimateRelativeError(DropStats s)
    {
        if (s.Records >= MinRecordsForVariance && s.Times > 0)
        {
            var rate = s.Count / s.Times;

            // Σ(c − r·t)² 展开为 Σc² − 2r·Σc·t + r²·Σt²；按 k/(k−1) 修正自由度
            var deviation = Math.Max(0, s.CountSquared - (2 * rate * s.CountTimes) + (rate * rate * s.TimesSquared));
            var k = s.Records;
            var rateStdError = Math.Sqrt(deviation * k / (k - 1)) / s.Times;
            var df = k - 1;
            var t = df < _tCritical.Length ? _tCritical[df] : 1.96;
            return t * rateStdError / rate;
        }

        return 1.96 / Math.Sqrt(Math.Max(1, Math.Min(s.Count, s.Times)));
    }

    /// <summary>
    /// Gets 关卡。
    /// </summary>
    public string Stage { get; }

    /// <summary>
    /// Gets 该物品在该关卡的掉落数。
    /// </summary>
    public int Count { get; }

    /// <summary>
    /// Gets 单件平均理智消耗。
    /// </summary>
    public double SanityPerItem { get; }

    /// <summary>
    /// Gets 单件理智 95% 置信区间的相对半宽。
    /// </summary>
    public double RelativeError { get; }

    /// <summary>
    /// Gets a value indicating whether 样本足够：掉落数不少于 <see cref="MinReliableCount"/> 且误差不超过 <see cref="MaxReliableError"/>。
    /// </summary>
    public bool IsReliable => Count >= MinReliableCount && RelativeError <= MaxReliableError;

    /// <summary>
    /// Gets 单件理智文本。
    /// </summary>
    public string ValueText { get; }

    /// <summary>
    /// Gets 误差文本（相对值）。
    /// </summary>
    public string ErrorText { get; }

    /// <summary>
    /// Gets 悬停提示：置信区间、掉落数与场次。
    /// </summary>
    public string ToolTip { get; }

    /// <summary>
    /// Gets or sets a value indicating whether 该行是当前关注的关卡（从按关卡浮层跳转时的来源关卡）。
    /// </summary>
    public bool IsHighlighted { get; set; }

    /// <summary>
    /// Gets 横条比例：基准单件理智与本关之比，越划算越长。
    /// </summary>
    public double BarRatio => _baseline / SanityPerItem;

    /// <summary>
    /// Gets 置信区间上界对应的横条比例相对 <see cref="BarRatio"/> 的倍数（单件理智取区间下限）；误差不小于 100% 时区间下限为 0，倍数为正无穷。
    /// </summary>
    public double UpperBoundFactor => RelativeError < 1 ? 1 / (1 - RelativeError) : double.PositiveInfinity;

    private double _baseline = 1;
    private double _scale = 1;

    /// <summary>
    /// 设置横条坐标，排名生成后、显示前调用一次。
    /// </summary>
    /// <param name="baseline">计算横条比例的基准单件理智。</param>
    /// <param name="scale">横条满长对应的比例，超出的部分截断在满长处。</param>
    public void SetBarScale(double baseline, double scale)
    {
        _baseline = baseline;
        _scale = scale;
    }

    // 实心部分画到置信区间的比例下界（单件理智取区间上限），误差带从下界画到上界
    private double BarLower => Math.Clamp(BarRatio / (1 + RelativeError) / _scale, 0.01, 1);

    private double BarUpper => Math.Clamp(BarRatio * UpperBoundFactor / _scale, BarLower, 1);

    /// <summary>
    /// Gets 横条实心部分所占的星号列宽。
    /// </summary>
    public GridLength BarWidth => new(BarLower, GridUnitType.Star);

    /// <summary>
    /// Gets 半透明误差带所占的星号列宽。
    /// </summary>
    public GridLength BandWidth => new(BarUpper - BarLower, GridUnitType.Star);

    /// <summary>
    /// Gets 横条右侧空白所占的星号列宽。
    /// </summary>
    public GridLength RestWidth => new(1 - BarUpper, GridUnitType.Star);
}
