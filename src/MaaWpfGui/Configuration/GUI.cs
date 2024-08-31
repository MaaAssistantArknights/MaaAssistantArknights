// <copyright file="GUI.cs" company="MaaAssistantArknights">
// <copyright file="GUI.cs" company="MaaAssistantArknights">
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

using System.ComponentModel;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using static MaaWpfGui.Models.PostActionSetting;
using static MaaWpfGui.ViewModels.UI.TaskQueueViewModel;

namespace MaaWpfGui.Configuration
{
    public class GUI : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public DarkModeType DarkMode { get; set; } = DarkModeType.Light;

        public bool UseNotify { get; set; } = true;

        public string Localization { get; set; } = LocalizationHelper.DefaultLanguage;

        public bool MinimizeToTray { get; set; } = false;

        public bool MinimizeDirectly { get; set; } = false;

        public bool HideCloseButton { get; set; } = false;

        public string LogItemDateFormat { get; set; } = "HH:mm:ss";

        public WindowPlacement? WindowPlacement { get; set; } = null;

        public bool LoadWindowPlacement { get; set; } = true;

        public bool SaveWindowPlacement { get; set; } = true;

        public bool UseAlternateStage { get; set; } = false;

        public bool HideUnavailableStage { get; set; } = true;

        public bool CustomStageCode { get; set; } = false;

        public PostActions PostActions { get; set; } = PostActions.None;

        public InverseClearType InverseClearMode { get; set; } = InverseClearType.Clear;

        public InverseClearType InverseClearShow { get; set; } = InverseClearType.Clear;

        public string WindowTitlePrefix { get; set; } = string.Empty;

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

        public enum InverseClearType
        {
            /// <summary>
            /// 清空
            /// </summary>
            Clear = 0,

            /// <summary>
            /// 反选
            /// </summary>
            Inverse,

            /// <summary>
            /// 二者可切
            /// </summary>
            ClearInverse,
        }

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
