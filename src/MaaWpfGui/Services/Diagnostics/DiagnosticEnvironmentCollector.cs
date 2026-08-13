// <copyright file="DiagnosticEnvironmentCollector.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using System.Windows.Media;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Services.Diagnostics;

internal static class DiagnosticEnvironmentCollector
{
    public static Dictionary<string, string> Collect()
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["os"] = RuntimeInformation.OSDescription,
            ["osArchitecture"] = RuntimeInformation.OSArchitecture.ToString(),
            ["processArchitecture"] = RuntimeInformation.ProcessArchitecture.ToString(),
            ["framework"] = RuntimeInformation.FrameworkDescription,
            ["wpfRenderMode"] = RenderOptions.ProcessRenderMode.ToString(),
            ["maaCorePresent"] = File.Exists(Path.Combine(PathsHelper.BaseDir, "MaaCore.dll")).ToString(),
            ["resourcePresent"] = Directory.Exists(PathsHelper.ResourceDir).ToString(),
            ["sessionName"] = Environment.GetEnvironmentVariable("SESSIONNAME") ?? string.Empty,
            ["remoteDesktopSession"] = (Environment.GetEnvironmentVariable("SESSIONNAME")?.StartsWith("RDP-", StringComparison.OrdinalIgnoreCase) == true).ToString(),
        };

        TryAddConfiguration(result);
        return result;
    }

    private static void TryAddConfiguration(IDictionary<string, string> result)
    {
        try
        {
            result["gpuInferenceEnabled"] = ConfigFactory.CurrentConfig.Gui.Performance.UseGpu.ToString();
            result["gpuPreference"] = ConfigFactory.CurrentConfig.Gui.Performance.GpuDescription;
            result["softwareRenderingConfigured"] = ConfigFactory.Root.Gui.IgnoreBadModulesAndUseSoftwareRendering.ToString();
            result["locale"] = ConfigFactory.Root.Gui.Localization;
        }
        catch (Exception ex)
        {
            result["configurationProbe"] = $"unavailable:{ex.GetType().Name}";
        }
    }
}
