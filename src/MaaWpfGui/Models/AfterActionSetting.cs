// <copyright file="AfterActionSetting.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.Models;

public class AfterActionSetting : PropertyChangedBase
{
    private bool _once;

    public bool Once
    {
        get => _once;
        set
        {
            SetAndNotify(ref _once, value);
            RefreshDescription();
        }
    }

    private bool _exitArknights = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitArknights, bool.FalseString));

    public bool ExitArknights
    {
        get => _exitArknights;
        set
        {
            SetAndNotify(ref _exitArknights, value);
            RefreshDescription();
        }
    }

    private bool _backToAndroidHome = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BackToAndroidHome, bool.FalseString));

    public bool BackToAndroidHome
    {
        get => _backToAndroidHome;
        set
        {
            SetAndNotify(ref _backToAndroidHome, value);
            RefreshDescription();
        }
    }

    private bool _exitEmulator = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitEmulator, bool.FalseString));

    public bool ExitEmulator
    {
        get => _exitEmulator;
        set
        {
            if (value)
            {
                ExitArknights = false;
                BackToAndroidHome = false;
            }

            SetAndNotify(ref _exitEmulator, value);
            RefreshDescription();
        }
    }

    private bool _exitSelf = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExitSelf, bool.FalseString));

    public bool ExitSelf
    {
        get => _exitSelf;
        set
        {
            SetAndNotify(ref _exitSelf, value);
            RefreshDescription();
        }
    }

    private bool _ifNoOtherMaa = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IfNoOtherMaa, bool.FalseString));

    public bool IfNoOtherMaa
    {
        get => _ifNoOtherMaa;
        set
        {
            SetAndNotify(ref _ifNoOtherMaa, value);
            RefreshDescription();
        }
    }

    private bool _hibernate = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Hibernate, bool.FalseString));

    public bool Hibernate
    {
        get => _hibernate;
        set
        {
            if (!value)
            {
                IfNoOtherMaa = false;
            }

            SetAndNotify(ref _hibernate, value);
            RefreshDescription();
        }
    }

    private bool _shutdown = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Shutdown, bool.FalseString));

    public bool Shutdown
    {
        get => _shutdown;
        set
        {
            if (value)
            {
                ExitSelf = false;
                ExitArknights = false;
                BackToAndroidHome = false;
                ExitEmulator = false;
            }
            else
            {
                IfNoOtherMaa = false;
            }

            SetAndNotify(ref _shutdown, value);
            RefreshDescription();
        }
    }

    public static AfterActionSetting Current { get; } = new();

    public void Save()
    {
        if (Once)
        {
            return;
        }

        ConfigurationHelper.SetValue(ConfigurationKeys.ExitArknights, ExitArknights.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.BackToAndroidHome, BackToAndroidHome.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.ExitEmulator, ExitEmulator.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.ExitSelf, ExitSelf.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.IfNoOtherMaa, IfNoOtherMaa.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.Hibernate, Hibernate.ToString());
        ConfigurationHelper.SetValue(ConfigurationKeys.Shutdown, Shutdown.ToString());
    }

    private string _actionDescription = string.Empty;

    public string ActionDescription
    {
        get => _actionDescription;
        private set => SetAndNotify(ref _actionDescription, value);
    }

    private void RefreshDescription()
    {
        List<string> actions = [];
        if (ExitArknights)
        {
            actions.Add(LocalizationHelper.GetString("ExitArknights"));
        }

        if (BackToAndroidHome)
        {
            actions.Add(LocalizationHelper.GetString("BackToAndroidHome"));
        }

        if (ExitEmulator)
        {
            actions.Add(LocalizationHelper.GetString("ExitEmulator"));
        }

        if (ExitSelf)
        {
            actions.Add(LocalizationHelper.GetString("ExitSelf"));
        }

        if (IfNoOtherMaa)
        {
            actions.Add(LocalizationHelper.GetString("IfNoOtherMaa"));
        }

        if (Hibernate)
        {
            actions.Add(LocalizationHelper.GetString("Hibernate"));
        }

        if (Shutdown)
        {
            actions.Add(LocalizationHelper.GetString("Shutdown"));
        }

        ActionDescription = actions.Count == 0
            ? LocalizationHelper.GetString("DoNothing")
            : string.Join(" -> ", actions);
    }

    private AfterActionSetting()
    {
        RefreshDescription();
    }
}
