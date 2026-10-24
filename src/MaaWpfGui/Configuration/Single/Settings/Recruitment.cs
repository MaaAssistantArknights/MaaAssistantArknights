// <copyright file="Recruitment.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using MaaWpfGui.Models;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 公招识别相关设置
/// </summary>
public class Recruitment : NotifyPropertyChangedWithValue
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
    }

    public bool ChooseLevel3 { get; set; } = true;

    public int ChooseLevel3Time { get; set; } = 540;

    public bool ChooseLevel4 { get; set; } = true;

    public int ChooseLevel4Time { get; set; } = 540;

    public bool ChooseLevel5 { get; set; } = true;

    public bool ChooseLevel6 { get; set; } = true;

    public bool AutoSetTime { get; set; } = true;

    public bool ShowPotential { get; set; } = true;
}
