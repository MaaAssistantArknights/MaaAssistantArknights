// <copyright file="ResourceIntegrityChecker.cs" company="MaaAssistantArknights">
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
using System.IO;
using MaaWpfGui.Helper;
using Serilog;

namespace MaaWpfGui.Services;

/// <summary>
/// 检查安装文件完整性。
/// filelist.txt 由 CI 在打完整包时生成（安装目录内全部文件的相对路径列表，每行一个，正斜杠分隔），
/// 与 zipota 生成的 OTA 增量包内 filelist.txt 规则一致（均为全量清单）；
/// OTA 增量包会携带新版本的 filelist.txt，应用更新后随之刷新，无需单独维护。
/// 检查时跳过 dll/exe：程序文件缺失时 UI 或核心会直接报错，无需此检查兜底，
/// 也可避免打包与开发环境目录布局差异造成的误报。
/// 只做存在性检查：资源更新链路只增不删且只覆盖，清单不会因日常资源更新误报；
/// 文件缺失通常由安全软件拦截或手动删除造成，且在任务加载侧完全无提示。
/// </summary>
internal static class ResourceIntegrityChecker
{
    private const string FileListName = "filelist.txt";

    private static readonly ILogger _logger = Log.ForContext(typeof(ResourceIntegrityChecker));

    private static readonly HashSet<string> s_skippedExtensions = new(StringComparer.OrdinalIgnoreCase)
    {
        ".dll",
        ".exe",
        ".py",
    };

    /// <summary>
    /// 对照 filelist.txt 检查安装目录，返回缺失文件的相对路径列表。
    /// 清单不存在（旧版本升级 / 开发环境）时视为通过，返回空列表。
    /// </summary>
    /// <returns>缺失文件的相对路径（以 / 分隔），无缺失或无法检查时为空列表。</returns>
    public static IReadOnlyList<string> GetMissingFiles()
    {
        string fileListPath = Path.Combine(PathsHelper.BaseDir, FileListName);
        if (!File.Exists(fileListPath))
        {
            _logger.Information("File list not found, skipping integrity check: {FileListPath}", fileListPath);
            return [];
        }

        string[] expectedFiles;
        try
        {
            expectedFiles = File.ReadAllLines(fileListPath);
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to read file list: {FileListPath}", fileListPath);
            return [];
        }

        var missingFiles = new List<string>();
        int checkedFiles = 0;
        foreach (string relativePath in expectedFiles)
        {
            if (string.IsNullOrWhiteSpace(relativePath))
            {
                continue;
            }

            string trimmedPath = relativePath.Trim();

            // 兼容旧版 zipota 清单：其目录条目以 / 结尾（File.Exists 对目录恒为 false，会全部误报）
            if (trimmedPath.EndsWith('/'))
            {
                continue;
            }

            // Python/ 目录供用户自行接入调用，允许删除
            if (s_skippedExtensions.Contains(Path.GetExtension(trimmedPath)))
            {
                continue;
            }

            checkedFiles++;
            string fullPath = Path.Combine(PathsHelper.BaseDir, trimmedPath);
            if (!File.Exists(fullPath))
            {
                missingFiles.Add(trimmedPath);
            }
        }

        if (missingFiles.Count > 0)
        {
            _logger.Error("Integrity check failed, {Count} file(s) missing: {MissingFiles}",
                missingFiles.Count, string.Join(", ", missingFiles));
        }
        else
        {
            _logger.Information("Integrity check passed, {Count} file(s) verified", checkedFiles);
        }

        return missingFiles;
    }
}
