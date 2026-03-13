// <copyright file="JsonPredictSerializationModifier.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Linq;
using System.Reflection;
using System.Text.Json.Serialization.Metadata;
using MaaWpfGui.Utilities;

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 为 <see cref="DefaultJsonTypeInfoResolver"/> 提供 Modifier：对带 <see cref="JsonPredictAttribute"/> 的属性按条件属性/路径与 <see cref="JsonPredictAttribute.CompareValue"/> 的比较结果决定是否序列化。
/// </summary>
public static class JsonPredictSerializationModifier
{
    private const BindingFlags PropertyFlags = BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;

    /// <summary>
    /// 修改类型元数据：对标记了 <see cref="JsonPredictAttribute"/> 的属性按条件设置是否参与序列化。
    /// </summary>
    /// <param name="typeInfo">当前正在解析的类型的 JSON 序列化元数据。</param>
    public static void Modify(JsonTypeInfo typeInfo)
    {
        if (typeInfo.Kind != JsonTypeInfoKind.Object)
        {
            return;
        }

        for (var t = typeInfo.Type; t != null; t = t.BaseType)
        {
            foreach (var pi in t.GetProperties(PropertyFlags))
            {
                if (pi.GetCustomAttribute<JsonPredictAttribute>() is not { } attr)
                {
                    continue;
                }

                var jsonProp = typeInfo.Properties.FirstOrDefault(p => p.Name == pi.Name);
                if (jsonProp == null)
                {
                    continue;
                }

                if (attr.ConditionPropertyName is not { } conditionName)
                {
                    continue;
                }

                var compareValue = attr.CompareValue;
                var whenEqual = attr.SerializeWhenEqual;
                jsonProp.ShouldSerialize = (parent, _) => ShouldSerializeByCondition(parent, conditionName, compareValue, whenEqual);
            }
        }
    }

    /// <summary>
    /// 根据 serializeWhenEqual：为 true 时仅当条件值等于 comparedValue 才序列化；为 false 时仅当不等于才序列化。
    /// conditionPropertyName 支持点号路径，如 "StagePlan.Count"。
    /// 条件值为 null 时：若 comparedValue 也为 null 且 serializeWhenEqual 为 true 则序列化，否则按 comparedValue 与 serializeWhenEqual 决定是否序列化。
    /// comparedValue 为 null 时，按 value 类型推断默认比较值：bool 视为与 true 比较，int 视为与 0 比较（此时语义为“不等于 0 才序列化”）；
    /// 若 value 为容器（IEnumerable/ICollection），则先将 value 替换为元素个数再按整数与 0 比较；其他类型不设默认比较值，会序列化。
    /// </summary>
    private static bool ShouldSerializeByCondition(object? parent, string conditionPropertyName, object? comparedValue, bool serializeWhenEqual = true)
    {
        if (parent == null)
        {
            return false;
        }

        var value = GetValueByPath(parent, conditionPropertyName);
        if (value is null)
        {
            if (serializeWhenEqual)
            {
                return comparedValue is null;
            }
            else
            {
                return comparedValue is not null;
            }
        }

        if (comparedValue is null)
        {
            // 容器：将 value 替换为元素个数，后续按整数处理
            if (value is ICollection col)
            {
                value = col.Count;
            }
            else if (value is IEnumerable enumerable && value is not string)
            {
                value = enumerable.Cast<object>().Count();
            }

            object? effectiveCompareValue = null;
            if (value is bool)
            {
                effectiveCompareValue = true;
            }
            else if (value is int or long or short or byte)
            {
                effectiveCompareValue = 0;
                serializeWhenEqual = !serializeWhenEqual; // 与 0 比较时语义相反：为 true 时仅当不等于 0 才序列化；为 false 时仅当等于 0 才序列化
            }

            // 其他类型 effectiveCompareValue 保持 null
            if (effectiveCompareValue != null)
            {
                bool isEqual = (effectiveCompareValue is int or long or short or byte) && (value is int or long or short or byte)
                    ? Convert.ToInt64(value) == Convert.ToInt64(effectiveCompareValue)
                    : Equals(value, effectiveCompareValue);
                return serializeWhenEqual ? isEqual : !isEqual;
            }

            return true;
        }

        var equal = Equals(value, comparedValue);
        return serializeWhenEqual ? equal : !equal;
    }

    /// <summary>
    /// 按点号路径从对象取值，如 "StagePlan.Count" 表示 obj.StagePlan.Count。
    /// </summary>
    private static object? GetValueByPath(object obj, string path)
    {
        const BindingFlags flags = BindingFlags.Public | BindingFlags.Instance;
        var current = obj;
        var segments = path.Split('.');

        for (var i = 0; i < segments.Length; i++)
        {
            if (current == null)
            {
                return null;
            }

            var segment = segments[i].Trim();
            if (segment.Length == 0)
            {
                return null;
            }

            var type = current.GetType();
            PropertyInfo? prop = null;
            for (var t = type; t != null; t = t.BaseType)
            {
                prop = t.GetProperty(segment, flags);
                if (prop != null)
                {
                    break;
                }
            }

            if (prop == null)
            {
                return null;
            }

            current = prop.GetValue(current);
            if (i == segments.Length - 1)
            {
                return current;
            }
        }

        return current;
    }
}
