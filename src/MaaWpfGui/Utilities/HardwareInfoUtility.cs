// <copyright file="HardwareInfoUtility.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Management;
using System.Security.Cryptography;
using System.Text;
using Microsoft.Win32;

namespace MaaWpfGui.Utilities
{
    public static class HardwareInfoUtility
    {
        public static string GetMachineGuid()
        {
            const string Key = @"SOFTWARE\Microsoft\Cryptography";
            using RegistryKey? rk = Registry.LocalMachine.OpenSubKey(Key);
            return rk?.GetValue("MachineGuid")?.ToString() ?? string.Empty;
        }

        private static string GetCpuInfo()
        {
            using var mc = new ManagementClass("Win32_Processor");
            foreach (var mo in mc.GetInstances())
            {
                return mo["ProcessorId"]?.ToString() ?? string.Empty;
            }

            return string.Empty;
        }

        private static string GetDiskSerialNumber()
        {
            using var mc = new ManagementClass("Win32_DiskDrive");
            foreach (var mo in mc.GetInstances())
            {
                return mo["SerialNumber"]?.ToString()?.Trim() ?? string.Empty;
            }

            return string.Empty;
        }
    }
}
