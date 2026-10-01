// <copyright file="ArpsExtra.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.EmulatorConnectionExtra;

public class ArpsExtra : ExtraConfig
{
    public List<CombinedData> CompressionOptions { get; } =
    [
        new() { Display = "lz4_block", Value = "lz4_block" },
        new() { Display = "raw", Value = "raw" },
    ];

    public List<CombinedData> CaptureModeOptions { get; } =
    [
        new() { Display = "auto", Value = "auto" },
        new() { Display = "hardware", Value = "hardware" },
        new() { Display = "bitmap", Value = "bitmap" },
    ];

    public List<CombinedData> ExitPowerModeOptions { get; } =
    [
        new() { Display = "restore_previous", Value = "restore_previous" },
        new() { Display = "keep_on", Value = "keep_on" },
        new() { Display = "turn_off", Value = "turn_off" },
    ];

    public string Compression
    {
        get => Settings.Compression;
        set {
            if (Settings.Compression == value)
            {
                return;
            }

            Settings.Compression = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public int MaxFps
    {
        get => Settings.MaxFps;
        set {
            value = Math.Clamp(value, 0, 240);
            if (Settings.MaxFps == value)
            {
                return;
            }

            Settings.MaxFps = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public string CaptureMode
    {
        get => NormalizeCaptureMode(Settings.CaptureMode);
        set {
            value = NormalizeCaptureMode(value);
            if (Settings.CaptureMode == value)
            {
                return;
            }

            Settings.CaptureMode = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public bool PowerOnIfScreenOff
    {
        get => Settings.PowerOnIfScreenOff;
        set {
            if (Settings.PowerOnIfScreenOff == value)
            {
                return;
            }

            Settings.PowerOnIfScreenOff = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public bool TurnScreenOff
    {
        get => Settings.TurnScreenOff;
        set {
            if (Settings.TurnScreenOff == value)
            {
                return;
            }

            Settings.TurnScreenOff = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public bool KeepScreenOn
    {
        get => Settings.KeepScreenOn;
        set {
            if (Settings.KeepScreenOn == value)
            {
                return;
            }

            Settings.KeepScreenOn = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public string ExitPowerMode
    {
        get => Settings.ExitPowerMode;
        set {
            if (Settings.ExitPowerMode == value)
            {
                return;
            }

            Settings.ExitPowerMode = value;
            NotifyOfPropertyChange();
            Instances.AsstProxy.Connected = false;
        }
    }

    public string Config => JsonConvert.SerializeObject(new JObject
    {
        ["type"] = "ARPS",
        ["compression"] = Compression,
        ["max_fps"] = MaxFps,
        ["capture_mode"] = CaptureMode,
        ["power_on_if_screen_off"] = PowerOnIfScreenOff,
        ["turn_screen_off"] = TurnScreenOff,
        ["keep_screen_on"] = KeepScreenOn,
        ["exit_power_mode"] = ExitPowerMode,
    });

    private static string NormalizeCaptureMode(string value) => value == "surface" ? "hardware" : value;

    private static Configuration.Single.Settings.ConnectionExtra.ArpsExtra Settings =>
        ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Arps;
}
