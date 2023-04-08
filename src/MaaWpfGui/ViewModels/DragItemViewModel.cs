// <copyright file="DragItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// The view model of drag item.
    /// </summary>
    public class DragItemViewModel : PropertyChangedBase
    {
        private readonly string _storageKey;

        /// <summary>
        /// Initializes a new instance of the <see cref="DragItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="storageKey">The storage key.</param>
        public DragItemViewModel(string name, string storageKey)
        {
            Name = name;
            OriginalName = name;
            _storageKey = storageKey;
            IsChecked = Convert.ToBoolean(ConfigurationHelper.GetCheckedStorage(storageKey, name, bool.TrueString));
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DragItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name (viewed name).</param>
        /// <param name="originalName">The original name (may not be the same as viewed name).</param>
        /// <param name="storageKey">The storage key.</param>
        public DragItemViewModel(string name, string originalName, string storageKey)
        {
            Name = name;
            OriginalName = originalName;
            _storageKey = storageKey;
            IsChecked = Convert.ToBoolean(ConfigurationHelper.GetCheckedStorage(storageKey, name, bool.TrueString));
        }

        private string _originalName;

        /// <summary>
        /// Gets or sets the original_name.
        /// </summary>
        public string OriginalName
        {
            get => _originalName;
            set => SetAndNotify(ref _originalName, value);
        }

        private string _name;

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name
        {
            get => _name;
            set => SetAndNotify(ref _name, value);
        }

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
                ConfigurationHelper.SetCheckedStorage(_storageKey, Name, value.ToString());
            }
        }

        // 换成图标的话要这个，暂时没用
        private string _iconPath;

        /// <summary>
        /// Gets or sets the icon path.
        /// </summary>
        public string IconPath
        {
            get => _iconPath;
            set => SetAndNotify(ref _iconPath, value);
        }

        private string _token;

        /// <summary>
        /// Gets or sets the token.
        /// </summary>
        public string Token
        {
            get => _token;
            set => SetAndNotify(ref _token, value);
        }

        private string _runStatus;

        /// <summary>
        /// Gets or sets the running status.
        /// </summary>
        public string RunStatus
        {
            get => _runStatus;
            set => SetAndNotify(ref _runStatus, value);
        }
    }
}
