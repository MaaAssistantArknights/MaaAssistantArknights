// <copyright file="ExtraConfig.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using Stylet;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

[JsonDerivedType(typeof(MuMu12Extra), typeDiscriminator: nameof(MuMu12Extra))]
[JsonDerivedType(typeof(LDPlayerExtra), typeDiscriminator: nameof(LDPlayerExtra))]
[JsonDerivedType(typeof(Win32Extra), typeDiscriminator: nameof(Win32Extra))]
public class ExtraConfig : PropertyChangedBase
{
}
