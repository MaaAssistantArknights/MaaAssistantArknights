// <copyright file="MuMu12Extra.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using System.Windows;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class MuMu12Extra : ExtraConfig, IJsonOnDeserialized
{
    private static readonly ILogger _logger = Log.ForContext<MuMu12Extra>();

    public void OnDeserialized()
    {
        _emulatorPath = Directory.Exists(_emulatorPath) ? _emulatorPath : string.Empty;
    }

    [JsonInclude]
    [JsonPropertyName("IsEnabled")]
    private bool _enable;

    [JsonIgnore]
    public bool Enable
    {
        get => _enable;
        set {
            if (!SetAndNotify(ref _enable, value))
            {
                return;
            }

            if (value)
            {
                AutoDetectEmulatorPath();
            }

            Instances.AsstProxy.Connected = false;
        }
    }

    private void AutoDetectEmulatorPath()
    {
        MessageBoxHelper.Show(LocalizationHelper.GetString("MuMu12ExtrasEnabledTip"));

        // 读取mumu注册表地址 并填充GUI
        if (!string.IsNullOrEmpty(EmulatorPath))
        {
            return;
        }

        try
        {
            // 按版本从新到旧排列，新增版本只需追加一项
            string[] possibleUninstallKeys =
            [
                @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayer-15.0",
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayer-12.0",
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayer",
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayerGlobal-15.0",
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MuMuPlayerGlobal-12.0",
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\YXArkNights-12.0",
                ];

            const string UninstallExeName = @"\uninstall.exe";
            var detectedPaths = new List<string>();

            foreach (var keyPath in possibleUninstallKeys)
            {
                using var driverKey = Registry.LocalMachine.OpenSubKey(keyPath);
                if (driverKey == null)
                {
                    continue;
                }

                var uninstallString = driverKey.GetValue("UninstallString") as string;
                if (string.IsNullOrEmpty(uninstallString) || !uninstallString.Contains(UninstallExeName))
                {
                    continue;
                }

                var match = Regex.Match(uninstallString,
                    $"""
                         ^"(.*?){Regex.Escape(UninstallExeName)}
                         """,
                    RegexOptions.IgnoreCase);

                if (match.Success && Directory.Exists(match.Groups[1].Value))
                {
                    var path = match.Groups[1].Value;
                    if (!detectedPaths.Contains(path))
                    {
                        detectedPaths.Add(path);
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

    [JsonInclude]
    [JsonPropertyName("EmulatorPath")]
    private string _emulatorPath = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating the path of the emulator.
    /// </summary>
    [JsonIgnore]
    public string EmulatorPath
    {
        get => _emulatorPath;
        set {
            if (Enable && !string.IsNullOrEmpty(value) && !Directory.Exists(value))
            {
                MessageBoxHelper.Show(LocalizationHelper.GetString("MuMuEmulatorPathNotFound"));
                MessageBoxHelper.Show(LocalizationHelper.GetString("MuMu12ExtrasEnabledTip"));
                return;
            }

            // 当路径存在时，检查 external_renderer_ipc.dll 是否可用（兼容多个 MuMu 版本路径）
            // 新增版本只需在此列表追加一项
            if (!string.IsNullOrEmpty(value) && Directory.Exists(value))
            {
                string[] candidateRelativePaths =
                [
                    Path.Combine("nx_device", "15.0", "shell", "sdk", "external_renderer_ipc.dll"),  // MuMu 6.0
                        Path.Combine("nx_device", "12.0", "shell", "sdk", "external_renderer_ipc.dll"),  // MuMu 5.0 / MuMu 12
                        Path.Combine("shell", "sdk", "external_renderer_ipc.dll"),                          // MuMu 旧版本
                    ];

                if (!candidateRelativePaths.Any(relPath => File.Exists(Path.Combine(value, relPath))))
                {
                    MessageBoxHelper.Show(LocalizationHelper.GetString("MuMuExternalRendererMissing"));
                    MessageBoxHelper.Show(LocalizationHelper.GetString("MuMu12ExtrasEnabledTip"));
                    return;
                }
            }

            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _emulatorPath, value);
        }
    }

    [JsonInclude]
    [JsonPropertyName("EnableBridgeConnection")]
    private bool _enableBridgeConnection = ConfigurationHelper.GetValue(ConfigurationKeys.MumuBridgeConnection, false);

    [JsonIgnore]
    public bool EnableBridgeConnection
    {
        get => _enableBridgeConnection;
        set {
            if (_enableBridgeConnection == value)
            {
                return;
            }

            if (value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("MuMuBridgeConnectionTip"),
                    icon: MessageBoxImage.Warning,
                    buttons: MessageBoxButton.YesNo,
                    no: LocalizationHelper.GetString("Confirm"),
                    yes: LocalizationHelper.GetString("Cancel"));
                if (result != MessageBoxResult.No)
                {
                    return;
                }
            }

            SetAndNotify(ref _enableBridgeConnection, value);
            Instances.AsstProxy.Connected = false;
        }
    }

    [JsonInclude]
    [JsonPropertyName("InstanceIndex")]
    private int _instanceIndex;

    /// <summary>
    /// Gets or sets the index of the emulator.
    /// </summary>
    [JsonIgnore]
    public int InstanceIndex
    {
        get => _instanceIndex;
        set {
            Instances.AsstProxy.Connected = false;
            SetAndNotify(ref _instanceIndex, value);
        }
    }

    [JsonIgnore]
    public string Config
    {
        get {
            if (!Enable)
            {
                return Newtonsoft.Json.JsonConvert.SerializeObject(new JObject());
            }

            var configObject = new JObject {
                ["path"] = EmulatorPath,
            };

            if (EnableBridgeConnection)
            {
                configObject["index"] = InstanceIndex;
            }

            return Newtonsoft.Json.JsonConvert.SerializeObject(configObject);
        }
    }
}
