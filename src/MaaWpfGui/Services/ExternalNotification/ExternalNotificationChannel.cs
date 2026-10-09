// <copyright file="ExternalNotificationChannel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.Services.ExternalNotification;

// Configuration, editor, sender and display names are declared together once.
public sealed record ExternalNotificationChannel(
    Type ConfigType,
    Type EditorType,
    string LocalizationKey,
    string FallbackName,
    Func<BaseConfig> CreateEditor,
    Func<Base, BaseConfig> ReadConfig,
    Func<BaseConfig, IExternalNotificationProvider> CreateProvider)
{
    public static IReadOnlyList<ExternalNotificationChannel> All { get; } = Array.AsReadOnly<ExternalNotificationChannel>([
        Define<ServerChan, ServerChanConfig>("ServerChan", config => new(config), config => new ServerChanNotificationProvider(Instances.HttpService, config), "Server Chan"),
        Define<Telegram, TelegramConfig>("Telegram", config => new(config), config => new TelegramNotificationProvider(Instances.HttpService, config)),
        Define<Discord, DiscordConfig>("Discord", config => new(config), config => new DiscordNotificationProvider(Instances.HttpService, config)),
        Define<DingTalk, DingTalkConfig>("DingTalk", config => new(config), config => new DingTalkNotificationProvider(Instances.HttpService, config)),
        Define<Smtp, SmtpConfig>("SMTP", config => new(config), config => new SmtpNotificationProvider(config)),
        Define<Bark, BarkConfig>("Bark", config => new(config), config => new BarkNotificationProvider(Instances.HttpService, config)),
        Define<Qmsg, QmsgConfig>("Qmsg", config => new(config), config => new QmsgNotificationProvider(Instances.HttpService, config)),
        Define<Gotify, GotifyConfig>("Gotify", config => new(config), config => new GotifyNotificationProvider(Instances.HttpService, config)),
        Define<CustomWebhook, CustomWebhookConfig>("ExternalNotificationCustomWebhook", config => new(config), config => new CustomWebhookNotificationProvider(Instances.HttpService, config)),
    ]);

    public string DisplayName => LocalizationHelper.TryGetString(LocalizationKey, out var name) ? name : FallbackName;

    public static ExternalNotificationChannel ForConfig(Base config) =>
        All.FirstOrDefault(channel => channel.ConfigType == config.GetType())
        ?? throw new NotSupportedException($"Unsupported config type: {config.GetType()}");

    public static ExternalNotificationChannel? ForEditor(BaseConfig config) =>
        All.FirstOrDefault(channel => channel.EditorType == config.GetType());

    private static ExternalNotificationChannel Define<TConfig, TEditor>(string key,
        Func<TConfig, TEditor> readConfig, Func<TEditor, IExternalNotificationProvider> createProvider, string? displayName = null)
        where TConfig : Base
        where TEditor : BaseConfig, new() =>
        new(typeof(TConfig), typeof(TEditor), key, displayName ?? key,
            () => new TEditor(), config => readConfig((TConfig)config), config => createProvider((TEditor)config));
}
