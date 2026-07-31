// <copyright file="Background.cs" company="MaaAssistantArknights">
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
using System.Windows.Media;
using PropertyChanged;

namespace MaaWpfGui.Configuration.Global;

/// <summary>
/// 背景设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class Background
{
    public string ImagePath { get; set; } = "background/background.png";
    public string ImagePath { get; set; } = "Res/Backgrounds/Wallpapers/background.png";

    public Stretch StretchMode { get; set; } = Stretch.Fill;

    public int Opacity { get; set; } = 50;

    public int BlurEffectRadius { get; set; } = 5;
}
