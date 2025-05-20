// <copyright file="TaskViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Main;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.ViewModels;

public abstract class TaskViewModel : PropertyChangedBase
{
    private readonly Dictionary<string, List<string>> _propertyDependencies = []; // 属性依赖关系, key为被订阅的属性名, value为依赖于该属性的属性名列表

    protected TaskViewModel()
    {
        InitializePropertyDependencies();
    }

    /// <summary>
    /// 初始化属性依赖关系
    /// </summary>
    private void InitializePropertyDependencies()
    {
        var type = GetType();
        var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);

        foreach (var property in properties)
        {
            var dependsOnAttributes = property.GetCustomAttributes<PropertyDependsOnAttribute>(true);
            foreach (var attribute in dependsOnAttributes.Where(i => i.PropertyNames.Length > 0))
            {
                foreach (var key in attribute.PropertyNames)
                {
                    if (!_propertyDependencies.TryGetValue(key, out var value))
                    {
                        value = [];
                        _propertyDependencies[key] = value;
                    }

                    if (_propertyDependencies.TryGetValue(property.Name, out var values) && values.Contains(key))
                    {
                        throw new ArgumentException($"属性 {property.Name} 依赖于属性 {key}，但它们之间存在循环依赖关系");
                    }

                    value.Add(property.Name);
                }
            }
        }

        PropertyChanged += OnPropertyChanged;
    }

    private void OnPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.IsNullOrEmpty(e.PropertyName) || !_propertyDependencies.TryGetValue(e.PropertyName, out var value))
        {
            return;
        }

        foreach (var dependentProperty in value)
        {
            NotifyOfPropertyChange(dependentProperty);
        }
    }

    public virtual void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
    }

    /// <summary>
    /// 序列化MAA任务
    /// </summary>
    /// <returns>返回(Asst任务类型, 参数)</returns>
    public abstract (AsstTaskType Type, JObject Params) Serialize();
}
