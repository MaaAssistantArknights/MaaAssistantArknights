// <copyright file="Connection.cs" company="MaaAssistantArknights">
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

#nullable enable
using System.ComponentModel;

namespace MaaWpfGui.Configuration
{
    public class Connection : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        /// <summary>
        /// Gets or sets a value indicating whether 自动检测adb地址及端口
        /// </summary>
        public bool AutoDetect { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether 每次都自动检测
        /// </summary>
        public bool AlwaysAutoDetect { get; set; }

        public string AdbPath { get; set; } = string.Empty;

        public string AdbAddress { get; set; } = string.Empty;

        public bool RetryOnAdbDisconnected { get; set; }

        public bool AllAdbRestart { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether allow for killing ADB process（未核对
        /// </summary>
        public bool AllAdbHardRestart { get; set; } = true;

        public bool AdbLiteEnable { get; set; }

        public bool KillAdbWhenExit { get; set; }

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
