// <copyright file="PropertyDependsOnUtility.cs" company="MaaAssistantArknights">
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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using Serilog;

namespace MaaWpfGui.Utilities;

/// <summary>
/// 提供对 PropertyDependsOnAttribute 的支持：当源属性改变时，自动通知依赖的属性。
/// 这是一个静态工具类，可以被 ViewModel 和 Screen 使用。
/// 支持跨实例依赖：使用 "OwnerType.PropertyName" 格式表示依赖于另一个实例的属性。
/// </summary>
public static class PropertyDependsOnUtility
{
    // 存储每个实例的属性依赖关系：使用 ConditionalWeakTable 避免内存泄漏
    private static readonly ConditionalWeakTable<object, Dictionary<string, List<string>>> _instanceDependencies = [];

    // 存储每个实例的事件处理器，以便在需要时可以取消订阅
    private static readonly ConditionalWeakTable<object, PropertyChangedEventHandler> _instanceHandlers = [];

    // 缓存每个类型的 MethodInfo，避免重复反射
    private static readonly ConcurrentDictionary<Type, MethodInfo?> _methodInfoCache = new();

    // 跨实例依赖：外部实例 → (外部属性名 → 依赖它的本实例列表)
    // 使用 object 作为 key（弱引用），value 中存储需要刷新的 (本实例, 依赖属性名列表)
    private static readonly Dictionary<object, Dictionary<string, List<(object Instance, List<string> DependentProperties)>>> _externalDependencies = [];

    // 跨实例事件处理器，用于取消订阅
    private static readonly Dictionary<object, PropertyChangedEventHandler> _externalHandlers = [];

    // 缓存静态属性访问器，key: "OwnerType.PropertyName" → Func<object?>
    private static readonly ConcurrentDictionary<string, Func<object?>?> _externalInstanceCache = new();

    /// <summary>
    /// 扫描指定实例的属性，查找 PropertyDependsOnAttribute 并设置通知。
    /// 从派生类型的构造函数中调用此方法。
    /// </summary>
    /// <param name="instance">要实现属性依赖的实例</param>
    /// <exception cref="ArgumentNullException">当 instance 为 null 时抛出</exception>
    public static void InitializePropertyDependencies(INotifyPropertyChanged instance)
    {
        ArgumentNullException.ThrowIfNull(instance);

        // 检查是否已经初始化，避免重复初始化和双重订阅
        if (_instanceDependencies.TryGetValue(instance, out _))
        {
            return;
        }

        var type = instance.GetType();
        var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);

        var propertyDependencies = new Dictionary<string, List<string>>(); // key: source property name, value: dependent property names

        // 跨实例依赖：外部实例 → (外部属性名 → 依赖它的本实例列表)
        // key 为 Type（OwnerType）或 string（旧的 "TypeName.PropertyName" 格式）
        var externalDependenciesByType = new Dictionary<Type, Dictionary<string, List<(object Instance, List<string> DependentProperties)>>>();

        // 第一步：构建依赖关系图
        // 无参 [PropertyDependsOn] 标记的属性会在任何 NotifyOfPropertyChange("") 时被刷新
        var emptyDependsOnProperties = new List<string>();

        foreach (var property in properties)
        {
            var dependsOnAttributes = property.GetCustomAttributes<PropertyDependsOnAttribute>(true);
            foreach (var attribute in dependsOnAttributes.Where(i => i.PropertyNames.Length > 0))
            {
                if (attribute.OwnerType != null)
                {
                    // 跨实例依赖（类型安全）：通过 OwnerType 直接解析
                    foreach (var sourcePropertyName in attribute.PropertyNames)
                    {
                        if (!externalDependenciesByType.TryGetValue(attribute.OwnerType, out var propMap))
                        {
                            propMap = [];
                            externalDependenciesByType[attribute.OwnerType] = propMap;
                        }

                        if (!propMap.TryGetValue(sourcePropertyName, out var instanceList))
                        {
                            instanceList = [];
                            propMap[sourcePropertyName] = instanceList;
                        }

                        instanceList.Add((instance, [property.Name]));
                    }
                }
                else
                {
                    foreach (var sourcePropertyName in attribute.PropertyNames)
                    {
                        if (sourcePropertyName.Contains('.'))
                        {
                            // 跨实例依赖（旧格式）："OwnerType.PropertyName" — 保留向后兼容
                            if (!externalDependenciesByType.TryGetValue(typeof(object), out var legacyMap))
                            {
                                legacyMap = [];
                                externalDependenciesByType[typeof(object)] = legacyMap;
                            }

                            if (!legacyMap.TryGetValue(sourcePropertyName, out var instanceList))
                            {
                                instanceList = [];
                                legacyMap[sourcePropertyName] = instanceList;
                            }

                            instanceList.Add((instance, [property.Name]));
                        }
                        else
                        {
                            // 同实例依赖
                            if (!propertyDependencies.TryGetValue(sourcePropertyName, out var dependents))
                            {
                                dependents = [];
                                propertyDependencies[sourcePropertyName] = dependents;
                            }

                            dependents.Add(property.Name);
                        }
                    }
                }
            }

            // 无参 [PropertyDependsOn] — 语言切换等全局刷新时触发
            if (dependsOnAttributes.Any(i => i.PropertyNames.Length == 0))
            {
                emptyDependsOnProperties.Add(property.Name);
            }
        }

