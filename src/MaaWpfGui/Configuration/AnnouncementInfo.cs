// <copyright file="AnnouncementInfo.cs" company="MaaAssistantArknights">
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
    public class AnnouncementInfo : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        /// <summary>
        /// Gets or sets 公告内容
        /// </summary>
        public string Info { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets a value indicating whether 下次不再显示公告
        /// </summary>
        public bool DoNotShowAgain { get; set; } = false;

        /// <summary>
        /// Gets or sets a value indicating whether 不显示公告
        /// </summary>
        public bool DoNotShow { get; set; } = false;

        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
