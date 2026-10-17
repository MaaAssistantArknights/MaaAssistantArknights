// <copyright file="TaskExecutionService.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Serilog;
using Stylet;

namespace MaaWpfGui.Services.TaskExecution;

/// <summary>
/// 供非 GUI 入口（企业微信机器人指令等）复用的任务执行原语。
/// 所有会触碰界面绑定状态的步骤都回到 UI 线程执行。
/// </summary>
public class TaskExecutionService
{
    private static readonly ILogger _logger = Log.ForContext<TaskExecutionService>();
    private readonly RunningState _runningState = RunningState.Instance;

    /// <summary>
    /// Gets a value indicating whether 当前有任务在跑。
    /// </summary>
    public bool IsRunning => !_runningState.GetIdle();

    /// <summary>
    /// Gets a value indicating whether 本次启动只跑任务子集（例如只跑自动肉鸽），
    /// 子集启动不参与账号轮换。
    /// </summary>
    public bool IsSubsetRun { get; private set; }

    /// <summary>
    /// Gets a value indicating whether 当前这一轮不参与账号轮换（例如指定单个账号执行）。
    /// </summary>
    public bool SuppressRotation { get; private set; }

    /// <summary>
    /// 标记下一轮是否为任务子集，供 GUI 的 LinkStartWithTasks 入口调用。
    /// </summary>
    /// <param name="value">是否为子集运行。</param>
    public void SetSubsetRun(bool value) => IsSubsetRun = value;

    /// <summary>
    /// Gets 当前配置档案名。
    /// </summary>
    public string CurrentConfiguration => ConfigFactory.Root.Current;

    /// <summary>
    /// Gets 当前档案里配置的第一个理智作战关卡，用于状态汇报。
    /// </summary>
    public string CurrentStage => ConfigFactory.CurrentConfig.TaskQueue.OfType<FightTask>().FirstOrDefault()?.StagePlan.FirstOrDefault() ?? string.Empty;

    /// <summary>
    /// 按当前档案的任务队列跑一轮完整任务（等价于界面上的 LinkStart）。
    /// </summary>
    /// <param name="allowRotation">是否允许本轮结束后接力其他账号。</param>
    /// <returns>成功进入并跑完返回 <c>true</c>；当前非空闲或启动被拒绝时返回 <c>false</c>。</returns>
    public async Task<bool> StartTaskQueueAsync(bool allowRotation = true)
    {
        if (!_runningState.GetIdle())
        {
            _logger.Information("Task queue is busy, start request ignored");
            return false;
        }

        SuppressRotation = !allowRotation;
        try
        {
            await Execute.OnUIThreadAsync(async () => {
                await Instances.TaskQueueViewModel.LinkStart();
            }).ConfigureAwait(false);

            if (_runningState.GetIdle())
            {
                _logger.Information("Task queue did not start (rejected or empty)");
                return false;
            }

            await _runningState.UntilIdleAsync().ConfigureAwait(false);
            return true;
        }
        finally
        {
            SuppressRotation = false;
        }
    }

    /// <summary>
    /// 执行一次清日常。开启账号轮换时从列表里第一个可用账号起步，
    /// 其余账号由 <see cref="DailyRotationService"/> 在每轮结束后接力。
    /// </summary>
    /// <returns>成功进入并跑完返回 <c>true</c>。</returns>
    public async Task<bool> StartDailyAsync()
    {
        var bot = ConfigFactory.Root.WeComBot;
        if (bot.RotateAccounts)
        {
            var first = bot.Accounts.FirstOrDefault(account => account.Enable && ConfigFactory.ConfigurationExists(account.Configuration));
            if (first is not null)
            {
                SwitchConfiguration(first.Configuration);
                SetFightStage(first.Stage);
            }
        }

        return await StartTaskQueueAsync().ConfigureAwait(false);
    }

    /// <summary>
    /// 只跑指定的任务（例如只跑自动肉鸽）。
    /// </summary>
    /// <param name="tasks">任务列表。</param>
    /// <returns>成功进入并跑完返回 <c>true</c>；当前非空闲时返回 <c>false</c>。</returns>
    public async Task<bool> StartTasksAsync(IEnumerable<BaseTask> tasks)
    {
        var taskList = tasks.ToList();
        if (taskList.Count == 0)
        {
            return false;
        }

        if (!_runningState.GetIdle())
        {
            _logger.Information("Task queue is busy, start request ignored");
            return false;
        }

        IsSubsetRun = true;
        var originalStates = taskList.ToDictionary(task => task, task => task.IsEnable);
        foreach (var task in taskList)
        {
            task.IsEnable = true;
        }

        try
        {
            await Execute.OnUIThreadAsync(async () => {
                await Instances.TaskQueueViewModel.LinkStartWithTasks(taskList);
            }).ConfigureAwait(false);

            if (_runningState.GetIdle())
            {
                _logger.Information("Task subset did not start (rejected or empty)");
                return false;
            }

            await _runningState.UntilIdleAsync().ConfigureAwait(false);
            return true;
        }
        finally
        {
            foreach (var pair in originalStates)
            {
                pair.Key.IsEnable = pair.Value;
            }

            IsSubsetRun = false;
        }
    }

    /// <summary>
    /// 停止当前任务，与界面上的停止按钮同语义。
    /// </summary>
    /// <returns>是否完整走完本次停止。</returns>
    public async Task<bool> StopAsync()
    {
        if (_runningState.GetIdle())
        {
            return false;
        }

        var stopped = false;
        await Execute.OnUIThreadAsync(async () => {
            stopped = await Instances.TaskQueueViewModel.StopManuallyAsync();
        }).ConfigureAwait(false);
        return stopped;
    }

    /// <summary>
    /// 切换配置档案。
    /// </summary>
    /// <param name="configuration">档案名。</param>
    /// <returns>切换成功返回 <c>true</c>。</returns>
    public bool SwitchConfiguration(string configuration)
    {
        if (string.IsNullOrWhiteSpace(configuration) || !ConfigFactory.ConfigurationExists(configuration))
        {
            _logger.Warning("Configuration {Configuration} does not exist", configuration);
            return false;
        }

        Execute.OnUIThread(() => ConfigFactory.SwitchConfig(configuration));
        return true;
    }

    /// <summary>
    /// 修改当前档案的理智作战关卡。
    /// </summary>
    /// <param name="stage">关卡名，留空表示不修改。</param>
    /// <returns>改动成功返回 <c>true</c>。</returns>
    public bool SetFightStage(string stage)
    {
        if (string.IsNullOrWhiteSpace(stage))
        {
            return false;
        }

        Execute.OnUIThread(() => {
            var task = ConfigFactory.CurrentConfig.TaskQueue.OfType<FightTask>().FirstOrDefault();
            if (task is null)
            {
                return;
            }

            if (task.StagePlan.Count != 1)
            {
                task.StagePlan.Clear();
                task.StagePlan.Add(stage);
                return;
            }

            task.StagePlan[0] = stage;
        });
        return true;
    }
}
