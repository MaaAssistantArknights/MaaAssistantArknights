// <copyright file="TutorialChallengeSandboxModels.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;

#pragma warning disable SA1402, SA1649 // 关卡沙盒 Model 小类集中放本文件
namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// 理智作战任务的沙盒 Model：复用真实任务控件的全部行为，任务读写数据源指向关卡 VM 的沙盒任务集合，
/// core 下发被拦截（沙盒不产生真实 core 任务），拖拽态与高级设置开关转发关卡 VM。
/// </summary>
public sealed class SandboxFightTaskModel : FightTaskUserControlModel
{
    private readonly TutorialChallengeLevelViewModel _owner;

    /// <summary>
    /// Initializes a new instance of the <see cref="SandboxFightTaskModel"/> class.
    /// 构造时即填充活动信息行——沙盒不走真实 Model 的刷新链路（UpdateStageList 会锁真实任务序列化锁），直接调只读活动表的刷新。
    /// </summary>
    /// <param name="owner">所属关卡 VM。</param>
    public SandboxFightTaskModel(TutorialChallengeLevelViewModel owner)
        : base(attachGlobalEvents: false)
    {
        _owner = owner;
        RefreshActivityInfo();
    }

    /// <inheritdoc/>
    protected override IList<BaseTask> TaskConfigQueue => _owner.SandboxTasks;

    /// <inheritdoc/>
    protected override int TaskConfigIndex => _owner.SelectedItem is { } item ? _owner.SandboxItems.IndexOf(item) : -1;

    /// <inheritdoc/> 弹窗自有开关，不联动主窗口
    public override bool EnableAdvancedSettings => _owner.EnableAdvancedSettings;

    /// <inheritdoc/> 沙盒任务永不运行
    public override bool IsCurrentTaskRunning => false;

    /// <inheritdoc/> 沙盒队列恒空闲，不受真实队列运行态联动
    public override bool IsQueueIdle => true;

    /// <inheritdoc/> 沙盒面板不联动真实理智面板的拖拽禁用
    public override bool IsFightStageItemDragging => false;

    /// <inheritdoc/> 转发关卡 VM 的拖拽态（教程侧拖拽逻辑写在关卡 VM 上）
    public override bool IsStageItemDragging
    {
        get => _owner.IsStageItemDragging;
        set => _owner.IsStageItemDragging = value;
    }

    /// <inheritdoc/> 沙盒任务不序列化下发 core
    protected override bool? SetFightParams() => null;

    /// <summary>
    /// 关卡 VM 侧状态（高级设置开关、拖拽态）变化时转发为本 Model 的绑定通知。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">属性变更参数。</param>
    public void OnOwnerPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        switch (e.PropertyName)
        {
            case nameof(TutorialChallengeLevelViewModel.EnableAdvancedSettings):
                NotifyOfPropertyChange(nameof(EnableAdvancedSettings));
                break;
            case nameof(TutorialChallengeLevelViewModel.IsStageItemDragging):
                NotifyOfPropertyChange(nameof(IsStageItemDragging));
                break;
        }
    }

    /// <summary>
    /// 关卡关闭时释放对静态实例与关卡 VM 的订阅。
    /// </summary>
    public void Detach() => DetachGlobalStateForwarding();
}

/// <summary>
/// 公招任务的沙盒 Model：复用真实任务控件，任务读写数据源指向关卡 VM 的沙盒任务集合。
/// </summary>
/// <param name="owner">所属关卡 VM。</param>
public sealed class SandboxRecruitTaskModel(TutorialChallengeLevelViewModel owner) : RecruitTaskUserControlModel
{
    /// <inheritdoc/>
    protected override IList<BaseTask> TaskConfigQueue => owner.SandboxTasks;

    /// <inheritdoc/>
    protected override int TaskConfigIndex => owner.SelectedItem is { } item ? owner.SandboxItems.IndexOf(item) : -1;

