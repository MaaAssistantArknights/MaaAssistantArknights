// <copyright file="WpfDiagnosticManager.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.Diagnostics;
using Serilog;
using FailureSource = MaaWpfGui.Models.Diagnostics.DiagnosticSource;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class WpfDiagnosticManager
{
    private static readonly Lazy<WpfDiagnosticManager> LazyInstance = new(static () => new());
    private readonly ILogger _logger = Log.ForContext<WpfDiagnosticManager>();
    private bool _initialized;
    private string _appVersion = string.Empty;
    private string _resourceVersion = string.Empty;

    private WpfDiagnosticManager()
    {
        string diagnosticDataDirectory = Path.Combine(PathsHelper.DataDir, "diagnostics");
        _store = new(diagnosticDataDirectory);
        _reports = new(PathsHelper.BaseDir, PathsHelper.ReportsDir, _store);
        _nativeCrashImporter = new(_store);
    }

    private readonly DiagnosticStateStore _store;

    private readonly DiagnosticReportService _reports;

    private readonly DiagnosticNativeCrashImporter _nativeCrashImporter;

    internal static WpfDiagnosticManager Instance => LazyInstance.Value;

    internal string StartupPhase { get; private set; } = DiagnosticStartupPhase.ProcessStarted;

    internal void Initialize()
    {
        if (_initialized)
        {
            return;
        }

        _initialized = true;
        _appVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "unknown";
        try
        {
            string resourceVersionFile = Path.Combine(PathsHelper.ResourceDir, "version.json");
            _resourceVersion = File.Exists(resourceVersionFile)
                ? File.GetLastWriteTimeUtc(resourceVersionFile).ToString("O")
                : string.Empty;
        }
        catch
        {
            _resourceVersion = string.Empty;
        }

        try
        {
            AppDomain.CurrentDomain.UnhandledException += OnAppDomainUnhandledException;
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to attach diagnostic exception listeners");
        }
    }

    internal void MarkPhase(string phase)
    {
        StartupPhase = phase;
    }

    internal DiagnosticStoreWriteResult RecordException(
        Exception exception,
        string source,
        bool makePending = true,
        string severity = DiagnosticSeverity.Fatal)
    {
        DiagnosticFailure failure;
        try
        {
            var details = exception.ToString();
            var input = new DiagnosticInput {
                Source = source,
                StartupPhase = StartupPhase,
                Severity = severity,
                ExceptionType = exception.GetType().FullName ?? exception.GetType().Name,
                HResult = exception.HResult,
                Message = exception.Message,
                Details = details,
                Modules = DiagnosticNativeCrashLog.GetRelevantModules(details),
            };
            failure = DiagnosticRuleEngine.Classify(input, _appVersion, _resourceVersion);
        }
        catch (Exception diagnosticException)
        {
            _logger.Warning(diagnosticException, "Diagnostic classification failed; recording a minimal fallback");
            failure = new() {
                Source = source,
                StartupPhase = StartupPhase,
                Severity = severity,
                Code = DiagnosticErrorCode.UiError,
                Confidence = DiagnosticConfidence.Unknown,
                Fingerprint = $"{exception.GetType().FullName}:{exception.HResult:X8}",
                RuleId = "diagnostic-fallback",
                ExceptionType = exception.GetType().FullName ?? exception.GetType().Name,
                HResult = exception.HResult,
                Message = exception.Message,
                TechnicalDetails = exception.ToString(),
                AppVersion = _appVersion,
                ResourceVersion = _resourceVersion,
                RuleVersion = "fallback",
            };
        }

        TryAttachFailureProcessSnapshot(failure);
        if (!string.Equals(severity, DiagnosticSeverity.Fatal, StringComparison.Ordinal))
        {
            TryAttachEnvironment(failure);
        }

        return _store.RecordFailure(failure, makePending);
    }

    internal IReadOnlyList<DiagnosticFailure> ImportNativeCrashLogs()
    {
        var failures = new List<DiagnosticFailure>();
        var candidates = GetNativeCrashLogCandidates();
        foreach (var candidate in candidates.DistinctBy(static item => item.Path, StringComparer.OrdinalIgnoreCase))
        {
            string path = candidate.Path;
            if (!File.Exists(path))
            {
                continue;
            }

            try
            {
                string details = DiagnosticNativeCrashLog.ReadBounded(path);
                if (string.IsNullOrWhiteSpace(details))
                {
                    continue;
                }

                string latestRecord = DiagnosticNativeCrashLog.GetLatestRecord(details);
                var failure = DiagnosticRuleEngine.Classify(new() {
                    Source = FailureSource.NativeCrash,
                    StartupPhase = DiagnosticStartupPhase.PreviousProcess,
                    Severity = DiagnosticSeverity.Fatal,
                    ExceptionType = "NativeCrash",
                    Message = DiagnosticNativeCrashLog.GetMessage(latestRecord),
                    Details = latestRecord,
                    Modules = DiagnosticNativeCrashLog.GetRelevantModules(latestRecord),
                }, _appVersion, _resourceVersion);
                failure.TimestampUtc = File.GetLastWriteTimeUtc(path);
                TryAttachEnvironment(failure);
                var importedFailure = _nativeCrashImporter.Import(path, candidate.SourceKind, latestRecord, failure);
                if (importedFailure is null)
                {
                    continue;
                }

                failures.Add(importedFailure);
            }
            catch (Exception ex)
            {
                _logger.Warning(ex, "Failed to import native crash log {CrashLog}", path);
            }
        }

        return failures;
    }

    private static IReadOnlyList<(string Path, string SourceKind)> GetNativeCrashLogCandidates()
    {
        var candidates = new List<(string Path, string SourceKind)>
        {
            (Path.Combine(PathsHelper.BaseDir, "crash.log"), "root"),
            (Path.Combine(PathsHelper.DebugDir, "crash.log"), "debug"),
        };
        AddFallbacks(PathsHelper.BaseDir, "root");
        AddFallbacks(PathsHelper.DebugDir, "debug");
        return candidates.DistinctBy(static item => item.Path, StringComparer.OrdinalIgnoreCase).ToArray();

        void AddFallbacks(string directory, string sourceKind)
        {
            try
            {
                candidates.AddRange(Directory.EnumerateFiles(directory, "crash-*.log", SearchOption.TopDirectoryOnly)
                    .OrderByDescending(File.GetLastWriteTimeUtc)
                    .Take(10)
                    .Select(path => (path, $"{sourceKind}-fallback")));
            }
            catch
            {
                // A fallback discovery failure must not block normal startup.
            }
        }
    }

    internal DiagnosticFailure? TryConsumePendingFailure()
    {
        try
        {
            var pending = _store.TryConsumePending();
            if (pending is null)
            {
                return null;
            }

            TryAttachEnvironment(pending);
            _store.UpdateFailure(pending);
            return pending;
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to consume pending diagnostic failure");
            return null;
        }
    }

    internal DiagnosticFailure? GetLatestFailure() => _store.GetLatestFailure();

    internal Task<DiagnosticReportResult> BuildReportAsync(
        DiagnosticFailure failure,
        DiagnosticReportOptions? options = null) => _reports.BuildAsync(failure, options);

    internal void MarkMainWindowShown()
    {
        MarkPhase(DiagnosticStartupPhase.MainWindowShown);
    }

    private void TryAttachEnvironment(DiagnosticFailure failure)
    {
        try
        {
            foreach (var pair in DiagnosticEnvironmentCollector.Collect())
            {
                failure.Environment[pair.Key] = pair.Value;
            }
        }
        catch (Exception ex)
        {
            failure.Environment["environmentProbe"] = $"unavailable:{ex.GetType().Name}";
        }
    }

    private static void TryAttachFailureProcessSnapshot(DiagnosticFailure failure)
    {
        try
        {
            using var process = Process.GetCurrentProcess();
            failure.Environment["failureSnapshot.capturedAtUtc"] = DateTimeOffset.UtcNow.ToString("O");
            failure.Environment["failureSnapshot.processStartTimeUtc"] = process.StartTime.ToUniversalTime().ToString("O");
            failure.Environment["failureSnapshot.workingSetBytes"] = process.WorkingSet64.ToString();
            failure.Environment["failureSnapshot.privateMemoryBytes"] = process.PrivateMemorySize64.ToString();
            failure.Environment["failureSnapshot.handleCount"] = process.HandleCount.ToString();
            failure.Environment["failureSnapshot.threadCount"] = process.Threads.Count.ToString();
            failure.Environment["failureSnapshot.totalManagedMemoryBytes"] = GC.GetTotalMemory(forceFullCollection: false).ToString();
        }
        catch (Exception ex)
        {
            failure.Environment["failureSnapshot.probe"] = $"unavailable:{ex.GetType().Name}";
        }
    }

    private void OnAppDomainUnhandledException(object? sender, UnhandledExceptionEventArgs args)
    {
        if (args.ExceptionObject is Exception exception)
        {
            try
            {
                RecordException(exception, FailureSource.AppDomain);
            }
            catch
            {
                // Never throw from a last-chance exception handler.
            }
        }
    }
}
