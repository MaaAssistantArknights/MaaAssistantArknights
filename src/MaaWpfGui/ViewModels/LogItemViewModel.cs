// <copyright file="LogItemViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using System.Windows.Media;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// The view model of log item.
    /// </summary>
    public class LogItemViewModel : PropertyChangedBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogItemViewModel"/> class.
        /// </summary>
        /// <param name="content">The content.</param>
        /// <param name="color">The font color.</param>
        /// <param name="weight">The font weight.</param>
        /// <param name="dateFormat">The Date format string</param>
        /// <param name="showTime">The showtime bool.</param>
        /// <param name="toolTip">The toolTip</param>
        public LogItemViewModel(string content, string color = UiLogColor.Message, string weight = "Regular", string dateFormat = "", bool showTime = true, ToolTip? toolTip = null)
        {
            if (string.IsNullOrEmpty(dateFormat))
            {
                dateFormat = SettingsViewModel.GuiSettings.LogItemDateFormatString;
            }

            _time = DateTime.Now.ToString(dateFormat);
            _content = content;
            _color = color;
            _weight = weight;
            _showTime = showTime;
            _toolTip = toolTip;
        }

        private string _time;

        /// <summary>
        /// Gets or sets the time.
        /// </summary>
        public string Time
        {
            get => _time;
            set => SetAndNotify(ref _time, value);
        }

        private bool _showTime;

        public bool ShowTime
        {
            get => _showTime;
            set => SetAndNotify(ref _showTime, value);
        }

        private string _content;

        /// <summary>
        /// Gets or sets the content.
        /// </summary>
        public string Content
        {
            get => _content;
            set => SetAndNotify(ref _content, value);
        }

        private string _color;

        /// <summary>
        /// Gets or sets the font color.
        /// </summary>
        public string Color
        {
            get => _color;
            set => SetAndNotify(ref _color, value);
        }

        private string _weight;

        /// <summary>
        /// Gets or sets the font weight.
        /// </summary>
        public string Weight
        {
            get => _weight;
            set => SetAndNotify(ref _weight, value);
        }

        [PropertyDependsOn(nameof(ToolTip))]
        public bool ShowToolTip => _toolTip is { Content: not null };

        private ToolTip? _toolTip;

        /// <summary>
        /// Gets or sets the toolTip.
        /// </summary>
        public ToolTip? ToolTip
        {
            get => _toolTip;
            set => SetAndNotify(ref _toolTip, value);
        }
    }
}
