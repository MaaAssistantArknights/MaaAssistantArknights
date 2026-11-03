// <copyright file="UpdateMirrorSelection.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;

namespace MaaWpfGui.Helper;

public static class UpdateMirrorSelection
{
    public static int SelectFastest(IReadOnlyList<Uri> uris, IReadOnlyList<(double Latency, bool UsesProxy)> probes)
    {
        if (uris.Count != probes.Count)
        {
            throw new ArgumentException("Each mirror must have exactly one probe result.", nameof(probes));
        }

        var selected = -1;
        var bestScore = double.PositiveInfinity;
        for (int i = 0; i < probes.Count; i++)
        {
            var (latency, usesProxy) = probes[i];
            if (latency < 0 || !double.IsFinite(latency))
            {
                continue;
            }

            var host = uris[i].Host;
            var isInChina = host.Equals("s3.maa-org.net", StringComparison.OrdinalIgnoreCase)
                || host.Equals("maa-ota.annangela.cn", StringComparison.OrdinalIgnoreCase);
            var score = latency + (usesProxy && isInChina ? 6480 : 0);
            if (score < bestScore)
            {
                selected = i;
                bestScore = score;
            }
        }

        return selected;
    }
}
