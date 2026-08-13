// <copyright file="DiagnosticNativeCrashImporter.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1636
#nullable enable

using System;
using System.IO;
using System.Linq;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class DiagnosticNativeCrashImporter
{
    private readonly DiagnosticStateStore _store;

    internal DiagnosticNativeCrashImporter(DiagnosticStateStore store)
    {
        _store = store;
    }

    internal DiagnosticFailure? Import(
        string sourcePath,
        string sourceKind,
        string details,
        DiagnosticFailure nativeFailure)
    {
        var history = _store.GetHistory();

        // native 日志通常在托管异常记录之后才由下次启动导入，优先并入同一时间附近的记录，
        // 使归档日志、待处理状态和最终报告始终使用同一个 Case ID。
        var managedRecord = history.FirstOrDefault(failure =>
            failure.Source is DiagnosticSource.Dispatcher or DiagnosticSource.AppDomain &&
            Math.Abs((failure.TimestampUtc - nativeFailure.TimestampUtc).TotalSeconds) <= 10);
        var existingNativeRecord = history.FirstOrDefault(failure =>
            string.Equals(failure.Source, DiagnosticSource.NativeCrash, StringComparison.Ordinal) &&
            string.Equals(failure.Fingerprint, nativeFailure.Fingerprint, StringComparison.Ordinal) &&
            Math.Abs((failure.TimestampUtc - nativeFailure.TimestampUtc).TotalSeconds) <= 5);
        var finalRecord = managedRecord ?? existingNativeRecord ?? nativeFailure;

        string safeCaseId = DiagnosticPathSafety.FileNamePart(finalRecord.CaseId, "unknown-case", 64);
        string archivedName = $"crash-{safeCaseId}-{sourceKind}.log";
        var archiveResult = ArchiveCrashLogAtomically(sourcePath, archivedName);
        string evidenceValue = $"{sourceKind}/{Path.GetFileName(sourcePath)}";
        if (!finalRecord.Evidence.Any(item =>
                string.Equals(item.Kind, "nativeCrashLog", StringComparison.Ordinal) &&
                string.Equals(item.Value, evidenceValue, StringComparison.Ordinal)))
        {
            finalRecord.Evidence.Add(new("nativeCrashLog", evidenceValue));
        }

        string detailMarker = $"--- Native crash record ({sourceKind}) ---";
        if (!finalRecord.TechnicalDetails.Contains(detailMarker, StringComparison.Ordinal))
        {
            string currentDetails = finalRecord.TechnicalDetails.Trim();
            string importedDetails = details.Trim();
            finalRecord.TechnicalDetails = string.Equals(currentDetails, importedDetails, StringComparison.Ordinal)
                ? $"{detailMarker}{Environment.NewLine}{details}"
                : $"{finalRecord.TechnicalDetails}{Environment.NewLine}{Environment.NewLine}{detailMarker}{Environment.NewLine}{details}";
        }

        var writeResult = managedRecord is not null || existingNativeRecord is not null
            ? _store.UpdateFailure(finalRecord)
            : _store.RecordFailure(finalRecord);

        // 只有历史记录确认落盘后才能删除源日志；否则保留现场，供下次启动重新导入。
        if (!writeResult.HistoryWritten)
        {
            if (archiveResult.Created)
            {
                DeleteBestEffort(archiveResult.Path);
            }

            return null;
        }

        DeleteBestEffort(sourcePath);
        CleanupArchiveRetentionBestEffort();
        return finalRecord;
    }

    private NativeArchiveResult ArchiveCrashLogAtomically(string sourcePath, string archivedName)
    {
        string destinationDirectory = Path.Combine(_store.DirectoryPath, "native");
        Directory.CreateDirectory(destinationDirectory);
        string destinationPath = Path.Combine(destinationDirectory, archivedName);
        if (File.Exists(destinationPath))
        {
            return new(destinationPath, Created: false);
        }

        string temporaryPath = $"{destinationPath}.{Environment.ProcessId}.{Guid.NewGuid():N}.tmp";
        try
        {
            File.Copy(sourcePath, temporaryPath, true);
            File.Move(temporaryPath, destinationPath);
            return new(destinationPath, Created: true);
        }
        finally
        {
            DeleteBestEffort(temporaryPath);
        }
    }

    private void CleanupArchiveRetentionBestEffort()
    {
        try
        {
            string destinationDirectory = Path.Combine(_store.DirectoryPath, "native");
            var cutoff = DateTime.UtcNow.AddDays(-30);
            foreach (var oldFile in new DirectoryInfo(destinationDirectory).EnumerateFiles("crash-*.log")
                         .OrderByDescending(static file => file.LastWriteTimeUtc)
                         .Where((file, index) => index >= 20 || file.LastWriteTimeUtc < cutoff))
            {
                DeleteBestEffort(oldFile.FullName);
            }
        }
        catch
        {
            // Retention cleanup must never turn a successfully imported crash into a failure.
        }
    }

    private static void DeleteBestEffort(string path)
    {
        try
        {
            File.Delete(path);
        }
        catch
        {
            // The imported record and source log remain sufficient for a later retry.
        }
    }

    private sealed record NativeArchiveResult(string Path, bool Created);
}
