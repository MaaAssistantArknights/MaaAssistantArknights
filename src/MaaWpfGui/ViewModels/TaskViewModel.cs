// <copyright file="TaskViewModel.cs" company="MaaAssistantArknights">
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
using System.Threading.Tasks;
using MaaWpfGui.Configuration;
using MaaWpfGui.Models;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    public class TaskViewModel : PropertyChangedBase
    {
        public TaskViewModel(int index, string name, bool? isCheckedWithNull)
        {
            _index = index;
            _name = name;
            _isCheckedWithNull = isCheckedWithNull;
        }

        private string _name;

        public string Name
        {
            get => _name;
            set
            {
                SetAndNotify(ref _name, value);
                ConfigFactory.CurrentConfig.TaskQueue[_index].Name = value;
            }
        }

        private bool? _isCheckedWithNull;

        public bool? IsCheckedWithNull
        {
            get => _isCheckedWithNull;
            set
            {
                SetAndNotify(ref _isCheckedWithNull, value);
                ConfigFactory.CurrentConfig.TaskQueue[_index].IsEnable = value;
            }
        }

        private int _index;

        public int Index
        {
            get => _index;
            set => SetAndNotify(ref _index, value);
        }

        private bool _enableSetting;

        /// <summary>
        /// Gets or sets a value indicating whether gets or sets whether the setting enabled.
        /// </summary>
        public bool EnableSetting
        {
            get => _enableSetting;
            set
            {
                SetAndNotify(ref _enableSetting, value);
                TaskSettingVisibilityInfo.Current.Set(_index, value);
            }
        }
    }
}
