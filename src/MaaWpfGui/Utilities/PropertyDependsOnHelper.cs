// <copyright file="PropertyDependsOnHelper.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Utilities;

/// <summary>
/// 提供对 PropertyDependsOnAttribute 的支持：当源属性改变时，自动通知依赖的属性。
/// 这是一个静态工具类，可以被 ViewModel 和 Screen 使用。
/// </summary>
public static class PropertyDependsOnHelper
{
    // 存储每个实例的属性依赖关系：使用 ConditionalWeakTable 避免内存泄漏
    private static readonly ConditionalWeakTable<object, Dictionary<string, List<string>>> _instanceDependencies = new();

    // 存储每个实例的事件处理器，以便在需要时可以取消订阅
    private static readonly ConditionalWeakTable<object, PropertyChangedEventHandler> _instanceHandlers = new();

    /// <summary>
    /// 扫描指定实例的属性，查找 PropertyDependsOnAttribute 并设置通知。
    /// 从派生类型的构造函数中调用此方法。
    /// </summary>
    /// <param name="instance">要实现属性依赖的实例（必须实现 INotifyPropertyChanged）</param>
    /// <exception cref="ArgumentNullException">当 instance 为 null 时抛出</exception>
    /// <exception cref="ArgumentException">当 instance 不实现 INotifyPropertyChanged 时抛出</exception>
    public static void InitializePropertyDependencies(object instance)
    {
        ArgumentNullException.ThrowIfNull(instance);

        if (instance is not INotifyPropertyChanged notifyPropertyChanged)
        {
            throw new ArgumentException("实例必须实现 INotifyPropertyChanged 接口", nameof(instance));
        }

        var type = instance.GetType();
        var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);

        var propertyDependencies = new Dictionary<string, List<string>>(); // key: source property name, value: dependent property names

        foreach (var property in properties)
        {
            var dependsOnAttributes = property.GetCustomAttributes<PropertyDependsOnAttribute>(true);
            foreach (var attribute in dependsOnAttributes.Where(i => i.PropertyNames.Length > 0))
            {
                foreach (var key in attribute.PropertyNames)
                {
                    if (!propertyDependencies.TryGetValue(key, out var value))
                    {
                        value = [];
                        propertyDependencies[key] = value;
                    }

                    if (propertyDependencies.TryGetValue(property.Name, out var values) && values.Contains(key))
                    {
                        throw new ArgumentException($"属性 {key} 依赖于属性 {property.Name}, 但它们之间存在循环依赖关系");
                    }

                    value.Add(property.Name);
                }
            }
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
            notifyPropertyChanged.PropertyChanged += handler;
        }
    }

    /// <summary>
    /// 取消指定实例的属性依赖初始化，移除事件订阅。
    /// </summary>
    /// <param name="instance">要取消初始化的实例</param>
    public static void UninitializePropertyDependencies(object instance)
    {
        if (instance == null)
        {
            return;
        }

        if (instance is INotifyPropertyChanged notifyPropertyChanged && _instanceHandlers.TryGetValue(instance, out var handler))
        {
            notifyPropertyChanged.PropertyChanged -= handler;
            _instanceHandlers.Remove(instance);
        }

        _instanceDependencies.Remove(instance);
    }

    /// <summary>
    /// 通知属性改变。尝试使用 NotifyOfPropertyChange 方法（Stylet），如果不存在则直接触发 PropertyChanged 事件。
    /// </summary>
    private static void NotifyPropertyChange(object instance, string propertyName)
    {
        // 首先尝试使用反射调用 NotifyOfPropertyChange 方法（Stylet 的 PropertyChangedBase 和 Screen 都有这个方法）
        var type = instance.GetType();
        var method = type.GetMethod("NotifyOfPropertyChange", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, new[] { typeof(string) }, null);

        if (method != null)
        {
            method.Invoke(instance, new object[] { propertyName });
            return;
        }

        // 如果没有 NotifyOfPropertyChange 方法，尝试直接触发 PropertyChanged 事件
        if (instance is INotifyPropertyChanged notifyPropertyChanged)
        {
            var eventField = type.GetField("PropertyChanged", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Public);
            if (eventField?.GetValue(instance) is PropertyChangedEventHandler eventHandler)
            {
                eventHandler.Invoke(instance, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}