        // 第二步：检测复杂循环依赖（使用DFS）
        foreach (var sourceProperty in propertyDependencies.Keys)
        {
            DetectCycle(sourceProperty, propertyDependencies, new HashSet<string>(), new List<string> { sourceProperty });
        }

        // 确保实例被注册（即使只有无参 [PropertyDependsOn] 或跨实例依赖）
        if (propertyDependencies.Count > 0 || emptyDependsOnProperties.Count > 0 || externalDependenciesByType.Count > 0)
        {
            _instanceDependencies.Add(instance, propertyDependencies);

            void handler(object? sender, PropertyChangedEventArgs e)
            {
                if (sender != instance)
                {
                    return;
                }

                // 如果 PropertyName 为空或 null，通知所有依赖属性（包括无参 [PropertyDependsOn] 标记的）
                if (string.IsNullOrEmpty(e.PropertyName))
                {
                    var allDependentProperties = propertyDependencies.Values.SelectMany(dependents => dependents).Distinct()
                        .Concat(emptyDependsOnProperties).Distinct();
                    foreach (var dependentProperty in allDependentProperties)
                    {
                        NotifyPropertyChange(instance, dependentProperty);
                    }
                    return;
                }

                if (!propertyDependencies.TryGetValue(e.PropertyName, out var dependentProperties))
                {
                    return;
                }

                foreach (var dependentProperty in dependentProperties)
                {
                    NotifyPropertyChange(instance, dependentProperty);
                }
            }

            _instanceHandlers.Add(instance, handler);
            instance.PropertyChanged += handler;
        }

