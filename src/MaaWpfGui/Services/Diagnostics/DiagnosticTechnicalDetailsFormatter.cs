// <copyright file="DiagnosticTechnicalDetailsFormatter.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal static class DiagnosticTechnicalDetailsFormatter
{
    public static string Format(DiagnosticFailure failure)
    {
        var lines = new List<string>
        {
            $"Summary: {DescribeFailure(failure.Code)}",
            $"Time: {failure.TimestampUtc.ToLocalTime():yyyy-MM-dd HH:mm:ss zzz}  |  Source: {Compact(failure.Source, 32)} / {Compact(failure.StartupPhase, 32)}  |  Severity: {Compact(failure.Severity, 16)}  |  Occurrences: {failure.OccurrenceCount}",
            $"Code: {Compact(failure.Code, 56)}  |  Confidence: {Compact(failure.Confidence, 16)}  |  Case: {Compact(failure.CaseId, 64)}",
            $"Rule: {Compact(failure.RuleId, 64)} ({Compact(failure.RuleVersion, 32)})  |  Fingerprint: {Compact(failure.Fingerprint, 32)}",
        };

        var exceptionParts = new List<string>();
        AddPart(exceptionParts, "Exception", failure.ExceptionType, 96);
        if (failure.HResult is not null)
        {
            exceptionParts.Add($"HRESULT: 0x{unchecked((uint)failure.HResult.Value):X8}");
        }

        exceptionParts.Add($"PID: {failure.ProcessId}");
        lines.Add(string.Join("  |  ", exceptionParts));

        var buildParts = new List<string>();
        AddPart(buildParts, "MAA", failure.AppVersion, 40);
        AddPart(buildParts, "Resource", failure.ResourceVersion, 48);
        AddEnvironmentPart(buildParts, failure, ".NET", 64, "framework");
        if (buildParts.Count > 0)
        {
            lines.Add($"Build: {string.Join("  |  ", buildParts)}");
        }

        var contextParts = new List<string>();
        AddEnvironmentPart(contextParts, failure, "OS", 80, "os", "os.description");
        AddEnvironmentPart(contextParts, failure, "Arch", 20, "processArchitecture", "process.architecture");
        AddEnvironmentPart(contextParts, failure, "Locale", 20, "locale");
        AddEnvironmentPart(contextParts, failure, "GPU", 80, "gpuPreference", "gpu.0.description");
        AddEnvironmentPart(contextParts, failure, "GPU inference", 8, "gpuInferenceEnabled");
        AddEnvironmentPart(contextParts, failure, "WPF", 24, "wpfRenderMode");
        AddEnvironmentPart(contextParts, failure, "Software render", 8, "softwareRenderingConfigured");
        AddEnvironmentPart(contextParts, failure, "RDP", 8, "remoteDesktopSession");
        if (contextParts.Count > 0)
        {
            lines.Add($"Context: {string.Join("  |  ", contextParts)}");
        }

        var snapshotParts = new List<string>();
        AddByteEnvironmentPart(snapshotParts, failure, "Working set", "failureSnapshot.workingSetBytes");
        AddByteEnvironmentPart(snapshotParts, failure, "Private", "failureSnapshot.privateMemoryBytes");
        AddEnvironmentPart(snapshotParts, failure, "Handles", 10, "failureSnapshot.handleCount");
        AddEnvironmentPart(snapshotParts, failure, "Threads", 10, "failureSnapshot.threadCount");
        if (snapshotParts.Count > 0)
        {
            lines.Add($"Process snapshot: {string.Join("  |  ", snapshotParts)}");
        }

        string evidence = string.Join("; ", (failure.Evidence ?? [])
            .Where(static item => item is not null &&
                                  !string.Equals(item.Kind, "hresult", StringComparison.OrdinalIgnoreCase) &&
                                  !string.Equals(item.Kind, "exceptionType", StringComparison.OrdinalIgnoreCase))
            .Take(4)
            .Select(static item => $"{Compact(item.Kind, 24)}={Compact(item.Value, 72)}"));
        if (!string.IsNullOrWhiteSpace(evidence))
        {
            lines.Add($"Signals: {evidence}");
        }

        string message = Compact(failure.Message, 200);
        string technicalDetails = failure.TechnicalDetails.Trim();
        string firstTechnicalLine = Compact(technicalDetails.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries).FirstOrDefault(), 200);
        bool messageDuplicatesDetails = !string.IsNullOrWhiteSpace(firstTechnicalLine) &&
                                        (firstTechnicalLine.Contains(message, StringComparison.OrdinalIgnoreCase) ||
                                         message.Contains(firstTechnicalLine, StringComparison.OrdinalIgnoreCase));
        if (!string.IsNullOrWhiteSpace(message) && !messageDuplicatesDetails)
        {
            lines.Add($"Message: {message}");
        }

        if (!string.IsNullOrWhiteSpace(technicalDetails))
        {
            lines.Add("--- Original exception details ---");
            lines.Add(technicalDetails);
        }

        return string.Join(Environment.NewLine, lines);
    }

    private static string DescribeFailure(string? code) => code switch
    {
        DiagnosticErrorCode.StartupDependencyMissing => "A required startup dependency could not be loaded.",
        DiagnosticErrorCode.StartupPackageMissing => "A required MAA application file or resource was missing.",
        DiagnosticErrorCode.DwmUnavailable => "Windows desktop composition was unavailable to WPF.",
        DiagnosticErrorCode.GraphicsDeviceLost => "The graphics device was removed, reset, or stopped responding.",
        DiagnosticErrorCode.GraphicsRenderingFailed => "The WPF rendering pipeline reported a failure.",
        DiagnosticErrorCode.GpuInferenceFailed => "GPU inference initialization or execution failed.",
        DiagnosticErrorCode.InjectedModuleConflict => "A known overlay or injected module was present near the failure.",
        DiagnosticErrorCode.NativeCrash => "A native crash record was detected from the previous process.",
        DiagnosticErrorCode.ManagedCrash => "An unhandled managed exception ended or interrupted the previous run.",
        DiagnosticErrorCode.StartupUnknown => "Startup failed without a recognized diagnostic signature.",
        _ => "MAA recorded an unexpected application failure.",
    };

    private static void AddPart(ICollection<string> parts, string label, string? value, int maxLength)
    {
        if (!string.IsNullOrWhiteSpace(value))
        {
            parts.Add($"{label}: {Compact(value, maxLength)}");
        }
    }

    private static void AddEnvironmentPart(ICollection<string> parts, DiagnosticFailure failure, string label, int maxLength, params string[] keys)
    {
        string? value = keys
            .Select(key => failure.Environment?.GetValueOrDefault(key))
            .FirstOrDefault(static item => !string.IsNullOrWhiteSpace(item));
        if (!string.IsNullOrWhiteSpace(value))
        {
            parts.Add($"{label}: {Compact(value, maxLength)}");
        }
    }

    private static void AddByteEnvironmentPart(ICollection<string> parts, DiagnosticFailure failure, string label, string key)
    {
        if (failure.Environment is not null &&
            failure.Environment.TryGetValue(key, out string? value) &&
            long.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out long bytes) &&
            bytes >= 0)
        {
            parts.Add($"{label}: {FormatBytes(bytes)}");
        }
    }

    private static string FormatBytes(long bytes)
    {
        string[] units = ["B", "KiB", "MiB", "GiB", "TiB"];
        double size = bytes;
        int unit = 0;
        while (size >= 1024 && unit < units.Length - 1)
        {
            size /= 1024;
            unit++;
        }

        return $"{size:0.#} {units[unit]}";
    }

    private static string Compact(string? value, int maxLength)
    {
        string compact = (value ?? string.Empty)
            .Replace('\r', ' ')
            .Replace('\n', ' ')
            .Trim();
        while (compact.Contains("  ", StringComparison.Ordinal))
        {
            compact = compact.Replace("  ", " ", StringComparison.Ordinal);
        }

        return compact.Length <= maxLength ? compact : $"{compact[..Math.Max(1, maxLength - 1)]}…";
    }
}
