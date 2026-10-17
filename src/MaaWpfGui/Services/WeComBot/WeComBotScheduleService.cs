// <copyright file="WeComBotScheduleService.cs" company="MaaAssistantArknights">
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
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using Serilog;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 企业微信机器人自带的每日定时清日常。
/// <para>
/// 由「企业微信机器人」设置页配置时间，到点后按轮换列表执行；与 MAA 自带的「定时执行」二选一，
/// 避免同一时间重复启动。当前有任务在跑时会等待最多 10 分钟。
/// </para>
/// </summary>
public class WeComBotScheduleService : IDisposable
{
    private static readonly ILogger _logger = Log.ForContext<WeComBotScheduleService>();
    private readonly HashSet<string> _fired = [];
    private readonly Timer _timer = new(20_000) { AutoReset = true };
    private readonly SemaphoreSlim _runGate = new(1, 1);
    private bool _disposed;

    /// <summary>
    /// Initializes a new instance of the <see cref="WeComBotScheduleService"/> class.
    /// </summary>
    public WeComBotScheduleService()
    {
        _timer.Elapsed += (_, _) => Tick();
        _timer.Start();
    }

    private static Configuration.Global.WeComBot Settings => ConfigFactory.Root.WeComBot;

    /// <inheritdoc/>
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _timer.Stop();
        _timer.Dispose();
    }

    private void Tick()
    {
        try
        {
            if (!Settings.EnableSchedule)
            {
                return;
            }

            var now = DateTime.Now;
            var dayKey = now.ToString("yyyyMMdd");
            lock (_fired)
            {
                _fired.RemoveWhere(key => !key.StartsWith(dayKey, StringComparison.Ordinal));
            }

            foreach (var schedule in Settings.Schedules.Where(item => item.Enable).ToList())
            {
                if (schedule.Hour != now.Hour || schedule.Minute != now.Minute)
                {
                    continue;
                }

                var key = $"{dayKey}-{schedule.Hour:00}{schedule.Minute:00}";
                lock (_fired)
                {
                    if (!_fired.Add(key))
                    {
                        continue;
                    }
                }

                _logger.Information("WeCom bot schedule triggered at {Hour}:{Minute}", schedule.Hour, schedule.Minute);
                _ = RunAsync();
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "WeCom bot schedule tick failed");
        }
    }

    private async Task RunAsync()
    {
        await _runGate.WaitAsync().ConfigureAwait(false);
        try
        {
            var execution = Instances.TaskExecutionService;
            var deadline = DateTime.UtcNow.AddMinutes(10);
            while (execution.IsRunning && DateTime.UtcNow < deadline)
            {
                await Task.Delay(TimeSpan.FromSeconds(5)).ConfigureAwait(false);
            }

            if (execution.IsRunning)
            {
                _logger.Warning("WeCom bot scheduled run skipped: task queue is still busy");
                return;
            }

            await execution.StartDailyAsync().ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "WeCom bot scheduled run failed");
        }
        finally
        {
            _runGate.Release();
        }
    }
}
