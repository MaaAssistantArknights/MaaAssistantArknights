// <copyright file="Performance.cs" company="MaaAssistantArknights">
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
    /// <summary>
    /// 性能设置
    /// </summary>
    public class Performance : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        public bool UseGpu { get; set; }

        public string? GpuDescription { get; set; }

        public string? GpuInstancePath { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 允许使用不推荐的GPU
        /// </summary>
        public bool AllowDeprecatedGpu { get; set; }

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
