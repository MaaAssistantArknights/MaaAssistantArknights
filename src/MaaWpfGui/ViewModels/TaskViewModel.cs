// <copyright file="TaskViewModel.cs" company="MaaAssistantArknights">
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

public abstract class TaskViewModel : PropertyChangedBase
{
    protected TaskViewModel()
    {
        PropertyDependsOnHelper.InitializePropertyDependencies(this);
    }

    protected T? GetTaskConfig<T>()
        where T : BaseTask
    {
        return ConfigFactory.CurrentConfig.TaskQueue[TaskSettingVisibilityInfo.Instance.CurrentIndex] as T;
    }

    protected bool SetTaskConfig<T>(Func<T, bool> isEqual, Action<T> @setValue, [CallerMemberName] string propertyName = "")
        where T : BaseTask
    {
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
    public virtual void RefreshUI(BaseTask baseTask)
    {
    }

    /// <summary>
    /// 序列化MAA任务
    /// </summary>
    /// <returns>返回(Asst任务类型, 参数)</returns>
    /// [Obsolete] 重构后弃用
    public abstract (AsstTaskType Type, JObject Params) Serialize();

    public virtual bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        return null;
    }
}
