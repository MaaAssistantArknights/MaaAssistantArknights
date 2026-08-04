// <copyright file="RunningState.cs" company="MaaAssistantArknights">
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
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using Serilog;

namespace MaaWpfGui.States;

public class RunningState
{
    public class RunningStateChangedEventArgs(StateSnapshot oldState, bool idle, bool inited, bool stopping) : EventArgs
    {
        public StateSnapshot OldState { get; } = oldState;

        public StateSnapshot NewState { get; } = new(idle, inited, stopping);
    }

    public record StateSnapshot(bool Idle, bool Inited, bool Stopping);

    private static RunningState? _instance;
    private static readonly ILogger _logger = Log.Logger.ForContext<RunningState>();

    private RunningState()
    {
        if (ReminderIntervalMinutes < 1)
        {
            ReminderIntervalMinutes = 1;
        }

        _timeoutReminderTimer.Interval = LongTaskTimeoutMinutes * 60 * 1000;
        _timeoutReminderTimer.Elapsed += TimeoutReminderTimer_Elapsed;
        _stallTimer.Elapsed += StallTimer_Elapsed;
    }

    public static RunningState Instance
    {
        get {
            _instance ??= new();
            return _instance;
        }
    }

    // 超时相关字段
    private readonly System.Timers.Timer _timeoutReminderTimer = new();
    private readonly System.Timers.Timer _stallTimer = new();
    private int _stallAccumulatedCount = 0;
    private bool _stallIsFirstFire = true;
    private DateTime? _taskStartTime;

    // 防止乘以 60000 毫秒时 int 溢出，int.MaxValue / 60000 ≈ 35791
    private const int MaxMinutes = 11451;
    private const int LongTaskTimeoutMinutes = 60;