        // 第三步：处理跨实例依赖
        foreach (var (ownerType, propMap) in externalDependenciesByType)
        {
            if (ownerType == typeof(object))
            {
                // 旧格式 "TypeName.PropertyName" 兼容处理
                foreach (var (fullPath, instanceEntries) in propMap)
                {
                    var dotIndex = fullPath.IndexOf('.');
                    var ownerTypeName = fullPath[..dotIndex];
                    var externalPropertyName = fullPath[(dotIndex + 1)..];
                    var externalInstance = ResolveExternalInstanceByName(ownerTypeName);
                    if (externalInstance is INotifyPropertyChanged notifyInstance)
                    {
                        RegisterExternalDependency(notifyInstance, externalPropertyName, instanceEntries);
                    }
                    else
                    {
                        Log.Warning("跨实例依赖 {Path}：找不到外部实例或它不支持 PropertyChanged", fullPath);
                    }
                }
            }
            else
            {
                // 类型安全格式：直接通过 OwnerType 解析实例
                var externalInstance = ResolveExternalInstanceByType(ownerType);
                if (externalInstance is not INotifyPropertyChanged notifyInstance)
                {
                    Log.Warning("跨实例依赖 {OwnerType}：找不到外部实例或它不支持 PropertyChanged", ownerType.Name);
                    continue;
                }

                foreach (var (propertyName, instanceEntries) in propMap)
                {
                    RegisterExternalDependency(notifyInstance, propertyName, instanceEntries);
                }
            }
        }
    }

    /// <summary>
    /// 注册一个外部实例的属性依赖。
    /// </summary>
    private static void RegisterExternalDependency(INotifyPropertyChanged externalInstance, string externalPropertyName, List<(object Instance, List<string> DependentProperties)> instanceEntries)
    {
        if (!_externalDependencies.TryGetValue(externalInstance, out var propMap))
        {
            propMap = [];
            _externalDependencies[externalInstance] = propMap;

            // 订阅外部实例的 PropertyChanged
            void externalHandler(object? sender, PropertyChangedEventArgs e)
            {
                Log.Debug("跨实例 handler 触发: sender={Sender}, property={Property}", sender?.GetType().Name, e.PropertyName);

                if (!_externalDependencies.TryGetValue(externalInstance, out var currentPropMap))
                {
                    return;
                }

                if (string.IsNullOrEmpty(e.PropertyName))
                {
                    foreach (var (_, instances) in currentPropMap)
                    {
                        foreach (var (inst, depProps) in instances)
                        {
                            foreach (var prop in depProps)
                            {
                                NotifyPropertyChange(inst, prop);
                            }
                        }
                    }

                    return;
                }

                if (currentPropMap.TryGetValue(e.PropertyName, out var affectedInstances))
                {
                    Log.Debug("跨实例依赖匹配: property={Property}, 实例数={Count}", e.PropertyName, affectedInstances.Count);
                    foreach (var (inst, depProps) in affectedInstances)
                    {
                        foreach (var prop in depProps)
                        {
                            NotifyPropertyChange(inst, prop);
                        }
                    }
                }
            }

            _externalHandlers[externalInstance] = externalHandler;
            externalInstance.PropertyChanged += externalHandler;
            Log.Debug("已注册跨实例依赖: externalType={Type}, property={Property}", externalInstance.GetType().Name, externalPropertyName);
        }

        if (!propMap.TryGetValue(externalPropertyName, out var existingList))
        {
            existingList = [];
            propMap[externalPropertyName] = existingList;
        }

        existingList.AddRange(instanceEntries);
    }

    /// <summary>
    /// 解析外部实例：通过类型名查找包含静态 "Instance" 属性的类型。
    /// </summary>
    private static object? ResolveExternalInstanceByName(string typeName)
    {
        var resolver = _externalInstanceCache.GetOrAdd(typeName, _ =>
        {
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var asm in assemblies)
            {
                Type? foundType = null;
                try
                {
                    foundType = asm.GetTypes().FirstOrDefault(t => t.Name == typeName || t.FullName == typeName);
                }
                catch
                {
                    continue;
                }

                if (foundType == null)
                {
                    continue;
                }

                return CreateInstanceAccessor(foundType);
            }

            return null;
        });

        return resolver?.Invoke();
    }

    /// <summary>
    /// 解析外部实例：通过 Type 直接查找。
    /// </summary>
    private static object? ResolveExternalInstanceByType(Type ownerType)
    {
        var resolver = _externalInstanceCache.GetOrAdd(ownerType.FullName!, _ => CreateInstanceAccessor(ownerType));
        return resolver?.Invoke();
    }

    /// <summary>
    /// 为指定类型创建实例访问器（查找静态 Instance 属性/字段）。
    /// </summary>
    private static Func<object?>? CreateInstanceAccessor(Type type)
    {
        // 查找静态属性 "Instance"
        var instanceProp = type.GetProperty("Instance", BindingFlags.Public | BindingFlags.Static);
        if (instanceProp != null && instanceProp.PropertyType.IsAssignableTo(typeof(INotifyPropertyChanged)))
        {
            return () => instanceProp.GetValue(null);
        }

        // 查找静态字段
        var instanceField = type.GetField("_instance", BindingFlags.NonPublic | BindingFlags.Static) ??
                            type.GetField("Instance", BindingFlags.Public | BindingFlags.Static);
        if (instanceField != null && instanceField.FieldType.IsAssignableTo(typeof(INotifyPropertyChanged)))
        {
            return () => instanceField.GetValue(null);
        }

        return null;
    }

    /// <summary>
    /// 取消指定实例的属性依赖初始化，移除事件订阅。
    /// </summary>
    /// <param name="instance">要取消初始化的实例</param>
    public static void UnInitializePropertyDependencies(INotifyPropertyChanged? instance)
    {
        if (instance == null)
        {
            return;
        }

        if (_instanceHandlers.TryGetValue(instance, out var handler))
        {
            instance.PropertyChanged -= handler;
            _instanceHandlers.Remove(instance);
        }

        _instanceDependencies.Remove(instance);
    }

    /// <summary>
    /// 使用深度优先搜索检测循环依赖。
    /// </summary>
    /// <param name="currentProperty">当前正在检查的属性</param>
    /// <param name="dependencies">属性依赖关系图</param>
    /// <param name="visited">已访问的属性集合</param>
    /// <param name="path">当前路径（用于显示循环）</param>
    private static void DetectCycle(string currentProperty, Dictionary<string, List<string>> dependencies, HashSet<string> visited, List<string> path)
    {
        if (visited.Contains(currentProperty))
        {
            // 找到循环：从 path 中找到循环的起点，构建循环路径
            var cycleStartIndex = path.IndexOf(currentProperty);
            var cycle = path.Skip(cycleStartIndex).Append(currentProperty).ToList();
            throw new ArgumentException($"检测到循环依赖: {string.Join(" -> ", cycle)}");
        }

        visited.Add(currentProperty);

        // 检查当前属性的所有依赖属性
        if (dependencies.TryGetValue(currentProperty, out var dependents))
        {
            foreach (var dependent in dependents)
            {
                var newPath = new List<string>(path) { dependent };
                DetectCycle(dependent, dependencies, visited, newPath);
            }
        }

        visited.Remove(currentProperty);
    }

    /// <summary>
    /// 通知属性改变。尝试使用 NotifyOfPropertyChange 方法（Stylet），如果不存在则记录警告。
    /// 使用缓存避免重复反射。
    /// </summary>
    private static void NotifyPropertyChange(object instance, string propertyName)
    {
        // 首先尝试使用反射调用 NotifyOfPropertyChange 方法（Stylet 的 PropertyChangedBase 和 Screen 都有这个方法）
        var type = instance.GetType();

        // 从缓存中获取或查找 MethodInfo
        var method = _methodInfoCache.GetOrAdd(type, t =>
            t.GetMethod("NotifyOfPropertyChange", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, [typeof(string)], null));

        if (method != null)
        {
            method.Invoke(instance, [propertyName]);
            return;
        }

        // 如果没有 NotifyOfPropertyChange 方法，记录警告
        Log.Warning("类型 {Type} 的属性 {PropertyName} 需要通知变更，但未找到 NotifyOfPropertyChange 方法", type.FullName, propertyName);
    }
}
