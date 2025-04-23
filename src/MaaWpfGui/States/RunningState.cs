// <copyright file="RunningState.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.States
{
    public class RunningState
    {
        private static RunningState _instance;

        private RunningState()
        {
            if (ReminderIntervalMinutes < 1)
            {
                ReminderIntervalMinutes = 1;
            }

            _timeoutReminderTimer.Interval = ReminderIntervalMinutes * 60 * 1000;
            _timeoutReminderTimer.Elapsed += TimeoutReminderTimer_Elapsed;
        }

        public static RunningState Instance
        {
            get
            {
                _instance ??= new();
                return _instance;
            }
        }

        // 超时相关字段
        private readonly System.Timers.Timer _timeoutReminderTimer = new();
        private DateTime? _taskStartTime;

        public int TaskTimeoutMinutes { get; set; } = SettingsViewModel.GameSettings.TaskTimeoutMinutes;

        private int _reminderIntervalMinutes = SettingsViewModel.GameSettings.ReminderIntervalMinutes;

        public int ReminderIntervalMinutes
        {
            get => _reminderIntervalMinutes;
            set
            {
                if (value < 1)
                {
                    return;
                }

                _reminderIntervalMinutes = value;
                TimeoutReminderTimer_Elapsed(null, null);
                _timeoutReminderTimer.Interval = value * 60 * 1000;
            }
        }

        // 超时事件
        public event EventHandler<string> TimeoutOccurred;

        public void StartTimeoutTimer()
        {
            _taskStartTime = DateTime.Now;
            _timeoutReminderTimer.Start();
        }

        public void StopTimeoutTimer()
        {
            _timeoutReminderTimer.Stop();
            _taskStartTime = null;
        }

        public void ResetTimeout()
        {
            _taskStartTime = DateTime.Now;
        }

        // 超时计时器回调
        private void TimeoutReminderTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (!_taskStartTime.HasValue || _idle)
            {
                return;
            }

            var elapsedMinutes = (DateTime.Now - _taskStartTime.Value).TotalMinutes;

            // 如果任务运行时间未超过超时时间，则直接返回
            if (elapsedMinutes <= TaskTimeoutMinutes)
            {
                return;
            }

            // 每隔 ReminderIntervalMinutes 提示一次
            var message = string.Format(
                LocalizationHelper.GetString("TaskTimeoutWarning"),
                TaskTimeoutMinutes,
                Math.Round(elapsedMinutes));

            TimeoutOccurred?.Invoke(this, message);
        }

        // values
        // 由初始化设置为 true
        private bool _idle;

        public bool Idle
        {
            get => _idle;
            set
            {
                if (_idle == value)
                {
                    return;
                }

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

                OnIdleChanged(value);
            }
        }

        // getters
        public bool GetIdle() => Idle;

        // action
        public void SetIdle(bool idle) => Idle = idle;

        // subscribes
        public event EventHandler<bool> IdleChanged;

        public virtual void OnIdleChanged(bool newIdleValue)
        {
            IdleChanged?.Invoke(this, newIdleValue);
        }

        /// <summary>
        /// 等待状态变为闲置
        /// </summary>
        /// <param name="time">查询间隔</param>
        /// <returns>Task</returns>
        public async Task UntilIdleAsync(int time = 1000)
        {
            while (!GetIdle())
            {
                await Task.Delay(time);
            }
        }
    }
}
