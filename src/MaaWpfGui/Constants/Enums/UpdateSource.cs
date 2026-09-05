// <copyright file="UpdateSource.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums;

/// <summary>
/// 更新源
/// </summary>
public enum UpdateSource
{
    /// <summary>
    /// GitHub（海外源）
    /// </summary>
    GitHub,

    /// <summary>
    /// GitHub 镜像（使用自定义镜像站前缀加速 GitHub 下载）
    /// </summary>
    GitHubMirror,

    /// <summary>
    /// Mirror酱
    /// </summary>
    MirrorChyan,
}
