// <copyright file="DiagnosticNativeCrashLog.cs" company="MaaAssistantArknights">
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
using System.Text;

namespace MaaWpfGui.Services.Diagnostics;

internal static class DiagnosticNativeCrashLog
{
    private const int SectionByteLimit = 512 * 1024;
    private const string TruncationMarker = "[middle of native crash log omitted by diagnostics]";
    private static readonly string[] KnownModules =
    [
        "MaaCore",
        "DirectML",
        "onnxruntime",
        "MilCore",
        "NahimicOSD",
        "AudioDevProps2",
        "GTII-OSD64",
        "GTIII-OSD64",
    ];

    internal static string ReadBounded(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        long length = stream.Length;
        if (length <= SectionByteLimit * 2L)
        {
            return Decode(ReadAtMost(stream, checked((int)length)));
        }

        byte[] head = ReadAtMost(stream, SectionByteLimit);
        stream.Seek(Math.Max(head.Length, length - SectionByteLimit), SeekOrigin.Begin);
        byte[] tail = ReadAtMost(stream, SectionByteLimit);
        return $"{Decode(head)}{Environment.NewLine}{TruncationMarker}{Environment.NewLine}{Decode(tail)}";
    }

    internal static string GetLatestRecord(string details)
    {
        const string header = "=== FATAL ERROR ===";
        int index = details.LastIndexOf(header, StringComparison.OrdinalIgnoreCase);
        return index >= 0 ? details[index..] : details;
    }

    internal static string GetMessage(string details)
    {
        string[] lines = details.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries);
        string? reason = lines.LastOrDefault(static line => line.StartsWith("Reason:", StringComparison.OrdinalIgnoreCase));
        string? detail = lines.LastOrDefault(static line => line.StartsWith("Detail:", StringComparison.OrdinalIgnoreCase));
        string reasonText = ValueAfterColon(reason);
        string detailText = ValueAfterColon(detail);
        string message = string.IsNullOrWhiteSpace(detailText)
            ? reasonText
            : $"{reasonText}: {detailText}".TrimStart(' ', ':');
        if (string.IsNullOrWhiteSpace(message))
        {
            return "Native crash record found";
        }

        return message.Length <= 500 ? message : message[..500];
    }

    internal static string[] GetRelevantModules(string details) => KnownModules
        .Where(module => details.Contains(module, StringComparison.OrdinalIgnoreCase))
        .ToArray();

    internal static string GetFingerprintDetail(string details)
    {
        string[] keys = ["ExceptionCode", "FaultingModule", "ModuleOffset", "AccessType"];
        var parts = new List<string>(keys.Length);
        foreach (string key in keys)
        {
            string value = GetField(details, key);
            if (key == "FaultingModule")
            {
                value = value.Replace('\\', '/').Split('/').LastOrDefault() ?? value;
            }

            if (!string.IsNullOrWhiteSpace(value))
            {
                parts.Add($"{key}={value}");
            }
        }

        return parts.Count > 0 ? string.Join('|', parts) : GetMessage(details);
    }

    private static byte[] ReadAtMost(Stream stream, int byteLimit)
    {
        var buffer = new byte[byteLimit];
        int total = 0;
        while (total < buffer.Length)
        {
            int count = stream.Read(buffer, total, buffer.Length - total);
            if (count == 0)
            {
                break;
            }

            total += count;
        }

        return total == buffer.Length ? buffer : buffer[..total];
    }

    private static string Decode(byte[] bytes) => Encoding.UTF8.GetString(bytes);

    private static string ValueAfterColon(string? value)
    {
        int index = value?.IndexOf(':') ?? -1;
        return index >= 0 ? value![(index + 1)..].Trim() : string.Empty;
    }

    private static string GetField(string details, string key)
    {
        string prefix = $"{key}:";
        int start = details.LastIndexOf(prefix, StringComparison.OrdinalIgnoreCase);
        if (start < 0)
        {
            return string.Empty;
        }

        start += prefix.Length;
        int end = details.IndexOfAny([';', '\r', '\n'], start);
        return details[start..(end < 0 ? details.Length : end)].Trim();
    }
}
