// <copyright file="ModAvdConnection.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;

namespace MaaWpfGui.Helper;

/// <summary>
/// Finds a running ModAVD instance from its emulator process.
/// ModAVD registers its instances only with its own adb server, never the default one on port 5037,
/// so they do not show up in `adb devices`; the adb port is read from the process command line instead.
/// </summary>
internal static class ModAvdConnection
{
    // The emulator process: <root>\sdk\emulator\qemu\windows-x86_64\qemu-system-x86_64[-headless].exe
    private const int RootLevelsAboveProcessDirectory = 4;

    private const string RootMarkerFile = "mas-avd.json";

    private static readonly string _adbRelativePath = Path.Combine("sdk", "platform-tools", "adb.exe");

    // "-ports <console>,<adb>"; the arguments may be quoted.
    private static readonly Regex _portsRegex = new(
        "(?:^|[\\s\"])-ports\"?\\s+\"?(?<console>[0-9]+),(?<adb>[0-9]+)(?=$|[\\s\"])",
        RegexOptions.CultureInvariant,
        TimeSpan.FromSeconds(1));

    public static bool IsEmulatorProcessName(string processName) =>
        processName.Equals("qemu-system-x86_64", StringComparison.OrdinalIgnoreCase) ||
        processName.Equals("qemu-system-x86_64-headless", StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Gets the ModAVD root directory an emulator executable belongs to.
    /// </summary>
    /// <param name="processPath">The emulator executable path.</param>
    /// <returns>The root directory, or null when the emulator is not part of a ModAVD root (e.g. an Android Studio AVD).</returns>
    public static string? GetRoot(string? processPath)
    {
        if (string.IsNullOrEmpty(processPath))
        {
            return null;
        }

        var directory = Directory.GetParent(processPath);
        for (var i = 0; i < RootLevelsAboveProcessDirectory && directory != null; i++)
        {
            directory = directory.Parent;
        }

        return directory != null && File.Exists(Path.Combine(directory.FullName, RootMarkerFile)) ? directory.FullName : null;
    }

    public static string? GetAdbPath(string root)
    {
        var adbPath = Path.Combine(root, _adbRelativePath);
        return File.Exists(adbPath) ? adbPath : null;
    }

    /// <summary>
    /// Gets the adb address of the instance from the emulator command line.
    /// </summary>
    /// <param name="commandLine">The emulator process command line.</param>
    /// <returns>127.0.0.1:&lt;adb port&gt;, or null when the command line has no valid -ports.</returns>
    public static string? GetAddress(string? commandLine)
    {
        if (string.IsNullOrEmpty(commandLine))
        {
            return null;
        }

        try
        {
            var match = _portsRegex.Match(commandLine);
            if (!match.Success ||
                !int.TryParse(match.Groups["adb"].Value, NumberStyles.None, CultureInfo.InvariantCulture, out var port) ||
                port is <= 0 or > 65535)
            {
                return null;
            }

            return $"127.0.0.1:{port.ToString(CultureInfo.InvariantCulture)}";
        }
        catch (RegexMatchTimeoutException)
        {
            return null;
        }
    }
}
