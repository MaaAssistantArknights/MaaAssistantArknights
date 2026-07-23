// <copyright file="GotifyConfig.cs" company="MaaAssistantArknights">
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

public class GotifyConfig(string server = "", string token = "") : BaseConfig
{
    public GotifyConfig()
        : this(string.Empty, string.Empty)
    {
    }

    public GotifyConfig(Gotify gotify)
        : this(SimpleEncryptionHelper.Decrypt(gotify.Server), SimpleEncryptionHelper.Decrypt(gotify.Token))
    {
    }

    public string Server { get; set => SetAndNotify(ref field, value); } = server;

    public string Token { get; set => SetAndNotify(ref field, value); } = token;

    public override Gotify ToConfig() => new(SimpleEncryptionHelper.Encrypt(Server), SimpleEncryptionHelper.Encrypt(Token));
}
