// <copyright file="DiagnosticRuleEngine.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using MaaWpfGui.Models.Diagnostics;

namespace MaaWpfGui.Services.Diagnostics;

internal static class DiagnosticRuleEngine
{
    private const string EmbeddedRuleVersion = "wpf-2026.08.3";
    private static readonly DiagnosticRule[] Rules = CreateRules()
        .OrderByDescending(static rule => rule.Priority)
        .ToArray();

    internal static DiagnosticFailure Classify(DiagnosticInput input, string appVersion = "", string resourceVersion = "")
    {
        var rule = Rules.FirstOrDefault(x => Matches(x, input))
            ?? CreateUnknownRule(input.Source);

        var normalized = $"{rule.Code}|{input.ExceptionType}|{FormatHResult(input.HResult)}|{FirstStableDetail(input)}";
        var fingerprintBytes = SHA256.HashData(Encoding.UTF8.GetBytes(normalized));
        var failure = new DiagnosticFailure {
            Source = input.Source,
            StartupPhase = input.StartupPhase,
            Severity = input.Severity,
            Code = rule.Code,
            Confidence = rule.Confidence,
            Fingerprint = Convert.ToHexString(fingerprintBytes)[..16],
            RuleId = rule.Id,
            ExceptionType = input.ExceptionType,
            HResult = input.HResult,
            Message = input.Message,
            TechnicalDetails = input.Details,
            AppVersion = appVersion,
            ResourceVersion = resourceVersion,
            RuleVersion = EmbeddedRuleVersion,
        };

        if (input.HResult is not null)
        {
            failure.Evidence.Add(new("hresult", FormatHResult(input.HResult)));
        }

        if (!string.IsNullOrWhiteSpace(input.ExceptionType))
        {
            failure.Evidence.Add(new("exceptionType", input.ExceptionType));
        }

        foreach (var module in input.Modules.Take(8))
        {
            failure.Evidence.Add(new("module", Path.GetFileName(module)));
        }

        return failure;
    }

    private static bool Matches(DiagnosticRule rule, DiagnosticInput input)
    {
        if (rule.Sources.Count > 0 && !rule.Sources.Any(x => EqualsIgnoreCase(x, input.Source)))
        {
            return false;
        }

        if (rule.ExceptionTypes.Count > 0 && !rule.ExceptionTypes.Any(x => ContainsIgnoreCase(input.ExceptionType, x)))
        {
            return false;
        }

        if (rule.HResults.Count > 0 && !rule.HResults.Any(x => EqualsIgnoreCase(x, FormatHResult(input.HResult))))
        {
            return false;
        }

        if (rule.DetailContains.Count > 0 && !rule.DetailContains.Any(x => ContainsIgnoreCase(input.Details, x) || ContainsIgnoreCase(input.Message, x)))
        {
            return false;
        }

        if (rule.ModuleContains.Count > 0 && !rule.ModuleContains.Any(x => input.Modules.Any(m => ContainsIgnoreCase(m, x))))
        {
            return false;
        }

        return rule.ExceptionTypes.Count + rule.HResults.Count + rule.DetailContains.Count + rule.ModuleContains.Count > 0;
    }

