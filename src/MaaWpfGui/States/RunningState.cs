// <copyright file="RunningState.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
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
        private static RunningState instance;

        private RunningState()
        {
        }

        public static RunningState Instance
        {
            get
            {
                instance ??= new RunningState();
                return instance;
            }
        }

        // values
        private bool _idle = true;

        public bool Idle
        {
            get => _idle;
            set
            {
                if (_idle != value)
                {
                    _idle = value;
                    OnIdleChanged(value);
                }
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

        public async Task UntilIdleAsync(int time = 1000)
        {
            while (!GetIdle())
            {
                await Task.Delay(time);
            }
        }
    }
}
