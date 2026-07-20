// <copyright file="QmsgConfig.cs" company="MaaAssistantArknights">
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

public class QmsgConfig(string server = "", string key = "", string user = "", string bot = "") : BaseConfig
{
    public QmsgConfig(Qmsg qmsg)
        : this(SimpleEncryptionHelper.Decrypt(qmsg.Server), SimpleEncryptionHelper.Decrypt(qmsg.Key), SimpleEncryptionHelper.Decrypt(qmsg.User), SimpleEncryptionHelper.Decrypt(qmsg.Bot))
    {
    }

    public string Server { get; set => SetAndNotify(ref field, value); } = server;

    public string Key { get; set => SetAndNotify(ref field, value); } = key;

    public string User { get; set => SetAndNotify(ref field, value); } = user;

    public string Bot { get; set => SetAndNotify(ref field, value); } = bot;

    public override Qmsg ToConfig() => new(SimpleEncryptionHelper.Encrypt(Server), SimpleEncryptionHelper.Encrypt(Key), SimpleEncryptionHelper.Encrypt(User), SimpleEncryptionHelper.Encrypt(Bot));
}
