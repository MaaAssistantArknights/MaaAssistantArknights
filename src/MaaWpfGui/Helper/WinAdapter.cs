// <copyright file="WinAdapter.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// The emulator adapter.
/// </summary>
public class WinAdapter
{
    private static readonly ILogger _logger = Log.ForContext<WinAdapter>();

    public sealed class DetectedEmulatorInfo(string emulatorName, string? adbPath)
    {
        public string EmulatorName { get; } = emulatorName;

        public string? AdbPath { get; } = adbPath;

        public string SelectionDisplayText => string.IsNullOrEmpty(AdbPath)
            ? EmulatorName
            : $"{EmulatorName} ({AdbPath})";
    }

    private static readonly Dictionary<string, string> _emulatorIdDict = new()
    {
        { "HD-Player", "BlueStacks" },
        { "dnplayer", "LDPlayer" },
        { "Nox", "Nox" },
        { "MuMuPlayer", "MuMuEmulator12" }, // MuMu 12
        { "MuMuNxDevice", "MuMuEmulator12" }, // MuMu 12 5.0
        { "MEmu", "XYAZ" },
    };

    private static readonly Dictionary<string, List<string>> _adbRelativePathDict = new()
    {
        {
            "BlueStacks", [
                @".\HD-Adb.exe",
                @".\Engine\ProgramFiles\HD-Adb.exe"
            ]
        },
        { "LDPlayer", [@".\adb.exe"] },
        { "Nox", [@".\nox_adb.exe"] },
        {
            "MuMuEmulator12", [
                @"..\..\..\nx_main\adb.exe",

                @"..\vmonitor\bin\adb_server.exe",
                @"..\..\MuMu\emulator\nemu\vmonitor\bin\adb_server.exe",
                @".\adb.exe"
            ]
        },
        { "XYAZ", [@".\adb.exe"] },
    };

    /// <summary>
    /// Refreshes emulator information.
    /// </summary>
    /// <returns>The detected emulator infos.</returns>
    public List<DetectedEmulatorInfo> RefreshEmulatorsInfo()
    {
        var allProcess = Process.GetProcesses();
        var emulators = new List<DetectedEmulatorInfo>();
        var detectedEmulators = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var process in allProcess)
        {
            if (!_emulatorIdDict.TryGetValue(process.ProcessName, out var emulatorId))
            {
                continue;
            }

            var adbPath = GetAdbPathByProcessPath(process.MainModule?.FileName, emulatorId);
            var detectionKey = $"{emulatorId}\n{adbPath}";
            if (detectedEmulators.Add(detectionKey))
            {
                emulators.Add(new DetectedEmulatorInfo(emulatorId, adbPath));
            }
        }

        return emulators;
    }

    /// <summary>
    /// Gets the preferred ADB path for a process.
    /// </summary>
    /// <param name="processPath">The emulator executable path.</param>
    /// <param name="emulatorName">The name of the emulator.</param>
    /// <returns>The ADB path of the emulator.</returns>
    private static string? GetAdbPathByProcessPath(string? processPath, string emulatorName)
    {
        var processDirectory = Path.GetDirectoryName(processPath);
        if (string.IsNullOrEmpty(processDirectory))
        {
            return null;
        }

        return _adbRelativePathDict[emulatorName]
            .Select(path => Path.GetFullPath(Path.Combine(processDirectory, path)))
            .FirstOrDefault(File.Exists);
    }

    /// <summary>
    /// Gets ADB addresses by an ADB path.
    /// </summary>
    /// <param name="adbPath">The ADB path.</param>
    /// <returns>The list of ADB addresses.</returns>
    public static List<string> GetAdbAddresses(string adbPath)
    {
        string output = ExecuteAdbCommand(adbPath, "devices");
        _logger.Information(output);
        var lines = output.Split('\r', '\n');
        return [.. lines
            .Where(line => !line.StartsWith("List of devices attached") &&
                           !string.IsNullOrWhiteSpace(line) &&
                           line.Contains("device"))
            .Select(line => line.Split('\t')[0])];
    }

    /// <summary>
    /// Executes an ADB command.
    /// </summary>
    /// <param name="adbPath">adb path</param>
    /// <param name="command">adb command</param>
    /// <returns>output</returns>
    public static string ExecuteAdbCommand(string adbPath, string command)
    {
        try
        {
            var process = new Process
            {
                StartInfo = new()
                {
                    FileName = adbPath,
                    Arguments = command,
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                },
            };

            process.Start();
            string output = process.StandardOutput.ReadToEnd();
            process.WaitForExit();
            return output;
        }
        catch (Exception e)
        {
            _logger.Error(e.Message);
            return string.Empty;
        }
    }
}