    private static DiagnosticRule[] CreateRules() =>
        [
            Rule("missing-core", 1000, DiagnosticErrorCode.StartupPackageMissing, DiagnosticConfidence.High, details: ["MaaCore.dll not found", "resource folder not found"]),
            Rule("missing-vcpp", 990, DiagnosticErrorCode.StartupDependencyMissing, DiagnosticConfidence.High, exceptionTypes: ["DllNotFoundException"], details: ["MaaCore", "VCRUNTIME", "MSVCP"]),
            Rule("dwm-disabled", 980, DiagnosticErrorCode.DwmUnavailable, DiagnosticConfidence.High, hresults: ["0x80263001"]),
            Rule("dwm-disabled-text", 979, DiagnosticErrorCode.DwmUnavailable, DiagnosticConfidence.High, details: ["Desktop composition is disabled"]),
            Rule("gpu-inference-device-lost", 977, DiagnosticErrorCode.GpuInferenceFailed, DiagnosticConfidence.High, hresults: ["0x887A0005", "0x887A0006", "0x887A0007"], moduleContains: ["onnxruntime", "DirectML"]),
            Rule("gpu-inference-device-lost-text", 976, DiagnosticErrorCode.GpuInferenceFailed, DiagnosticConfidence.High, details: ["DXGI_ERROR_DEVICE_REMOVED", "DXGI_ERROR_DEVICE_RESET", "DXGI_ERROR_DEVICE_HUNG", "0x887A0005", "0x887A0006", "0x887A0007"], moduleContains: ["onnxruntime", "DirectML"]),
            Rule("graphics-device-lost", 975, DiagnosticErrorCode.GraphicsDeviceLost, DiagnosticConfidence.High, hresults: ["0x887A0005", "0x887A0006", "0x887A0007"]),
            Rule("graphics-device-lost-text", 974, DiagnosticErrorCode.GraphicsDeviceLost, DiagnosticConfidence.High, details: ["DXGI_ERROR_DEVICE_REMOVED", "DXGI_ERROR_DEVICE_RESET", "DXGI_ERROR_DEVICE_HUNG", "0x887A0005", "0x887A0006", "0x887A0007"]),
            Rule("injected-module", 960, DiagnosticErrorCode.InjectedModuleConflict, DiagnosticConfidence.High, details: ["NahimicOSD", "AudioDevProps2", "GTII-OSD64", "GTIII-OSD64"]),
            Rule("gpu-inference-provider", 910, DiagnosticErrorCode.GpuInferenceFailed, DiagnosticConfidence.High, details: ["DmlExecutionProvider", "OrtSessionOptionsAppendExecutionProvider_DML"]),
            Rule("gpu-inference", 900, DiagnosticErrorCode.GpuInferenceFailed, DiagnosticConfidence.Medium, details: ["DirectML", "DmlExecutionProvider", "OrtSessionOptionsAppendExecutionProvider_DML", "onnxruntime", "D3D12"], moduleContains: ["MaaCore", "onnxruntime", "DirectML"]),
            Rule("wpf-render", 850, DiagnosticErrorCode.GraphicsRenderingFailed, DiagnosticConfidence.Medium, details: ["System.Windows.Media", "MilCore", "UCEERR", "D3DImage"]),
            Rule("native-crash", 300, DiagnosticErrorCode.NativeCrash, DiagnosticConfidence.Unknown, sources: [DiagnosticSource.NativeCrash], details: ["FATAL ERROR", "UNHANDLED EXCEPTION", "SIGNAL"]),
            Rule("managed-crash", 200, DiagnosticErrorCode.ManagedCrash, DiagnosticConfidence.Unknown, sources: [DiagnosticSource.Dispatcher, DiagnosticSource.AppDomain], details: ["Exception"]),
        ];

    private static DiagnosticRule Rule(
        string id,
        int priority,
        string code,
        string confidence,
        string[]? sources = null,
        string[]? exceptionTypes = null,
        string[]? hresults = null,
        string[]? details = null,
        string[]? moduleContains = null) => new() {
            Id = id,
            Priority = priority,
            Code = code,
            Confidence = confidence,
            Sources = sources ?? [],
            ExceptionTypes = exceptionTypes ?? [],
            HResults = hresults ?? [],
            DetailContains = details ?? [],
            ModuleContains = moduleContains ?? [],
        };

    private static DiagnosticRule CreateUnknownRule(string source) => new() {
        Id = "unknown",
        Code = source switch {
            DiagnosticSource.Startup => DiagnosticErrorCode.StartupUnknown,
            DiagnosticSource.NativeCrash => DiagnosticErrorCode.NativeCrash,
            _ => DiagnosticErrorCode.ManagedCrash,
        },
        Confidence = DiagnosticConfidence.Unknown,
    };

    private static string FirstStableDetail(DiagnosticInput input)
    {
        string details = string.Equals(input.Source, DiagnosticSource.NativeCrash, StringComparison.Ordinal)
            ? DiagnosticNativeCrashLog.GetFingerprintDetail(input.Details)
            : input.Details;
        if (string.IsNullOrWhiteSpace(details))
        {
            return string.Empty;
        }

        string[] lines = details.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries);
        var line = lines.FirstOrDefault(static value =>
                !value.Contains("FATAL ERROR", StringComparison.OrdinalIgnoreCase) &&
                !value.All(static character => character is '=' or '-' or ' '))
            ?? lines.FirstOrDefault()
            ?? details;
        return line.Length <= 240 ? line : line[..240];
    }

    private static string FormatHResult(int? value) => value is null ? string.Empty : $"0x{unchecked((uint)value.Value):X8}";

    private static bool ContainsIgnoreCase(string value, string expected) => value.Contains(expected, StringComparison.OrdinalIgnoreCase);

    private static bool EqualsIgnoreCase(string left, string right) => string.Equals(left, right, StringComparison.OrdinalIgnoreCase);

    private sealed record DiagnosticRule
    {
        internal string Id { get; init; } = string.Empty;

        internal int Priority { get; init; }

        internal IReadOnlyList<string> Sources { get; init; } = [];

        internal IReadOnlyList<string> ExceptionTypes { get; init; } = [];

        internal IReadOnlyList<string> HResults { get; init; } = [];

        internal IReadOnlyList<string> DetailContains { get; init; } = [];

        internal IReadOnlyList<string> ModuleContains { get; init; } = [];

        internal string Code { get; init; } = DiagnosticErrorCode.UiError;

        internal string Confidence { get; init; } = DiagnosticConfidence.Unknown;
    }
}
