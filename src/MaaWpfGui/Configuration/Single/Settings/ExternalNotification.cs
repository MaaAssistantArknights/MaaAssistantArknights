// <copyright file="ExternalNotification.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Models;
using MaaWpfGui.Services.Notification;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 外部通知设置
/// </summary>
public partial class ExternalNotification : NotifyPropertyChangedWithValue, IJsonOnDeserialized
{
    [JsonInclude]
    public ObservableCollection<Base> Configs { get; private set; } = [];

    public bool SendBeforeScheduledStart { get; set; }

    [JsonInclude]
    public ContentSettings Content { get; private set; } = new();

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Content.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Content) + ".");
        Configs.CollectionChanged += Handler.OnCollectionChangedFactory<Base>(prefix);
    }

    public void OnDeserialized() => Content ??= new();

    public class ContentSettings : NotifyPropertyChangedWithValue, IJsonOnDeserialized
    {
        public bool UseIndependent { get; set; }

        public bool Enable { get; set; } = true;

        public NotificationFilterMode FilterMode { get; set; } = NotificationFilterMode.Whitelist;

        public string FilterList { get; set; } = string.Join("|",
            new[] { NotificationTag.TaskError, NotificationTag.TaskComplete, NotificationTag.Stalled }.Select(NotificationMessage.FormatTag));

        public int MaxEntries { get; set; } = 100;

        public int TimeMinutes { get; set; } = 60;

        public void OnDeserialized()
        {
            FilterList ??= string.Empty;
            MaxEntries = Math.Clamp(MaxEntries, 0, 10000);
            TimeMinutes = Math.Clamp(TimeMinutes, 0, 10080);
        }
    }

    [JsonDerivedType(typeof(Smtp), typeDiscriminator: nameof(Smtp))]
    [JsonDerivedType(typeof(ServerChan), typeDiscriminator: nameof(ServerChan))]
    [JsonDerivedType(typeof(Discord), typeDiscriminator: nameof(Discord))]
    [JsonDerivedType(typeof(DingTalk), typeDiscriminator: nameof(DingTalk))]
    [JsonDerivedType(typeof(Telegram), typeDiscriminator: nameof(Telegram))]
    [JsonDerivedType(typeof(Bark), typeDiscriminator: nameof(Bark))]
    [JsonDerivedType(typeof(Qmsg), typeDiscriminator: nameof(Qmsg))]
    [JsonDerivedType(typeof(Gotify), typeDiscriminator: nameof(Gotify))]
    [JsonDerivedType(typeof(CustomWebhook), typeDiscriminator: nameof(CustomWebhook))]
    public record class Base();

    public record class Smtp(string Server = "", string Port = "", string User = "", string Password = "", string From = "", string To = "", bool UseSsl = false, bool RequiresAuthentication = false) : Base;

    public record class ServerChan(string SendKey = "") : Base;

    public record class Discord(string BotToken = "", string UserId = "") : Base;

    public record class DingTalk(string AccessToken = "", string Secret = "") : Base;

    public record class Telegram(string BotToken = "", string ChatId = "", string TopicId = "") : Base;

    public record class Bark(string SendKey = "", string Server = "") : Base;

    public record class Qmsg(string Server = "", string Key = "", string User = "", string Bot = "") : Base;

    public record class Gotify(string Server = "", string Token = "") : Base;

    public record class CustomWebhook(string Url = "", string Headers = "", string Body = "") : Base;
}
