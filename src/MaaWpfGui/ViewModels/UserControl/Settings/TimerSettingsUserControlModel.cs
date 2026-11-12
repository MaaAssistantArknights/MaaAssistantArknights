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
using MaaWpfGui.States;
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
        : this(attachGlobalEvents: true)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimerSettingsUserControlModel"/> class.
    /// </summary>
    /// <param name="attachGlobalEvents">是否订阅全局定时状态（成就检查与计划任务同步）。默认单例构造传 true；
    /// 短生命周期派生 Model（如教程沙盒）传 false 以隔离真实定时器的计划任务注册。</param>
    protected TimerSettingsUserControlModel(bool attachGlobalEvents)
    {
        if (attachGlobalEvents)
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

            // 本类型为 Instance 单例，转发队列运行态通知后无需取消订阅
            RunControlState.Instance.PropertyChanged += ForwardQueueRunStateChanged;
        }
    }

    public static TimerSettingsUserControlModel Instance { get; }

    /// <summary>
    /// 定时设置的读写数据源。默认为全局持久化配置，子类可覆写为自有定时状态（如教程沙盒）。
    /// </summary>
    protected virtual TimerSettings TimerSettingsSource => ConfigFactory.Root.Timers;

    /// <summary>
    /// 任务队列是否空闲（定时配置项可编辑）。默认转发全局运行控制状态，子类可覆写为自有状态（如沙盒恒空闲）。
    /// </summary>
    public virtual bool IsQueueIdle => RunControlState.Instance.Idle;

    /// <summary>
    /// 将全局队列运行态的变更转发为本 Model 的 <see cref="IsQueueIdle"/> 通知，
    /// 使定时控件 XAML 经 DataContext 绑定转发属性时仍能收到变更通知。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">属性变更参数。</param>
    private void ForwardQueueRunStateChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(RunControlState.Idle))
        {
            NotifyOfPropertyChange(nameof(IsQueueIdle));
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to force scheduled start.
    /// </summary>
    public bool ForceScheduledStart
    {
        get => TimerSettingsSource.ForceScheduledStart;
        set
        {
            TimerSettingsSource.ForceScheduledStart = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether show window before force scheduled start.
    /// </summary>
    public bool ShowWindowBeforeForceScheduledStart
    {
        get => TimerSettingsSource.ShowWindowBeforeForceScheduledStart;
        set
        {
            TimerSettingsSource.ShowWindowBeforeForceScheduledStart = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use custom config.
    /// </summary>
    public bool CustomConfig
    {
        get => TimerSettingsSource.CustomConfig;
        set
        {
            TimerSettingsSource.CustomConfig = value;
            NotifyOfPropertyChange();
        }
    }

    public ObservableCollection<Timer> TimerList => TimerSettingsSource.List;

    public ExternalNotificationSettingsUserControlModel ExternalNotificationSettings => SettingsViewModel.ExternalNotificationSettings;

    public bool NotifyBeforeScheduledStart
    {
        get => TimerSettingsSource.NotifyBeforeScheduledStart;
        set
        {
            TimerSettingsSource.NotifyBeforeScheduledStart = value;
            NotifyOfPropertyChange();
        }
    }

    public int ScheduledStartNotificationMinutes
    {
        get => Math.Clamp(TimerSettingsSource.ScheduledStartNotificationMinutes, 1, 1439);
        set
        {
            value = Math.Clamp(value, 1, 1439);
            TimerSettingsSource.ScheduledStartNotificationMinutes = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to wake the computer before scheduled timers.
    /// </summary>
    public bool ScheduledWakeUp
    {
        get => TimerSettingsSource.ScheduledWakeUp;
        set
        {
            TimerSettingsSource.ScheduledWakeUp = value;
            NotifyOfPropertyChange();

            SyncScheduledWakeUpRegistrations();
        }
    }

    /// <summary>
    /// 定时唤醒开关变更后的联动。默认后台同步重建计划任务注册并按结果提示，子类可覆写为空操作（如教程沙盒不碰真实计划任务）。
    /// </summary>
    protected virtual void SyncScheduledWakeUpRegistrations()
    {
        // COM 注册较慢，放后台线程执行，完成后按结果提示
        Task.Run(() =>
        {
            if (ScheduledWakeUpRegistrar.SyncAll(out var error))
            {
                return;
            }

            Execute.OnUIThread(() =>
                MessageBoxHelper.Show(error, LocalizationHelper.GetString("Common.Warning"), icon: MessageBoxImage.Warning));
        });
    }

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
