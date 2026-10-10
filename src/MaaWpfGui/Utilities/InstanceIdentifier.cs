// <copyright file="InstanceIdentifier.cs" company="MaaAssistantArknights">
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

using System;

namespace MaaWpfGui.Utilities;

/// <summary>
/// Provides the per-deployment identifier derived from the process path, shared by OS-level
/// registrations (registry auto-start key, scheduled wake-up task names) so that multiple
/// MAA deployments never overwrite each other's entries.
/// 提供基于进程路径的实例标识，注册表自启键与定时唤醒计划任务名共用同源标识区分多实例。
/// </summary>
public static class InstanceIdentifier
{
    /// <summary>
    /// Computes an 8-digit uppercase hex identifier for the input string.
    /// </summary>
    /// <param name="input">The input string, typically the process path.</param>
    /// <returns>The 8-digit uppercase hex identifier.</returns>
    public static string GetHash(string input)
    {
        int hash1 = (5381 << 16) + 5381;
        int hash2 = hash1;

        for (int i = 0; i < input.Length; i += 2)
        {
            hash1 = ((hash1 << 5) + hash1) ^ input[i];
            if (i == input.Length - 1)
            {
                break;
            }

            hash2 = ((hash2 << 5) + hash2) ^ input[i + 1];
        }

        return (hash1 + (hash2 * 1566083941)).ToString("X");
    }
}
