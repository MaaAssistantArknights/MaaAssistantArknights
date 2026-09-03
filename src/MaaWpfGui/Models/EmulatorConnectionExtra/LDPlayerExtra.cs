// <copyright file="LDPlayerExtra.cs" company="MaaAssistantArknights">
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

#nullable enable
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class LDPlayerExtra() : ExtraConfig
{
    private static readonly ILogger _logger = Log.ForContext<LDPlayerExtra>();

    public bool Enable
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.IsEnabled = value;
            if (value)
            {
                AutoDetectEmulatorPath();
            }

            Instances.AsstProxy.Connected = false;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.IsEnabled;

    private void AutoDetectEmulatorPath()
    {
        MessageBoxHelper.Show(LocalizationHelper.GetRawString("LdExtrasEnabledTip"));

        // 读取 LD 注册表地址 并填充GUI
        if (!string.IsNullOrEmpty(EmulatorPath))
        {
            return;
        }

        try
        {
            // 原版路径优先
            string[] possiblePaths =
            [
                @"Software\leidian\ldplayer14",
                    @"Software\leidian\ldplayer9",
                    @"Software\mrfz\mrfz"
            ];

            const string InstallDirValueName = "InstallDir";
            var detectedPaths = new List<string>();

            foreach (var regPath in possiblePaths)
            {
                using var driverKey = Registry.CurrentUser.OpenSubKey(regPath);
                if (driverKey == null)
                {
                    continue;
                }

                var installDir = driverKey.GetValue(InstallDirValueName) as string;
                if (!string.IsNullOrEmpty(installDir) && Directory.Exists(installDir))
                {
                    if (!detectedPaths.Contains(installDir))
                    {
                        detectedPaths.Add(installDir);
                    }
                }
            }

            if (detectedPaths.Count == 0)
            {
                EmulatorPath = string.Empty;
                return;
            }

            if (detectedPaths.Count == 1)
            {
                EmulatorPath = detectedPaths[0];
                return;
            }

            var selectedPath = ConnectSettingsUserControlModel.ShowItemSelectionDialog(
                detectedPaths,
                LocalizationHelper.GetString("SelectEmulatorPath"),
                LocalizationHelper.GetString("MultipleEmulatorsDetected"));
            if (!string.IsNullOrEmpty(selectedPath))
            {
                EmulatorPath = selectedPath;
            }
        }
        catch (Exception e)
        {
            _logger.Warning("An error occurred: {EMessage}", e.Message);
            EmulatorPath = string.Empty;
        }
    }

    /// <summary>
    /// Gets or sets a value indicating the path of the emulator.
    /// </summary>
    public string EmulatorPath
    {
        get; set {
            if (Enable && !string.IsNullOrEmpty(value) && !Directory.Exists(value))
            {
                MessageBoxHelper.Show(LocalizationHelper.GetString("LdPlayerEmulatorPathNotFound"));
                MessageBoxHelper.Show(LocalizationHelper.GetRawString("LdExtrasEnabledTip"));
                return;
            }

            // 当路径存在时，检查 ldopengl64.dll 是否存在
            if (!string.IsNullOrEmpty(value) && Directory.Exists(value))
            {
                var libPath = Path.Combine(value, "ldopengl64.dll");
                if (!File.Exists(libPath))
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("LdPlayerOpenglMissing"));
                    MessageBoxHelper.Show(LocalizationHelper.GetRawString("LdExtrasEnabledTip"));
                    return;
                }
            }

            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.EmulatorPath = value;
        }
    } = Directory.Exists(ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.EmulatorPath) ? ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.EmulatorPath : string.Empty;

    public bool ManualSetIndex
    {
        get; set {
            if (field == value)
            {
                return;
            }

            if (value)
            {
                InstanceIndex = GetEmulatorIndex(SettingsViewModel.ConnectSettings.ConnectAddress);
            }

            SetAndNotify(ref field, value);
            Instances.AsstProxy.Connected = false;
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.ManualSetIndex = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.ManualSetIndex;

    /// <summary>
    /// Gets or sets the index of the emulator.
    /// </summary>
    public int InstanceIndex
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.InstanceIndex = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer.InstanceIndex;

    private int GetEmulatorPid(int index)
    {
        var emulatorPath = $@"{EmulatorPath}\ldconsole.exe";
        if (!File.Exists(emulatorPath))
        {
            return 0;
        }

        var startInfo = new ProcessStartInfo {
            FileName = emulatorPath,
            Arguments = "list2",
            RedirectStandardOutput = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var process = Process.Start(startInfo);
        if (process == null)
        {
            _logger.Warning("Failed to start ldconsole list2");
            return 0;
        }

        using var reader = process.StandardOutput;
        var result = reader.ReadToEnd();
        var lines = result.Split([Environment.NewLine], StringSplitOptions.RemoveEmptyEntries);

        if (lines.Length <= 0)
        {
            _logger.Warning("Failed to get emulator PID.");
            return 0;
        }

        foreach (var line in lines)
        {
            var parts = line.Split(',');
            if (parts.Length < 6 || !int.TryParse(parts[0], out var currentIndex) || currentIndex != index)
            {
                continue;
            }

            if (int.TryParse(parts[5], out var pid))
            {
                return pid;
            }
        }

        _logger.Warning("Failed to get emulator PID.");
        return 0;
    }

    private static int GetEmulatorIndex(string address)
    {
        int index = 0;
        if (string.IsNullOrEmpty(address))
        {
            return index;
        }

        const int BaseEmulatorPort = 5554;
        const int BaseAdbPort = 5555;

        if (address.StartsWith("emulator-") && int.TryParse(address[9..], out int port))
        {
            index = (port - BaseEmulatorPort) / 2;
        }
        else if (address.StartsWith("127.0.0.1:") && int.TryParse(address[10..], out int port2))
        {
            index = (port2 - BaseAdbPort) / 2;
        }

        return index;
    }

    public string Config
    {
        get {
            if (!Enable)
            {
                return Newtonsoft.Json.JsonConvert.SerializeObject(new JObject());
            }

            int index;
            if (ManualSetIndex)
            {
                index = InstanceIndex;
            }
            else
            {
                index = GetEmulatorIndex(SettingsViewModel.ConnectSettings.ConnectAddress);
            }

            var configObject = new JObject {
                ["path"] = EmulatorPath,
                ["index"] = index,
                ["pid"] = GetEmulatorPid(index),
            };

            return Newtonsoft.Json.JsonConvert.SerializeObject(configObject);
        }
    }
}
