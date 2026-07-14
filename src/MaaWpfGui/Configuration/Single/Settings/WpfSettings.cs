// <copyright file="WpfSettings.cs" company="MaaAssistantArknights">
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
#nullable enable
using System;
using System.ComponentModel;

namespace MaaWpfGui.Configuration.Single.Settings;

public class WpfSettings : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix, Func<string, PropertyChangedEventHandler> valueFactory)
    {
        PropertyChanged += valueFactory.Invoke(prefix + nameof(WpfSettings) + ".");
        Performance.PropertyChanged += valueFactory.Invoke(prefix + nameof(WpfSettings) + "." + nameof(Performance) + ".");
    }

    public Performance Performance { get; set; } = new();
}
