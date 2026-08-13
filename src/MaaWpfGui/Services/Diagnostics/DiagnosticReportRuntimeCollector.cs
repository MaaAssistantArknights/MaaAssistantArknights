// <copyright file="DiagnosticReportRuntimeCollector.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

namespace MaaWpfGui.Services.Diagnostics;

internal sealed class DiagnosticReportRuntimeCollector
{
    private readonly string _baseDirectory;

    internal DiagnosticReportRuntimeCollector(string baseDirectory)
    {
        _baseDirectory = baseDirectory;
    }

    internal static Dictionary<string, object?> CollectProcessRuntime()
    {
        var result = new Dictionary<string, object?>
        {
            ["capturedAtUtc"] = DateTimeOffset.UtcNow,
            ["frameworkDescription"] = RuntimeInformation.FrameworkDescription,
            ["runtimeIdentifier"] = RuntimeInformation.RuntimeIdentifier,
            ["osDescription"] = RuntimeInformation.OSDescription,
            ["osArchitecture"] = RuntimeInformation.OSArchitecture.ToString(),
            ["processArchitecture"] = RuntimeInformation.ProcessArchitecture.ToString(),
            ["environmentVersion"] = Environment.Version.ToString(),
            ["processorCount"] = Environment.ProcessorCount,
            ["is64BitOperatingSystem"] = Environment.Is64BitOperatingSystem,
            ["is64BitProcess"] = Environment.Is64BitProcess,
            ["isServerGc"] = GCSettings.IsServerGC,
            ["gcLatencyMode"] = GCSettings.LatencyMode.ToString(),
            ["currentCulture"] = CultureInfo.CurrentCulture.Name,
            ["currentUiCulture"] = CultureInfo.CurrentUICulture.Name,
            ["timeZone"] = TimeZoneInfo.Local.Id,
            ["commandLine"] = Environment.CommandLine,
            ["baseDirectory"] = AppContext.BaseDirectory,
            ["currentDirectory"] = Environment.CurrentDirectory,
            ["systemUptime"] = TimeSpan.FromMilliseconds(Environment.TickCount64).ToString(),
        };

        try
        {
            using var process = Process.GetCurrentProcess();
            result["processId"] = process.Id;
            result["processName"] = process.ProcessName;
            result["processStartTimeUtc"] = process.StartTime.ToUniversalTime();
            result["processUptime"] = DateTime.UtcNow - process.StartTime.ToUniversalTime();
            result["workingSetBytes"] = process.WorkingSet64;
            result["privateMemoryBytes"] = process.PrivateMemorySize64;
            result["virtualMemoryBytes"] = process.VirtualMemorySize64;
            result["handleCount"] = process.HandleCount;
            result["threadCount"] = process.Threads.Count;
        }
        catch (Exception ex)
        {
            result["processProbe"] = $"unavailable:{ex.GetType().Name}";
        }

        return result;
    }

    internal static IReadOnlyList<Dictionary<string, object?>> CollectLoadedModules()
    {
        var result = new List<Dictionary<string, object?>>();
        try
        {
            using var process = Process.GetCurrentProcess();
            foreach (ProcessModule module in process.Modules)
            {
                try
                {
                    var version = module.FileVersionInfo;
                    result.Add(new()
                    {
                        ["moduleName"] = module.ModuleName,
                        ["fileName"] = module.FileName,
                        ["fileVersion"] = version.FileVersion,
                        ["productVersion"] = version.ProductVersion,
                        ["companyName"] = version.CompanyName,
                        ["fileDescription"] = version.FileDescription,
                        ["moduleMemorySize"] = module.ModuleMemorySize,
                        ["baseAddress"] = $"0x{module.BaseAddress.ToInt64():X}",
                    });
                }
                catch (Exception ex)
                {
                    result.Add(new() { ["moduleName"] = module.ModuleName, ["probe"] = $"unavailable:{ex.GetType().Name}" });
                }
            }
        }
        catch (Exception ex)
        {
            result.Add(new() { ["moduleEnumeration"] = $"unavailable:{ex.GetType().Name}" });
        }

        return result.OrderBy(static item => item.GetValueOrDefault("moduleName")?.ToString(), StringComparer.OrdinalIgnoreCase).ToList();
    }

    internal IReadOnlyList<Dictionary<string, object?>> CollectApplicationFiles()
    {
        var result = new List<Dictionary<string, object?>>();
        string[] fileNames;
        try
        {
            fileNames = Directory.Exists(_baseDirectory)
                ? Directory.EnumerateFiles(_baseDirectory, "*", SearchOption.TopDirectoryOnly)
                    .Where(IsRelevantApplicationFile)
                    .Select(Path.GetFileName)
                    .Where(static name => !string.IsNullOrWhiteSpace(name))
                    .Cast<string>()
                    .OrderBy(static name => name, StringComparer.OrdinalIgnoreCase)
                    .ToArray()
                : [];
        }
        catch (Exception ex)
        {
            return [new() { ["applicationFileEnumeration"] = $"unavailable:{ex.GetType().Name}" }];
        }

        foreach (string fileName in fileNames)
        {
            string path = Path.Combine(_baseDirectory, fileName);
            var item = new Dictionary<string, object?> { ["name"] = fileName, ["exists"] = File.Exists(path) };
            if (File.Exists(path))
            {
                try
                {
                    var info = new FileInfo(path);
                    var version = FileVersionInfo.GetVersionInfo(path);
                    using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                    item["length"] = info.Length;
                    item["lastWriteTimeUtc"] = info.LastWriteTimeUtc;
                    item["fileVersion"] = version.FileVersion;
                    item["productVersion"] = version.ProductVersion;
                    item["sha256"] = Convert.ToHexString(SHA256.HashData(stream));
                }
                catch (Exception ex)
                {
                    item["probe"] = $"unavailable:{ex.GetType().Name}";
                }
            }

            result.Add(item);
        }

        return result;
    }

    private static bool IsRelevantApplicationFile(string path)
    {
        string fileName = Path.GetFileName(path);
        string extension = Path.GetExtension(fileName);
        if (!extension.Equals(".dll", StringComparison.OrdinalIgnoreCase) &&
            !extension.Equals(".exe", StringComparison.OrdinalIgnoreCase) &&
            !extension.Equals(".json", StringComparison.OrdinalIgnoreCase) &&
            !extension.Equals(".pdb", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        return fileName.StartsWith("MAA", StringComparison.OrdinalIgnoreCase) ||
               fileName.StartsWith("Maa", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("DirectML", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("onnxruntime", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("opencv", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("fastdeploy", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("vcruntime", StringComparison.OrdinalIgnoreCase) ||
               fileName.Contains("D3DCompiler", StringComparison.OrdinalIgnoreCase) ||
               fileName.Equals("libloader.dll", StringComparison.OrdinalIgnoreCase);
    }
}
