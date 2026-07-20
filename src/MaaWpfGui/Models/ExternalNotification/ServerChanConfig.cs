// <copyright file="ServerChanConfig.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.Models.ExternalNotification;

public class ServerChanConfig(string sendKey = "") : BaseConfig
{
    public ServerChanConfig(ServerChan serverChan)
        : this(SimpleEncryptionHelper.Decrypt(serverChan.SendKey))
    {
    }

    public string SendKey { get; set => SetAndNotify(ref field, value); } = sendKey;

    public override ServerChan ToConfig() => new(SimpleEncryptionHelper.Encrypt(SendKey));
}
