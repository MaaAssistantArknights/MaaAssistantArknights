// <copyright file="RemoteControlUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
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

    public string RemoteControlGetTaskEndpointUri
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            Instances.RemoteControlService.InitializePollJobTask();
            ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlGetTaskEndpointUri = SimpleEncryptionHelper.Encrypt(value);
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlGetTaskEndpointUri);

    public string RemoteControlReportStatusUri
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlReportStatusUri = SimpleEncryptionHelper.Encrypt(value);
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlReportStatusUri);

    public string RemoteControlUserIdentity
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlUserIdentity = SimpleEncryptionHelper.Encrypt(value);
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlUserIdentity);

    public string RemoteControlDeviceIdentity
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlDeviceIdentity = SimpleEncryptionHelper.Encrypt(value);
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlDeviceIdentity);

    public int RemoteControlPollIntervalMs
    {
        get => ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlPollIntervalMs;
        set {
            ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlPollIntervalMs = value;
            NotifyOfPropertyChange();
        }
    }
}
