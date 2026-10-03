// <copyright file="InferenceBackend.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums;

public enum InferenceBackend
{
    /// <summary>
    /// 自动选择最成熟稳定的后端（推荐）。
    /// </summary>
    Auto = 0,

    /// <summary>
    /// Microsoft DirectML 后端。
    /// </summary>
    DirectML = 1,

    /// <summary>
    /// Google Dawn WebGPU 后端（实验性）。
    /// </summary>
    WebGPU = 2,
}
