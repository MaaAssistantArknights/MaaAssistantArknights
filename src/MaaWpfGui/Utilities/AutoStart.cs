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
using System.Runtime.InteropServices.ComTypes;
using Microsoft.Win32;

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

        static string GetUniqueIdentifierFromPath(string path)
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
            // 弃用注册表检查，迁移到启动文件夹
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", true);
                if (key?.GetValue("MeoAsst") != null)
                {
                    key.DeleteValue("MeoAsst");
                    SetStart(true);
                    return true;
                }
            }
            catch
            {
                // ignored
            }

            return File.Exists(_startupShortcutPath);

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
                    // ReSharper disable once SuspiciousTypeConversion.Global
                    var shellLink = (IShellLink)new ShellLink();
                    shellLink.SetPath(_fileValue);
                    shellLink.SetWorkingDirectory(Path.GetDirectoryName(_fileValue));
                    shellLink.Resolve(IntPtr.Zero, 1);

                    // ReSharper disable once SuspiciousTypeConversion.Global
                    var file = (IPersistFile)shellLink;
                    file.Save(_startupShortcutPath, false);
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