    public int ReminderIntervalMinutes
    {
        get; set {
            value = value.Clamp(1, MaxMinutes);
            field = value;
            TimeoutReminderTimer_Elapsed(null, null);
            _timeoutReminderTimer.Interval = value * 60 * 1000;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutReminderIntervalMinutes;

    public int StallTimeoutMinutes
    {
        get; set {
            value = value.Clamp(0, MaxMinutes);
            field = value;
            _stallIsFirstFire = true;
            if (_stallTimer.Enabled)
            {
                _stallTimer.Stop();
                if (value > 0)
                {
                    _stallTimer.Interval = value * 60 * 1000;
                    _stallTimer.Start();
                }
            }
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutMinutes;

    /// <summary>
    /// Gets or sets a value indicating whether 启用停滞检测
    /// </summary>
    public bool EnableStallTimeout
    {
        get; set {
            field = value;
            if (!value && _stallTimer.Enabled)
            {
                _stallTimer.Stop();
            }
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.EnableStallTimeout;

    public event EventHandler<string>? StallOccurred;

    public void NotifyOutputActivity()
    {
        _stallAccumulatedCount = 0;
        _stallIsFirstFire = true;
        if (_stallTimer.Enabled && EnableStallTimeout && StallTimeoutMinutes > 0)
        {
            _stallTimer.Interval = StallTimeoutMinutes * 60 * 1000;
            _stallTimer.Stop();
            _stallTimer.Start();
        }
    }

    // 超时事件
    public void StartTimeoutTimer()
    {
        _taskStartTime = DateTime.Now;
        _timeoutReminderTimer.Start();
        _stallAccumulatedCount = 0;
        _stallIsFirstFire = true;
        if (EnableStallTimeout && StallTimeoutMinutes > 0)
        {
            _stallTimer.Interval = StallTimeoutMinutes * 60 * 1000;
            _stallTimer.Start();
        }
    }

    public void StopTimeoutTimer()
    {
        _timeoutReminderTimer.Stop();
        _stallTimer.Stop();
        _stallAccumulatedCount = 0;
        _stallIsFirstFire = true;
        _taskStartTime = null;
    }

    public void ResetTimeout()
    {
        _taskStartTime = DateTime.Now;
    }

    // 超时计时器回调
    private void TimeoutReminderTimer_Elapsed(object? sender, System.Timers.ElapsedEventArgs? e)
    {
        if (!_taskStartTime.HasValue || _idle)
        {
            return;
        }

        var elapsedMinutes = (DateTime.Now - _taskStartTime.Value).TotalMinutes;
        if (elapsedMinutes > 3 * 60)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.ProxyOnline3Hours);
        }
    }

    private void StallTimer_Elapsed(object? sender, System.Timers.ElapsedEventArgs e)
    {
        _stallTimer.Stop();
        _stallAccumulatedCount++;
        var accumulatedMinutes = StallTimeoutMinutes + ((_stallAccumulatedCount - 1) * ReminderIntervalMinutes);
        var message = LocalizationHelper.GetStringFormat(
            "TaskStallWarning",
            StallTimeoutMinutes,
            accumulatedMinutes);
        StallOccurred?.Invoke(this, message);
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.LongTaskTimeout);
        if (EnableStallTimeout && StallTimeoutMinutes > 0)
        {
            if (_stallIsFirstFire)
            {
                _stallTimer.Interval = ReminderIntervalMinutes * 60 * 1000;
                _stallIsFirstFire = false;
            }

            _stallTimer.Start();
        }
    }

    private bool _idle = true;

    public bool Idle
    {
        get => _idle;
        set {
            if (_idle == value)
            {
                return;
            }

            var oldState = new StateSnapshot(_idle, _inited, _stopping);
            _idle = value;
            if (value)
            {
                StopTimeoutTimer();
                SleepManagement.AllowSleep();
            }
            else
            {
                StartTimeoutTimer();
                SleepManagement.BlockSleep();
            }

            RaiseStateChanged(oldState);
        }
    }

    /// <summary>
    /// 是否空闲（仅反映任务运行状态）。
    /// 仅供 UI 绑定和按钮状态使用。需要判断"是否可以安全执行打断性操作"（如自动更新重启）时，
    /// 应使用 <see cref="CanInterrupt"/>，它会额外排除倒计时等情况。
    /// </summary>
    /// <returns>当前是否空闲。</returns>
    public bool GetIdle() => Idle;

    public void SetIdle(bool idle, [CallerMemberName] string caller = "")
    {
        _logger.Information("Idle: {Old} to {New} (called from {Caller})", Idle, idle, caller);
        Idle = idle;
    }

    // 引用计数：CheckAfterCompleted 外层和 TimerCanceledAsync 内层各自 Enter/Leave，
    // 只有当计数归零时才真正清除倒计时状态，避免嵌套 try-finally 提前清零的竞态。
    private int _countingDownDepth;

    /// <summary>
    /// 进入倒计时状态（引用计数 +1）。
    /// 倒计时（关机/休眠/启动自动运行等）期间，<see cref="CanInterrupt"/> 返回 false，
    /// <see cref="UntilIdleAsync"/> 会持续等待。
    /// </summary>
    /// <param name="caller">调用方名称。</param>
    public void EnterCountingDown([CallerMemberName] string caller = "")
    {
        var newValue = Interlocked.Increment(ref _countingDownDepth);
        _logger.Information("CountingDown enter: depth={Depth} (called from {Caller})", newValue, caller);
    }

    /// <summary>
    /// 离开倒计时状态（引用计数 -1，不小于 0）。
    /// </summary>
    /// <param name="caller">调用方名称。</param>
    public void LeaveCountingDown([CallerMemberName] string caller = "")
    {
        var newValue = Interlocked.Decrement(ref _countingDownDepth);
        if (newValue < 0)
        {
            _logger.Warning("CountingDown leave: depth underflow, clamping to 0 (called from {Caller})", caller);
            newValue = Interlocked.Exchange(ref _countingDownDepth, 0);
        }
        else
        {
            _logger.Information("CountingDown leave: depth={Depth} (called from {Caller})", newValue, caller);
        }
    }

    /// <summary>
    /// 当前是否正处于倒计时状态。
    /// </summary>
    public bool GetCountingDown() => Volatile.Read(ref _countingDownDepth) > 0;

    /// <summary>
    /// 当前是否可以安全打断（空闲且没在倒计时）。
    /// 倒计时（关机/休眠/启动自动运行等）期间不属于可安全打断的状态。
    /// </summary>
    /// <returns>空闲且没在倒计时返回 <see langword="true"/>，否则返回 <see langword="false"/>。</returns>
    public bool CanInterrupt() => GetIdle() && !GetCountingDown();

    private bool _inited;

    public bool Inited
    {
        get => _inited;
        set {
            if (_inited != value)
            {
                var oldState = new StateSnapshot(_idle, _inited, _stopping);
                _inited = value;
                RaiseStateChanged(oldState);
            }
        }
    }

    public bool GetInit() => Inited;

    public void SetInit(bool init, [CallerMemberName] string caller = "")
    {
        _logger.Information("Init: {Old} to {New} (called from {Caller})", Inited, init, caller);
        Inited = init;
    }

    private bool _stopping;

    public bool Stopping
    {
        get => _stopping;
        set {
            if (_stopping != value)
            {
                var oldState = new StateSnapshot(_idle, _inited, _stopping);
                _stopping = value;
                RaiseStateChanged(oldState);
            }
        }
    }

    public bool GetStopping() => Stopping;

    public void SetStopping(bool stopping, [CallerMemberName] string caller = "")
    {
        _logger.Information("Stopping: {Old} to {New} (called from {Caller})", Stopping, stopping, caller);
        Stopping = stopping;
    }

    public event EventHandler<RunningStateChangedEventArgs>? StateChanged;

    private void RaiseStateChanged(StateSnapshot oldState)
    {
        StateChanged?.Invoke(this, new(oldState, _idle, _inited, _stopping));
    }

    /// <summary>
    /// 等待状态变为闲置
    /// </summary>
    /// <param name="time">查询间隔(ms)</param>
    /// <param name="confirmInterval">确认间隔(ms)</param>
    /// <param name="confirmTimes">确认次数</param>
    /// <returns>Task</returns>
    public async Task UntilIdleAsync(int time = 1000, int confirmInterval = 1000, int confirmTimes = 3)
    {
        while (true)
        {
            while (!CanInterrupt())
            {
                await Task.Delay(time);
            }

            int confirmed = 0;
            while (confirmed < confirmTimes)
            {
                await Task.Delay(confirmInterval);

                if (CanInterrupt())
                {
                    confirmed++;
                }
                else
                {
                    _logger.Information("Idle state changed during confirmation, resetting confirmation count.");
                    break;
                }
            }

            if (confirmed >= confirmTimes)
            {
                _logger.Information("Idle state confirmed after {ConfirmTimes} checks.", confirmTimes);
                return;
            }
        }
    }
}
