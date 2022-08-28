// <copyright file="DragItemViewModel.cs" company="MaaAssistantArknights">
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

using Stylet;

namespace MeoAsstGui
{
    /// <summary>
    /// The view model of drag item.
    /// </summary>
    public class DragItemViewModel : PropertyChangedBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="storageKey">The storage key.</param>
        public DragItemViewModel(string name, string storageKey)
        {
            this.Name = name;
            this.OriginalName = name;
            this._isCheckedStorageKey = storageKey + name + ".IsChecked";
            this.IsChecked = System.Convert.ToBoolean(ViewStatusStorage.Get(_isCheckedStorageKey, bool.TrueString));
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DragItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name (viewed name).</param>
        /// <param name="original_name">The original name (may not be the same as viewed name).</param>
        /// <param name="storageKey">The storage key.</param>
        public DragItemViewModel(string name, string original_name, string storageKey)
        {
            this.Name = name;
            this.OriginalName = original_name;
            this._isCheckedStorageKey = storageKey + original_name + ".IsChecked";
            this.IsChecked = System.Convert.ToBoolean(ViewStatusStorage.Get(_isCheckedStorageKey, bool.TrueString));
        }

        private string _original_name;

        /// <summary>
        /// Gets or sets the original_name.
        /// </summary>
        public string OriginalName
        {
            get => _original_name;
            set => SetAndNotify(ref _original_name, value);
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

        private readonly string _isCheckedStorageKey;
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
                ViewStatusStorage.Set(_isCheckedStorageKey, value.ToString());
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
