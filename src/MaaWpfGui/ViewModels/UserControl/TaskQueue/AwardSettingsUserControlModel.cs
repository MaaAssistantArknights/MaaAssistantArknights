// <copyright file="AwardSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using System.Windows;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class AwardSettingsUserControlModel:PropertyChangedBase
{

    private bool _receiveAward = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether receive award is enabled.
    /// </summary>
    public bool ReceiveAward
    {
        get => _receiveAward;
        set
        {
            SetAndNotify(ref _receiveAward, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveAward, value.ToString());
        }
    }

    private bool _receiveMail = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether receive mail is enabled.
    /// </summary>
    public bool ReceiveMail
    {
        get => _receiveMail;
        set
        {
            SetAndNotify(ref _receiveMail, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveMail, value.ToString());
        }
    }

    private bool _receiveFreeRecruit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeRecruit, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether receive mail is enabled.
    /// </summary>
    public bool ReceiveFreeRecruit
    {
        get => _receiveFreeRecruit;
        set
        {
            if (value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("GachaWarning"),
                    LocalizationHelper.GetString("Warning"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning,
                    no: LocalizationHelper.GetString("Confirm"),
                    yes: LocalizationHelper.GetString("Cancel"),
                    iconBrushKey: "DangerBrush");
                if (result != MessageBoxResult.No)
                {
                    return;
                }
            }

            SetAndNotify(ref _receiveFreeRecruit, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveFreeRecruit, value.ToString());
        }
    }

    private bool _receiveOrundum = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether receive orundum is enabled.
    /// </summary>
    public bool ReceiveOrundum
    {
        get => _receiveOrundum;
        set
        {
            SetAndNotify(ref _receiveOrundum, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveOrundum, value.ToString());
        }
    }

    private bool _receiveMining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether receive mining is enabled.
    /// </summary>
    public bool ReceiveMining
    {
        get => _receiveMining;
        set
        {
            SetAndNotify(ref _receiveMining, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveMining, value.ToString());
        }
    }

    private bool _receiveReceiveSpecialAccess = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to collect special access rewards.
    /// </summary>
    public bool ReceiveSpecialAccess
    {
        get => _receiveReceiveSpecialAccess;
        set
        {
            SetAndNotify(ref _receiveReceiveSpecialAccess, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReceiveSpecialAccess, value.ToString());
        }
    }
}
