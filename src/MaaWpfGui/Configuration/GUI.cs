// <copyright file="GUI.cs" company="MaaAssistantArknights">
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

using System.ComponentModel;

namespace MaaWpfGui.Configuration
{
    public class GUI : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public DarkModeType DarkMode { get; set; } = DarkModeType.SyncWithOs;

        public bool UseNotify { get; set; } = true;

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }

        /// <summary>
        /// 表示深色模式的类型。
        /// </summary>
        public enum DarkModeType
        {
            /// <summary>
            /// 与操作系统的深色模式同步。
            /// </summary>
            SyncWithOs = 0,

            /// <summary>
            /// 明亮的主题。
            /// </summary>
            Light,

            /// <summary>
            /// 暗黑的主题。
            /// </summary>
            Dark,
        }
    }
}
