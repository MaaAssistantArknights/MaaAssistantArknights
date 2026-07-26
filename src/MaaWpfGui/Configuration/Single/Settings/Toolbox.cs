// <copyright file="Toolbox.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;
using static MaaWpfGui.ViewModels.UI.ToolboxViewModel;

namespace MaaWpfGui.Configuration.Single.Settings;

public class Toolbox : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
    }

    public OperBoxExportFormat OperBoxExportFormat { get; set; } = OperBoxExportFormat.Clipboard;

    public bool GachaShowDisclaimerNoMore { get; set; }

    public int PeepTargetFps { get; set; } = 20;

    public bool ChooseLevel3 { get; set; } = true;

    public int ChooseLevel3Time { get; set; } = 540;

    public bool ChooseLevel4 { get; set; } = true;

    public int ChooseLevel4Time { get; set; } = 540;

    public bool ChooseLevel5 { get; set; } = true;

    public bool ChooseLevel6 { get; set; } = true;

    public bool AutoSetTime { get; set; } = true;

    public bool ShowPotential { get; set; } = true;
}
