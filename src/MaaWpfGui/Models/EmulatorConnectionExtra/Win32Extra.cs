// <copyright file="Win32Extra.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using JetBrains.Annotations;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Serilog;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class Win32Extra() : ExtraConfig
{
    private static readonly ILogger _logger = Log.ForContext<Win32Extra>();

    private const string GameExecutableName = "Arknights.exe";
    private const string LauncherDirectoryName = "Hypergryph Launcher";
    private const string LauncherGameRelativePath = @"games\Arknights\Arknights.exe";
    private const string UninstallRegistryPath = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall";

    #region Enums
#pragma warning disable SA1602 // Enumeration items should be documented
    // 遵循 AsstCaller.h 中的定义，确保与 AsstCaller.h 中的枚举值对应
    public enum AsstWin32ScreencapMethod
    {
        FramePool = 2,
        PrintWindow = 16,
        ScreenDC = 32,
        DesktopDupWindow = 8,
    }

    public enum AsstWin32InputMethod
    {
        Seize = 1,
        SendMessageWithCursorPos = 32,
        SendMessageWithWindowPos = 128,
    }

    public enum AsstWin32KeyboardInputMethod
    {
        Seize = 1,
        SendMessage = 2,
        PostMessage = 4,
    }
#pragma warning restore SA1602 // Enumeration items should be documented
    #endregion Enums

    public Win32Extra(AsstWin32ScreencapMethod screencapMethod, AsstWin32InputMethod inputMethod, AsstWin32KeyboardInputMethod keyboardInputMethod)
        : this()
    {
        _screencapMethod = screencapMethod;
        _mouseMethod = inputMethod;
        _KeyboardMethod = keyboardInputMethod;
    }

    [JsonInclude]
    [JsonPropertyName("GamePath")]
    private string _gamePath = string.Empty;

    /// <summary>
    /// Gets or sets the fallback path of the PC game executable.
    /// </summary>
    [JsonIgnore]
    public string GamePath
    {
        get => _gamePath;
        set => SetAndNotify(ref _gamePath, (value ?? string.Empty).Trim().Trim('"'));
    }

    /// <summary>
    /// Gets a value indicating whether the selected capture and input methods support minimized operation.
    /// </summary>
    [JsonIgnore]
    public bool ShouldMinimizeWindow =>
        MouseMethod == AsstWin32InputMethod.SendMessageWithWindowPos &&
        ScreencapMethod is AsstWin32ScreencapMethod.FramePool or AsstWin32ScreencapMethod.PrintWindow;

    /// <summary>
    /// Detects the PC client path and fills the path setting when successful.
    /// </summary>
    /// <returns>Whether a valid executable was detected.</returns>
    public bool AutoDetectGamePath()
    {
        var path = DetectGameExecutablePath();
        if (string.IsNullOrEmpty(path))
        {
            return false;
        }

        GamePath = path;
        return true;
    }

    /// <summary>
    /// Resolves the executable path, preferring automatic detection over the configured fallback.
    /// </summary>
    /// <returns>A valid executable path, or an empty string.</returns>
    public string ResolveGameExecutablePath()
    {
        var detectedPath = DetectGameExecutablePath();
        if (!string.IsNullOrEmpty(detectedPath))
        {
            _logger.Information("Detected Arknights PC client: {Path}", detectedPath);
            return detectedPath;
        }

        if (File.Exists(GamePath))
        {
            _logger.Information("Using configured Arknights PC client path: {Path}", GamePath);
            return Path.GetFullPath(GamePath);
        }

        return string.Empty;
    }

    /// <summary>
    /// Selects the PC game executable.
    /// </summary>
    [UsedImplicitly]
    public void SelectGameExecutable()
    {
        var dialog = new OpenFileDialog {
            CheckFileExists = true,
            FileName = GameExecutableName,
            Filter = $"{GameExecutableName}|{GameExecutableName}|{LocalizationHelper.GetString("Executable")}|*.exe",
        };

        if (File.Exists(GamePath))
        {
            dialog.InitialDirectory = Path.GetDirectoryName(GamePath);
        }

        if (dialog.ShowDialog() == true)
        {
            GamePath = dialog.FileName;
        }
    }

    private static string? DetectGameExecutablePath()
    {
        var processes = Process.GetProcessesByName(Path.GetFileNameWithoutExtension(GameExecutableName));
        try
        {
            foreach (var process in processes)
            {
                try
                {
                    var runningPath = process.MainModule?.FileName;
                    if (IsGameExecutable(runningPath))
                    {
                        return Path.GetFullPath(runningPath!);
                    }
                }
                catch (Exception e)
                {
                    _logger.Debug("Unable to read Arknights process path: {Message}", e.Message);
                }
            }
        }
        finally
        {
            foreach (var process in processes)
            {
                process.Dispose();
            }
        }

        var candidates = new List<string>();
        AddRegistryCandidates(candidates);

        AddProgramFilesCandidate(candidates, Environment.SpecialFolder.ProgramFiles);
        AddProgramFilesCandidate(candidates, Environment.SpecialFolder.ProgramFilesX86);

        return candidates.FirstOrDefault(File.Exists);
    }

    private static void AddRegistryCandidates(List<string> candidates)
    {
        foreach (var hive in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
        {
            foreach (var view in new[] { RegistryView.Registry64, RegistryView.Registry32 })
            {
                try
                {
                    using var baseKey = RegistryKey.OpenBaseKey(hive, view);
                    using var uninstallKey = baseKey.OpenSubKey(UninstallRegistryPath);
                    if (uninstallKey is null)
                    {
                        continue;
                    }

                    foreach (var subKeyName in uninstallKey.GetSubKeyNames())
                    {
                        using var appKey = uninstallKey.OpenSubKey(subKeyName);
                        if (appKey is null || !IsHypergryphLauncherEntry(appKey))
                        {
                            continue;
                        }

                        AddLauncherDirectoryCandidate(candidates, appKey.GetValue("InstallLocation") as string);
                        AddLauncherFileCandidate(candidates, appKey.GetValue("DisplayIcon") as string);
                        AddLauncherFileCandidate(candidates, appKey.GetValue("UninstallString") as string);
                    }
                }
                catch (Exception e)
                {
                    _logger.Debug("Unable to inspect {Hive} {View} uninstall registry: {Message}", hive, view, e.Message);
                }
            }
        }
    }

    private static bool IsHypergryphLauncherEntry(RegistryKey appKey)
    {
        var displayName = appKey.GetValue("DisplayName") as string ?? string.Empty;
        var installLocation = appKey.GetValue("InstallLocation") as string ?? string.Empty;
        var displayIcon = appKey.GetValue("DisplayIcon") as string ?? string.Empty;

        return displayName.Contains("鹰角启动器", StringComparison.OrdinalIgnoreCase) ||
               displayName.Contains("Hypergryph Launcher", StringComparison.OrdinalIgnoreCase) ||
               installLocation.Contains("Hypergryph Launcher", StringComparison.OrdinalIgnoreCase) ||
               displayIcon.Contains("Hypergryph Launcher", StringComparison.OrdinalIgnoreCase);
    }

    private static void AddLauncherFileCandidate(List<string> candidates, string? fileValue)
    {
        if (string.IsNullOrWhiteSpace(fileValue))
        {
            return;
        }

        var path = fileValue.Trim();
        if (path.StartsWith('"'))
        {
            var quote = path.IndexOf('"', 1);
            path = quote > 1 ? path[1..quote] : path.Trim('"');
        }
        else
        {
            var executableEnd = path.IndexOf(".exe", StringComparison.OrdinalIgnoreCase);
            if (executableEnd >= 0)
            {
                path = path[..(executableEnd + 4)];
            }
        }

        if (IsGameExecutable(path))
        {
            AddCandidate(candidates, path);
            return;
        }

        AddLauncherDirectoryCandidate(candidates, Path.GetDirectoryName(path));
    }

    private static void AddProgramFilesCandidate(List<string> candidates, Environment.SpecialFolder specialFolder)
    {
        var programFiles = Environment.GetFolderPath(specialFolder);
        if (!string.IsNullOrWhiteSpace(programFiles))
        {
            AddLauncherDirectoryCandidate(candidates, Path.Combine(programFiles, LauncherDirectoryName));
        }
    }

    private static void AddLauncherDirectoryCandidate(List<string> candidates, string? launcherDirectory)
    {
        if (string.IsNullOrWhiteSpace(launcherDirectory))
        {
            return;
        }

        AddCandidate(candidates, Path.Combine(launcherDirectory.Trim().Trim('"'), LauncherGameRelativePath));
    }

    private static void AddCandidate(List<string> candidates, string path)
    {
        if (!candidates.Contains(path, StringComparer.OrdinalIgnoreCase))
        {
            candidates.Add(path);
        }
    }

    private static bool IsGameExecutable(string? path) =>
        !string.IsNullOrWhiteSpace(path) &&
        File.Exists(path) &&
        Path.GetFileName(path).Equals(GameExecutableName, StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Gets win32 截图方式枚举（与 AsstCaller.h 中 AsstWin32ScreencapMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32ScreencapMethod>> _screencapMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowScreencapFramePool"),  AsstWin32ScreencapMethod.FramePool),
        new(LocalizationHelper.GetString("AttachWindowScreencapPrintWindow"),  AsstWin32ScreencapMethod.PrintWindow),
        new(LocalizationHelper.GetString("AttachWindowScreencapScreenDC"),  AsstWin32ScreencapMethod.ScreenDC),
        new(LocalizationHelper.GetString("AttachWindowScreencapDesktopDupWindow"),  AsstWin32ScreencapMethod.DesktopDupWindow),
    ];

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32ScreencapMethod>> ScreencapMethodList => _screencapMethodList;

    [JsonInclude]
    [JsonPropertyName("ScreencapMethod")]
    private AsstWin32ScreencapMethod _screencapMethod = AsstWin32ScreencapMethod.FramePool; // 默认 FramePool

    /// <summary>
    /// Gets or sets the screencap method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32ScreencapMethod ScreencapMethod
    {
        get => _screencapMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _screencapMethod, value);
        }
    }

    /// <summary>
    /// Win32 鼠标输入方式枚举（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32InputMethod>> _mouseMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowInputSeize"),  AsstWin32InputMethod.Seize),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithCursor"),  AsstWin32InputMethod.SendMessageWithCursorPos),
        new(LocalizationHelper.GetString("AttachWindowInputSendWithWindowPos"),  AsstWin32InputMethod.SendMessageWithWindowPos),
    ];

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32InputMethod>> MouseMethodList => _mouseMethodList;

    [JsonInclude]
    [JsonPropertyName("MouseMethod")]
    private AsstWin32InputMethod _mouseMethod = AsstWin32InputMethod.SendMessageWithCursorPos; // 默认 SendMessageWithCursor

    /// <summary>
    /// Gets or sets the mouse input method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32InputMethod MouseMethod
    {
        get => _mouseMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _mouseMethod, value);
        }
    }

    /// <summary>
    /// Win32 键盘输入方式枚举（与 AsstCaller.h 中 AsstWin32InputMethodEnum 对应）
    /// </summary>
    private static readonly List<GenericCombinedData<AsstWin32KeyboardInputMethod>> _KeyboardMethodList =
    [
        new(LocalizationHelper.GetString("AttachWindowInputSeize"),  AsstWin32KeyboardInputMethod.Seize),
        new(LocalizationHelper.GetString("AttachWindowInputSendMsg"),  AsstWin32KeyboardInputMethod.SendMessage),
        new(LocalizationHelper.GetString("AttachWindowInputPostMsg"),  AsstWin32KeyboardInputMethod.PostMessage),
    ];

    [JsonIgnore]
    public List<GenericCombinedData<AsstWin32KeyboardInputMethod>> KeyboardMethodList => _KeyboardMethodList;

    [JsonInclude]
    [JsonPropertyName("KeyboardMethod")]
    private AsstWin32KeyboardInputMethod _KeyboardMethod = AsstWin32KeyboardInputMethod.SendMessage; // 默认 SendMessage

    /// <summary>
    /// Gets or sets the keyboard input method for AttachWindow mode.
    /// </summary>
    [JsonIgnore]
    public AsstWin32KeyboardInputMethod KeyboardMethod
    {
        get => _KeyboardMethod;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _KeyboardMethod, value);
        }
    }
}
