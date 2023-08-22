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
using Serilog;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// The emulator adapter.
    /// </summary>
    public class WinAdapter
    {
        private static readonly ILogger _logger = Log.ForContext<WinAdapter>();

        private static readonly Dictionary<string, string> _emulatorIdDict = new Dictionary<string, string>
        {
            { "HD-Player",  "BlueStacks" },
            { "dnplayer", "LDPlayer" },
            { "Nox", "Nox" },
            { "NemuPlayer", "MuMuEmulator" },
            { "MuMuPlayer", "MuMuEmulator12" }, // MuMu 12
            { "MEmu", "XYAZ" },
        };

        private static readonly Dictionary<string, List<string>> _adbRelativePathDict = new Dictionary<string, List<string>>
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
                    ".\\adb.exe",
                }
            },
            {
                "MuMuEmulator12",  new List<string>
                {
                    "..\\vmonitor\\bin\\adb_server.exe",
                    "..\\..\\MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe",
                    ".\\adb.exe",
                }
            },
            { "XYAZ",  new List<string> { ".\\adb.exe" } },
        };

        private readonly Dictionary<string, string> _adbAbsolutePathDict = new Dictionary<string, string>();

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
                if (_emulatorIdDict.Keys.Contains(process.ProcessName))
                {
                    var emulatorId = _emulatorIdDict[process.ProcessName];
                    emulators.Add(emulatorId);
                    var processPath = process.MainModule.FileName;
                    foreach (var path in _adbRelativePathDict[emulatorId])
                    {
                        var adbPath = Path.GetDirectoryName(processPath) + "\\" + path;
                        if (File.Exists(adbPath))
                        {
                            _adbAbsolutePathDict[emulatorId] = adbPath;
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
            if (_adbAbsolutePathDict.Keys.Contains(emulatorName))
            {
                return _adbAbsolutePathDict[emulatorName];
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
                _logger.Information(adbPath + " devices | output:\n" + output);
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
