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
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class IssueReportUserControlModel : PropertyChangedBase
{

    static IssueReportUserControlModel()
    {
        Instance = new();
    }

    public static IssueReportUserControlModel Instance { get; }

    public void OpenDebugFolder()
    {
        string path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "debug");

        if (Directory.Exists(path))
        {
            Process.Start("explorer.exe", path);
        }
        else
        {
            ShowGrowl(LocalizationHelper.GetString("DebugFolderMissing"));
        }
    }

    public void GenerateSupportPayload()
    {
        try
        {
            string path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "debug");
            string zipPath = Path.Combine(path, "debug.zip");
            string tempDir = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());

            if (!Directory.Exists(path))
            {
                ShowGrowl(LocalizationHelper.GetString("DebugFolderMissing"));
                return;
            }

            if (File.Exists(zipPath))
            {
                File.Delete(zipPath);
            }

            Directory.CreateDirectory(tempDir);
            foreach (var file in Directory.GetFiles(path, "*.log"))
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
        }
        catch (Exception ex)
        {
            ShowGrowl($"{LocalizationHelper.GetString("GenerateSupportPayloadException")}\n{ex.Message}");
        }
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
