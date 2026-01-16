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
using Stylet;

namespace MaaWpfGui.Models;

/// <summary>
/// The task setting enable info.
/// </summary>
public class TaskSettingVisibilityInfo : PropertyChangedBase
{
    // public const string DefaultVisibleTaskSetting = "Combat";
    public bool StartUp { get => field; set => SetAndNotify(ref field, value); }

    public bool Recruit { get => field; set => SetAndNotify(ref field, value); }

    public bool Infrast { get => field; set => SetAndNotify(ref field, value); }

    public bool Fight { get => field; set => SetAndNotify(ref field, value); }

    public bool Mall { get => field; set => SetAndNotify(ref field, value); }

    public bool Award { get => field; set => SetAndNotify(ref field, value); }

    public bool Roguelike { get => field; set => SetAndNotify(ref field, value); }

    public bool Reclamation { get => field; set => SetAndNotify(ref field, value); }

    public bool AfterAction { get => field; set => SetAndNotify(ref field, value); }

    public bool Custom { get => field; set => SetAndNotify(ref field, value); }

    public static TaskSettingVisibilityInfo Instance { get; } = new();

    // 长草任务当前选中
    public int CurrentIndex
    {
        get {
            if (ConfigFactory.CurrentConfig.TaskSelectedIndex < 0 || ConfigFactory.CurrentConfig.TaskSelectedIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
            {
                ConfigFactory.CurrentConfig.TaskSelectedIndex = 0;
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
    /// 修改任务设置可见性, 用于 TaskQueueItem 设置按钮的切换. 先设 true 再设 false 刷新 UI.
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
        if (enable || ConfigFactory.CurrentConfig.TaskQueue[taskIndex].GetType() != ConfigFactory.CurrentConfig.TaskQueue[CurrentIndex].GetType())
        {
            _ = ConfigFactory.CurrentConfig.TaskQueue[taskIndex] switch {
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
        }
        EnableAdvancedSettings = false;
        AdvancedSettingsVisibility = !Award && !StartUp;

        if (enable)
        {
            Instances.TaskQueueViewModel.RefreshTaskModel(ConfigFactory.CurrentConfig.TaskQueue[taskIndex]);
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
        AfterAction = value;
    }

    private bool _enableAdvancedSettings;

    public bool EnableAdvancedSettings
    {
        get => _enableAdvancedSettings;
        set => SetAndNotify(ref _enableAdvancedSettings, value);
    }

    private bool _advancedSettingsVisibility;

    public bool AdvancedSettingsVisibility
    {
        get => _advancedSettingsVisibility;
        set => SetAndNotify(ref _advancedSettingsVisibility, value);
    }

    private bool _guide = ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, 0) < SettingsViewModel.GuideMaxStep;

    public bool Guide
    {
        get => _guide;
        set => SetAndNotify(ref _guide, value);
    }
}
