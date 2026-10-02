// <copyright file="PersonalizationSettingsUserControlModel.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Configuration.Factory;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class PersonalizationSettingsUserControlModel : PropertyChangedBase
{
    public static PersonalizationSettingsUserControlModel Instance { get; } = new();

    private PersonalizationSettingsUserControlModel()
    {
    }

    public bool ShowOperatorIcons
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.ShowOperatorIcons = value;
        }
    } = ConfigFactory.Root.Gui.ShowOperatorIcons;

    public bool ShowMaterialIcons
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.ShowMaterialIcons = value;
        }
    } = ConfigFactory.Root.Gui.ShowMaterialIcons;
}
