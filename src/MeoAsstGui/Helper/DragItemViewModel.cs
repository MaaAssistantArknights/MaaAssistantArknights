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
            this._isCheckedStorageKey = storageKey + name + ".IsChecked";
            this.IsChecked = System.Convert.ToBoolean(ViewStatusStorage.Get(_isCheckedStorageKey, bool.TrueString));
        }

        private string _name;

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name
        {
            get { return _name; }
            set { SetAndNotify(ref _name, value); }
        }

        private readonly string _isCheckedStorageKey;
        private bool _isChecked;

        /// <summary>
        /// Gets or sets a value indicating whether gets or sets whether the key is checked.
        /// </summary>
        public bool IsChecked
        {
            get
            {
                return _isChecked;
            }

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
            get { return _iconPath; }
            set { SetAndNotify(ref _iconPath, value); }
        }

        private string _token;

        /// <summary>
        /// Gets or sets the token.
        /// </summary>
        public string Token
        {
            get { return _token; }
            set { SetAndNotify(ref _token, value); }
        }

        private string _runStatus;

        /// <summary>
        /// Gets or sets the running status.
        /// </summary>
        public string RunStatus
        {
            get { return _runStatus; }
            set { SetAndNotify(ref _runStatus, value); }
        }
    }
}
