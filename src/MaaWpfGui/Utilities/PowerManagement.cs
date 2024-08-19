// <copyright file="PowerManagement.cs" company="MaaAssistantArknights">
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

using System;
using System.Runtime.InteropServices;
using Serilog;

namespace MaaWpfGui.Utilities
{
    public class PowerManagement
    {
        private static readonly ILogger _logger = Log.ForContext<PowerManagement>();

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool ExitWindowsEx(uint uFlags, uint dwReason);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool AdjustTokenPrivileges(IntPtr TokenHandle, bool DisableAllPrivileges, ref TOKEN_PRIVILEGES NewState, uint BufferLengthInBytes, IntPtr PreviousState, IntPtr ReturnLengthInBytes);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetCurrentProcess();

        [DllImport("advapi32.dll", CharSet = CharSet.Auto)]
        private static extern bool LookupPrivilegeValue(string lpSystemName, string lpName, out LUID lpLuid);

        [DllImport("powrprof.dll", SetLastError = true, ExactSpelling = true)]
        private static extern bool SetSuspendState(bool hibernate, bool forceCritical, bool disableWakeEvent);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct TOKEN_PRIVILEGES
        {
            public int PrivilegeCount;
            public LUID Luid;
            public int Attributes;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct LUID
        {
            public uint LowPart;
            public int HighPart;
        }

        private const int SE_PRIVILEGE_ENABLED = 0x00000002;
        private const string SE_SHUTDOWN_NAME = "SeShutdownPrivilege";
        private const int TOKEN_ADJUST_PRIVILEGES = 0x0020;
        private const int TOKEN_QUERY = 0x0008;
        private const int EWX_SHUTDOWN = 0x00000001;
        private const int EWX_FORCE = 0x00000004;

        public static bool Shutdown()
        {
            IntPtr hToken = IntPtr.Zero;
            TOKEN_PRIVILEGES tkp;

            try
            {
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, out hToken))
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                }

                if (!LookupPrivilegeValue(null, SE_SHUTDOWN_NAME, out tkp.Luid))
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                }

                tkp.PrivilegeCount = 1;
                tkp.Attributes = SE_PRIVILEGE_ENABLED;

                if (!AdjustTokenPrivileges(hToken, false, ref tkp, 0, IntPtr.Zero, IntPtr.Zero))
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                }

                if (Marshal.GetLastWin32Error() != 0)
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                }

                if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0))
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                }

                return true;
            }
            catch (Exception ex)
            {
                _logger.Error($"Shutdown error: {ex.Message}");
                return false;
            }
            finally
            {
                if (hToken != IntPtr.Zero)
                {
                    CloseHandle(hToken);
                }
            }
        }

        public static bool Hibernate()
        {
            try
            {
                return SetSuspendState(true, true, true);
            }
            catch (Exception ex)
            {
                _logger.Error($"Hibernate error: {ex.Message}");
                return false;
            }
        }

        public static bool Sleep()
        {
            try
            {
                return SetSuspendState(false, true, true);
            }
            catch (Exception ex)
            {
                _logger.Error($"Sleep error: {ex.Message}");
                return false;
            }
        }
    }
}
