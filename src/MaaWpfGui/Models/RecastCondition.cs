// <copyright file="RecastCondition.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Constants;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.Models;

/// <summary>
/// Represents a condition for stopping the roguelike coppers recast operation.
/// 重新投钱停止条件
/// </summary>
public class RecastCondition : PropertyChangedBase
{
    /// <summary>
    /// Gets or sets the metric type (监控项).
    /// </summary>
    private string _metric = RecastConstants.MetricTypes.RecastCount;

    /// <summary>
    /// Gets or sets the metric type (监控项).
    /// </summary>
    [JsonProperty("metric")]
    public string Metric
    {
        get => _metric;
        set => SetAndNotify(ref _metric, value);
    }

    /// <summary>
    /// Gets or sets the comparator (比较方式).
    /// </summary>
    private string _comparator = RecastConstants.ComparatorTypes.Equal;

    /// <summary>
    /// Gets or sets the comparator (比较方式).
    /// </summary>
    [JsonProperty("comparator")]
    public string Comparator
    {
        get => _comparator;
        set => SetAndNotify(ref _comparator, value);
    }

    /// <summary>
    /// Gets or sets the threshold value (阈值).
    /// </summary>
    private int _threshold;

    /// <summary>
    /// Gets or sets the threshold value (阈值).
    /// </summary>
    [JsonProperty("threshold")]
    public int Threshold
    {
        get => _threshold;
        set => SetAndNotify(ref _threshold, value);
    }

    /// <summary>
    /// Gets the display text for metric.
    /// </summary>
    [JsonIgnore]
    public string MetricDisplay => Metric switch
    {
        RecastConstants.MetricTypes.RecastCount => Helper.LocalizationHelper.GetString("MiniGame@CoppersRecastMetricCount"),
        RecastConstants.MetricTypes.Hp => Helper.LocalizationHelper.GetString("MiniGame@CoppersRecastMetricHp"),
        RecastConstants.MetricTypes.Hope => Helper.LocalizationHelper.GetString("MiniGame@CoppersRecastMetricHope"),
        RecastConstants.MetricTypes.Ingot => Helper.LocalizationHelper.GetString("MiniGame@CoppersRecastMetricIngot"),
        RecastConstants.MetricTypes.Ticket => Helper.LocalizationHelper.GetString("MiniGame@CoppersRecastMetricTicket"),
        _ => Metric,
    };

    /// <summary>
    /// Gets the display text for comparator.
    /// </summary>
    [JsonIgnore]
    public string ComparatorDisplay => Comparator switch
    {
        RecastConstants.ComparatorTypes.Less => "<",
        RecastConstants.ComparatorTypes.LessOrEqual => "≤",
        RecastConstants.ComparatorTypes.Equal => "=",
        RecastConstants.ComparatorTypes.Greater => ">",
        RecastConstants.ComparatorTypes.GreaterOrEqual => "≥",
        _ => Comparator,
    };

    /// <summary>
    /// Creates a deep copy of this condition.
    /// </summary>
    /// <returns>A new instance of <see cref="RecastCondition"/> with the same property values.</returns>
    public RecastCondition Clone()
    {
        return new RecastCondition
        {
            Metric = Metric,
            Comparator = Comparator,
            Threshold = Threshold,
        };
    }

    /// <summary>
    /// Converts this condition to a JObject for task parameters.
    /// </summary>
    /// <returns>A JObject containing the condition data.</returns>
    public JObject ToJObject()
    {
        return new JObject
        {
            ["metric"] = Metric,
            ["comparator"] = Comparator,
            ["threshold"] = Threshold,
        };
    }
}
