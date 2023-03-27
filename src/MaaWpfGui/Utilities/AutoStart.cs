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

using System.Diagnostics;
using Microsoft.Win32;

namespace MaaWpfGui.Utilities
{
    /// <summary>
    /// The model of auto-starting settings.
    /// </summary>
    public static class AutoStart
    {
        private static readonly string fileValue = Process.GetCurrentProcess().MainModule?.FileName;

        /// <summary>
        /// Checks whether this program starts up with OS.
        /// </summary>
        /// <returns>The value.</returns>
        public static bool CheckStart()
        {
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run", false);
                return key.GetValue("MeoAsst") != null;
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
                using var key = Registry.CurrentUser.OpenSubKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run", true);
                if (set)
                {
                    key.SetValue("MeoAsst", "\"" + fileValue + "\"");
                }
                else
                {
                    key.DeleteValue("MeoAsst");
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