    /// <inheritdoc/> 弹窗自有开关，不联动主窗口
    public override bool EnableAdvancedSettings => owner.EnableAdvancedSettings;

    /// <inheritdoc/> 沙盒任务永不运行
    public override bool IsCurrentTaskRunning => false;

    /// <inheritdoc/> 沙盒队列恒空闲，不受真实队列运行态联动
    public override bool IsQueueIdle => true;

    /// <inheritdoc/> 沙盒面板不联动真实理智面板的拖拽禁用
    public override bool IsFightStageItemDragging => false;

    /// <summary>
    /// 关卡 VM 侧状态变化时转发为本 Model 的绑定通知。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">属性变更参数。</param>
    public void OnOwnerPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(TutorialChallengeLevelViewModel.EnableAdvancedSettings))
        {
            NotifyOfPropertyChange(nameof(EnableAdvancedSettings));
        }
    }

    /// <summary>
    /// 关卡关闭时释放对静态实例与关卡 VM 的订阅。
    /// </summary>
    public void Detach() => DetachGlobalStateForwarding();
}

/// <summary>
/// 库存保持任务的沙盒 Model：复用真实任务控件，任务读写数据源指向关卡 VM 的沙盒任务集合。
/// </summary>
/// <param name="owner">所属关卡 VM。</param>
public sealed class SandboxDepotMaintainTaskModel(TutorialChallengeLevelViewModel owner) : DepotMaintainTaskUserControlModel(attachGlobalEvents: false)
{
    /// <inheritdoc/>
    protected override IList<BaseTask> TaskConfigQueue => owner.SandboxTasks;

    /// <inheritdoc/>
    protected override int TaskConfigIndex => owner.SelectedItem is { } item ? owner.SandboxItems.IndexOf(item) : -1;

    /// <inheritdoc/> 弹窗自有开关，不联动主窗口
    public override bool EnableAdvancedSettings => owner.EnableAdvancedSettings;

    /// <inheritdoc/> 沙盒任务永不运行
    public override bool IsCurrentTaskRunning => false;

    /// <inheritdoc/> 沙盒队列恒空闲，不受真实队列运行态联动
    public override bool IsQueueIdle => true;

    /// <inheritdoc/> 沙盒面板不联动真实理智面板的拖拽禁用
    public override bool IsFightStageItemDragging => false;

    /// <summary>
    /// 关卡 VM 侧状态变化时转发为本 Model 的绑定通知。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">属性变更参数。</param>
    public void OnOwnerPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(TutorialChallengeLevelViewModel.EnableAdvancedSettings))
        {
            NotifyOfPropertyChange(nameof(EnableAdvancedSettings));
        }
    }

    /// <summary>
    /// 关卡关闭时释放对静态实例与关卡 VM 的订阅。
    /// </summary>
    public void Detach() => DetachGlobalStateForwarding();
}

/// <summary>
/// 定时设置的沙盒 Model：复用真实定时设置控件，读写数据源指向关卡 VM 的 <see cref="TutorialChallengeSandboxSchedule"/>
/// 持有的同一内存配置实例（判定谓词读同一对象），计划任务注册联动被拦截（沙盒不碰真实计划任务）。
/// </summary>
/// <param name="schedule">所属关卡的定时设置沙盒状态。</param>
public sealed class SandboxTimerSettingsModel(TutorialChallengeSandboxSchedule schedule) : TimerSettingsUserControlModel(attachGlobalEvents: false)
{
    /// <inheritdoc/>
    protected override TimerSettings TimerSettingsSource => schedule.Settings;

    /// <inheritdoc/> 沙盒队列恒空闲，定时配置项不受真实队列运行态禁用
    public override bool IsQueueIdle => true;

    /// <inheritdoc/> 沙盒不注册真实计划任务
    protected override void SyncScheduledWakeUpRegistrations()
    {
    }
}
