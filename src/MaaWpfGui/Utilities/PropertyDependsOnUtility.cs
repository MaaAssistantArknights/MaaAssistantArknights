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
/// </summary>
public static class PropertyDependsOnUtility
{
    // 存储每个实例的属性依赖关系：使用 ConditionalWeakTable 避免内存泄漏
    private static readonly ConditionalWeakTable<object, Dictionary<string, List<string>>> _instanceDependencies = [];

    // 存储每个实例的事件处理器，以便在需要时可以取消订阅
    private static readonly ConditionalWeakTable<object, PropertyChangedEventHandler> _instanceHandlers = [];

    /// <summary>
    /// 扫描指定实例的属性，查找 PropertyDependsOnAttribute 并设置通知。
    /// 从派生类型的构造函数中调用此方法。
    /// </summary>
    /// <param name="instance">要实现属性依赖的实例</param>
    /// <exception cref="ArgumentNullException">当 instance 为 null 时抛出</exception>
    public static void InitializePropertyDependencies(INotifyPropertyChanged instance)
    {
        ArgumentNullException.ThrowIfNull(instance);

        var type = instance.GetType();
        var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);

        var propertyDependencies = new Dictionary<string, List<string>>(); // key: source property name, value: dependent property names

        // 第一步：构建依赖关系图
        foreach (var property in properties)
        {
            var dependsOnAttributes = property.GetCustomAttributes<PropertyDependsOnAttribute>(true);
            foreach (var attribute in dependsOnAttributes.Where(i => i.PropertyNames.Length > 0))
            {
                foreach (var sourcePropertyName in attribute.PropertyNames)
                {
                    if (!propertyDependencies.TryGetValue(sourcePropertyName, out var dependents))
                    {
                        dependents = [];
                        propertyDependencies[sourcePropertyName] = dependents;
                    }

                    dependents.Add(property.Name);
                }
            }
        }

        // 第二步：检测复杂循环依赖（使用DFS）
        foreach (var sourceProperty in propertyDependencies.Keys)
        {
            DetectCycle(sourceProperty, propertyDependencies, new HashSet<string>(), new List<string> { sourceProperty });
        }

        if (propertyDependencies.Count > 0)
        {
            _instanceDependencies.Add(instance, propertyDependencies);

            void handler(object? sender, PropertyChangedEventArgs e)
            {
                if (sender != instance)
                {
                    return;
                }

                if (string.IsNullOrEmpty(e.PropertyName) || !propertyDependencies.TryGetValue(e.PropertyName, out var dependentProperties))
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
    /// </summary>
    private static void NotifyPropertyChange(object instance, string propertyName)
    {
        // 首先尝试使用反射调用 NotifyOfPropertyChange 方法（Stylet 的 PropertyChangedBase 和 Screen 都有这个方法）
        var type = instance.GetType();
        var method = type.GetMethod("NotifyOfPropertyChange", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, new[] { typeof(string) }, null);

        if (method != null)
        {
            method.Invoke(instance, [propertyName]);
            return;
        }

        // 如果没有 NotifyOfPropertyChange 方法，记录警告
        Log.Warning("类型 {Type} 的属性 {PropertyName} 需要通知变更，但未找到 NotifyOfPropertyChange 方法", type.FullName, propertyName);
    }
}
