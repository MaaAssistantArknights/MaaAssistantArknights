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
    // public const string DefaultVisibleTaskSetting = "Combat";
    public int StartUp { get => field; set => SetAndNotify(ref field, value); }

    public int Recruit { get => field; set => SetAndNotify(ref field, value); }

    public int Infrast { get => field; set => SetAndNotify(ref field, value); }

    public int Fight { get => field; set => SetAndNotify(ref field, value); }

    public int Mall { get => field; set => SetAndNotify(ref field, value); }

    public int Award { get => field; set => SetAndNotify(ref field, value); }

    public int Roguelike { get => field; set => SetAndNotify(ref field, value); }

    public int Reclamation { get => field; set => SetAndNotify(ref field, value); }

    public int Custom { get => field; set => SetAndNotify(ref field, value); }

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
        Instances.AsstProxy.TasksStatus.TryGetValue(ConfigFactory.CurrentConfig.TaskQueue[CurrentIndex].TaskId, out var status) &&
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
            // TODO Config修复引导
            // _currentEnableSetting = taskName;
            enable = false;
        }

        if (enable)
        {
            CurrentIndex = taskIndex;
        }

        if (ConfigFactory.CurrentConfig.TaskQueue.Count <= taskIndex || taskIndex < 0)
        {
            Log.Warning("尝试设置不存在的任务设置可见性, 索引: {TaskIndex}", taskIndex);
        }
        else
        {
            SetTaskSettingVisible(ConfigFactory.CurrentConfig.TaskQueue[taskIndex], enable);
        }

        if (enable)
        {
            Instances.TaskQueueViewModel.RefreshTaskModel(ConfigFactory.CurrentConfig.TaskQueue[taskIndex]);
        }
    }

    public void SetTaskSettingVisible(BaseTask task, bool enable)
    {
        _ = task switch {
            StartUpTask => enable ? ++StartUp : --StartUp,
            RecruitTask => enable ? ++Recruit : --Recruit,
            InfrastTask => enable ? ++Infrast : --Infrast,
            FightTask => enable ? ++Fight : --Fight,
            MallTask => enable ? ++Mall : --Mall,
            AwardTask => enable ? ++Award : --Award,
            RoguelikeTask => enable ? ++Roguelike : --Roguelike,
            ReclamationTask => enable ? ++Reclamation : --Reclamation,
            CustomTask => enable ? ++Custom : --Custom,
            _ => throw new NotImplementedException(),
        };
        EnableAdvancedSettings = false;
        AdvancedSettingsVisibility = Award == 0 && StartUp == 0;

        if (StartUp == 0 && Recruit == 0 && Infrast == 0 && Fight == 0 && Mall == 0 && Award == 0 && Roguelike == 0 && Reclamation == 0 && Custom == 0)
        {
            CurrentIndex = -1;
        }
    }

    public void SetPostAction(bool value)
    {
        /*WakeUp = false;
        Recruiting = false;
        Base = false;
        Combat = false;
        Mall = false;
        Mission = false;
        AutoRoguelike = false;
        Reclamation = false;
        Custom = false;*/
        PostAction = value;
    }

    public bool EnableAdvancedSettings { get => field; set => SetAndNotify(ref field, value); }

    public bool AdvancedSettingsVisibility { get => field; set => SetAndNotify(ref field, value); }

    private bool _guide = ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, 0) < SettingsViewModel.GuideMaxStep;

    public bool Guide
    {
        get => _guide;
        set => SetAndNotify(ref _guide, value);
    }
}
