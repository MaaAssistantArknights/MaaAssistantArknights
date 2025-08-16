// <copyright file="SettingItemViewModel.cs" company="MaaAssistantArknights">
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

using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// The view model of setting item.
    /// </summary>
    public class SettingItemViewModel(string key, string display, int value) : PropertyChangedBase
    {
        private string _key = key;

        public string Key
        {
            get => _key;
            set => SetAndNotify(ref _key, value);
        }

        private string _display = display;

        public string Display
        {
            get => _display;
            set => SetAndNotify(ref _display, value);
        }

        private int _value = value;

        public int Value
        {
            get => _value;
            set => SetAndNotify(ref _value, value);
        }
    }
}
