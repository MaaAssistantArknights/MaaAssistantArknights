// <copyright file="DiagnosticStateStore.cs" company="MaaAssistantArknights">
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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class DiagnosticStateStore
{
    private const int MaxHistoryCount = 20;
    private static readonly TimeSpan MaxHistoryAge = TimeSpan.FromDays(30);
    private static readonly ConcurrentDictionary<string, object> DirectoryGates = new(StringComparer.OrdinalIgnoreCase);
    private readonly object _gate;
    private readonly string _directory;
    private readonly JsonSerializerOptions _jsonOptions = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase, WriteIndented = true };

    internal DiagnosticStateStore(string directory)
    {
        _directory = Path.GetFullPath(directory);
        _gate = DirectoryGates.GetOrAdd(_directory, static _ => new());
    }

    internal string DirectoryPath => _directory;

    private string PendingPath => Path.Combine(_directory, "pending-failure.json");

    private string HistoryPath => Path.Combine(_directory, "history.json");

    internal DiagnosticStoreWriteResult RecordFailure(DiagnosticFailure failure, bool makePending = true)
    {
        lock (_gate)
        {
            Normalize(failure);
            bool historyWritten = false;
            bool pendingWritten = !makePending;
            try
            {
                Directory.CreateDirectory(_directory);
                var history = ReadHistory();
                var now = DateTimeOffset.UtcNow;
                var cutoff = now - MaxHistoryAge;
                history.RemoveAll(x => x.TimestampUtc < cutoff);

                // 同一次未处理异常可能先后进入 Dispatcher 和 AppDomain 回调；复用已有记录，
                // 避免重复增加发生次数，也避免在下次启动时生成两条待处理提示。
                var duplicate = history.LastOrDefault(x =>
                    x.ProcessId == failure.ProcessId &&
                    string.Equals(x.Fingerprint, failure.Fingerprint, StringComparison.Ordinal) &&
                    Math.Abs((x.TimestampUtc - failure.TimestampUtc).TotalSeconds) < 5);
                if (duplicate is null)
                {
                    int occurrences = history.Count(x =>
                        string.Equals(x.Fingerprint, failure.Fingerprint, StringComparison.Ordinal) &&
                        x.TimestampUtc >= now.AddHours(-24));
                    failure.OccurrenceCount = occurrences + 1;
                    history.Add(failure);
                }
                else
                {
                    failure = duplicate;
                }

                history = history.OrderByDescending(static x => x.TimestampUtc).Take(MaxHistoryCount).OrderBy(static x => x.TimestampUtc).ToList();
                WriteAtomic(HistoryPath, history);
                historyWritten = true;
            }
            catch
            {
                // Diagnostics are strictly best effort and may never break the calling feature.
            }

            if (makePending)
            {
                try
                {
                    WriteAtomic(PendingPath, failure);
                    pendingWritten = true;
                }
                catch
                {
                    // A pending write failure must not invalidate an otherwise persisted history record.
                }
            }

            return new(failure, historyWritten, pendingWritten);
        }
    }

    internal DiagnosticFailure? TryConsumePending()
    {
        lock (_gate)
        {
            var pending = Read<DiagnosticFailure>(PendingPath);
            if (pending is null)
            {
                return null;
            }

            Normalize(pending);

            try
            {
                File.Delete(PendingPath);
                return File.Exists(PendingPath) ? null : pending;
            }
            catch
            {
                // Do not display a prompt that cannot be acknowledged, or it may recur forever.
                return null;
            }
        }
    }

    internal IReadOnlyList<DiagnosticFailure> GetHistory()
    {
        lock (_gate)
        {
            return ReadHistory().OrderByDescending(static x => x.TimestampUtc).ToList();
        }
    }

    internal IReadOnlyList<DiagnosticFailure> GetHistoryForReport()
    {
        lock (_gate)
        {
            if (!File.Exists(HistoryPath))
            {
                return [];
            }

            var history = JsonSerializer.Deserialize<List<DiagnosticFailure>>(
                File.ReadAllText(HistoryPath),
                _jsonOptions) ?? [];
            foreach (var failure in history)
            {
                Normalize(failure);
            }

            return history.OrderByDescending(static failure => failure.TimestampUtc).ToList();
        }
    }

    internal DiagnosticFailure? GetLatestFailure() => GetHistory()
        .FirstOrDefault(static failure => string.Equals(failure.Severity, DiagnosticSeverity.Fatal, StringComparison.Ordinal));

    internal DiagnosticStoreWriteResult UpdateFailure(DiagnosticFailure failure)
    {
        lock (_gate)
        {
            Normalize(failure);
            bool historyWritten = false;
            bool pendingWritten = true;
            try
            {
                var history = ReadHistory();
                int index = history.FindIndex(x => string.Equals(x.CaseId, failure.CaseId, StringComparison.Ordinal));
                if (index >= 0)
                {
                    history[index] = failure;
                    WriteAtomic(HistoryPath, history);
                    historyWritten = true;
                }
            }
            catch
            {
                // Best effort enrichment only.
            }

            try
            {
                var pending = Read<DiagnosticFailure>(PendingPath);
                if (pending is not null && string.Equals(pending.CaseId, failure.CaseId, StringComparison.Ordinal))
                {
                    WriteAtomic(PendingPath, failure);
                }
            }
            catch
            {
                // Best effort enrichment only.
                pendingWritten = false;
            }

            return new(failure, historyWritten, pendingWritten);
        }
    }

    private T? Read<T>(string path)
    {
        try
        {
            if (!File.Exists(path))
            {
                return default;
            }

            return JsonSerializer.Deserialize<T>(File.ReadAllText(path), _jsonOptions);
        }
        catch
        {
            return default;
        }
    }

    private List<DiagnosticFailure> ReadHistory()
    {
        var history = (Read<List<DiagnosticFailure>>(HistoryPath) ?? [])
            .OfType<DiagnosticFailure>()
            .ToList();
        foreach (var failure in history)
        {
            Normalize(failure);
        }

        return history;
    }

    private static void Normalize(DiagnosticFailure failure)
    {
        failure.CaseId ??= Guid.NewGuid().ToString("N");
        failure.Source ??= DiagnosticSource.Unknown;
        failure.StartupPhase ??= DiagnosticStartupPhase.Unknown;
        failure.Code ??= DiagnosticErrorCode.UiError;
        failure.Severity ??= DiagnosticSeverity.Fatal;
        failure.Confidence ??= DiagnosticConfidence.Unknown;
        failure.Fingerprint ??= string.Empty;
        failure.RuleId ??= string.Empty;
        failure.ExceptionType ??= string.Empty;
        failure.Message ??= string.Empty;
        failure.Evidence ??= [];
        failure.TechnicalDetails ??= string.Empty;
        failure.AppVersion ??= string.Empty;
        failure.ResourceVersion ??= string.Empty;
        failure.RuleVersion ??= string.Empty;
        failure.Environment ??= new(StringComparer.OrdinalIgnoreCase);
    }

    private void WriteAtomic<T>(string path, T value)
    {
        Directory.CreateDirectory(_directory);
        string tempPath = $"{path}.{Environment.ProcessId}.{Guid.NewGuid():N}.tmp";
        try
        {
            File.WriteAllText(tempPath, JsonSerializer.Serialize(value, _jsonOptions));
            File.Move(tempPath, path, true);
        }
        finally
        {
            try
            {
                File.Delete(tempPath);
            }
            catch
            {
                // Best effort cleanup after a failed atomic replace.
            }
        }
    }
}
