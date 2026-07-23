// <copyright file="BarkConfig.cs" company="MaaAssistantArknights">
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

public class BarkConfig(string sendKey = "", string server = "https://api.day.app") : BaseConfig
{
    public BarkConfig()
        : this(string.Empty)
    {
    }

    public BarkConfig(Bark bark)
        : this(SimpleEncryptionHelper.Decrypt(bark.SendKey), SimpleEncryptionHelper.Decrypt(bark.Server, "https://api.day.app"))
    {
    }

    public string SendKey { get; set => SetAndNotify(ref field, value); } = sendKey;

    public string Server { get; set => SetAndNotify(ref field, value); } = server;

    public override Bark ToConfig() => new(SimpleEncryptionHelper.Encrypt(SendKey), SimpleEncryptionHelper.Encrypt(Server));
}
