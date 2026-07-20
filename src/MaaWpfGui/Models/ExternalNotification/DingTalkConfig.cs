// <copyright file="DingTalkConfig.cs" company="MaaAssistantArknights">
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

public class DingTalkConfig(string accessToken = "", string secret = "") : BaseConfig
{
    public DingTalkConfig(DingTalk dingTalk)
        : this(SimpleEncryptionHelper.Decrypt(dingTalk.AccessToken), SimpleEncryptionHelper.Decrypt(dingTalk.Secret))
    {
    }

    public string AccessToken { get; set => SetAndNotify(ref field, value); } = accessToken;

    public string Secret { get; set => SetAndNotify(ref field, value); } = secret;

    public override DingTalk ToConfig() => new(SimpleEncryptionHelper.Encrypt(AccessToken), SimpleEncryptionHelper.Encrypt(Secret));
}
