// <copyright file="Copilot.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.Items;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;
using static MaaWpfGui.Models.AsstTasks.AsstCopilotTask;
using static MaaWpfGui.ViewModels.UI.CopilotViewModel;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 自动战斗设置
/// </summary>
public partial class Copilot : NotifyPropertyChangedWithValue
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
    }

    public List<CopilotItemViewModel> TaskList { get; set; } = [];

    public bool EnableUserAdditional { get; set; } = false;

    public List<UserAdditional> UserAdditional { get; set; } = [];

    public CopilotSupportMode SupportMode { get; set; } = CopilotSupportMode.WhenNeeded;

    public int SelectFormation { get; set; } = 1;

    public int LoopTimes { get; set; } = 1;
}
