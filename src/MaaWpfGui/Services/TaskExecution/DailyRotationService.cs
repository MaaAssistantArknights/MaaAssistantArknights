// <copyright file="DailyRotationService.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using Serilog;

namespace MaaWpfGui.Services.TaskExecution;

/// <summary>
/// 账号轮换执行器。
/// <para>
/// 不区分启动入口：只要「一次清日常依次跑完所有账号」开启，任何一次完整任务队列启动
/// （界面 LinkStart、MAA 定时执行、聊天指令）结束后，都会按轮换列表继续下一个配置档案，
/// 全部跑完后切回本次会话开始时的配置。只跑任务子集（如自动肉鸽）的启动不参与轮换。
/// </para>
/// </summary>
public class DailyRotationService : IDisposable
{
    private static readonly ILogger _logger = Log.ForContext<DailyRotationService>();
    private readonly RunningState _runningState = RunningState.Instance;
    private Task? _session;
    private bool _cancelled;
    private bool _disposed;

    /// <summary>
    /// Initializes a new instance of the <see cref="DailyRotationService"/> class.
    /// </summary>
    public DailyRotationService() => _runningState.StateChanged += OnStateChanged;

    private static Configuration.Global.WeComBot Settings => ConfigFactory.Root.WeComBot;

    /// <inheritdoc/>
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _runningState.StateChanged -= OnStateChanged;
    }

    private static List<Configuration.Global.WeComBotAccount> RotationTargets() => Settings.Accounts
        .Where(account => account.Enable)
        .Where(account => !string.IsNullOrWhiteSpace(account.Configuration))
        .Where(account => ConfigFactory.ConfigurationExists(account.Configuration))
        .ToList();

    private void OnStateChanged(object? sender, RunningState.RunningStateChangedEventArgs e)
    {
        if (!e.OldState.Stopping && e.NewState.Stopping)
        {
            // 手动停止后不再继续后面的账号
            _cancelled = true;
            return;
        }

        // 只关心主任务队列的启动；自动战斗 / 工具箱等归属不参与清日常轮换
        if (!e.OldState.Idle || e.NewState.Idle || e.NewState.Owner != RunOwner.TaskQueue || _session is not null)
        {
            return;
        }

        if (!Settings.RotateAccounts
            || Instances.TaskExecutionService?.IsSubsetRun != false
            || Instances.TaskExecutionService?.SuppressRotation != false)
        {
            return;
        }

        if (RotationTargets().Count == 0)
        {
            return;
        }

        _cancelled = false;
        _session = Task.Run(RunSessionAsync);
    }

    private async Task RunSessionAsync()
    {
        var origin = ConfigFactory.Root.Current;
        var targets = RotationTargets().Where(account => account.Configuration != origin).ToList();
        _logger.Information("Daily rotation started from {Origin}, {Count} remaining account(s)", origin, targets.Count);

        try
        {
            // 当前这轮是在进入空闲前启动的，先等它自然结束
            await _runningState.UntilIdleAsync().ConfigureAwait(false);

            foreach (var account in targets)
            {
                if (_cancelled || _runningState.GetStopping())
                {
                    _logger.Information("Daily rotation cancelled");
                    return;
                }

                var execution = Instances.TaskExecutionService;
                execution.SwitchConfiguration(account.Configuration);
                execution.SetFightStage(account.Stage);

                if (!await execution.StartTaskQueueAsync().ConfigureAwait(false))
                {
                    _logger.Warning("Daily rotation stopped because a run could not start");
                    return;
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Daily rotation failed");
        }
        finally
        {
            if (!_cancelled && _runningState.GetIdle())
            {
                Instances.TaskExecutionService.SwitchConfiguration(origin);
            }

            _logger.Information("Daily rotation finished");
            _session = null;
        }
    }
}
