// <copyright file="MarkdownDataHelper.cs" company="MaaAssistantArknights">
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
using System.IO;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 提供对 data 目录下 Markdown 文件的读写操作。
/// </summary>
public static class MarkdownDataHelper
{
    private static readonly object _lock = new();
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "MarkdownDataHelper");

    /// <summary>
    /// 从 data/{key}.md 读取文本内容，如果文件不存在则返回 defaultValue
    /// </summary>
    /// <param name="key">文件名（不含扩展名）</param>
    /// <param name="defaultValue">文件不存在时的默认值</param>
    /// <param name="dataDir">目标文件夹，默认为 PathsHelper.DataDir</param>
    /// <returns>文件内容</returns>
    public static string Get(string key, string defaultValue = "", string? dataDir = null)
    {
        var filePath = Path.Combine(dataDir ?? PathsHelper.DataDir, $"{key}.md");

        if (!File.Exists(filePath))
        {
            return defaultValue;
        }

        try
        {
            return File.ReadAllText(filePath);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to load markdown file for key {Key}", key);
            return defaultValue;
        }
    }

    /// <summary>
    /// 将文本内容写入 data/{key}.md
    /// </summary>
    /// <param name="key">文件名（不含扩展名）</param>
    /// <param name="content">要写入的文本内容</param>
    /// <param name="dataDir">目标文件夹，默认为 PathsHelper.DataDir</param>
    /// <returns>是否写入成功</returns>
    public static bool Set(string key, string content, string? dataDir = null)
    {
        var filePath = Path.Combine(dataDir ?? PathsHelper.DataDir, $"{key}.md");

        lock (_lock)
        {
            try
            {
                Directory.CreateDirectory(dataDir ?? PathsHelper.DataDir);
                File.WriteAllText(filePath, content);
                return true;
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to save markdown file for key {Key}", key);
                return false;
            }
        }
    }

    /// <summary>
    /// 删除 data/{key}.md
    /// </summary>
    /// <param name="key">文件名（不含扩展名）</param>
    /// <returns>是否成功删除</returns>
    public static bool Delete(string key)
    {
        var filePath = Path.Combine(PathsHelper.DataDir, $"{key}.md");

        try
        {
            if (!File.Exists(filePath))
            {
                return false;
            }

            File.Delete(filePath);
            return true;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to delete markdown file for key {Key}", key);
            return false;
        }
    }

    /// <summary>
    /// 判断 data/{key}.md 是否存在
    /// </summary>
    /// <param name="key">文件名（不含扩展名）</param>
    /// <returns>是否存在</returns>
    public static bool Exists(string key)
    {
        var filePath = Path.Combine(PathsHelper.DataDir, $"{key}.md");
        return File.Exists(filePath);
    }
}
