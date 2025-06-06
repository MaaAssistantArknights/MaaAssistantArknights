// <copyright file="RemoteControlUserControlModel.cs" company="MaaAssistantArknights">
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
/// 远程控制
/// </summary>
public class RemoteControlUserControlModel : PropertyChangedBase
{
    static RemoteControlUserControlModel()
    {
        Instance = new();
    }

    public static RemoteControlUserControlModel Instance { get; }

    private string _remoteControlGetTaskEndpointUri = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty));

    public string RemoteControlGetTaskEndpointUri
    {
        get => _remoteControlGetTaskEndpointUri;
        set
        {
            if (!SetAndNotify(ref _remoteControlGetTaskEndpointUri, value))
            {
                return;
            }

            Instances.RemoteControlService.InitializePollJobTask();
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, value);
        }
    }

    private string _remoteControlReportStatusUri = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlReportStatusUri, string.Empty));

    public string RemoteControlReportStatusUri
    {
        get => _remoteControlReportStatusUri;
        set
        {
            SetAndNotify(ref _remoteControlReportStatusUri, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlReportStatusUri, value);
        }
    }

    private string _remoteControlUserIdentity = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlUserIdentity, string.Empty));

    public string RemoteControlUserIdentity
    {
        get => _remoteControlUserIdentity;
        set
        {
            SetAndNotify(ref _remoteControlUserIdentity, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlUserIdentity, value);
        }
    }

    private string _remoteControlDeviceIdentity = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty));

    public string RemoteControlDeviceIdentity
    {
        get => _remoteControlDeviceIdentity;
        set
        {
            SetAndNotify(ref _remoteControlDeviceIdentity, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlDeviceIdentity, value);
        }
    }

    private int _remoteControlPollIntervalMs = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlPollIntervalMs, "1000"));

    public int RemoteControlPollIntervalMs
    {
        get => _remoteControlPollIntervalMs;
        set
        {
            if (!SetAndNotify(ref _remoteControlPollIntervalMs, value))
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.RemoteControlPollIntervalMs, value.ToString());
        }
    }
}
