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

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// Helper class for managing commonly used application paths.
    /// </summary>
    public static class PathsHelper
    {
        private static string? _workingDirectory;
        private static string? _resourceDirectory;
        private static string? _sourceResourceDirectory;

        /// <summary>
        /// Gets the application's working directory (AppDomain.CurrentDomain.BaseDirectory).
        /// </summary>
        public static string WorkingDirectory
            => _workingDirectory ??= AppDomain.CurrentDomain.BaseDirectory;

        /// <summary>
        /// Gets the resource directory under the working directory.
        /// </summary>
        public static string ResourceDirectory
            => _resourceDirectory ??= Path.Combine(WorkingDirectory, "resource");

        /// <summary>
        /// Gets the resource directory under the third-level parent of the working directory (for source code soft link).
        /// </summary>
        public static string SourceResourceDirectory
        {
            get
            {
                if (_sourceResourceDirectory != null)
                {
                    return _sourceResourceDirectory;
                }

                var dir = new DirectoryInfo(WorkingDirectory);
                for (int i = 0; i < 3 && dir.Parent != null; i++)
                {
                    dir = dir.Parent;
                }

                return _sourceResourceDirectory = Path.Combine(dir.FullName, "resource");
            }
        }

        /// <summary>
        /// Gets the full path to the application's cache directory.
        /// </summary>
        public static string CacheDirectory
            => Path.Combine(WorkingDirectory, "cache");
    }
}
