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

namespace MaaWpfGui.States
{
    public class RunningState
    {
        private static RunningState _instance;

        private RunningState()
        {
        }

        public static RunningState Instance
        {
            get
            {
                _instance ??= new RunningState();
                return _instance;
            }
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
