// <copyright file="DiagnosticContracts.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1402, SA1636, SA1649
#nullable enable

using System;
using System.Collections.Generic;

namespace MaaWpfGui.Models.Diagnostics;

/// <summary>
/// Stable error codes shared by the WPF diagnostic protocol. Existing MAAUnified
/// codes are reused where they already describe the same failure.
/// </summary>
internal static class DiagnosticErrorCode
{
    public const string UiError = "UiError";
    public const string StartupDependencyMissing = "WpfStartupDependencyMissing";
    public const string StartupPackageMissing = "WpfStartupPackageMissing";
    public const string DwmUnavailable = "WpfDwmUnavailable";
    public const string GraphicsDeviceLost = "WpfGraphicsDeviceLost";
    public const string GraphicsRenderingFailed = "WpfGraphicsRenderingFailed";
    public const string GpuInferenceFailed = "WpfGpuInferenceFailed";
    public const string InjectedModuleConflict = "WpfInjectedModuleConflict";
    public const string ManagedCrash = "WpfManagedCrash";
    public const string NativeCrash = "WpfNativeCrash";
    public const string StartupUnknown = "WpfStartupUnknown";
}

internal static class DiagnosticSource
{
    public const string Startup = "startup";
    public const string Dispatcher = "dispatcher";
    public const string AppDomain = "appDomain";
    public const string NativeCrash = "nativeCrash";
    public const string Unknown = "unknown";
}

internal static class DiagnosticSeverity
{
    public const string Fatal = "fatal";
    public const string Error = "error";
}

internal static class DiagnosticConfidence
{
    public const string High = "high";
    public const string Medium = "medium";
    public const string Unknown = "unknown";
}

internal static class DiagnosticStartupPhase
{
    public const string ProcessStarted = "processStarted";
    public const string ConfigurationLoaded = "configurationLoaded";
    public const string PreflightPassed = "preflightPassed";
    public const string StartupFailed = "startupFailed";
    public const string PreviousProcess = "previousProcess";
    public const string MainWindowShown = "mainWindowShown";
    public const string Unknown = "unknown";
}

internal sealed class DiagnosticFailure
{
    public int SchemaVersion { get; set; } = 1;

    public string CaseId { get; set; } = Guid.NewGuid().ToString("N");

    public DateTimeOffset TimestampUtc { get; set; } = DateTimeOffset.UtcNow;

    public int ProcessId { get; set; } = System.Environment.ProcessId;

    public string Source { get; set; } = DiagnosticSource.Dispatcher;

    public string StartupPhase { get; set; } = DiagnosticStartupPhase.Unknown;

    public string Code { get; set; } = DiagnosticErrorCode.UiError;

    public string Severity { get; set; } = DiagnosticSeverity.Fatal;

    public string Confidence { get; set; } = DiagnosticConfidence.Unknown;

    public string Fingerprint { get; set; } = string.Empty;

    public string RuleId { get; set; } = string.Empty;

    public string ExceptionType { get; set; } = string.Empty;

    public int? HResult { get; set; }

    public string Message { get; set; } = string.Empty;

    public List<DiagnosticEvidence> Evidence { get; set; } = [];

    public string TechnicalDetails { get; set; } = string.Empty;

    public string AppVersion { get; set; } = string.Empty;

    public string ResourceVersion { get; set; } = string.Empty;

    public string RuleVersion { get; set; } = string.Empty;

    public Dictionary<string, string> Environment { get; set; } = new(StringComparer.OrdinalIgnoreCase);

    public int OccurrenceCount { get; set; } = 1;
}

internal sealed record DiagnosticEvidence(string Kind, string Value);

internal sealed class DiagnosticInput
{
    public string Source { get; init; } = DiagnosticSource.Dispatcher;

    public string StartupPhase { get; init; } = DiagnosticStartupPhase.Unknown;

    public string Severity { get; init; } = DiagnosticSeverity.Fatal;

    public string ExceptionType { get; init; } = string.Empty;

    public int? HResult { get; init; }

    public string Message { get; init; } = string.Empty;

    public string Details { get; init; } = string.Empty;

    public IReadOnlyList<string> Modules { get; init; } = [];
}

internal sealed class DiagnosticReportOptions
{
    public bool IncludeConfigurationSummary { get; set; } = true;

    public bool IncludeScreenshots { get; set; } = true;

    public bool IncludeDumps { get; set; } = true;
}

internal sealed record DiagnosticReportResult(string ReportPath, IReadOnlyList<string> AttachmentParts, bool IsMinimal);

internal sealed record DiagnosticStoreWriteResult(
    DiagnosticFailure Failure,
    bool HistoryWritten,
    bool PendingWritten);

internal sealed record DiagnosticAttachmentBuildResult(
    IReadOnlyList<string> TemporaryPartPaths,
    DiagnosticCollectionStatus CollectionStatus);

internal sealed class DiagnosticCollectionStatus
{
    public Dictionary<string, DiagnosticCollectorStatus> Collectors { get; } = new(StringComparer.OrdinalIgnoreCase);

    public DiagnosticAttachmentCategoryStatus Screenshots { get; } = new();

    public DiagnosticAttachmentCategoryStatus Dumps { get; } = new();

    public List<DiagnosticAttachmentStatus> Attachments { get; } = [];
}

internal class DiagnosticCollectorStatus
{
    public string Status { get; set; } = "success";

    public string? ExceptionType { get; set; }

    public string? ExceptionMessage { get; set; }
}

internal sealed class DiagnosticAttachmentCategoryStatus : DiagnosticCollectorStatus
{
    internal DiagnosticAttachmentCategoryStatus()
    {
        Status = "unavailable";
    }

    public bool Requested { get; set; }

    public int CandidatesFound { get; set; }

    public int Included { get; set; }

    public int Failed { get; set; }
}

internal sealed class DiagnosticAttachmentStatus : DiagnosticCollectorStatus
{
    public string EntryName { get; set; } = string.Empty;

    public long Size { get; set; }

    public string? PartFileName { get; set; }
}
