// <copyright file="FakeUpdateHelper.cs" company="MaaAssistantArknights">
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

using System;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Main;

namespace MaaWpfGui.Helper;

public static class FakeUpdateHelper
{
    private sealed class FakeUpdateInfo
    {
        public string CurrentVersion { get; set; } = string.Empty;

        public string TargetVersion { get; set; } = string.Empty;

        public bool IsUpdated { get; set; }
    }

    private const string FileName = "FakeUpdate";

    private static readonly FakeUpdateInfo? s_updateInfo = JsonDataHelper.Get<FakeUpdateInfo>(FileName);

    public static bool IsEnabled =>
        s_updateInfo is { } &&
        !string.IsNullOrWhiteSpace(s_updateInfo.CurrentVersion) &&
        !string.IsNullOrWhiteSpace(s_updateInfo.TargetVersion);

    public static string CurrentVersion =>
        !IsEnabled
            ? string.Empty
            : s_updateInfo!.IsUpdated
                ? s_updateInfo.TargetVersion
                : s_updateInfo.CurrentVersion;

    public static string TargetVersion => IsEnabled ? s_updateInfo!.TargetVersion : string.Empty;

    public static bool HasPendingFakeUpdate =>
        IsEnabled &&
        !s_updateInfo!.IsUpdated &&
        !string.Equals(s_updateInfo.CurrentVersion, s_updateInfo.TargetVersion, StringComparison.OrdinalIgnoreCase);

    public static bool Updating()
    {
        if (!HasPendingFakeUpdate || s_updateInfo is null)
        {
            return false;
        }

        s_updateInfo.IsUpdated = true;
        ConfigFactory.Root.Update.Name = s_updateInfo.TargetVersion;
        ConfigFactory.Root.Update.IsFirstBoot = true;
        return SaveAndRestart();
    }

    private static bool SaveAndRestart()
    {
        if (!JsonDataHelper.Set(FileName, s_updateInfo))
        {
            return false;
        }

        Bootstrapper.ShutdownAndRestartWithoutArgs();
        return true;
    }
}
