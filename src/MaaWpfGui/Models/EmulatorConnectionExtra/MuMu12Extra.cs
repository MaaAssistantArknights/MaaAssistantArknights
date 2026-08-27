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
using System.Text.RegularExpressions;
using System.Windows;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class MuMu12Extra() : ExtraConfig
{
    private static readonly ILogger _logger = Log.ForContext<MuMu12Extra>();

    public bool Enable
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.IsEnabled = value;

            if (value)
            {
                AutoDetectEmulatorPath();
            }

            // 通知 ConnectSettings 动态增删触控模式下拉项并自动切换
            ConnectSettingsUserControlModel.Instance.OnMuMuExtrasEnableChanged(value);
            Instances.AsstProxy.Connected = false;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.IsEnabled;

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

    /// <summary>
    /// Gets or sets a value indicating the path of the emulator.
    /// </summary>
    public string EmulatorPath
    {
        get; set {
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
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EmulatorPath = value;
            SetAndNotify(ref field, value);
        }
    } = Directory.Exists(ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EmulatorPath) ? ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EmulatorPath : string.Empty;

    public bool EnableBridgeConnection
    {
        get; set {
            if (field == value)
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

            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EnableBridgeConnection = value;
            SetAndNotify(ref field, value);
            Instances.AsstProxy.Connected = false;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EnableBridgeConnection;

    /// <summary>
    /// Gets or sets a value indicating whether MuMu extras is also used for touch input, not just screencap.
    /// 勾选时自动切换触控模式为 MuMu 触控，取消勾选时回到默认 Minitouch。
    /// </summary>
    public bool EnableTouch
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            if (value)
            {
                ConnectSettingsUserControlModel.Instance.TouchMode = TouchMode.MumuExtras;
            }
            else if (ConnectSettingsUserControlModel.Instance.TouchMode == TouchMode.MumuExtras)
            {
                // 仅在当前仍处于 MuMu 触控时才回退到默认，
                // 避免用户主动切换到其他触控模式时被覆盖
                ConnectSettingsUserControlModel.Instance.TouchMode = TouchMode.MiniTouch;
            }
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EnableTouch = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.EnableTouch;

    /// <summary>
    /// Gets or sets the index of the emulator.
    /// </summary>
    public int InstanceIndex
    {
        get; set {
            Instances.AsstProxy.Connected = false;
            ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.InstanceIndex = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Mumu12.InstanceIndex;

    public string Config
    {
        get {
            if (!Enable)
            {
                return Newtonsoft.Json.JsonConvert.SerializeObject(new JObject());
            }

            var configObject = new JObject {
                ["path"] = EmulatorPath,
                ["touch"] = EnableTouch,

                // MuMu get_display_id 需要包名，按当前客户端类型映射明日方舟包名
                ["client_type"] = SettingsViewModel.GameSettings.ClientType.ToCustomString(),
            };

            if (EnableBridgeConnection)
            {
                configObject["index"] = InstanceIndex;
            }

            return Newtonsoft.Json.JsonConvert.SerializeObject(configObject);
        }
    }
}
