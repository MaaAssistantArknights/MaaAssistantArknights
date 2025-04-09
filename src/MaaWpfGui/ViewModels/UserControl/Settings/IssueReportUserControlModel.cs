// <copyright file="IssueReportUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Helper;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 问题反馈
/// </summary>
public class IssueReportUserControlModel : PropertyChangedBase
{
    static IssueReportUserControlModel()
    {
        Instance = new();
    }

    private static readonly string[] LogFileNames = ["gui.log", "gui.bak.log", "asst.log", "asst.bak.log"];

    public static IssueReportUserControlModel Instance { get; }

    public void OpenDebugFolder()
    {
        string path = "debug";
        try
        {
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory("debug");
            }

            Process.Start("explorer.exe", path);
        }
        catch (Exception ex)
        {
            ToastNotification.ShowDirect($"Failed to open debug folder\n{ex.Message}");
            Log.Error(ex, "Failed to open debug folder");
        }
    }

    public void GenerateSupportPayload()
    {
        try
        {
            string path = "debug";
            string zipPath = Path.Combine(path, "log.zip");
            string tempDir = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());

            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory("debug");
                return;
            }

            if (File.Exists(zipPath))
            {
                File.Delete(zipPath);
            }

            Directory.CreateDirectory(tempDir);
            foreach (var file in Directory.EnumerateFiles(path, "*.log").Where(f => LogFileNames.Contains(Path.GetFileName(f))))
            {
                string dest = Path.Combine(tempDir, Path.GetFileName(file));
                File.Copy(file, dest, overwrite: true);
            }

            using (FileStream zipToOpen = new FileStream(zipPath, FileMode.Create))
            {
                using var archive = new ZipArchive(zipToOpen, ZipArchiveMode.Create);
                foreach (var file in Directory.GetFiles(tempDir))
                {
                    string entryName = Path.GetFileName(file);
                    archive.CreateEntryFromFile(file, entryName, CompressionLevel.Optimal);
                }
            }

            Directory.Delete(tempDir, recursive: true);
            ShowGrowl(LocalizationHelper.GetString("GenerateSupportPayloadSuccessful"));
            OpenDebugFolder();
        }
        catch (Exception ex)
        {
            ShowGrowl($"{LocalizationHelper.GetString("GenerateSupportPayloadException")}\n{ex.Message}");
            Log.Error(ex, "Failed to create log zip");
        }
    }

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public void SetAcknowledgedNightlyWarning()
    {
        VersionUpdateSettingsUserControlModel.Instance.HasAcknowledgedNightlyWarning = true;
    }

    private static void ShowGrowl(string message)
    {
        var growlInfo = new GrowlInfo
        {
            IsCustom = true,
            Message = message,
            IconKey = "HangoverGeometry",
            IconBrushKey = "PallasBrush",
        };
        Growl.Info(growlInfo);
    }
}
