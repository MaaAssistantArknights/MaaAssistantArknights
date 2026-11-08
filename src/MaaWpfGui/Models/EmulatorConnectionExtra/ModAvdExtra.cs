// <copyright file="ModAvdExtra.cs" company="MaaAssistantArknights">
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
using System.IO;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

/// <summary>
/// ModAVD 截图增强：MaaCore 从 ModAVD 根目录下的 extras 载入 modavd_extras，实例由连接地址推出，不需要实例编号
/// </summary>
public class ModAvdExtra() : ExtraConfig
{
    /// <summary>
    /// Gets or sets the ModAVD root directory. Empty means screenshots go through ADB only.
    /// </summary>
    public string RootPath
    {
        get; set {
            if (!string.IsNullOrEmpty(value) && !File.Exists(Path.Combine(value, "extras", "modavd_extras.dll")))
            {
                MessageBoxHelper.Show(LocalizationHelper.GetString("Connection.ModAVD.LibraryMissing"));
                return;
            }

            Instances.AsstProxy.Connected = false;
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.ModAvd.RootPath = value;
            SetAndNotify(ref field, value);
        }
    } = Directory.Exists(ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.ModAvd.RootPath) ? ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.ModAvd.RootPath : string.Empty;

    /// <summary>
    /// Selects the ModAVD root directory.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectRootPath()
    {
        var dialog = new OpenFolderDialog();
        if (dialog.ShowDialog() == true)
        {
            RootPath = dialog.FolderName;
        }
    }

    public string Config =>
        Newtonsoft.Json.JsonConvert.SerializeObject(string.IsNullOrEmpty(RootPath) ? new JObject() : new JObject { ["root"] = RootPath });
}
