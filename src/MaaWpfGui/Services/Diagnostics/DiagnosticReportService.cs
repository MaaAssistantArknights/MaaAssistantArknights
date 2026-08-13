// <copyright file="DiagnosticReportService.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class DiagnosticReportService
{
    private const int MaxTextCharacters = 4_000_000;
    private const int TailLineCount = 500;
    private const string SharingWarning = "This report is stored locally and is not uploaded automatically. Its contents are not sanitized and may include user paths, account information, credentials from logs, screenshots, or dumps. Review it before sharing.";
    private const string TransactionDirectoryPrefix = ".maa-diagnostic-transaction-";
    private readonly string _baseDirectory;
    private readonly string _reportsDirectory;
    private readonly DiagnosticStateStore _store;
    private readonly DiagnosticReportRuntimeCollector _runtimeCollector;
    private readonly DiagnosticReportAttachmentWriter _attachmentWriter;
    private readonly SemaphoreSlim _buildGate = new(1, 1);
    private readonly JsonSerializerOptions _jsonOptions = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase, WriteIndented = true };

    internal DiagnosticReportService(string baseDirectory, string reportsDirectory, DiagnosticStateStore store)
    {
        _baseDirectory = baseDirectory;
        _reportsDirectory = reportsDirectory;
        _store = store;
        _runtimeCollector = new(baseDirectory);
        _attachmentWriter = new(baseDirectory);
    }

    internal async Task<DiagnosticReportResult> BuildAsync(
        DiagnosticFailure failure,
        DiagnosticReportOptions? options = null)
    {
        // 报告文件名、事务目录和分卷发布共享同一输出位置，串行化可避免重复点击造成发布竞争。
        await _buildGate.WaitAsync().ConfigureAwait(false);
        try
        {
            return await Task.Run(() => BuildCore(failure, options ?? new())).ConfigureAwait(false);
        }
        finally
        {
            _buildGate.Release();
        }
    }

    private DiagnosticReportResult BuildCore(DiagnosticFailure failure, DiagnosticReportOptions options)
    {
        failure.Evidence ??= [];
        failure.Environment ??= new(StringComparer.OrdinalIgnoreCase);
        var outputFailures = new List<Exception>();
        foreach (string reportsDirectory in GetOutputDirectories())
        {
            try
            {
                return BuildFullReport(reportsDirectory, failure, options);
            }
            catch (Exception fullReportException)
            {
                try
                {
                    return BuildMinimalReport(reportsDirectory, failure, fullReportException);
                }
                catch (Exception minimalException)
                {
                    outputFailures.Add(new AggregateException(fullReportException, minimalException));
                }
            }
        }

        throw new IOException("No writable location was available for the diagnostic report.", new AggregateException(outputFailures));
    }

    private DiagnosticReportResult BuildFullReport(
        string reportsDirectory,
        DiagnosticFailure failure,
        DiagnosticReportOptions options)
    {
        Directory.CreateDirectory(reportsDirectory);
        CleanupStaleTransactions(reportsDirectory);
        string reportFileStem = CreateReportFileStem(failure);
        string outputPath = Path.Combine(reportsDirectory, $"{reportFileStem}.zip");

        // 事务目录放在目标目录内，确保最终发布是同卷移动，并将所有半成品限制在可统一清理的范围内。
        string transactionDirectory = Path.Combine(reportsDirectory, $"{TransactionDirectoryPrefix}{Guid.NewGuid():N}.tmpdir");
        Directory.CreateDirectory(transactionDirectory);
        var publishedParts = new List<string>();
        try
        {
            DiagnosticAttachmentBuildResult attachmentResult;
            try
            {
                attachmentResult = _attachmentWriter.Build(transactionDirectory, reportFileStem, failure, options);
            }
            catch (Exception ex)
            {
                DeleteDirectoryContentsBestEffort(transactionDirectory);
                attachmentResult = CreateFailedAttachmentResult(options, ex);
            }

            var status = attachmentResult.CollectionStatus;
            var entries = CollectMainEntries(failure, options, status);
            var manifest = CreateManifest(failure, options, "full", attachmentResult.TemporaryPartPaths.Count);
            entries.Insert(0, new("report-manifest.json", JsonSerializer.Serialize(manifest, _jsonOptions)));
            entries.Add(new("diagnostics/collection-status.json", JsonSerializer.Serialize(status, _jsonOptions)));

            string temporaryMainPath = Path.Combine(transactionDirectory, $"{reportFileStem}.zip");
            WriteArchive(temporaryMainPath, entries);
            ValidateArchive(temporaryMainPath);

            // 先发布附件分卷，主 ZIP 最后发布并作为事务完成标记；主 ZIP 失败时回滚本轮分卷。
            foreach (string temporaryPartPath in attachmentResult.TemporaryPartPaths)
            {
                string finalPartPath = Path.Combine(reportsDirectory, Path.GetFileName(temporaryPartPath));
                File.Move(temporaryPartPath, finalPartPath);
                publishedParts.Add(finalPartPath);
            }

            File.Move(temporaryMainPath, outputPath);
            return new(outputPath, publishedParts, IsMinimal: false);
        }
        catch
        {
            foreach (string publishedPart in publishedParts)
            {
                DeleteBestEffort(publishedPart);
            }

            throw;
        }
        finally
        {
            DeleteDirectoryBestEffort(transactionDirectory);
        }
    }

    private DiagnosticReportResult BuildMinimalReport(
        string reportsDirectory,
        DiagnosticFailure failure,
        Exception reportException)
    {
        Directory.CreateDirectory(reportsDirectory);
        string reportFileStem = $"{CreateReportFileStem(failure)}_minimal";
        string outputPath = Path.Combine(reportsDirectory, $"{reportFileStem}.zip");
        string temporaryPath = Path.Combine(reportsDirectory, $".{reportFileStem}.{Guid.NewGuid():N}.tmp");
        var manifest = CreateManifest(failure, new(), "minimal", 0);
        var summary = new StringBuilder()
            .AppendLine($"Case ID: {failure.CaseId}")
            .AppendLine($"Error code: {failure.Code}")
            .AppendLine($"Time: {failure.TimestampUtc:O}")
            .AppendLine($"Exception: {failure.ExceptionType}")
            .AppendLine($"Message: {failure.Message}")
            .AppendLine()
            .AppendLine("--- Original technical details ---")
            .AppendLine(failure.TechnicalDetails)
            .ToString();
        var error = $"{reportException.GetType().FullName}: {reportException.Message}{Environment.NewLine}{reportException}";

        try
        {
            WriteArchive(temporaryPath,
            [
                new("report-manifest.json", JsonSerializer.Serialize(manifest, _jsonOptions)),
                new("failure-summary.txt", summary),
                new("report-generation-error.txt", error),
            ]);
            ValidateArchive(temporaryPath);
            File.Move(temporaryPath, outputPath);
            return new(outputPath, [], IsMinimal: true);
        }
        finally
        {
            DeleteBestEffort(temporaryPath);
        }
    }

    private List<ReportTextEntry> CollectMainEntries(
        DiagnosticFailure failure,
        DiagnosticReportOptions options,
        DiagnosticCollectionStatus status)
    {
        var entries = new List<ReportTextEntry>
        {
            new("failure.json", JsonSerializer.Serialize(failure, _jsonOptions)),
            new("issue.md", BuildIssueMarkdown(failure)),
        };
        CollectJson(entries, status, "failureHistory", "diagnostics/failure-history.json", _store.GetHistoryForReport);
        CollectJson(entries, status, "processRuntime", "runtime/process.json", DiagnosticReportRuntimeCollector.CollectProcessRuntime);
        CollectJson(entries, status, "loadedModules", "runtime/loaded-modules.json", DiagnosticReportRuntimeCollector.CollectLoadedModules);
        CollectJson(entries, status, "applicationFiles", "runtime/application-files.json", _runtimeCollector.CollectApplicationFiles);

        AddLog(entries, status, Path.Combine(_baseDirectory, "debug", "gui.log"), "logs/gui.log", failure, required: true);
        AddLog(entries, status, Path.Combine(_baseDirectory, "debug", "asst.log"), "logs/asst.log", failure, required: true);
        AddLog(entries, status, Path.Combine(_baseDirectory, "debug", "gui.bak.log"), "logs/gui.bak.log", failure);
        AddLog(entries, status, Path.Combine(_baseDirectory, "debug", "asst.bak.log"), "logs/asst.bak.log", failure);
        AddNativeLogs(entries, status, failure);
        AddLog(entries, status, Path.Combine(_baseDirectory, "crash.log"), "logs/unconsumed-crash.log", failure);
        AddLog(entries, status, Path.Combine(_baseDirectory, "debug", "crash.log"), "logs/unconsumed-debug-crash.log", failure);

        AddTextFile(entries, status, Path.Combine(_baseDirectory, "MAA.runtimeconfig.json"), "application/MAA.runtimeconfig.json");
        AddTextFile(entries, status, Path.Combine(_baseDirectory, "MAA.deps.json"), "application/MAA.deps.json");
        AddTextFile(entries, status, Path.Combine(_baseDirectory, "resource", "version.json"), "application/resource-version.json");

        if (options.IncludeConfigurationSummary)
        {
            var summary = new Dictionary<string, string> {
                ["locale"] = failure.Environment.GetValueOrDefault("locale", string.Empty),
                ["gpuInferenceEnabled"] = failure.Environment.GetValueOrDefault("gpuInferenceEnabled", string.Empty),
                ["gpuPreference"] = failure.Environment.GetValueOrDefault("gpuPreference", string.Empty),
                ["softwareRenderingConfigured"] = failure.Environment.GetValueOrDefault("softwareRenderingConfigured", string.Empty),
            };
            entries.Add(new("configuration-summary.json", JsonSerializer.Serialize(summary, _jsonOptions)));
            status.Collectors["configurationSummary"] = new();
        }

        return entries;
    }

    private void CollectJson<T>(
        ICollection<ReportTextEntry> entries,
        DiagnosticCollectionStatus status,
        string collectorName,
        string entryName,
        Func<T> collect)
    {
        try
        {
            entries.Add(new(entryName, JsonSerializer.Serialize(collect(), _jsonOptions)));
            status.Collectors[collectorName] = new();
        }
        catch (Exception ex)
        {
            entries.Add(new($"{entryName}.error.txt", FormatCollectionError(ex)));
            status.Collectors[collectorName] = ErrorStatus(ex);
        }
    }

    private void AddNativeLogs(
        ICollection<ReportTextEntry> entries,
        DiagnosticCollectionStatus status,
        DiagnosticFailure failure)
    {
        string directory = Path.Combine(_store.DirectoryPath, "native");
        try
        {
            if (!Directory.Exists(directory))
            {
                status.Collectors["nativeCrashLogs"] = new() { Status = "unavailable" };
                return;
            }

            string safeCaseId = DiagnosticPathSafety.FileNamePart(failure.CaseId, "unknown-case", 64);
            string prefix = $"crash-{safeCaseId}-";
            string[] paths = Directory.EnumerateFiles(directory, "crash-*.log", SearchOption.TopDirectoryOnly)
                .Where(path => Path.GetFileName(path).StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                .OrderBy(static path => path, StringComparer.OrdinalIgnoreCase)
                .ToArray();
            if (paths.Length == 0)
            {
                status.Collectors["nativeCrashLogs"] = new() { Status = "unavailable" };
                return;
            }

            foreach (string path in paths)
            {
                AddLog(entries, status, path, $"logs/native/{Path.GetFileName(path)}", failure);
            }

            status.Collectors["nativeCrashLogs"] = new();
        }
        catch (Exception ex)
        {
            status.Collectors["nativeCrashLogs"] = ErrorStatus(ex);
        }
    }

    private static void AddLog(
        ICollection<ReportTextEntry> entries,
        DiagnosticCollectionStatus status,
        string sourcePath,
        string entryName,
        DiagnosticFailure failure,
        bool required = false)
    {
        string collectorName = $"log:{entryName}";
        if (!File.Exists(sourcePath))
        {
            status.Collectors[collectorName] = new() { Status = "unavailable" };
            if (required)
            {
                entries.Add(new($"{entryName}.missing.txt", "Log file was not available when the report was generated."));
            }

            return;
        }

        try
        {
            entries.Add(new(entryName, ReadRelevantLog(sourcePath, failure.TimestampUtc)));
            status.Collectors[collectorName] = new();
        }
        catch (Exception ex)
        {
            entries.Add(new($"{entryName}.error.txt", FormatCollectionError(ex)));
            status.Collectors[collectorName] = ErrorStatus(ex);
        }
    }

    private static void AddTextFile(
        ICollection<ReportTextEntry> entries,
        DiagnosticCollectionStatus status,
        string sourcePath,
        string entryName)
    {
        string collectorName = $"file:{entryName}";
        if (!File.Exists(sourcePath))
        {
            status.Collectors[collectorName] = new() { Status = "unavailable" };
            return;
        }

        try
        {
            entries.Add(new(entryName, ReadTextBounded(sourcePath)));
            status.Collectors[collectorName] = new();
        }
        catch (Exception ex)
        {
            entries.Add(new($"{entryName}.error.txt", FormatCollectionError(ex)));
            status.Collectors[collectorName] = ErrorStatus(ex);
        }
    }

    private Dictionary<string, object?> CreateManifest(
        DiagnosticFailure failure,
        DiagnosticReportOptions options,
        string reportKind,
        int attachmentPartCount) => new() {
            ["schemaVersion"] = 3,
            ["reportKind"] = reportKind,
            ["generatedAtUtc"] = DateTimeOffset.UtcNow,
            ["caseId"] = failure.CaseId,
            ["code"] = failure.Code,
            ["appVersion"] = failure.AppVersion,
            ["resourceVersion"] = failure.ResourceVersion,
            ["ruleVersion"] = failure.RuleVersion,
            ["includesConfigurationSummary"] = options.IncludeConfigurationSummary,
            ["includesScreenshots"] = options.IncludeScreenshots,
            ["includesDumps"] = options.IncludeDumps,
            ["actualAttachmentPartCount"] = attachmentPartCount,
            ["collectionStatusEntry"] = reportKind == "full" ? "diagnostics/collection-status.json" : null,
            ["contentSanitized"] = false,
            ["sharingWarning"] = SharingWarning,
            ["defaultContents"] = "Failure history, crash-process snapshot, process/runtime details, loaded modules, application binary inventory, dependency manifests and relevant logs.",
            ["captureContext"] = "failure.json contains the recorded crash process snapshot; runtime entries describe the process that generated this report.",
        };

    private IEnumerable<string> GetOutputDirectories()
    {
        string[] candidates =
        [
            _reportsDirectory,
            Path.Combine(_baseDirectory, "diagnostic-reports"),
            Path.Combine(Path.GetTempPath(), "MaaAssistantArknights", "diagnostic-reports"),
        ];
        return candidates.Where(static path => !string.IsNullOrWhiteSpace(path)).Distinct(StringComparer.OrdinalIgnoreCase);
    }

    private static string CreateReportFileStem(DiagnosticFailure failure)
    {
        string incidentTime = failure.TimestampUtc.ToLocalTime().ToString("yyyy-MM-dd_HH-mm-ss", CultureInfo.InvariantCulture);
        string errorCode = DiagnosticPathSafety.FileNamePart(failure.Code, "UnknownError", 48);
        string caseId = DiagnosticPathSafety.FileNamePart(failure.CaseId, "unknown-case", 12);
        string uniqueSuffix = Guid.NewGuid().ToString("N")[..8];
        return $"{incidentTime}_MAA_diagnostic_report_{errorCode}_{caseId}_{uniqueSuffix}";
    }

    private static string BuildIssueMarkdown(DiagnosticFailure failure) => new StringBuilder()
        .AppendLine("## MAA diagnostic summary")
        .AppendLine()
        .AppendLine($"- Case ID: `{failure.CaseId}`")
        .AppendLine($"- Error code: `{failure.Code}`")
        .AppendLine($"- Time: `{failure.TimestampUtc:O}`")
        .AppendLine($"- App version: `{failure.AppVersion}`")
        .AppendLine($"- Rule version: `{failure.RuleVersion}`")
        .AppendLine($"- OS: `{failure.Environment.GetValueOrDefault("os", "unknown")}`")
        .AppendLine($"- GPU: `{failure.Environment.GetValueOrDefault("gpu.0.description", "unknown")}`")
        .AppendLine()
        .AppendLine("The report was created locally and was not uploaded automatically. Its contents were not sanitized; review it before sharing.")
        .ToString();

    private static string ReadRelevantLog(string sourcePath, DateTimeOffset failureTimestamp)
    {
        var selected = new BoundedLineBuffer(MaxTextCharacters, int.MaxValue);
        var tail = new BoundedLineBuffer(MaxTextCharacters, TailLineCount);
        var cutoffStart = failureTimestamp.ToLocalTime().AddMinutes(-10);
        var cutoffEnd = failureTimestamp.ToLocalTime().AddMinutes(10);
        bool matchedTimestamp = false;
        bool includeContinuation = false;
        using var stream = new FileStream(sourcePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true);
        while (reader.ReadLine() is { } line)
        {
            tail.Add(line);
            if (TryParseLogTimestamp(line, out var timestamp))
            {
                includeContinuation = timestamp >= cutoffStart && timestamp <= cutoffEnd;
                matchedTimestamp |= includeContinuation;
            }

            if (includeContinuation)
            {
                selected.Add(line);
            }
        }

        return matchedTimestamp ? selected.ToString() : tail.ToString();
    }

    private static bool TryParseLogTimestamp(string line, out DateTimeOffset timestamp)
    {
        timestamp = default;
        if (line.Length < 21 || line[0] != '[')
        {
            return false;
        }

        int end = line.IndexOf(']', 1, Math.Min(line.Length - 1, 40));
        if (end <= 1)
        {
            return false;
        }

        string value = line[1..end];
        string[] formats = ["yyyy-MM-dd HH:mm:ss.fff", "yyyy-MM-dd HH:mm:ss"];
        if (DateTime.TryParseExact(value, formats, CultureInfo.InvariantCulture, DateTimeStyles.None, out var localTime))
        {
            timestamp = new(localTime, TimeZoneInfo.Local.GetUtcOffset(localTime));
            return true;
        }

        return DateTimeOffset.TryParse(value, CultureInfo.InvariantCulture, DateTimeStyles.AssumeLocal, out timestamp);
    }

    private static string ReadTextBounded(string sourcePath)
    {
        var builder = new StringBuilder(Math.Min(MaxTextCharacters, 64 * 1024));
        char[] buffer = new char[8192];
        using var stream = new FileStream(sourcePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true);
        while (builder.Length < MaxTextCharacters)
        {
            int requested = Math.Min(buffer.Length, MaxTextCharacters - builder.Length);
            int count = reader.Read(buffer, 0, requested);
            if (count == 0)
            {
                return builder.ToString();
            }

            builder.Append(buffer, 0, count);
        }

        if (!reader.EndOfStream)
        {
            builder.AppendLine().Append("[truncated by diagnostic report]");
        }

        return builder.ToString();
    }

    private static void WriteArchive(string path, IEnumerable<ReportTextEntry> entries)
    {
        using var archive = ZipFile.Open(path, ZipArchiveMode.Create);
        foreach (var item in entries)
        {
            var entry = archive.CreateEntry(item.EntryName, CompressionLevel.SmallestSize);
            using var writer = new StreamWriter(entry.Open(), new UTF8Encoding(false));
            writer.Write(item.Contents);
        }
    }

    private static void ValidateArchive(string path)
    {
        using var archive = ZipFile.OpenRead(path);
        foreach (var entry in archive.Entries)
        {
            using var reader = entry.Open();
            reader.CopyTo(Stream.Null);
        }
    }

    private static DiagnosticAttachmentBuildResult CreateFailedAttachmentResult(
        DiagnosticReportOptions options,
        Exception exception)
    {
        var status = new DiagnosticCollectionStatus();
        status.Screenshots.Requested = options.IncludeScreenshots;
        status.Dumps.Requested = options.IncludeDumps;
        ApplyError(status.Screenshots, exception);
        ApplyError(status.Dumps, exception);
        status.Collectors["attachmentWriter"] = ErrorStatus(exception);
        return new([], status);
    }

    private static void ApplyError(DiagnosticCollectorStatus status, Exception exception)
    {
        status.Status = "error";
        status.ExceptionType = exception.GetType().FullName ?? exception.GetType().Name;
        status.ExceptionMessage = exception.Message;
    }

    private static DiagnosticCollectorStatus ErrorStatus(Exception exception)
    {
        var result = new DiagnosticCollectorStatus();
        ApplyError(result, exception);
        return result;
    }

    private static string FormatCollectionError(Exception exception) =>
        $"Could not collect this entry: {exception.GetType().FullName}: {exception.Message}";

    private static void CleanupStaleTransactions(string reportsDirectory)
    {
        DateTime cutoff = DateTime.UtcNow.AddHours(-24);
        try
        {
            foreach (string directory in Directory.EnumerateDirectories(reportsDirectory, $"{TransactionDirectoryPrefix}*.tmpdir"))
            {
                try
                {
                    if (Directory.GetLastWriteTimeUtc(directory) < cutoff)
                    {
                        Directory.Delete(directory, recursive: true);
                    }
                }
                catch
                {
                    // Stale cleanup is strictly best effort.
                }
            }

            foreach (string partPath in Directory.EnumerateFiles(reportsDirectory, "*_MAA_diagnostic_report_*_part??.zip"))
            {
                try
                {
                    if (File.GetLastWriteTimeUtc(partPath) >= cutoff)
                    {
                        continue;
                    }

                    string fileName = Path.GetFileNameWithoutExtension(partPath);
                    int partIndex = fileName.LastIndexOf("_part", StringComparison.OrdinalIgnoreCase);
                    if (partIndex > 0)
                    {
                        string mainPath = Path.Combine(reportsDirectory, $"{fileName[..partIndex]}.zip");
                        if (!File.Exists(mainPath))
                        {
                            File.Delete(partPath);
                        }
                    }
                }
                catch
                {
                    // Stale cleanup is strictly best effort.
                }
            }
        }
        catch
        {
            // Enumeration failures may not prevent report generation.
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
            // Cleanup must not mask the report result.
        }
    }

    private static void DeleteDirectoryBestEffort(string path)
    {
        try
        {
            Directory.Delete(path, recursive: true);
        }
        catch
        {
            // Cleanup must not mask the report result.
        }
    }

    private static void DeleteDirectoryContentsBestEffort(string path)
    {
        try
        {
            foreach (string directory in Directory.EnumerateDirectories(path))
            {
                Directory.Delete(directory, recursive: true);
            }

            foreach (string file in Directory.EnumerateFiles(path))
            {
                File.Delete(file);
            }
        }
        catch
        {
            // The final transaction cleanup gets another chance.
        }
    }

    private sealed record ReportTextEntry(string EntryName, string Contents);

    private sealed class BoundedLineBuffer
    {
        private readonly Queue<string> _lines = new();
        private readonly int _maxCharacters;
        private readonly int _maxLines;
        private int _characters;

        internal BoundedLineBuffer(int maxCharacters, int maxLines)
        {
            _maxCharacters = maxCharacters;
            _maxLines = maxLines;
        }

        internal void Add(string line)
        {
            int maxLineCharacters = Math.Max(1, _maxCharacters - Environment.NewLine.Length);
            string boundedLine = line.Length <= maxLineCharacters ? line : line[^maxLineCharacters..];
            _lines.Enqueue(boundedLine);
            _characters += boundedLine.Length + Environment.NewLine.Length;
            while (_lines.Count > _maxLines || _characters > _maxCharacters)
            {
                string removed = _lines.Dequeue();
                _characters -= removed.Length + Environment.NewLine.Length;
            }
        }

        public override string ToString() => string.Join(Environment.NewLine, _lines);
    }
}
