// <copyright file="DiagnosticPathSafety.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Services.Diagnostics;

internal static class DiagnosticPathSafety
{
    internal static string FileNamePart(string? value, string fallback, int maxLength)
    {
        char[] invalid = Path.GetInvalidFileNameChars();
        string sanitized = new((value ?? string.Empty)
            .Where(character => !invalid.Contains(character) &&
                                character != Path.DirectorySeparatorChar &&
                                character != Path.AltDirectorySeparatorChar &&
                                !char.IsControl(character))
            .Select(character => char.IsWhiteSpace(character) ? '-' : character)
            .ToArray());
        sanitized = sanitized.Trim('.', '-', '_');
        if (string.IsNullOrWhiteSpace(sanitized))
        {
            sanitized = fallback;
        }

        return sanitized.Length <= maxLength ? sanitized : sanitized[..maxLength];
    }
}
