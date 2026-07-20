// <copyright file="ExternalNotification.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration.Single.Settings;

public class ExternalNotification : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    [JsonInclude]
    public ObservableCollection<Base> Configs { get; private set; } = [];

    public bool SendWhenComplete { get; set; } = true;

    public bool ShowWhenCompleteWithDetails { get; set; }

    public bool SendWhenError { get; set; } = true;

    public bool SendWhenStalled { get; set; }

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
