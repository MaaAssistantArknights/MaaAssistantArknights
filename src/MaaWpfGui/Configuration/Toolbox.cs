// <copyright file="RemoteControl.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.ComponentModel;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Configuration
{
    public class Toolbox : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        /// <summary>
        /// Gets or sets TODO 类型修改
        /// </summary>
        public IReadOnlyList<OperData> OperBox { get; set; } = [];

        /// <summary>
        /// Gets or sets a value indicating whether 抽卡警告不再显示
        /// </summary>
        public bool GachaShowDisclaimerNoMore { get; set; }

        public class OperData
        {
            /// <summary>
            /// Gets or sets 干员Id，例如char_2023_ling
            /// </summary>
            public string? Id { get; set; }

            public string? Name { get; set; }

            public int Elite { get; set; }

            public int Level { get; set; }

            public bool Own { get; set; }

            /// <summary>
            /// Gets or sets 潜能
            /// </summary>
            public int Potential { get; set; }

            /// <summary>
            /// Gets or sets 几星
            /// </summary>
            public int Rarity { get; set; }
        }

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
