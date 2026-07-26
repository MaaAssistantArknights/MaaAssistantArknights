// <copyright file="ConnectSettings.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.ComponentModel;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Models.EmulatorConnectionExtra;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

public class ConnectSettings : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    private static string _bindingPrefix = string.Empty;

    public void EventBinding(string key)
    {
        _bindingPrefix = key;
        PropertyChanged += Handler.OnPropertyChangedFactory(_bindingPrefix + nameof(ConnectSettings) + ".");
        Extras.MuMuEmulator12.PropertyChanged += Handler.OnPropertyChangedFactory(_bindingPrefix + nameof(ConnectSettings) + "." + nameof(ExtraConfigs) + "." + nameof(Extras.MuMuEmulator12) + ".");
        Extras.LDPlayer.PropertyChanged += Handler.OnPropertyChangedFactory(_bindingPrefix + nameof(ConnectSettings) + "." + nameof(ExtraConfigs) + "." + nameof(Extras.LDPlayer) + ".");
        Extras.Win32Extra.PropertyChanged += Handler.OnPropertyChangedFactory(_bindingPrefix + nameof(ConnectSettings) + "." + nameof(ExtraConfigs) + "." + nameof(Extras.Win32Extra) + ".");
    }

    public bool AutoDetect { get; set; } = true;

    public bool AlwaysAutoDetect { get; set; }

    public ConnectConfig Config { get; set; } = ConnectConfig.General;

    public string AdbPath { get; set; } = string.Empty;

    public bool AdbReplaced { get; set; }

    public string Address { get; set; } = string.Empty;

    public List<string> AddressHistory { get; set; } = [];

    public ExtraConfigs Extras { get; set; } = new ExtraConfigs();

    public bool AllowAdbRestart { get; set; } = true;

    public bool AllowAdbHardRestart { get; set; } = true;

    public TouchMode TouchMode { get; set; } = TouchMode.MiniTouch;

    public bool EnableAdbLite { get; set; }

    public bool KillAdbOnExit { get; set; }

    public record class ExtraConfigs
    {
        public LDPlayerExtra LDPlayer { get; set; } = new();

        public MuMu12Extra MuMuEmulator12 { get; set; } = new();

        public Win32Extra Win32Extra { get; set; } = new();

        public Bluestacks BluestacksExtra { get; set; } = new();

        public record class Bluestacks
        {
            public string ConfigKeyword { get; set; } = string.Empty;

            public string ConfigPath { get; set; } = string.Empty;
        }
    }
}
