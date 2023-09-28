// <copyright file="CopilotItemViewModel.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    public class CopilotItemViewModel : PropertyChangedBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="filePath">The original Name of file</param>
        /// <param name="isChecked">isChecked</param>
        public CopilotItemViewModel(string name, string filePath, bool isChecked = true)
        {
            Name = name;
            FilePath = filePath;
            _isChecked = isChecked;
        }

        /// <summary>
        /// Gets the original_name.
        /// </summary>
        public string FilePath { get; }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string Name { get; }

        private bool _isChecked;

        /// <summary>
        /// Gets or sets a value indicating whether gets or sets whether the key is checked.
        /// </summary>
        public bool IsChecked
        {
            get => _isChecked;
            set
            {
                SetAndNotify(ref _isChecked, value);
                Instances.CopilotViewModel.SaveCopilotTask();
            }
        }

        private int _index;

        public int Index
        {
            get => _index;
            set => SetAndNotify(ref _index, value);
        }
    }
}
