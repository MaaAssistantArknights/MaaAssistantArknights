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
using System.Windows;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Configuration;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
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

    private static readonly string[] _payloadFileNames = [
        Bootstrapper.UiLogFilename,
        Bootstrapper.UiLogBakFilename,
        Bootstrapper.CoreLogFilename,
        Bootstrapper.CoreLogBakFilename,
        ConfigurationHelper.ConfigurationFile,
        ConfigFactory.ConfigFileName];

    private const string DebugDir = "debug";

    public static IssueReportUserControlModel Instance { get; }

    public void OpenDebugFolder()
    {
        try
        {
            if (!Directory.Exists(DebugDir))
            {
                Directory.CreateDirectory(DebugDir);
            }

            Process.Start("explorer.exe", DebugDir);
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
            var reportFileName = $"report_{DateTimeOffset.Now:MM-dd_HH-mm-ss}.zip";
            string zipPath = Path.Combine(DebugDir, reportFileName);
            string tempPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            string debugTempPath = Path.Combine(tempPath, "debug");
            string resourceTempPath = Path.Combine(tempPath, "resource");
            if (File.Exists(zipPath))
            {
                File.Delete(zipPath);
            }

            Directory.CreateDirectory(tempPath);
            Directory.CreateDirectory(debugTempPath);
            Directory.CreateDirectory(resourceTempPath);

            // 复制 _payloadFileNames 中的文件到 tempPath/debug
            foreach (var file in _payloadFileNames)
            {
                if (!File.Exists(file))
                {
                    continue;
                }

                string relativePath = Path.GetRelativePath(Environment.CurrentDirectory, file);
                string dest = Path.Combine(tempPath, relativePath);
                Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
                File.Copy(file, dest, overwrite: true);
            }

            // 遍历 resource 文件夹下带 _custom 后缀的文件，复制到 tempPath/resource
            const string ResourceDir = "resource";
            if (Directory.Exists(ResourceDir))
            {
                foreach (var file in Directory.EnumerateFiles(ResourceDir, "*_custom.*", SearchOption.AllDirectories))
                {
                    string relativePath = Path.GetRelativePath(ResourceDir, file);
                    string dest = Path.Combine(resourceTempPath, relativePath);
                    Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
                    File.Copy(file, dest, overwrite: true);
                }
            }

            using (FileStream zipToOpen = new FileStream(zipPath, FileMode.Create))
            {
                using var archive = new ZipArchive(zipToOpen, ZipArchiveMode.Create);
                foreach (var file in Directory.EnumerateFiles(tempPath, "*", SearchOption.AllDirectories))
                {
                    string entryName = Path.GetRelativePath(tempPath, file);
                    archive.CreateEntryFromFile(file, entryName, CompressionLevel.Optimal);
                }
            }

            Directory.Delete(tempPath, recursive: true);
            ShowGrowl($"{LocalizationHelper.GetString("GenerateSupportPayloadSuccessful")}\n{reportFileName}");
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
