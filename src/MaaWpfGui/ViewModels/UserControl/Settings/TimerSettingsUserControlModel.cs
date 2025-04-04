// <copyright file="TimerSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 定时设置
/// </summary>
public class TimerSettingsUserControlModel : PropertyChangedBase
{
    static TimerSettingsUserControlModel()
    {
        Instance = new();
    }

    public static TimerSettingsUserControlModel Instance { get; }

    private bool _forceScheduledStart = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ForceScheduledStart, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to force scheduled start.
    /// </summary>
    public bool ForceScheduledStart
    {
        get => _forceScheduledStart;
        set
        {
            SetAndNotify(ref _forceScheduledStart, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ForceScheduledStart, value.ToString());
        }
    }

    private bool _showWindowBeforeForceScheduledStart = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether show window before force scheduled start.
    /// </summary>
    public bool ShowWindowBeforeForceScheduledStart
    {
        get => _showWindowBeforeForceScheduledStart;
        set
        {
            SetAndNotify(ref _showWindowBeforeForceScheduledStart, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, value.ToString());
        }
    }

    private bool _customConfig = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.CustomConfig, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use custom config.
    /// </summary>
    public bool CustomConfig
    {
        get => _customConfig;
        set
        {
            SetAndNotify(ref _customConfig, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.CustomConfig, value.ToString());
        }
    }

    public TimerModel TimerModels { get; set; } = new();

    public class TimerModel
    {
        public class TimerProperties : PropertyChangedBase
        {
            public TimerProperties(int timeId, bool? isOn, int hour, int min, string? timerConfig)
            {
                TimerId = timeId;
                _isOn = isOn;
                _hour = hour;
                _min = min;
                if (timerConfig == null || !ConfigurationHelper.GetConfigurationList().Contains(timerConfig))
                {
                    _timerConfig = ConfigurationHelper.GetCurrentConfiguration();
                }
                else
                {
                    _timerConfig = timerConfig;
                }
            }

            public int TimerId { get; set; }

            private readonly string _timerName = LocalizationHelper.GetString("Timer");

            public string TimerName => TimerId == 8 ? $"{_timerName}*" : $"{_timerName} {TimerId + 1}";

            private bool? _isOn;

            /// <summary>
            /// Gets or sets a value indicating whether the timer is set.
            /// </summary>
            public bool? IsOn
            {
                get => _isOn;
                set
                {
                    if (TimerId == 8 && value == true)
                    {
                        value = null;
                    }

                    SetAndNotify(ref _isOn, value);
                    ConfigurationHelper.SetTimer(TimerId, value.ToString());
                }
            }

            private int _hour;

            /// <summary>
            /// Gets or sets the hour of the timer.
            /// </summary>
            public int Hour
            {
                get => _hour;
                set
                {
                    SetAndNotify(ref _hour, value);
                    ConfigurationHelper.SetTimerHour(TimerId, _hour.ToString());
                }
            }

            private int _min;

            /// <summary>
            /// Gets or sets the minute of the timer.
            /// </summary>
            public int Min
            {
                get => _min;
                set
                {
                    SetAndNotify(ref _min, value);
                    ConfigurationHelper.SetTimerMin(TimerId, _min.ToString());
                }
            }

            private string? _timerConfig;

            /// <summary>
            /// Gets or sets the config of the timer.
            /// </summary>
            public string? TimerConfig
            {
                get => _timerConfig;
                set
                {
                    SetAndNotify(ref _timerConfig, value ?? ConfigurationHelper.GetCurrentConfiguration());
                    ConfigurationHelper.SetTimerConfig(TimerId, _timerConfig);
                }
            }
        }

        public TimerProperties[] Timers { get; set; } = new TimerProperties[9];

        public TimerModel()
        {
            for (int i = 0; i < 8; i++)
            {
                Timers[i] = new(
                    i,
                    ConfigurationHelper.GetTimer(i, bool.FalseString) == bool.TrueString,
                    int.Parse(ConfigurationHelper.GetTimerHour(i, $"{i * 3}")),
                    int.Parse(ConfigurationHelper.GetTimerMin(i, "0")),
                    ConfigurationHelper.GetTimerConfig(i, ConfigurationHelper.GetCurrentConfiguration()));
            }

            Timers[8] = new(
                8,
                ConfigurationHelper.GetTimer(8, bool.FalseString) == string.Empty ? null : false,
                int.Parse(ConfigurationHelper.GetTimerHour(8, "0")),
                int.Parse(ConfigurationHelper.GetTimerMin(8, "0")),
                ConfigurationHelper.GetTimerConfig(8, ConfigurationHelper.GetCurrentConfiguration()));
        }
    }
}
