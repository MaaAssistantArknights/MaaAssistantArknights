// <copyright file="MuMuArmConnection.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text.Json;
using System.Text.RegularExpressions;
using Serilog;

namespace MaaWpfGui.Helper;

internal static class MuMuArmConnection
{
    private static readonly ILogger _logger = Log.ForContext(typeof(MuMuArmConnection));

    // MuMu ARM may quote its entire argument string, not just individual values.
    private static readonly Regex _instanceIndexRegex = new(
        "(?:^|[\\s\"])--VmIndex\\s+(?<index>[0-9]+)(?=$|[\\s\"])",
        RegexOptions.CultureInvariant,
        TimeSpan.FromSeconds(1));

    public static string? GetAddress(string? processPath, string? commandLine)
    {
        if (string.IsNullOrEmpty(processPath) || string.IsNullOrEmpty(commandLine))
        {
            return null;
        }

        try
        {
            var match = _instanceIndexRegex.Match(commandLine);
            if (!match.Success || !int.TryParse(match.Groups["index"].Value, NumberStyles.None, CultureInfo.InvariantCulture, out var index))
            {
                return null;
            }

            var installDirectory = Directory.GetParent(processPath)?.Parent;
            if (installDirectory == null)
            {
                return null;
            }

            // Read only the running instance. Other VM directories may retain stale addresses.
            var statePath = Path.Combine(installDirectory.FullName, "vms", $"vm{index.ToString(CultureInfo.InvariantCulture)}.madoa", "misc", "state.json");
            using var stream = new FileStream(statePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            using var state = JsonDocument.Parse(stream);
            var root = state.RootElement;
            if (root.ValueKind != JsonValueKind.Object ||
                !root.TryGetProperty("AdbHost", out var host) || host.ValueKind != JsonValueKind.String ||
                !IPAddress.TryParse(host.GetString(), out var address) ||
                !root.TryGetProperty("AdbPort", out var port) || port.ValueKind != JsonValueKind.Number ||
                !port.TryGetInt32(out var portNumber) || portNumber is <= 0 or > 65535 ||
                address.Equals(IPAddress.Any) || address.Equals(IPAddress.IPv6Any))
            {
                return null;
            }

            var formattedHost = address.AddressFamily == AddressFamily.InterNetworkV6 ? $"[{address}]" : address.ToString();
            return $"{formattedHost}:{portNumber.ToString(CultureInfo.InvariantCulture)}";
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or JsonException or ArgumentException or RegexMatchTimeoutException)
        {
            _logger.Warning(e, "Failed to read MuMu ARM connection state for {ProcessPath}", processPath);
            return null;
        }
    }
}
