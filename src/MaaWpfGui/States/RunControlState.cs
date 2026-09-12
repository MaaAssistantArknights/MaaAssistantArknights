// <copyright file="RunControlState.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.States;

/// <summary>
/// 运行控制按钮的共享状态：订阅一次 <see cref="RunningState.StateChanged"/> 镜像运行态，
/// 为各页面开始/停止按钮提供统一的可绑定条件，替代各 ViewModel 各自维护镜像。
/// 构造时先拉取一次当前值，保证初始值与源头一致（应用启动、Core 初始化完成前的窗口）。
/// </summary>
public class RunControlState : PropertyChangedBase
{
    /// <summary>
    /// Gets the singleton instance, initialized statically (thread-safe, no lazy double-construction races).
    /// </summary>
    public static RunControlState Instance { get; } = new();

    private RunControlState()
    {
        var runningState = RunningState.Instance;

        // 先订阅再拉初值：订阅与初值读取之间的状态变化事件虽仍丢失，但其效果已包含在初值里，
        // 反之（先拉初值再订阅）则丢失的事件会让镜像陈旧到下一个事件才自愈
        runningState.StateChanged += (_, e) => {
            Idle = e.NewState.Idle;
            Inited = e.NewState.Inited;
            Stopping = e.NewState.Stopping;
            Owner = e.NewState.Owner;
            NotifyOfPropertyChange(nameof(CanStart));
            NotifyOfPropertyChange(nameof(StopToolTip));
        };

        Idle = runningState.Idle;
        Inited = runningState.Inited;
        Stopping = runningState.Stopping;
        Owner = runningState.Owner;

        // 本类型为全应用生命周期单例，订阅后无需取消订阅
        LocalizationHelper.LanguageChanged += () => {
            NotifyOfPropertyChange(nameof(StopToolTip));
        };
    }

    private bool _idle;

    /// <summary>
    /// Gets a value indicating whether it is idle.
    /// </summary>
    public bool Idle
    {
        get => _idle;
        private set => SetAndNotify(ref _idle, value);
    }

    private bool _inited;

    /// <summary>
    /// Gets a value indicating whether core is initialized.
    /// </summary>
    public bool Inited
    {
        get => _inited;
        private set => SetAndNotify(ref _inited, value);
    }

    private bool _stopping;

    /// <summary>
    /// Gets a value indicating whether a stop is awaiting completion.
    /// </summary>
    public bool Stopping
    {
        get => _stopping;
        private set => SetAndNotify(ref _stopping, value);
    }

    private RunOwner _owner;

    /// <summary>
    /// Gets the owner of the current run, declared by the start entry.
    /// </summary>
    public RunOwner Owner
    {
        get => _owner;
        private set => SetAndNotify(ref _owner, value);
    }

    /// <summary>
    /// Gets a value indicating whether a new run can be started (initialized, idle and not stopping).
    /// </summary>
    public bool CanStart => Inited && Idle && !Stopping;

    /// <summary>
    /// Gets the tooltip of the stop button: names the owner of the currently running task,
    /// so that stopping from a page other than the one that started the run is still unambiguous.
    /// Null when idle (no tooltip rather than an empty one).
    /// </summary>
    public string? StopToolTip
    {
        get {
            var ownerKey = Owner switch
            {
                RunOwner.TaskQueue => "Farming",
                RunOwner.Copilot => "Copilot",
                RunOwner.MiniGame => "MiniGame",
                RunOwner.Toolbox => "Toolbox",
                _ => null,
            };

            return ownerKey is null
                ? null
                : LocalizationHelper.GetStringFormat("StopCurrentTaskFormat", LocalizationHelper.GetString(ownerKey));
        }
    }
}
