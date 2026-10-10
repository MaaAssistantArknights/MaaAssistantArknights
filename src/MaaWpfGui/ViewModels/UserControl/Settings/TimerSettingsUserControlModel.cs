// <copyright file="TimerSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using ScheduledWakeUpRegistrar = MaaWpfGui.Utilities.ScheduledWakeUp;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 定时设置
/// </summary>
public class TimerSettingsUserControlModel : PropertyChangedBase
{
    static TimerSettingsUserControlModel()
    {
        Instance = new();
    }

    public TimerSettingsUserControlModel()
    {
        // 订阅现有定时器，并在列表增删时重新订阅，用于触发 ｢时间管理大师｣ 成就
        SubscribeTimerChanges();
        TimerList.CollectionChanged += (_, _) =>
        {
            SubscribeTimerChanges();
            ScheduledWakeUpRegistrar.SyncAllDebounced();
        };

        // 启动对账：按当前开关与定时项状态全量重建计划任务
        Task.Run(() => ScheduledWakeUpRegistrar.SyncAll(out _));
    }

    public static TimerSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets a value indicating whether to force scheduled start.
    /// </summary>
    public bool ForceScheduledStart
    {
        get; set {
            ConfigFactory.Root.Timers.ForceScheduledStart = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.Timers.ForceScheduledStart;

    /// <summary>
    /// Gets or sets a value indicating whether show window before force scheduled start.
    /// </summary>
    public bool ShowWindowBeforeForceScheduledStart
    {
        get; set {
            ConfigFactory.Root.Timers.ShowWindowBeforeForceScheduledStart = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.Timers.ShowWindowBeforeForceScheduledStart;

    /// <summary>
    /// Gets or sets a value indicating whether to use custom config.
    /// </summary>
    public bool CustomConfig
    {
        get; set {
            ConfigFactory.Root.Timers.CustomConfig = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.Timers.CustomConfig;

    public ObservableCollection<Timer> TimerList => ConfigFactory.Root.Timers.List;

    public ExternalNotificationSettingsUserControlModel ExternalNotificationSettings => SettingsViewModel.ExternalNotificationSettings;

    public bool NotifyBeforeScheduledStart
    {
        get; set {
            ConfigFactory.Root.Timers.NotifyBeforeScheduledStart = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.Timers.NotifyBeforeScheduledStart;

    public int ScheduledStartNotificationMinutes
    {
        get; set {
            value = Math.Clamp(value, 1, 1439);
            ConfigFactory.Root.Timers.ScheduledStartNotificationMinutes = value;
            SetAndNotify(ref field, value);
        }
    } = Math.Clamp(ConfigFactory.Root.Timers.ScheduledStartNotificationMinutes, 1, 1439);

    /// <summary>
    /// Gets or sets a value indicating whether to wake the computer before scheduled timers.
    /// </summary>
    public bool ScheduledWakeUp
    {
        get; set {
            ConfigFactory.Root.Timers.ScheduledWakeUp = value;
            SetAndNotify(ref field, value);

            // COM 注册较慢，放后台线程执行，完成后按结果提示
            Task.Run(() =>
            {
                if (ScheduledWakeUpRegistrar.SyncAll(out var error))
                {
                    return;
                }

                Execute.OnUIThread(() =>
                    MessageBoxHelper.Show(error, LocalizationHelper.GetString("Warning"), icon: MessageBoxImage.Warning));
            });
        }
    } = ConfigFactory.Root.Timers.ScheduledWakeUp;

    /// <summary>
    /// 订阅所有定时器的启用状态变化，用于触发 ｢时间管理大师｣ 成就检查。
    /// </summary>
    private void SubscribeTimerChanges()
    {
        foreach (var timer in TimerList)
        {
            timer.PropertyChanged -= OnTimerPropertyChanged;
            timer.PropertyChanged += OnTimerPropertyChanged;
        }
    }

    private void OnTimerPropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(Timer.IsEnabled))
        {
            AchievementTrackerHelper.Instance.CheckTimeManagementMaster();
        }

        // 启用状态与时间变化都会改变计划任务的触发点，防抖合并后全量重建
        if (e.PropertyName is nameof(Timer.IsEnabled) or nameof(Timer.Hour) or nameof(Timer.Minute))
        {
            ScheduledWakeUpRegistrar.SyncAllDebounced();
        }
    }
}
