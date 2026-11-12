// <copyright file="TaskSettingsViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Runtime.CompilerServices;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels;

public abstract class TaskSettingsViewModel : PropertyChangedBase
{
    protected TaskSettingsViewModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);

        // 本类型子类均为与应用同生命周期的 Instance 单例，转发订阅后无需取消订阅
        TaskSettingVisibilityInfo.Instance.PropertyChanged += ForwardTaskSettingVisibilityChanged;
    }

    protected bool IsRefreshingUI { get; set; } = false; // 需手动赋值

    /// <summary>
    /// 任务设置读写的任务队列数据源。默认为当前配置的真实任务队列，子类可覆写为自有队列（如沙盒任务列表）。
    /// </summary>
    protected virtual IList<BaseTask> TaskConfigQueue => ConfigFactory.CurrentConfig.TaskQueue;

    /// <summary>
    /// 任务设置读写的当前任务在 <see cref="TaskConfigQueue"/> 中的索引。默认为真实任务队列的选中索引，子类可覆写为自有索引。
    /// </summary>
    protected virtual int TaskConfigIndex => TaskSettingVisibilityInfo.Instance.CurrentIndex;

    /// <summary>
    /// 任务设置面板当前是否显示高级设置区。默认转发全局 TaskSettingVisibilityInfo 的同名开关，子类可覆写为自有数据源。
    /// </summary>
    public virtual bool EnableAdvancedSettings => TaskSettingVisibilityInfo.Instance.EnableAdvancedSettings;

    /// <summary>
    /// 当前选中的任务设置是否处于运行中。默认转发全局 TaskSettingVisibilityInfo 的同名状态，子类可覆写为自有数据源。
    /// </summary>
    public virtual bool IsCurrentTaskRunning => TaskSettingVisibilityInfo.Instance.IsCurrentTaskRunning;

    /// <summary>
    /// 将全局任务设置可见性信息（普通/高级切换、当前任务运行状态）的变更转发为本 Model 的属性通知，
    /// 使任务控件 XAML 经 DataContext 绑定转发属性时仍能收到变更通知。
    /// </summary>
    private void ForwardTaskSettingVisibilityChanged(object? sender, PropertyChangedEventArgs e)
    {
        switch (e.PropertyName)
        {
            case nameof(EnableAdvancedSettings):
                NotifyOfPropertyChange(nameof(EnableAdvancedSettings));
                break;
            case nameof(IsCurrentTaskRunning):
                NotifyOfPropertyChange(nameof(IsCurrentTaskRunning));
                break;
        }
    }

    protected T GetTaskConfig<T>()
        where T : notnull, BaseTask, new()
    {
        var queue = TaskConfigQueue;
        var index = TaskConfigIndex;
        return index >= 0 && queue.Count > index && queue[index] is T t ? t : new T();
    }

    protected bool SetTaskConfig<T>(Func<T, bool> isEqual, Action<T> setValue, [CallerMemberName] string propertyName = "")
        where T : BaseTask
    {
        var queue = TaskConfigQueue;
        var index = TaskConfigIndex;
        if (index < 0 || index >= queue.Count)
        {
            return false;
        }

        if (queue[index] is T task)
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

    protected T? GetConfigByTaskId<T>(int taskId)
        where T : notnull, BaseTask, new()
    {
        var index = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(t => t.TaskIds.Contains(taskId))?.Index;
        if (index is null || index < 0 || index >= ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            return null;
        }
        return ConfigFactory.CurrentConfig.TaskQueue[index.Value] is T t ? t : null;
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
    public abstract (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null);
}

public interface ITaskQueueModelSerialize
{
    /// <inheritdoc cref="TaskSettingsViewModel.SerializeTask"/>
    public abstract (bool? IsSuccess, IEnumerable<int> TaskId) Serialize(BaseTask? baseTask, int? taskId);

    protected static (bool? IsSuccess, IEnumerable<int> TaskId) FromSingle((bool? IsSuccess, int TaskId) result) => (result.IsSuccess, result.TaskId > 0 ? [result.TaskId] : []);
}
