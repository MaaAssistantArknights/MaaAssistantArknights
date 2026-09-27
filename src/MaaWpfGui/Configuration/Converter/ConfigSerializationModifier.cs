// <copyright file="ConfigSerializationModifier.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization.Metadata;
using MaaWpfGui.Configuration.Single;
using MaaWpfGui.Configuration.Single.MaaTask;
using ObservableCollections;

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 序列化配置集合的浅快照，避免枚举器持有原集合锁时再读取子对象或触发属性求值。
/// </summary>
internal static class ConfigSerializationModifier
{
    /// <summary>
    /// 只替换序列化的取值委托，保留默认反序列化及其完整错误路径，供根配置容错恢复使用。
    /// </summary>
    /// <param name="typeInfo">当前类型的 JSON 元数据。</param>
    public static void Modify(JsonTypeInfo typeInfo)
    {
        if (typeInfo.Kind != JsonTypeInfoKind.Object)
        {
            return;
        }

        foreach (var property in typeInfo.Properties)
        {
            if (property.Get is not { } getter)
            {
                continue;
            }

            if (property.PropertyType == typeof(ObservableDictionary<string, SpecificConfig>))
            {
                property.Get = obj => {
                    if (getter(obj) is not ObservableDictionary<string, SpecificConfig> configurations)
                    {
                        return null;
                    }

                    lock (configurations.SyncRoot)
                    {
                        return new ObservableDictionary<string, SpecificConfig>(configurations, configurations.Comparer);
                    }
                };
            }
            else if (property.PropertyType == typeof(ObservableList<BaseTask>))
            {
                property.Get = obj => {
                    if (getter(obj) is not ObservableList<BaseTask> taskQueue)
                    {
                        return null;
                    }

                    // Count 与 CopyTo 必须处于同一个锁内，避免复制期间集合大小发生变化。
                    lock (taskQueue.SyncRoot)
                    {
                        return new ObservableList<BaseTask>(taskQueue);
                    }
                };
            }
        }
    }
}
