// <copyright file="PathsHelper.cs" company="MaaAssistantArknights">
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
using System.IO;

namespace MaaWpfGui.Helper;

/// <summary>
/// Helper class for managing commonly used application paths.
/// </summary>
public static class PathsHelper
{
    private static string? _base;

    /// <summary>
    /// Gets the application's working directory (AppDomain.CurrentDomain.BaseDirectory).
    /// </summary>
    public static string BaseDir => _base ??= AppDomain.CurrentDomain.BaseDirectory;

    private static string? _resource;

    /// <summary>
    /// Gets the resource directory under the working directory.
    /// </summary>
    public static string ResourceDir => _resource ??= Path.Combine(BaseDir, "resource");

    /// <summary>
    /// Gets the resource directory under the third-level parent of the working directory (for source code soft link).
    /// </summary>
    public static string SourceResourceDir
    {
        get
        {
            var dir = new DirectoryInfo(BaseDir);
            for (int i = 0; i < 3 && dir.Parent != null; i++)
            {
                dir = dir.Parent;
            }

            return Path.Combine(dir.FullName, "resource");
        }
    }

    private static string? _cache;

    /// <summary>
    /// Gets the full path to the application's cache directory.
    /// </summary>
    public static string CacheDir => _cache ??= Path.Combine(BaseDir, "cache");

    private static string? _config;

    /// <summary>
    /// Gets the full path to the directory containing GUI configuration files for the application.
    /// </summary>
    public static string ConfigDir => _config ??= Path.Combine(BaseDir, "config");

    private static string? _debug;

    /// <summary>
    /// Gets the full path to the directory used for storing debug files.
    /// </summary>
    /// <remarks>The debug directory is located within the application's base directory and is named
    /// "debug". The directory may not exist until created by the application or related processes.</remarks>
    public static string DebugDir => _debug ??= Path.Combine(BaseDir, "debug");

    private static string? _data;

    /// <summary>
    /// Gets the full path to the application's data directory.
    /// </summary>
    /// <remarks>The data directory is located within the application's base directory under a folder
    /// named "data". This property returns the resolved path, creating the directory if it does not exist is the
    /// responsibility of the caller.</remarks>
    public static string DataDir => _data ??= Path.Combine(BaseDir, "data");
}
