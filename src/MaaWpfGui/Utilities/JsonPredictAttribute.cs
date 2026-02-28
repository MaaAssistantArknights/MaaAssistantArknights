// <copyright file="JsonPredictAttribute.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Utilities;

/// <summary>
/// 标记在 JSON 序列化时是否忽略该属性。按条件属性/路径与 <see cref="CompareValue"/> 的比较结果决定是否序列化（由 <see cref="SerializeWhenEqual"/> 控制等于或不等于）。
/// </summary>
[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false)]
public class JsonPredictAttribute : Attribute
{
    /// <summary>
    /// Gets 条件属性名或点号路径。仅当该路径在父对象上的值满足与 <see cref="CompareValue"/> 的比较时才序列化（由 <see cref="SerializeWhenEqual"/> 决定等于或不等于）。
    /// 支持多级路径，如 <c>"StagePlan.Count"</c>。
    /// </summary>
    public string? ConditionPropertyName { get; }

    /// <summary>
    /// Gets 仅当条件值等于此值时才序列化；不等于则跳过序列化。未指定时，对 bool 条件视为 true（为 true 时才序列化）。
    /// 使用示例：<c>[JsonPredict("StagePlan.Count", 0)]</c> 表示仅当 StagePlan.Count == 0 时序列化本属性。
    /// </summary>
    public object? CompareValue { get; }

    /// <summary>
    /// Gets a value indicating whether 为 true（默认）时：条件值等于 <see cref="CompareValue"/> 才序列化；为 false 时：条件值不等于该值才序列化。
    /// </summary>
    public bool SerializeWhenEqual { get; }

    /// <summary>
    /// Initializes a new instance of the <see cref="JsonPredictAttribute"/> class。按条件属性控制：仅当条件属性与 <paramref name="value"/> 满足比较条件时序列化（由 <paramref name="serializeWhenEqual"/> 决定等于还是不等于）。
    /// </summary>
    /// <param name="conditionPropertyName">同一类型上的属性名（字符串常量，如 "UseWeeklySchedule"、"StoneCount"）</param>
    /// <param name="value">参与比较的值（常量，如 true、0）</param>
    /// <param name="serializeWhenEqual">为 true（默认）时：等于该值才序列化；为 false 时：不等于该值才序列化</param>
    public JsonPredictAttribute(string conditionPropertyName, object? value = null, bool serializeWhenEqual = true)
    {
        ConditionPropertyName = conditionPropertyName ?? throw new ArgumentNullException(nameof(conditionPropertyName));
        CompareValue = value;
        SerializeWhenEqual = serializeWhenEqual;
    }
}
