// <copyright file="ThirdPartyServiceSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class ThirdPartyServiceSettingsUserControlModel : PropertyChangedBase
{
    static ThirdPartyServiceSettingsUserControlModel()
    {
        Instance = new();
    }

    public static ThirdPartyServiceSettingsUserControlModel Instance { get; }

    #region 企鹅和一图流上报

    /// <summary>
    /// Gets or sets the id of PenguinStats.
    /// </summary>
    public string PenguinId
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.PenguinId = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.PenguinId;

    /// <summary>
    /// Gets or sets a value indicating whether to enable penguin upload.
    /// </summary>
    public bool EnablePenguin
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToPenguin = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToPenguin;

    /// <summary>
    /// Gets or sets a value indicating whether to enable yituliu upload.
    /// </summary>
    public bool EnableYituliu
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToYituliu = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToYituliu;

    #endregion 企鹅和一图流上报
}
