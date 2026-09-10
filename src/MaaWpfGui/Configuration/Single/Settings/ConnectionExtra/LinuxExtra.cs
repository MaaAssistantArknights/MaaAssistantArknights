// <copyright file="LinuxExtra.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums.Core;

namespace MaaWpfGui.Configuration.Single.Settings.ConnectionExtra;

public class LinuxExtra : BaseExtra
{
    public AsstLinuxScreencapMethod ScreencapMethod { get; set; } = AsstLinuxScreencapMethod.Wlr;

    public AsstLinuxInputMethod InputMethod { get; set; } = AsstLinuxInputMethod.Wlr;

    public string WlrSocketPath { get; set; } = string.Empty;

    public int PipeWireSocketFd { get; set; }

    public uint PipeWireNodeId { get; set; }

    public int UInputScreenWidth { get; set; }

    public int UInputScreenHeight { get; set; }

    public string UInputPath { get; set; } = string.Empty;

    public string EisSocketPath { get; set; } = string.Empty;
}
