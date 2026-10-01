// <copyright file="ArpsExtra.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Configuration.Single.Settings.ConnectionExtra;

public class ArpsExtra : BaseExtra
{
    public string Compression { get; set; } = "lz4_block";

    public int MaxFps { get; set; } = 30;

    public string CaptureMode { get; set; } = "auto";

    public bool PowerOnIfScreenOff { get; set; } = true;

    public bool TurnScreenOff { get; set; }

    public bool KeepScreenOn { get; set; } = true;

    public string ExitPowerMode { get; set; } = "restore_previous";
}
