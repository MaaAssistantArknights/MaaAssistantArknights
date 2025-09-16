// <copyright file="TaskSettingsViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.ViewModels;
public abstract class TaskSettingsViewModel : PropertyChangedBase
{
    private readonly Dictionary<string, List<string>> _propertyDependencies = []; // 属性依赖关系, key为被订阅的属性名, value为依赖于该属性的属性名列表

    protected TaskSettingsViewModel()
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
                        throw new ArgumentException($"属性 {key} 依赖于属性 {property.Name}, 但它们之间存在循环依赖关系");
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

    protected T GetTaskConfig<T>()
        where T : BaseTask, new()
    {
        return TaskSettingVisibilityInfo.Instance.CurrentIndex >= 0 &&
            ConfigFactory.CurrentConfig.TaskQueue.Count > TaskSettingVisibilityInfo.Instance.CurrentIndex &&
            ConfigFactory.CurrentConfig.TaskQueue[TaskSettingVisibilityInfo.Instance.CurrentIndex] is T t ? t : new T();
    }

    protected bool SetTaskConfig<T>(Func<T, bool> isEqual, Action<T> @setValue, [CallerMemberName] string propertyName = "")
        where T : BaseTask
    {
        if (TaskSettingVisibilityInfo.Instance.CurrentIndex < 0 || TaskSettingVisibilityInfo.Instance.CurrentIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            return false;
        }

        if (ConfigFactory.CurrentConfig.TaskQueue[TaskSettingVisibilityInfo.Instance.CurrentIndex] is T task)
        {
            if (!isEqual(task))
            {
                setValue(task);
                NotifyOfPropertyChange(propertyName);
                return true;
            }
        }

        return false;
    }

    public virtual void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
    }

    /// <summary>
    /// 刷新UI
    /// </summary>
    /// <param name="baseTask">需要刷新的任务</param>
    public abstract void RefreshUI(BaseTask baseTask);

    /// <summary>
    /// 序列化MAA任务
    /// </summary>
    /// <returns>返回(Asst任务类型, 参数)</returns>
    [Obsolete("使用SerializeTask作为代替")]
    public abstract (AsstTaskType Type, JObject Params) Serialize();

    /// <summary>序列化任务</summary>
    /// <param name="baseTask">存储的任务</param>
    /// <param name="taskId">任务id, null时追加任务, 非null为设置任务参数</param>
    /// <returns>null为未序列化, false失败, true成功</returns>
    public abstract bool? SerializeTask(BaseTask baseTask, int? taskId = null);
}
