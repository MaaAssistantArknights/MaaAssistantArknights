// <copyright file="Toolbox.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using MaaWpfGui.Models;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 小工具相关设置
/// </summary>
public partial class Toolbox : NotifyPropertyChangedWithValue
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Recruitment.EventBinding(prefix + nameof(Recruitment) + ".");
        OperBox.EventBinding(prefix + nameof(OperBox) + ".");
        Gacha.EventBinding(prefix + nameof(Gacha) + ".");
        Peep.EventBinding(prefix + nameof(Peep) + ".");
        MiniGame.EventBinding(prefix + nameof(MiniGame) + ".");
    }

    [JsonInclude]
    public Recruitment Recruitment { get; private set; } = new();

    [JsonInclude]
    public OperBox OperBox { get; private set; } = new();

    [JsonInclude]
    public Gacha Gacha { get; private set; } = new();

    [JsonInclude]
    public Peep Peep { get; private set; } = new();

    [JsonInclude]
    public MiniGame MiniGame { get; private set; } = new();
}
