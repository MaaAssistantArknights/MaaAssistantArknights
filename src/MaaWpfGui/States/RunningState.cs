// <copyright file="RunningState.cs" company="MaaAssistantArknights">
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
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using Serilog;

namespace MaaWpfGui.States;

public class RunningState
{
    public class RunningStateChangedEventArgs(StateSnapshot oldState, bool idle, bool inited, bool stopping, RunOwner owner) : EventArgs
    {
        public StateSnapshot OldState { get; } = oldState;

        public StateSnapshot NewState { get; } = new(idle, inited, stopping, owner);
    }

    public record StateSnapshot(bool Idle, bool Inited, bool Stopping, RunOwner Owner);

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

    // System.Timers.Timer 实例成员非线程安全，并发 Start/Stop/改 Interval 会抛 NRE；
    // 所有触碰两个计时器及其伴生状态（_taskStartTime/_stallAccumulatedCount/_stallIsFirstFire）
    // 的入口共用此锁。StallOccurred 事件与成就解锁等外部回调一律放锁外，防订阅者链路重入持锁
    private readonly Lock _timerLock = new();
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
            lock (_timerLock)
            {
                _timeoutReminderTimer.Interval = value * 60 * 1000;
            }
        }
    } = 30;

    public int StallTimeoutMinutes
    {
        get; set {
            value = value.Clamp(0, MaxMinutes);
            field = value;
            ResetStallTimer();
        }
    } = 30;

    /// <summary>
    /// Gets or sets a value indicating whether 启用停滞检测
    /// </summary>
    public bool EnableStallTimeout
    {
        get; set {
            field = value;
            ResetStallTimer();
        }
    } = true;

    // Reports the run owner, initial timeout and accumulated timeout in minutes.
    public event Action<RunOwner, int, int>? StallOccurred;

    public void NotifyOutputActivity() => ResetStallTimer();

    private void ResetStallTimer()
    {
        lock (_timerLock)
        {
            _stallTimer.Stop();
            _stallAccumulatedCount = 0;
            _stallIsFirstFire = true;
            if (!_idle && _taskStartTime is not null && EnableStallTimeout && StallTimeoutMinutes > 0)
            {
                _stallTimer.Interval = StallTimeoutMinutes * 60 * 1000;
                _stallTimer.Start();
            }
        }
    }

    // 超时事件
    public void StartTimeoutTimer()
    {
        lock (_timerLock)
        {
            _taskStartTime = DateTime.Now;
            _timeoutReminderTimer.Start();
            ResetStallTimer();
        }
    }

    public void StopTimeoutTimer()
    {
        lock (_timerLock)
        {
            _timeoutReminderTimer.Stop();
            _taskStartTime = null;
            ResetStallTimer();
        }
    }

    public void ResetTimeout()
    {
        lock (_timerLock)
        {
            _taskStartTime = DateTime.Now;
        }
    }

    // 运行时长上限相关字段，仅由主任务队列开始时设置，空闲时清除
    private readonly Lock _runDeadlineLock = new();
    private DateTime? _runDeadline;
    private int _runDurationLimitMinutes;
    private bool _runDurationLimitExecutePostActions;

    public void SetRunDeadline(int limitMinutes, bool executePostActions)
    {
        lock (_runDeadlineLock)
        {
            _runDurationLimitMinutes = limitMinutes;
            _runDurationLimitExecutePostActions = executePostActions;
            _runDeadline = DateTime.UtcNow.AddMinutes(limitMinutes);
        }
    }

    public void ClearRunDeadline()
    {
        lock (_runDeadlineLock)
        {
            _runDeadline = null;
        }
    }

    /// <summary>
    /// 若已到达运行截止时间，则清除截止时间并返回 true，保证每轮运行只触发一次。
    /// </summary>
    /// <param name="limitMinutes">设置的运行时长上限（分钟）。</param>
    /// <param name="executePostActions">停止后是否执行完成后动作。</param>
    /// <returns>是否已到达截止时间。</returns>
    public bool TryConsumeRunDeadline(out int limitMinutes, out bool executePostActions)
    {
        lock (_runDeadlineLock)
        {
            limitMinutes = _runDurationLimitMinutes;
            executePostActions = _runDurationLimitExecutePostActions;
            if (_runDeadline is not { } deadline || DateTime.UtcNow < deadline)
            {
                return false;
            }

            _runDeadline = null;
            return true;
        }
    }

    // 超时计时器回调
    private void TimeoutReminderTimer_Elapsed(object? sender, System.Timers.ElapsedEventArgs? e)
    {
        bool unlockProxyOnline;
        lock (_timerLock)
        {
            if (_taskStartTime is not { } start || _idle)
            {
                return;
            }

            unlockProxyOnline = (DateTime.Now - start).TotalMinutes > 3 * 60;
        }

        if (unlockProxyOnline)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.ProxyOnline3Hours);
        }
    }

    private void StallTimer_Elapsed(object? sender, System.Timers.ElapsedEventArgs e)
    {
        RunOwner owner;
        int initialMinutes;
        int accumulatedMinutes;
        lock (_timerLock)
        {
            // An Elapsed callback may already be queued when the timer is stopped.
            if (_idle || _taskStartTime is null || !_stallTimer.Enabled || !EnableStallTimeout || StallTimeoutMinutes <= 0)
            {
                return;
            }

            owner = Owner;
            initialMinutes = StallTimeoutMinutes;
            _stallTimer.Stop();
            _stallAccumulatedCount++;
            accumulatedMinutes = initialMinutes + ((_stallAccumulatedCount - 1) * ReminderIntervalMinutes);
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

        // 事件与成就在锁外触发：订阅者回调链可能重入本类的计时方法
        StallOccurred?.Invoke(owner, initialMinutes, accumulatedMinutes);
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.LongTaskTimeout);
    }

    private bool _idle = true;
    private RunOwner _runOwner;

    // 每次成功认领分配单调递增的令牌：清理路径只能结束自己那一笔运行，
    // 避免“旧操作在等待/连接结束后收尾时把后来者刚认领的运行清掉”
    private long _runClaimSequence;
    private long _activeRunClaimId;

    // 运行归属、空闲状态与中断锁的检查/变更共用同一把锁：开始入口可能来自 UI、远程控制、热键等
    // 多个线程，若先 GetIdle/CanInterrupt 再 BeginRun，两个调用方都会通过检查并依次接管归属，
    // 完成事件随即被错误归因；空闲切入与中断锁定也必须与认领互斥，否则收尾期间仍会被插入新运行
    private readonly Lock _runTransitionLock = new();

    /// <summary>
    /// 一轮运行的认领令牌。由 <see cref="BeginRun"/> 分配，只能用于 <see cref="EndRun"/> 结束它自己那一笔运行。
    /// </summary>
    /// <param name="Id">令牌序号；0 表示无效令牌。</param>
    public readonly record struct RunClaim(long Id);

    /// <summary>
    /// 当前运行轮次的发起入口归属，由开始入口经 <see cref="BeginRun"/> 声明，回到空闲时清零。
    /// </summary>
    public RunOwner Owner => _runOwner;

    /// <summary>
    /// 原子地认领本轮运行：仅当当前可安全打断（空闲且中断未锁定）时声明归属并进入运行态。
    /// 检查与声明在同一把锁内完成，所有开始入口都必须经此认领；已有运行、中断锁定（结束脚本、
    /// 关机倒计时等收尾）或已被其他入口抢先认领时返回 <see langword="false"/>，状态保持不变。
    /// </summary>
    /// <param name="owner">发起运行的入口归属。</param>
    /// <param name="claim">认领成功后返回本轮运行令牌；失败时为默认值。</param>
    /// <param name="caller">调用方名称。</param>
    /// <returns>认领成功返回 <see langword="true"/>。</returns>
    public bool BeginRun(RunOwner owner, out RunClaim claim, [CallerMemberName] string caller = "")
    {
        lock (_runTransitionLock)
        {
            if (!CanInterrupt())
            {
                claim = default;
                _logger.Information("BeginRun rejected: owner={Owner} (called from {Caller})", owner, caller);
                return false;
            }

            _logger.Information("BeginRun: owner={Owner} (called from {Caller})", owner, caller);
            _activeRunClaimId = ++_runClaimSequence;
            claim = new RunClaim(_activeRunClaimId);
            ApplyRunState(owner);
        }

        ReconcileSleepState();
        return true;
    }

    /// <summary>
    /// 结束指定令牌对应的本轮运行并回到空闲；令牌已失效（运行已被其他路径收尾或又被新一轮认领）时
    /// 不做任何变更，避免清掉其他入口新认领的运行。
    /// </summary>
    /// <param name="claim">本操作开始时由 <see cref="BeginRun"/> 返回的令牌。</param>
    /// <returns>是否实际结束并回到空闲。</returns>
    public bool EndRun(RunClaim claim)
    {
        if (claim.Id == 0)
        {
            return false;
        }

        lock (_runTransitionLock)
        {
            if (_idle || _activeRunClaimId != claim.Id)
            {
                _logger.Information(
                    "EndRun ignored: claim={ClaimId}, activeClaim={ActiveClaimId}, idle={Idle}",
                    claim.Id,
                    _activeRunClaimId,
                    _idle);
                return false;
            }

            ApplyIdleState(true);
        }

        ReconcileSleepState();
        return true;
    }

    /// <summary>
    /// 在锁内把本轮运行置为运行态并广播；休眠管理不在此处调用。
    /// </summary>
    /// <param name="owner">发起运行的入口归属。</param>
    private void ApplyRunState(RunOwner owner)
    {
        // 空闲起点直接写字段，归属与离开空闲合并为一次广播；避免先经 Owner 单独广播出
        // ｢空闲但已有归属｣ 的中间快照
        var oldState = new StateSnapshot(_idle, _inited, _stopping, _runOwner);
        _runOwner = owner;
        _idle = false;
        StartTimeoutTimer();
        RaiseStateChanged(oldState);
    }

    /// <summary>
    /// 应用空闲状态及其伴生状态（归属/令牌重置、计时器），调用方须持有 <see cref="_runTransitionLock"/>。
    /// 休眠管理会同步派发到 UI 线程，不在锁内调用。
    /// </summary>
    /// <param name="value">新的空闲状态。</param>
    private void ApplyIdleState(bool value)
    {
        var oldState = new StateSnapshot(_idle, _inited, _stopping, _runOwner);
        _idle = value;
        if (value)
        {
            // 回到空闲即本轮结束：归属、令牌与停止中在同一快照内清零。直接 SetIdle(true) 收尾
            // 的链路（工具箱各工具连接失败、测试连接等）不经 SetStopped，若不清 Stopping
            // 会留下 ｢空闲但停止中｣ 的死锁态——三页开始/停止按钮全部不可用
            _runOwner = RunOwner.None;
            _activeRunClaimId = 0;
            _stopping = false;
            StopTimeoutTimer();
            ClearRunDeadline();
        }
        else
        {
            StartTimeoutTimer();
        }

        RaiseStateChanged(oldState);
    }

    public bool Idle
    {
        get => _idle;
        set {
            bool changed;
            lock (_runTransitionLock)
            {
                changed = _idle != value;
                if (changed)
                {
                    // 变更与广播在同一把锁内完成：否则 SetIdle(false) 可能在别的开始入口通过
                    // CanInterrupt 之后才生效，或在认领成功后把归属清掉，造成重叠运行或归属丢失
                    ApplyIdleState(value);
                }
            }

            if (changed)
            {
                ReconcileSleepState();
            }
        }
    }

    /// <summary>
    /// 按最新运行状态对齐系统休眠保持。休眠管理会同步派发到 UI 线程，必须在
    /// <see cref="_runTransitionLock"/> 之外调用，且其内部按执行时的实时状态判定，
    /// 这样出锁后的调用即使与另一次状态切换交错，也不会留下错误的保持唤醒。
    /// </summary>
    private static void ReconcileSleepState()
    {
        SleepManagement.ReconcileWithRunState();
    }

    /// <summary>
    /// 是否空闲（仅反映任务运行状态）。
    /// 仅供 UI 绑定和按钮状态使用。需要判断"是否可以安全执行打断性操作"（如自动更新重启）时，
    /// 应使用 <see cref="CanInterrupt"/>，它会额外排除中断锁定（结束后脚本、倒计时等）的情况。
    /// </summary>
    /// <returns>当前是否空闲。</returns>
    public bool GetIdle() => Idle;

    public void SetIdle(bool idle, [CallerMemberName] string caller = "")
    {
        _logger.Information("Idle: {Old} to {New} (called from {Caller})", Idle, idle, caller);
        Idle = idle;
    }

    // 引用计数：CheckAfterCompleted 外层和 TimerCanceledAsync 内层各自 Lock/Unlock，
    // 只有当计数归零时才真正解除锁定，避免嵌套 try-finally 提前解锁的竞态。
    private int _interruptLockDepth;

    /// <summary>
    /// 锁定中断（引用计数 +1）。
    /// 锁定期间（关机/休眠倒计时、结束后脚本、退出游戏、杀模拟器等），
    /// <see cref="CanInterrupt"/> 返回 false，<see cref="UntilIdleAsync"/> 会持续等待。
    /// 计数变更与 <see cref="BeginRun"/> 的认领共用 <see cref="_runTransitionLock"/>，
    /// 保证 ｢检查可打断｣ 与 ｢声明运行归属｣ 之间不会被中断锁定插入。
    /// </summary>
    /// <param name="caller">调用方名称。</param>
    public void LockInterrupt([CallerMemberName] string caller = "")
    {
        int newValue;
        lock (_runTransitionLock)
        {
            newValue = Interlocked.Increment(ref _interruptLockDepth);
        }

        _logger.Information("InterruptLock: depth={Depth} (called from {Caller})", newValue, caller);
    }

    /// <summary>
    /// 若当前恰好可打断（空闲且中断未锁定），在同一把锁内原子地取得中断锁并返回 <see langword="true"/>。
    /// 供 ｢等待空闲后还要执行收尾脚本｣ 的路径使用：避免 <see cref="UntilIdleAsync"/> 返回后、
    /// 真正 <see cref="LockInterrupt"/> 之前被新的开始入口插入。
    /// </summary>
    /// <param name="caller">调用方名称。</param>
    /// <returns>是否成功取得中断锁。</returns>
    public bool TryLockInterruptOnIdle([CallerMemberName] string caller = "")
    {
        int newValue;
        lock (_runTransitionLock)
        {
            if (!CanInterrupt())
            {
                _logger.Information("TryLockInterruptOnIdle rejected (called from {Caller})", caller);
                return false;
            }

            newValue = Interlocked.Increment(ref _interruptLockDepth);
        }

        _logger.Information("InterruptLock: depth={Depth} (called from {Caller}, reserved on idle)", newValue, caller);
        return true;
    }

    /// <summary>
    /// 解除锁定（引用计数 -1，不小于 0）。
    /// </summary>
    /// <param name="caller">调用方名称。</param>
    public void UnlockInterrupt([CallerMemberName] string caller = "")
    {
        int newValue;
        lock (_runTransitionLock)
        {
            newValue = Interlocked.Decrement(ref _interruptLockDepth);
            if (newValue < 0)
            {
                // 仅当值仍为该负数时才归零，避免与并发 LockInterrupt 竞态抹掉合法的锁
                Interlocked.CompareExchange(ref _interruptLockDepth, 0, newValue);
            }
        }

        if (newValue < 0)
        {
            _logger.Warning("InterruptLock unlock: depth underflow, clamping to 0 (called from {Caller})", caller);
        }
        else
        {
            _logger.Information("InterruptLock: depth={Depth} (called from {Caller})", newValue, caller);
        }

        SignalCanInterrupt();
    }

    /// <summary>
    /// 当前中断是否被锁定。
    /// </summary>
    /// <returns>是否被锁定。</returns>
    public bool IsInterruptLocked() => Volatile.Read(ref _interruptLockDepth) > 0;

    /// <summary>
    /// 当前是否可以安全打断（空闲且中断未锁定）。
    /// 中断锁定期间（关机/休眠倒计时、结束后脚本、退出游戏、杀模拟器等）不属于可安全打断的状态。
    /// </summary>
    /// <returns>空闲且中断未锁定返回 <see langword="true"/>，否则返回 <see langword="false"/>。</returns>
    public bool CanInterrupt() => GetIdle() && !IsInterruptLocked();

    // 等待可打断的广播信号；状态跃迁或中断锁归零且恰好可打断时置位换新，等待方被即时唤醒
    private TaskCompletionSource _canInterruptSignal = new(TaskCreationOptions.RunContinuationsAsynchronously);

    private void SignalCanInterrupt()
    {
        if (!CanInterrupt())
        {
            return;
        }

        Interlocked.Exchange(ref _canInterruptSignal, new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously)).TrySetResult();
    }

    private bool _inited;

    public bool Inited
    {
        get => _inited;
        set {
            if (_inited != value)
            {
                var oldState = new StateSnapshot(_idle, _inited, _stopping, _runOwner);
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
                var oldState = new StateSnapshot(_idle, _inited, _stopping, _runOwner);
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
        StateChanged?.Invoke(this, new(oldState, _idle, _inited, _stopping, _runOwner));
        SignalCanInterrupt();
    }

    /// <summary>
    /// 等待状态变为闲置（可打断），状态广播即时唤醒，无轮询。
    /// </summary>
    /// <param name="confirmInterval">确认间隔(ms)</param>
    /// <param name="confirmTimes">确认次数；0 表示等到可打断即返回，不防抖确认</param>
    /// <param name="timeout">总等待上限(ms)；小于 0（如 <see cref="Timeout.Infinite"/>）表示无限等待</param>
    /// <returns>是否在超时内等到闲置；false 表示超时放弃</returns>
    public async Task<bool> UntilIdleAsync(int confirmInterval = 1000, int confirmTimes = 3, int timeout = Timeout.Infinite)
    {
        var deadline = timeout < 0 ? DateTime.MaxValue : DateTime.UtcNow.AddMilliseconds(timeout);
        while (true)
        {
            while (!CanInterrupt())
            {
                // 无限等待须传 InfiniteTimeSpan：deadline 为 DateTime.MaxValue 时的差值远超
                // Task.WaitAsync(TimeSpan) 的上限，直接传剩余时间会抛 ArgumentOutOfRangeException
                var remaining = timeout < 0 ? Timeout.InfiniteTimeSpan : deadline - DateTime.UtcNow;
                if (timeout >= 0 && remaining <= TimeSpan.Zero)
                {
                    _logger.Information("Idle not reached before timeout.");
                    return false;
                }

                // 读取信号与挂起之间广播可能已置位，挂起前双查兜底
                var signal = Volatile.Read(ref _canInterruptSignal);
                if (CanInterrupt())
                {
                    break;
                }

                try
                {
                    await signal.Task.WaitAsync(remaining);
                }
                catch (TimeoutException)
                {
                    _logger.Information("Idle not reached before timeout.");
                    return false;
                }
            }

            int confirmed = 0;
            while (confirmed < confirmTimes)
            {
                if (DateTime.UtcNow >= deadline)
                {
                    _logger.Information("Idle not confirmed before timeout.");
                    return false;
                }

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
                return true;
            }
        }
    }
}
