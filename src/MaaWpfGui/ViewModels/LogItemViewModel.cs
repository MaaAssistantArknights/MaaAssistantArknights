// <copyright file="LogItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
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
        public LogItemViewModel(string content, string color = UiLogColor.Message, string weight = "Regular")
        {
            Time = DateTime.Now.ToString("MM'-'dd'  'HH':'mm':'ss");
            Content = content;
            Color = color;
            Weight = weight;
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
    }
}
