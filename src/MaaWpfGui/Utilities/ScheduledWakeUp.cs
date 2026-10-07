// <copyright file="ScheduledWakeUp.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Main;
using MaaWpfGui.WineCompat;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.System.Com;
using Windows.Win32.System.TaskScheduler;
using Timer = MaaWpfGui.Configuration.Global.Timer;

namespace MaaWpfGui.Utilities;

/// <summary>
/// Registers a Windows scheduled task with one wake-enabled daily trigger per enabled in-app timer,
/// which wakes the computer and launches MAA a few minutes before the timer so that the existing
/// in-process timer logic can take over afterwards.
/// 通过 Windows 任务计划程序注册单个计划任务，每个启用的定时项对应一个每日触发器，提前唤醒计算机并启动 MAA。
/// </summary>
public static class ScheduledWakeUp
{
    private const int TimerSlotCount = 8;
    private const int WakeUpLeadMinutes = 5;
    private const int DebounceMilliseconds = 500;

    private static readonly ILogger _logger = Log.ForContext("SourceContext", "ScheduledWakeUp");
    private static readonly string _fileValue = Environment.ProcessPath ?? string.Empty;
    private static readonly string _taskName = $"MaaWakeup_{InstanceIdentifier.GetHash(_fileValue)}";

    // CLSID_TaskScheduler：taskschd.dll 的 Task Scheduler 服务 coclass（ProgID Schedule.Service）
    private static readonly Guid ClsidTaskScheduler = new("0F87369F-A4E5-4CFC-BD3E-73E6154572DD");

    private static CancellationTokenSource? _debounceCts;

    // 删任务与注册的序列不可交错，否则并发的 SyncAll 可能留下与开关终态相反的任务
    private static readonly object _syncLock = new();

    /// <summary>
    /// Rebuilds all scheduled wake-up tasks of this instance to match the current settings:
    /// removes every known slot task first, then registers one task per enabled timer when the
    /// global switch is on, and removes all tasks when it is off. Existing tasks are always
    /// overwritten, including ones manually disabled in Task Scheduler.
    /// </summary>
    /// <param name="error">Outputs the error message in case of failure.</param>
    /// <returns>Whether the operation is successful.</returns>
    public static bool SyncAll(out string error)
    {
        error = string.Empty;

        // 演示模式（README 截图）不应改动系统状态；Wine 下任务计划程序不可用
        if (Bootstrapper.IsDemoMode || WineRuntimeInformation.IsRunningUnderWine)
        {
            return true;
        }

        try
        {
            lock (_syncLock)
            {
                PInvoke.CoCreateInstance(ClsidTaskScheduler, null, CLSCTX.CLSCTX_ALL, out ITaskService taskService).ThrowOnFailure();
                taskService.Connect(null, null, null, null);
                ITaskFolder rootFolder = GetRootFolder(taskService);
                var timers = ConfigFactory.Root.Timers;

                // 一律覆盖式重建：先删本实例任务，再按当前状态决定是否注册
                DeleteTask(rootFolder);

                if (!timers.ScheduledWakeUp)
                {
                    _logger.Information("Scheduled wake-up is off, task removed");
                    return true;
                }

                RegisterTask(taskService, rootFolder, timers.List);
                return true;
            }
        }
        catch (COMException e)
        {
            error = "Failed to sync scheduled wake-up tasks (0x" + e.HResult.ToString("X8") + "): " + e.Message;
            _logger.Error(e, "{ErrorMessage}", error);
            return false;
        }
        catch (Exception e)
        {
            error = "Failed to sync scheduled wake-up tasks: " + e.GetType().Name + ": " + e.Message;
            _logger.Error(e, "{ErrorMessage}", error);
            return false;
        }
    }

    /// <summary>
    /// Debounced variant of <see cref="SyncAll(out string)"/> for high-frequency change sources such
    /// as the hour/minute spinners; consecutive calls within the debounce window collapse into one sync.
    /// </summary>
    public static void SyncAllDebounced()
    {
        var cts = new CancellationTokenSource();
        var previous = Interlocked.Exchange(ref _debounceCts, cts);
        if (previous != null)
        {
            previous.Cancel();
        }

        _ = Task.Run(async () =>
        {
            try
            {
                await Task.Delay(DebounceMilliseconds, cts.Token).ConfigureAwait(false);
            }
            catch (TaskCanceledException)
            {
                return;
            }

            SyncAll(out _);
        });
    }

    private static ITaskFolder GetRootFolder(ITaskService taskService)
    {
        return WithBStr("\\", rootPath =>
        {
            taskService.GetFolder(rootPath, out ITaskFolder rootFolder);
            return rootFolder;
        });
    }

    private static void DeleteTask(ITaskFolder rootFolder)
    {
        try
        {
            WithBStr(_taskName, taskName => rootFolder.DeleteTask(taskName, 0));
            _logger.Information("Deleted scheduled wake-up task {TaskName}", _taskName);
        }
        catch (Exception e) when (e.HResult == unchecked((int)0x80070002))
        {
            // The task does not exist; nothing to delete.
            // 0x80070002 在 .NET 被映射为 FileNotFoundException 而非 COMException，须按 HResult 过滤
        }
    }

