// <copyright file="StartSelfModel.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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

namespace MeoAsstGui
{
    /// <summary>
    /// The model of auto-starting settings.
    /// </summary>
    public class StartSelfModel
    {
        private static readonly RegistryKey _key = Registry.CurrentUser.OpenSubKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run", true);
        private static readonly string fileValue = Process.GetCurrentProcess().MainModule?.FileName;

        /// <summary>
        /// Checks whether this program starts up with OS.
        /// </summary>
        /// <returns>The value.</returns>
        public static bool CheckStart()
        {
            if (_key.GetValue("MeoAsst") == null)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        /// <summary>
        /// Sets whether this program starts up with OS.
        /// </summary>
        /// <param name="set">The new value.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool SetStart(bool set)
        {
            if (set)
            {
                _key.SetValue("MeoAsst", "\"" + fileValue + "\"");
                return CheckStart();
            }
            else
            {
                _key.DeleteValue("MeoAsst");
                return !CheckStart();
            }
        }
    }
}
