// <copyright file="MiniGame.cs" company="MaaAssistantArknights">
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
/// 工具箱小游戏标签页相关设置
/// </summary>
public class MiniGame : NotifyPropertyChangedWithValue
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Reclamation.EventBinding(prefix + nameof(Reclamation) + ".");
    }

    public string EventShopBlackList { get; set; } = string.Empty;

    public bool EventShopBlackListDataSupplementInstrument { get; set; }

    public bool EventShopBlackListDataSupplementStick { get; set; }

    public bool EventShopBlackListLmd { get; set; }

    public bool EventShopBlackListFurniturePart { get; set; }

    public bool EventShopBlackListOther { get; set; }

    [JsonInclude]
    public Reclamation Reclamation { get; private set; } = new();
}