    private static void RegisterTask(ITaskService taskService, ITaskFolder rootFolder, ObservableCollection<Timer> timers)
    {
        taskService.NewTask(0, out ITaskDefinition definition);
        WithBStr("Wakes up the computer and launches MAA before each enabled scheduled timer", description => definition.RegistrationInfo.Description = description);
        definition.Principal.LogonType = TASK_LOGON_TYPE.TASK_LOGON_INTERACTIVE_TOKEN;

        // 最高权限运行跟随 MAA 当前进程的权限状态，避免唤醒拉起后的权限与用户日常使用不一致
        definition.Principal.RunLevel = Bootstrapper.IsUserAdministrator()
            ? TASK_RUNLEVEL_TYPE.TASK_RUNLEVEL_HIGHEST
            : TASK_RUNLEVEL_TYPE.TASK_RUNLEVEL_LUA;

        // 每个启用的定时项一个每日触发器，Id 对应槽位号以保留排查时的对应关系；
        // null（右键清空的第三态）视为启用，与 TaskQueueViewModel.CheckTimers 的判定一致
        int triggerCount = 0;
        for (int slot = 0; slot < Math.Min(timers.Count, TimerSlotCount); slot++)
        {
            var timer = timers[slot];
            if (timer.IsEnabled == false)
            {
                continue;
            }

            CreateTrigger(definition.Triggers, slot + 1, timer.Hour, timer.Minute);
            triggerCount++;
        }

        definition.Actions.Create(TASK_ACTION_TYPE.TASK_ACTION_EXEC, out var action);
        var execAction = (IExecAction)action;
        WithBStr(_fileValue, path => execAction.Path = path);
        WithBStr(Bootstrapper.SkipStartupAutoRunArg, arguments => execAction.Arguments = arguments);
        WithBStr(Path.GetDirectoryName(_fileValue) ?? string.Empty, directory => execAction.WorkingDirectory = directory);

        ITaskSettings settings = definition.Settings;
        settings.WakeToRun = true;
        settings.DisallowStartIfOnBatteries = false;
        settings.StopIfGoingOnBatteries = false;

        // 禁用执行时限：默认 72 小时后终止任务会杀掉常驻的 GUI 进程
        WithBStr("PT0S", limit => settings.ExecutionTimeLimit = limit);

        WithBStr(_taskName, taskName => rootFolder.RegisterTaskDefinition(
            taskName,
            definition,
            (int)TASK_CREATION.TASK_CREATE_OR_UPDATE,
            null,
            null,
            TASK_LOGON_TYPE.TASK_LOGON_INTERACTIVE_TOKEN,
            null,
            out _));
        _logger.Information("Registered scheduled wake-up task {TaskName} with {TriggerCount} trigger(s)", _taskName, triggerCount);
    }

    private static void CreateTrigger(ITriggerCollection triggers, int slot, int hour, int minute)
    {
        // 触发时间取定时点前 5 分钟，跨午夜时落到前一晚（例如 00:03 的定时点在前一天 23:58 唤醒）
        int totalMinutes = (hour * 60) + minute - WakeUpLeadMinutes;
        int wakeUpMinuteOfDay = ((totalMinutes % 1440) + 1440) % 1440;
        string startTime = FormattableString.Invariant($"{wakeUpMinuteOfDay / 60:D2}:{wakeUpMinuteOfDay % 60:D2}:00");

        triggers.Create(TASK_TRIGGER_TYPE2.TASK_TRIGGER_DAILY, out var trigger);
        var dailyTrigger = (IDailyTrigger)trigger;
        WithBStr($"Timer{slot}", id => dailyTrigger.Id = id);
        WithBStr("2000-01-01T" + startTime, boundary => dailyTrigger.StartBoundary = boundary);
        dailyTrigger.DaysInterval = 1;
    }

    /// <summary>
    /// Runs <paramref name="use"/> with a COM BSTR built from <paramref name="value"/> and frees it afterwards.
    /// COM copies the string on assignment, so the temporary BSTR must be released by the caller.
    /// </summary>
    /// <param name="value">The string value to pass as a BSTR.</param>
    /// <param name="use">The action that receives the BSTR.</param>
    private static void WithBStr(string value, Action<BSTR> use)
    {
        IntPtr pointer = Marshal.StringToBSTR(value);
        try
        {
            use((BSTR)pointer);
        }
        finally
        {
            Marshal.FreeBSTR(pointer);
        }
    }

    /// <summary>
    /// Invokes <paramref name="use"/> with a COM BSTR built from <paramref name="value"/>, frees the BSTR
    /// afterwards and returns the result of <paramref name="use"/>.
    /// </summary>
    /// <typeparam name="TResult">The result type of <paramref name="use"/>.</typeparam>
    /// <param name="value">The string value to pass as a BSTR.</param>
    /// <param name="use">The function that receives the BSTR.</param>
    /// <returns>The result of <paramref name="use"/>.</returns>
    private static TResult WithBStr<TResult>(string value, Func<BSTR, TResult> use)
    {
        IntPtr pointer = Marshal.StringToBSTR(value);
        try
        {
            return use((BSTR)pointer);
        }
        finally
        {
            Marshal.FreeBSTR(pointer);
        }
    }
}
