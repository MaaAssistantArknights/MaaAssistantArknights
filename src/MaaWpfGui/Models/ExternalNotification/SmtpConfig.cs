// <copyright file="SmtpConfig.cs" company="MaaAssistantArknights">
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

public class SmtpConfig(string server = "", string port = "", string username = "", string password = "", string from = "", string to = "", string useSsl = "false", string requiresAuthentication = "false") : BaseConfig
{
    public SmtpConfig(Smtp smtp)
        : this(SimpleEncryptionHelper.Decrypt(smtp.Server), SimpleEncryptionHelper.Decrypt(smtp.Port), SimpleEncryptionHelper.Decrypt(smtp.User), SimpleEncryptionHelper.Decrypt(smtp.Password), SimpleEncryptionHelper.Decrypt(smtp.From), SimpleEncryptionHelper.Decrypt(smtp.To), smtp.UseSsl.ToString(), smtp.RequiresAuthentication.ToString())
    {
    }

    public string Server { get; set => SetAndNotify(ref field, value); } = server;

    public string Port { get; set => SetAndNotify(ref field, value); } = port;

    public string Username { get; set => SetAndNotify(ref field, value); } = username;

    public string Password { get; set => SetAndNotify(ref field, value); } = password;

    public bool UseSsl { get; set => SetAndNotify(ref field, value); } = bool.TryParse(useSsl, out var parsedUseSsl) && parsedUseSsl;

    public bool RequiresAuthentication { get; set => SetAndNotify(ref field, value); } = bool.TryParse(requiresAuthentication, out var parsedRequiresAuthentication) && parsedRequiresAuthentication;

    public string From { get; set => SetAndNotify(ref field, value); } = from;

    public string To { get; set => SetAndNotify(ref field, value); } = to;

    public override Smtp ToConfig() => new(SimpleEncryptionHelper.Encrypt(Server), SimpleEncryptionHelper.Encrypt(Port), SimpleEncryptionHelper.Encrypt(Username), SimpleEncryptionHelper.Encrypt(Password), SimpleEncryptionHelper.Encrypt(From), SimpleEncryptionHelper.Encrypt(To), UseSsl, RequiresAuthentication);
}
