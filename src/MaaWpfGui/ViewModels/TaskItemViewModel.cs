// <copyright file="TaskItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;

namespace MaaWpfGui.ViewModels;

public class TaskItemViewModel : PropertyChangedBase, IDisposable
{
    public TaskItemViewModel(bool? isCheckedWithNull = true)
    {
        _isEnable = isCheckedWithNull;
        Instances.AsstProxy.OnTaskStatusChanged += OnTaskStatusChanged;
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    /// <summary>
    /// Gets or sets 显示名称：优先返回用户自定义名称，否则返回本地化任务类型名。
    /// 始终从配置动态读取，语言切换或任务排序（Index 变化）时通过
    /// <see cref="PropertyDependsOnAttribute"/> 自动刷新。
    /// </summary>
    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    [PropertyDependsOn(nameof(Index))]
    public string Name
    {
        get => ConfigFactory.CurrentConfig.TaskQueue[Index].NameOrTaskType;
        set {
            ConfigFactory.CurrentConfig.TaskQueue[Index].Name = value;
            NotifyOfPropertyChange();
        }
    }

    private bool? _isEnable;

    public bool? IsEnable
    {
        get => _isEnable;
        set {
            if (!SetAndNotify(ref _isEnable, value))
            {
                return;
            }

            ConfigFactory.CurrentConfig.TaskQueue[Index].IsEnable = value;
            StatusDisplay = TaskItemStatus.Idle;
        }
    }

    public int Index { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets a value indicating whether the task can be copied.
    /// 开始唤醒任务至多一个，开始唤醒行禁止复制，其余行不受限。
    /// </summary>
    [PropertyDependsOn(nameof(Index))]
    public bool CanCopy => ConfigFactory.CurrentConfig.TaskQueue[Index] is not StartUpTask;

    /// <summary>
    /// Gets or sets a value indicating whether gets or sets whether the setting enabled.
    /// </summary>
    public bool EnableSetting
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            TaskSettingVisibilityInfo.Instance.Set(Index, value);
        }
    }

    /// <summary>
    /// Gets or sets 任务id，默认为[]，添加后任务id应 > 0；执行后应置为[]
    /// </summary>
    private List<int> _taskIds = [];

    public IReadOnlyList<int> TaskIds => _taskIds;

    public void SetTaskIds(IEnumerable<int> taskIds)
    {
        _taskIds = [.. taskIds];
        StatusList = [.. Enumerable.Repeat(TaskItemStatus.Idle, TaskIds.Count)];
    }

    private List<TaskItemStatus> StatusList { get; set; } = [];

    /// <summary>
    /// Gets 本条目已处理（完成或出错）的 chain 数，作为任务栏进度分子的组成单元（Error chain 视作已处理一格）；
    /// 与 <see cref="StatusDisplay"/> 的条目级聚合显示互不相干：聚合值不变（如部分 chain 完成后条目仍显示
    /// InProgress）时本计数仍在推进。
    /// </summary>
    public int CompletedChainCount => StatusList.Count(s => s is TaskItemStatus.Completed or TaskItemStatus.Error);

    /// <summary>
    /// Gets or sets 上次状态, 可能和当前不一致
    /// </summary>
    public TaskItemStatus StatusDisplay
    {
        get => field;
        set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            // 条目状态变化时联动重算主任务进度（幂等；非主任务轮次分母为 0，不产生进度）
            Instances.TaskQueueViewModel?.RefreshMainTasksProgress();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 该条目的 chain 计数计入本轮任务栏进度（与分母同源同生命周期：
    /// LinkStart 序列化成功时置位，新一轮开始与回到空闲时随分母一并复位）。
    /// 后台异步完成的赋值（如一图流 OpenAPI 拉取）可能晚于所属轮次结束才落地，不携带轮次上下文，
    /// 凭此标记被进度分子排除，避免跨轮污染。不驱动 UI，无需变更通知。
    /// </summary>
    public bool ParticipatesInCurrentRun { get; set; }

    private void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        if (taskId < 0)
        {
            return;
        }
        int index = _taskIds.IndexOf(taskId);
        if (index < 0)
        {
            return;
        }
        StatusList[index] = status;

        // chain 级进度分子随本列表变化，须在此显式重算：StatusDisplay 聚合结果可能同值
        // （如某 chain 完成后条目仍显示 InProgress），setter 同值短路不会触发那里的联动
        Instances.TaskQueueViewModel?.RefreshMainTasksProgress();

        if (StatusList.Any(s => s == TaskItemStatus.Error))
        {
            StatusDisplay = TaskItemStatus.Error;
        }
        else if (StatusList.Any(s => s == TaskItemStatus.InProgress))
        {
            StatusDisplay = TaskItemStatus.InProgress;
        }
        else if (StatusList.All(s => s == TaskItemStatus.Completed))
        {
            StatusDisplay = TaskItemStatus.Completed;
        }
        else if (StatusList.Any(s => s == TaskItemStatus.Skipped))
        {
            StatusDisplay = TaskItemStatus.Skipped;
        }
        else
        {
            StatusDisplay = TaskItemStatus.Idle;
        }
    }

    void IDisposable.Dispose()
    {
        Instances.AsstProxy.OnTaskStatusChanged -= OnTaskStatusChanged;

        // 清理跨实例依赖注册，避免 Dispose 后被静态 _externalDependencies 强引用持有
        // （导致内存泄漏与语言切换时的僵尸通知）
        PropertyDependsOnUtility.UnInitializePropertyDependencies(this);
        GC.SuppressFinalize(this);
    }
}
