// <copyright file="TelegramConfig.cs" company="MaaAssistantArknights">
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

public class TelegramConfig(string botToken = "", string chatId = "", string topicId = "") : BaseConfig
{
    public TelegramConfig()
        : this(string.Empty, string.Empty, string.Empty) // 无参给设计器用
    {
    }

    public TelegramConfig(Telegram telegram)
        : this(SimpleEncryptionHelper.Decrypt(telegram.BotToken), SimpleEncryptionHelper.Decrypt(telegram.ChatId), SimpleEncryptionHelper.Decrypt(telegram.TopicId))
    {
    }

    public string BotToken { get; set => SetAndNotify(ref field, value); } = botToken;

    public string ChatId { get; set => SetAndNotify(ref field, value); } = chatId;

    public string TopicId { get; set => SetAndNotify(ref field, value); } = topicId;

    public override Telegram ToConfig() => new(SimpleEncryptionHelper.Encrypt(BotToken), SimpleEncryptionHelper.Encrypt(ChatId), SimpleEncryptionHelper.Encrypt(TopicId));
}
