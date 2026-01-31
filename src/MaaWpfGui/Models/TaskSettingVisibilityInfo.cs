// <copyright file="TaskSettingVisibilityInfo.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Serilog;
using Stylet;

namespace MaaWpfGui.Models;

/// <summary>
/// The task setting enable info.
/// </summary>
public class TaskSettingVisibilityInfo : PropertyChangedBase
{
    public TaskSettingVisibilityInfo()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    // public const string DefaultVisibleTaskSetting = "Combat";
    public bool StartUp { get => field; set => SetAndNotify(ref field, value); }

    public bool Recruit { get => field; set => SetAndNotify(ref field, value); }

    public bool Infrast { get => field; set => SetAndNotify(ref field, value); }

    public bool Fight { get => field; set => SetAndNotify(ref field, value); }

    public bool Mall { get => field; set => SetAndNotify(ref field, value); }

    public bool Award { get => field; set => SetAndNotify(ref field, value); }

    public bool Roguelike { get => field; set => SetAndNotify(ref field, value); }

    public bool Reclamation { get => field; set => SetAndNotify(ref field, value); }

    public bool Custom { get => field; set => SetAndNotify(ref field, value); }

    public bool PostAction { get => field; set => SetAndNotify(ref field, value); }

    public static TaskSettingVisibilityInfo Instance { get; } = new();

    // 长草任务当前选中
    public int CurrentIndex
    {
        get {
            if (ConfigFactory.CurrentConfig.TaskSelectedIndex < -1 || ConfigFactory.CurrentConfig.TaskSelectedIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
            {
                ConfigFactory.CurrentConfig.TaskSelectedIndex = -1;
            }

            return ConfigFactory.CurrentConfig.TaskSelectedIndex;
        }

        set {
            ConfigFactory.CurrentConfig.TaskSelectedIndex = value;
            NotifyOfPropertyChange();
        }
    }

    [PropertyDependsOn(nameof(CurrentIndex))]
    public bool IsCurrentTaskRunning =>
        ConfigFactory.CurrentConfig.TaskQueue.Count > CurrentIndex &&
        CurrentIndex >= 0 &&
        Instances.AsstProxy.TasksStatus.TryGetValue(Instances.TaskQueueViewModel.TaskItemViewModels[CurrentIndex].TaskId, out var status) &&
        status.Status == TaskStatus.InProgress;

    public static BaseTask? CurrentTask =>
        ConfigFactory.CurrentConfig.TaskQueue.Count > Instance.CurrentIndex &&
        Instance.CurrentIndex >= 0 ? ConfigFactory.CurrentConfig.TaskQueue[Instance.CurrentIndex] : null;

    public void NotifyOfTaskStatus()
    {
        NotifyOfPropertyChange(nameof(IsCurrentTaskRunning));
    }

    /// <summary>
    /// 修改任务设置可见性, 用于 TaskQueueItem 设置按钮的切换.
    /// </summary>
    /// <param name="taskIndex">index</param>
    /// <param name="enable">启用与否</param>
    public void Set(int taskIndex, bool enable)
    {
        if (Guide && enable)
        {
            enable = false;
        }

        // 边界检查
        if (taskIndex < 0 || taskIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            Log.Error("尝试设置不存在的任务设置可见性, 索引: {TaskIndex}", taskIndex);
            return;
        }

        var task = ConfigFactory.CurrentConfig.TaskQueue[taskIndex];
        if (enable)
        {
            CurrentIndex = taskIndex;
            SetTaskSettingVisible(task, enable);
        }
        else if (CurrentIndex == taskIndex)
        {
            CurrentIndex = -1;
            SetTaskSettingVisible(task, enable);
        }

        if (enable)
        {
            Instances.TaskQueueViewModel.RefreshTaskModel(task);
        }
    }

    public void SetTaskSettingVisible(BaseTask task, bool enable)
    {
        if (enable)
        {
            ResetVisible();
        }
        _ = task switch {
            StartUpTask => StartUp = enable,
            RecruitTask => Recruit = enable,
            InfrastTask => Infrast = enable,
            FightTask => Fight = enable,
            MallTask => Mall = enable,
            AwardTask => Award = enable,
            RoguelikeTask => Roguelike = enable,
            ReclamationTask => Reclamation = enable,
            CustomTask => Custom = enable,
            _ => throw new NotImplementedException(),
        };
        EnableAdvancedSettings = false;
        AdvancedSettingsVisibility = !Award && !StartUp;
    }

    private void ResetVisible()
    {
        StartUp = false;
        Recruit = false;
        Infrast = false;
        Fight = false;
        Mall = false;
        Award = false;
        Roguelike = false;
        Reclamation = false;
        Custom = false;
    }

    public void SetPostAction(bool value)
    {
        if (value)
        {
            ResetVisible();
        }
        PostAction = value;
    }

    public bool EnableAdvancedSettings { get => field; set => SetAndNotify(ref field, value); }

    public bool AdvancedSettingsVisibility { get => field; set => SetAndNotify(ref field, value); }

    public bool Guide { get => field; set => SetAndNotify(ref field, value); } = ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, 0) < SettingsViewModel.GuideMaxStep
}
