// <copyright file="DailyReportStore.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models.DailyReport;
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 作战日报按日文件存储：data/DailyReport/&lt;yyyy-MM-dd&gt;.json，一天一个文件，历史文件不可变。
/// 日期为游戏日（各服时区 + 每日 4:00 刷新）。
/// </summary>
public static class DailyReportStore
{
    private const string DirName = "DailyReport";
    private const string FileNameFormat = "yyyy-MM-dd";

    private static readonly object _lock = new();
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "DailyReportStore");

    /// <summary>
    /// Gets 日报数据目录路径。
    /// </summary>
    public static string DirectoryPath => Path.Combine(PathsHelper.DataDir, DirName);

    /// <summary>
    /// 加载单日数据；崩溃遗留的无结束时刻记录在加载时补齐。
    /// 无法读取或解析的文件改名为 .bak 留存，避免随后的保存用空数据覆盖原内容。
    /// </summary>
    /// <param name="day">游戏日。</param>
    /// <returns>当日数据；文件不存在或损坏时为 null。</returns>
    public static DailyReportDayData? Load(DateOnly day) => Load(day, out _);

    /// <summary>
    /// 加载单日数据，并告知能否安全写入当日文件。
    /// </summary>
    /// <param name="day">游戏日。</param>
    /// <param name="writable">文件不存在、正常加载或已备份为 .bak 时为 true；备份失败时为 false，此时写入会覆盖无法读取的原文件。</param>
    /// <returns>当日数据；文件不存在或损坏时为 null。</returns>
    public static DailyReportDayData? Load(DateOnly day, out bool writable)
    {
        writable = true;
        var filePath = GetFilePath(day);
        if (!File.Exists(filePath))
        {
            return null;
        }

        try
        {
            var data = JsonConvert.DeserializeObject<DailyReportDayData>(File.ReadAllText(filePath));
            if (data?.Sessions is null)
            {
                writable = BackupCorrupted(filePath, day);
                return null;
            }

            // 崩溃遗留的未定稿记录补结束时刻，避免半截数据进入展示
            var now = DateTimeOffset.UtcNow;
            foreach (var session in data.Sessions.Where(s => s.EndTime is null))
            {
                session.EndTime = now;
            }

            return data;
        }
        catch (Exception ex)
        {
            // 解析失败或读取失败都把原文件留存为 .bak：调用方随后会以空数据新建当日文件，原内容不会被覆盖
            _logger.Error(ex, "Failed to load daily report for {Day}", day);
            writable = BackupCorrupted(filePath, day);
            return null;
        }
    }

    private static bool BackupCorrupted(string filePath, DateOnly day)
    {
        try
        {
            File.Move(filePath, filePath + ".bak", true);
            _logger.Warning("Corrupted daily report for {Day} moved to {Path}", day, filePath + ".bak");
            return true;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to back up corrupted daily report for {Day}", day);
            return false;
        }
    }

    /// <summary>
    /// 保存单日数据（全量覆盖写当日文件，历史文件不受影响）。
    /// 先写临时文件再替换，写入中途崩溃或断电时原文件保持完整。
    /// </summary>
    /// <param name="day">游戏日。</param>
    /// <param name="data">当日数据。</param>
    public static void Save(DateOnly day, DailyReportDayData data)
    {
        if (MaaWpfGui.Main.Bootstrapper.IsDemoMode)
        {
            // README 截图演示模式：不写任何 data/ 缓存文件
            return;
        }

        lock (_lock)
        {
            try
            {
                Directory.CreateDirectory(DirectoryPath);
                var filePath = GetFilePath(day);
                var tempPath = filePath + ".tmp";
                File.WriteAllText(tempPath, JsonConvert.SerializeObject(data, Formatting.Indented));
                File.Move(tempPath, filePath, true);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to save daily report for {Day}", day);
            }
        }
    }

    /// <summary>
    /// 列出全部已存在的游戏日。
    /// </summary>
    /// <returns>升序的游戏日列表。</returns>
    public static List<DateOnly> ListDays()
    {
        if (!Directory.Exists(DirectoryPath))
        {
            return [];
        }

        var days = new List<DateOnly>();
        try
        {
            foreach (var file in Directory.EnumerateFiles(DirectoryPath, "*.json"))
            {
                if (DateOnly.TryParseExact(Path.GetFileNameWithoutExtension(file), FileNameFormat, CultureInfo.InvariantCulture, DateTimeStyles.None, out var day))
                {
                    days.Add(day);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to enumerate daily report directory");
        }

        days.Sort();
        return days;
    }

    /// <summary>
    /// 加载日期范围内的全部日数据（周/月/年视图的数据源）。
    /// </summary>
    /// <param name="from">起始游戏日（含）。</param>
    /// <param name="to">结束游戏日（含）。</param>
    /// <returns>范围内按游戏日升序的日数据列表，缺日跳过。</returns>
    public static List<(DateOnly Day, DailyReportDayData Data)> LoadRange(DateOnly from, DateOnly to)
    {
        var result = new List<(DateOnly Day, DailyReportDayData Data)>();
        foreach (var day in ListDays().Where(d => d >= from && d <= to))
        {
            if (Load(day) is { } data)
            {
                result.Add((day, data));
            }
        }

        return result;
    }

    /// <summary>
    /// 按保留档位删除超期日文件；近 N 档保留最近 N 个游戏日（含今日），不限制档位不删除。
    /// </summary>
    /// <param name="retention">保留档位。</param>
    public static void Prune(DailyReportRetention retention)
    {
        foreach (var day in ListPrunableDays(retention))
        {
            try
            {
                File.Delete(GetFilePath(day));
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to prune daily report for {Day}", day);
            }
        }
    }

    /// <summary>
    /// 列出按保留档位将被 <see cref="Prune"/> 删除的游戏日，供切换档位前确认。
    /// </summary>
    /// <param name="retention">保留档位。</param>
    /// <returns>升序的超期游戏日列表；不限制档位为空。</returns>
    public static List<DateOnly> ListPrunableDays(DailyReportRetention retention)
    {
        var keepDays = retention switch
        {
            DailyReportRetention.Day => 1,
            DailyReportRetention.Week => 7,
            DailyReportRetention.Month => 31,
            DailyReportRetention.Year => 366,
            _ => int.MaxValue,
        };
        if (keepDays == int.MaxValue)
        {
            return [];
        }

        var today = DateOnly.FromDateTime(DateTime.UtcNow.ToYjDate());
        var cutoff = today.AddDays(-(keepDays - 1));
        return ListDays().Where(day => day < cutoff).ToList();
    }

    private static string GetFilePath(DateOnly day)
    {
        return Path.Combine(DirectoryPath, day.ToString(FileNameFormat, CultureInfo.InvariantCulture) + ".json");
    }
}
