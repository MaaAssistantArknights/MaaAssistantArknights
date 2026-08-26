// <copyright file="Update.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;
using PropertyChanged;
using static MaaWpfGui.ViewModels.UserControl.Settings.VersionUpdateSettingsUserControlModel;

namespace MaaWpfGui.Configuration.Global;

/// <summary>
/// 更新设置(全局)
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class Update
{
    public string Name { get; set; } = string.Empty;

    public string UpdatePackage { get; set; } = string.Empty;

    public bool IsFirstBoot { get; set; }

    public bool DoNotShowUpdate { get; set; }

    public UpdateVersionType VersionType { get; set; } = UpdateVersionType.Stable;

    public bool AllowNightlyUpdates { get; set; }

    public bool HasAcknowledgedNightlyWarning { get; set; }

    public UpdateSource UpdateSource { get; set; } = UpdateSource.GitHub;

    public bool ForceGithubGlobalSource { get; set; }

    public string MirrorChyanCdk { get; set; } = string.Empty;

    public long MirrorChyanCdkExpiredTime { get; set; }

    public bool CheckOnStartup { get; set; } = true;

    public bool CheckOnSchedule { get; set; }

    public string Proxy { get; set; } = string.Empty;

    public string ProxyType { get; set; } = "Http";

    public bool AutoDownloadUpdatePackage { get; set; } = true;

    public bool AutoInstallUpdatePackage { get; set; }

    public bool ShowUpdaterConsole { get; set; }

    public bool ShowUpdaterProgress { get; set; } = true;
}
