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
    protected TaskSettingsViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    protected bool IsRefreshingUI { get; set; } = false; // 需手动赋值

    protected T GetTaskConfig<T>()
        where T : BaseTask, new()
    {
        return TaskSettingVisibilityInfo.Instance.CurrentIndex >= 0 &&
            ConfigFactory.CurrentConfig.TaskQueue.Count > TaskSettingVisibilityInfo.Instance.CurrentIndex &&
            ConfigFactory.CurrentConfig.TaskQueue[TaskSettingVisibilityInfo.Instance.CurrentIndex] is T t ? t : new T();
    }

    protected bool SetTaskConfig<T>(Func<T, bool> isEqual, Action<T> setValue, [CallerMemberName] string propertyName = "")
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

    // 计划重构为event
    public virtual void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
    }

    /// <summary>
    /// 刷新UI
    /// </summary>
    /// <param name="baseTask">需要刷新的任务</param>
    public abstract void RefreshUI(BaseTask baseTask);

    /// <summary>序列化任务</summary>
    /// <param name="baseTask">存储的任务</param>
    /// <param name="taskId">任务id, null时追加任务, 非null为设置任务参数</param>
    /// <returns>null为未序列化, false失败, true成功</returns>
    public abstract bool? SerializeTask(BaseTask? baseTask, int? taskId = null);
}
