// <copyright file="DragItemViewModel.cs" company="MaaAssistantArknights">
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

using System;
using JetBrains.Annotations;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
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
        /// <param name="defaultCheck">The default checked status.</param>
        public DragItemViewModel(string name, string storageKey, bool defaultCheck = true)
        {
            Name = name;
            OriginalName = name;
            _storageKey = storageKey;
            IsChecked = Convert.ToBoolean(ConfigurationHelper.GetCheckedStorage(storageKey, name, defaultCheck.ToString()));
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DragItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name (viewed name).</param>
        /// <param name="originalName">The original name (may not be the same as viewed name).</param>
        /// <param name="storageKey">The storage key.</param>
        /// <param name="defaultCheck">The default checked status.</param>
        public DragItemViewModel(string name, string originalName, string storageKey, bool defaultCheck = true)
        {
            Name = name;
            OriginalName = originalName;
            _storageKey = storageKey;
            IsChecked = Convert.ToBoolean(ConfigurationHelper.GetCheckedStorage(storageKey, originalName, defaultCheck.ToString()));
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

        private bool? _isCheckedWithNull;

        /// <summary>
        /// Gets or sets a value indicating whether the key is checked with null.
        /// </summary>
        public bool? IsCheckedWithNull
        {
            get => _isCheckedWithNull;
            set
            {
                SetAndNotify(ref _isCheckedWithNull, value);
                value ??= false;
                ConfigurationHelper.SetCheckedStorage(_storageKey, OriginalName, value.ToString());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the key is checked.
        /// </summary>
        public bool IsChecked
        {
            get => IsCheckedWithNull != false;
            set => IsCheckedWithNull = value;
        }

        // 换成图标的话要这个，暂时没用
        private string _iconPath;

        /// <summary>
        /// Gets or sets the icon path.
        /// </summary>
        [UsedImplicitly]
        public string IconPath
        {
            get => _iconPath;
            set => SetAndNotify(ref _iconPath, value);
        }

        private string _token;

        /// <summary>
        /// Gets or sets the token.
        /// </summary>
        [UsedImplicitly]
        public string Token
        {
            get => _token;
            set => SetAndNotify(ref _token, value);
        }

        private string _runStatus;

        /// <summary>
        /// Gets or sets the running status.
        /// </summary>
        [UsedImplicitly]
        public string RunStatus
        {
            get => _runStatus;
            set => SetAndNotify(ref _runStatus, value);
        }

        private bool _enableSetting;

        /// <summary>
        /// Gets or sets a value indicating whether the setting enabled.
        /// </summary>
        public bool EnableSetting
        {
            get => _enableSetting;
            set
            {
                SetAndNotify(ref _enableSetting, value);
                TaskSettingVisibilityInfo.Instance.Set(OriginalName, value);
            }
        }
    }
}
