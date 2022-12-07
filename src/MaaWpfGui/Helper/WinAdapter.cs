// <copyright file="WinAdapter.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace MaaWpfGui
{
    /// <summary>
    /// The emulator adapter.
    /// </summary>
    public class WinAdapter
    {
        private static readonly Dictionary<string, string> emulatorIdDict = new Dictionary<string, string>
        {
            { "HD-Player",  "BlueStacks" },
            { "dnplayer", "LDPlayer" },
            { "Nox", "Nox" },
            { "NemuPlayer", "MuMuEmulator" },
            { "MEmu", "XYAZ" },
        };

        private static readonly Dictionary<string, List<string>> adbRelativePathDict = new Dictionary<string, List<string>>
        {
            {
                "BlueStacks", new List<string>
                {
                    ".\\HD-Adb.exe",
                    ".\\Engine\\ProgramFiles\\HD-Adb.exe",
                }
            },
            { "LDPlayer",  new List<string> { ".\\adb.exe" } },
            { "Nox",  new List<string> { ".\\nox_adb.exe" } },
            {
                "MuMuEmulator",  new List<string>
                {
                    "..\\vmonitor\\bin\\adb_server.exe",
                    "..\\..\\MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe",
                }
            },
            { "XYAZ",  new List<string> { ".\\adb.exe" } },
        };

        private readonly Dictionary<string, string> adbAbsolutePathDict = new Dictionary<string, string>();

        /// <summary>
        /// Refreshes emulator information.
        /// </summary>
        /// <returns>The list of emulators.</returns>
        public List<string> RefreshEmulatorsInfo()
        {
            var allProcess = Process.GetProcesses();
            var emulators = new List<string>();
            foreach (var process in allProcess)
            {
                if (emulatorIdDict.Keys.Contains(process.ProcessName))
                {
                    var emulatorId = emulatorIdDict[process.ProcessName];
                    emulators.Add(emulatorId);
                    var processPath = process.MainModule.FileName;
                    foreach (var path in adbRelativePathDict[emulatorId])
                    {
                        var adbPath = Path.GetDirectoryName(processPath) + "\\" + path;
                        if (File.Exists(adbPath))
                        {
                            adbAbsolutePathDict.Add(emulatorId, adbPath);
                        }
                    }
                }
            }

            return emulators;
        }

        /// <summary>
        /// Gets ADB path by emulator name.
        /// </summary>
        /// <param name="emulatorName">The name of the emulator.</param>
        /// <returns>The ADB path of the emulator.</returns>
        public string GetAdbPathByEmulatorName(string emulatorName)
        {
            if (adbAbsolutePathDict.Keys.Contains(emulatorName))
            {
                return adbAbsolutePathDict[emulatorName];
            }

            return null;
        }

        /// <summary>
        /// Gets ADB addresses by an ADB path.
        /// </summary>
        /// <param name="adbPath">The ADB path.</param>
        /// <returns>The list of ADB addresses.</returns>
        public List<string> GetAdbAddresses(string adbPath)
        {
            var addresses = new List<string>();
            using (Process process = new Process())
            {
                process.StartInfo.FileName = adbPath;
                process.StartInfo.Arguments = "devices";

                // 禁用操作系统外壳程序
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.RedirectStandardOutput = true;
                process.Start();
                var output = process.StandardOutput.ReadToEnd();
                AsstProxy.AsstLog(adbPath + " devices | output:\n" + output);
                var outLines = output.Split(new[] { '\r', '\n' });
                foreach (var line in outLines)
                {
                    if (line.StartsWith("List of devices attached") ||
                        line.Length == 0 ||
                        !line.Contains("device"))
                    {
                        continue;
                    }

                    var address = line.Split('\t')[0];
                    addresses.Add(address);
                }
            }

            return addresses;
        }
    }
}
