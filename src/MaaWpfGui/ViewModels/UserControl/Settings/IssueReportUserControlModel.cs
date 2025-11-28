// <copyright file="IssueReportUserControlModel.cs" company="MaaAssistantArknights">
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
using System.IO.Compression;
using System.Linq;
using HandyControl.Controls;
using HandyControl.Data;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
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

    public static IssueReportUserControlModel Instance { get; }

    public void OpenDebugFolder()
    {
        try
        {
            if (!Directory.Exists(PathsHelper.ReportsDir))
            {
                Directory.CreateDirectory(PathsHelper.ReportsDir);
            }

            Process.Start("explorer.exe", PathsHelper.ReportsDir);
        }
        catch (Exception ex)
        {
            ToastNotification.ShowDirect($"Failed to open reports folder\n{ex.Message}");
            Log.Error(ex, "Failed to open reports folder");
        }
    }

    /// <summary>
    /// 生成日志压缩包
    /// </summary>
    public void GenerateSupportPayload()
    {
        try
        {
            const int PartSize = 20 * 1024 * 1024; // 20 MB
            string reportNameBase = $"report_{DateTimeOffset.Now:MM-dd_HH-mm-ss}";
            string tempPath = Path.Combine(Path.GetTempPath(), $"maa-report-{Guid.NewGuid()}");
            Directory.CreateDirectory(tempPath);

            if (!Directory.Exists(PathsHelper.ReportsDir))
            {
                Directory.CreateDirectory(PathsHelper.ReportsDir);
            }

            // 复制文件
            CopyDirectoryIfExists(PathsHelper.DebugDir, Path.Combine(tempPath, "debug"),
                f => { return !Path.GetFileName(f).StartsWith("report", StringComparison.OrdinalIgnoreCase); });
            CopyDirectoryIfExists(PathsHelper.ResourceDir, Path.Combine(tempPath, "resource"),
                f => { return Path.GetFileName(f).Contains("_custom.", StringComparison.OrdinalIgnoreCase); });
            CopyDirectoryIfExists(PathsHelper.ConfigDir, Path.Combine(tempPath, "config"));
            CopyDirectoryIfExists(PathsHelper.CacheDir, Path.Combine(tempPath, "cache"));

            string partsFolder = Path.Combine(PathsHelper.ReportsDir, $"{reportNameBase}_parts");
            if (!Directory.Exists(partsFolder))
            {
                Directory.CreateDirectory(partsFolder);
            }

            // ====== part01：config + resource + cache + debug 根目录文件 ======
            List<string> part01Files = [];

            string[] categories = ["config", "resource", "cache"];
            foreach (string category in categories)
            {
                string categoryPath = Path.Combine(tempPath, category);
                if (Directory.Exists(categoryPath))
                {
                    part01Files.AddRange(Directory.EnumerateFiles(categoryPath, "*", SearchOption.AllDirectories));
                }
            }

            string debugPath = Path.Combine(tempPath, "debug");
            if (Directory.Exists(debugPath))
            {
                // 只取 debug 根目录文件
                var debugRootFiles = Directory.EnumerateFiles(debugPath, "*", SearchOption.TopDirectoryOnly).ToList();
                part01Files.AddRange(debugRootFiles);
            }

            if (part01Files.Count > 0)
            {
                string part01Path = Path.Combine(partsFolder, $"{reportNameBase}_part01.zip");
                using var fs = new FileStream(part01Path, FileMode.Create);
                using var archive = new ZipArchive(fs, ZipArchiveMode.Create);
                foreach (var file in part01Files)
                {
                    string entryName = Path.GetRelativePath(tempPath, file).Replace("\\", "/");
                    var entry = archive.CreateEntry(entryName, CompressionLevel.SmallestSize);

                    using var entryStream = entry.Open();
                    using var fileStream = File.OpenRead(file);
                    fileStream.CopyTo(entryStream);
                }
            }

            // ====== part02+：debug 子目录文件按 PartSize 分卷 ======
            var debugSubFiles = Directory.EnumerateFiles(debugPath, "*", SearchOption.AllDirectories)
                .Where(f => Path.GetDirectoryName(f) != debugPath).ToList();

            int partNumber = 2;
            while (debugSubFiles.Count > 0)
            {
                string partFileName = $"{reportNameBase}_part{partNumber:D2}.zip";
                string partPath = Path.Combine(partsFolder, partFileName);

                using (var fs = new FileStream(partPath, FileMode.Create))
                {
                    using var archive = new ZipArchive(fs, ZipArchiveMode.Create);
                    long currentSize = 0;
                    List<string> processedFiles = [];

                    foreach (var file in debugSubFiles.ToList())
                    {
                        var fileInfo = new FileInfo(file);

                        if (currentSize + fileInfo.Length > PartSize && currentSize > 0)
                        {
                            break;
                        }

                        string entryName = Path.GetRelativePath(tempPath, file).Replace("\\", "/");
                        var entry = archive.CreateEntry(entryName, CompressionLevel.SmallestSize);

                        using (var entryStream = entry.Open())
                        {
                            using var fileStream = File.OpenRead(file);
                            fileStream.CopyTo(entryStream);
                        }

                        currentSize += fileInfo.Length;
                        processedFiles.Add(file);
                    }

                    debugSubFiles.RemoveAll(f => processedFiles.Contains(f));
                }

                partNumber++;
            }

            // ====== 生成完整压缩包 ======
            string fullZipPath = Path.Combine(PathsHelper.ReportsDir, $"{reportNameBase}.zip");
            ZipFile.CreateFromDirectory(tempPath, fullZipPath, CompressionLevel.SmallestSize, includeBaseDirectory: false);

            // 清理临时目录
            Directory.Delete(tempPath, recursive: true);

            ShowGrowl($"{LocalizationHelper.GetString("GenerateSupportPayloadSuccessful")}\n{fullZipPath}");
            OpenDebugFolder();
        }
        catch (Exception ex)
        {
            ShowGrowl($"{LocalizationHelper.GetString("GenerateSupportPayloadException")}\n{ex.Message}");
            Log.Error(ex, "Failed to create support payload");
        }
    }

    /// <summary>
    /// 从 sourceDir 复制文件到 targetDir，支持过滤。
    /// </summary>
    private static void CopyDirectoryIfExists(string? sourceDir, string targetDir, Func<string, bool>? filter = null)
    {
        if (string.IsNullOrWhiteSpace(sourceDir) || !Directory.Exists(sourceDir))
        {
            return;
        }

        foreach (var file in Directory.EnumerateFiles(sourceDir, "*", SearchOption.AllDirectories))
        {
            if (filter != null && !filter(file))
            {
                continue;
            }

            string relative = Path.GetRelativePath(sourceDir, file);
            string dest = Path.Combine(targetDir, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
            try
            {
                File.Copy(file, dest, true);
            }
            catch (IOException)
            {
                // 某些文件可能被占用，忽略复制失败
            }
            catch (UnauthorizedAccessException)
            {
                // 也忽略权限问题
            }
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void SetAcknowledgedNightlyWarning()
    {
        // 其实不应该放这里，但懒得写一个新的方法，就塞到这里了
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.ProblemFeedback);
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
