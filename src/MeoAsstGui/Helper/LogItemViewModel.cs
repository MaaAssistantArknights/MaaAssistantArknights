// <copyright file="LogItemViewModel.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using Stylet;

namespace MeoAsstGui
{
    public class LogItemViewModel : PropertyChangedBase
    {
        public LogItemViewModel(string content, string color = "Black", string weight = "Regular")
        {
            Time = DateTime.Now.ToString("MM'-'dd'  'HH':'mm':'ss") + "  ";
            this.Content = content;
            this.Color = color;
            this.Weight = weight;
        }

        private string _time;

        public string Time
        {
            get { return _time; }
            set { SetAndNotify(ref _time, value); }
        }

        private string _content;

        public string Content
        {
            get { return _content; }
            set { SetAndNotify(ref _content, value); }
        }

        private string _color;

        public string Color
        {
            get { return _color; }
            set { SetAndNotify(ref _color, value); }
        }

        private string _weight;

        public string Weight
        {
            get { return _weight; }
            set { SetAndNotify(ref _weight, value); }
        }
    }
}
