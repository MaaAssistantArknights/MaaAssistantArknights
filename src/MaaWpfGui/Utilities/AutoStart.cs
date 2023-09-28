// <copyright file="AutoStart.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Diagnostics;
using System.IO;

namespace MaaWpfGui.Utilities
{
    /// <summary>
    /// The model of auto-starting settings.
    /// </summary>
    public static class AutoStart
    {
        private static readonly string _fileValue = Process.GetCurrentProcess().MainModule?.FileName;
        private static readonly string _uniqueIdentifier = GetUniqueIdentifierFromPath(_fileValue);

        private static readonly string _startupFolderPath = Environment.GetFolderPath(Environment.SpecialFolder.Startup);
        private static readonly string _startupShortcutPath = Path.Combine(_startupFolderPath, $"MAA_{_uniqueIdentifier}.lnk");

        private static string GetUniqueIdentifierFromPath(string path)
        {
            int hash = path.GetHashCode();
            return hash.ToString("X");
        }

        /// <summary>
        /// Checks whether this program starts up with OS.
        /// </summary>
        /// <returns>The value.</returns>
        public static bool CheckStart()
        {
            try
            {
                return File.Exists(_startupShortcutPath);
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Sets whether this program starts up with OS.
        /// </summary>
        /// <param name="set">The new value.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool SetStart(bool set)
        {
            try
            {
                if (set)
                {
                    // 创建启动文件夹的快捷方式
                    var shell = new IWshRuntimeLibrary.WshShell();
                    var shortcut = shell.CreateShortcut(_startupShortcutPath);
                    shortcut.TargetPath = _fileValue;
                    shortcut.WorkingDirectory = Path.GetDirectoryName(_fileValue);
                    shortcut.Save();
                }
                else
                {
                    // 删除启动文件夹的快捷方式
                    if (File.Exists(_startupShortcutPath))
                    {
                        File.Delete(_startupShortcutPath);
                    }
                }

                return set == CheckStart();
            }
            catch
            {
                return false;
            }
        }
    }
}
