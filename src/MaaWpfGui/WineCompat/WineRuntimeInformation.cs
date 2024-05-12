// <copyright file="WineRuntimeInformation.cs" company="MaaAssistantArknights">
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

#nullable enable

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MaaWpfGui.WineCompat
{
    internal class WineRuntimeInformation
    {
        [DllImport("ntdll.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr wine_get_version();

        [DllImport("ntdll.dll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
        private static extern void wine_get_host_version(out IntPtr sysname, out IntPtr release);

        public static bool IsRunningUnderWine { get; }

        public static string? WineVersion { get; }

        public static string? HostSystemName { get; }

        static WineRuntimeInformation()
        {
            try
            {
                var ptr = wine_get_version();
                if (ptr != IntPtr.Zero)
                {
                    IsRunningUnderWine = true;
                }

                WineVersion = Marshal.PtrToStringUTF8(ptr);
                wine_get_host_version(out var sysname, out var release);
                HostSystemName = Marshal.PtrToStringUTF8(sysname);
            }
            catch
            {
                IsRunningUnderWine = false;
            }
        }
    }
}
