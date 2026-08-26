// <copyright file="BaseExtra.cs" company="MaaAssistantArknights">
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

using System.Text.Json.Serialization;
using MaaWpfGui.Models;
using MaaWpfGui.Models.EmulatorConnectionExtra;

namespace MaaWpfGui.Configuration.Single.Settings.ConnectionExtra;

[JsonDerivedType(typeof(Mumu12Extra), typeDiscriminator: nameof(Mumu12Extra))]
[JsonDerivedType(typeof(LdPlayerExtra), typeDiscriminator: nameof(LdPlayerExtra))]
[JsonDerivedType(typeof(Win32Extra), typeDiscriminator: nameof(Win32Extra))]
public class BaseExtra : NotifyPropertyChangedWithValue
{
}
