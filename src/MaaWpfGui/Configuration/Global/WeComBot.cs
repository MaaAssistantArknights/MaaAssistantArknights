// <copyright file="WeComBot.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Models;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Global;

/// <summary>
/// 企业微信智能机器人设置（长连接模式）。
/// 机器人凭据按机器维度保存，账号轮换列表引用各配置档案，故挂在全局配置而非单个档案下。
/// </summary>
public class WeComBot : NotifyPropertyChangedWithValue, IJsonOnDeserialized
{
    /// <summary>
    /// 企业微信智能机器人长连接地址。
    /// </summary>
    public const string DefaultWebSocketUrl = "wss://openws.work.weixin.qq.com";

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Accounts.CollectionChanged += Handler.OnCollectionChangedFactory<WeComBotAccount>(prefix + nameof(Accounts) + ".");
        Schedules.CollectionChanged += Handler.OnCollectionChangedFactory<WeComBotSchedule>(prefix + nameof(Schedules) + ".");
        Accounts.CollectionChanged += (_, e) => {
            foreach (var account in e.OldItems?.OfType<WeComBotAccount>() ?? [])
            {
                account.PropertyChanged -= Handler.OnPropertyChangedFactory(prefix + nameof(Accounts) + ".");
            }

            foreach (var account in e.NewItems?.OfType<WeComBotAccount>() ?? [])
            {
                account.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Accounts) + ".");
            }
        };
        Schedules.CollectionChanged += (_, e) => {
            foreach (var schedule in e.OldItems?.OfType<WeComBotSchedule>() ?? [])
            {
                schedule.PropertyChanged -= Handler.OnPropertyChangedFactory(prefix + nameof(Schedules) + ".");
            }

            foreach (var schedule in e.NewItems?.OfType<WeComBotSchedule>() ?? [])
            {
                schedule.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Schedules) + ".");
            }
        };
        foreach (WeComBotAccount account in Accounts)
        {
            account.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Accounts) + ".");
        }

        foreach (WeComBotSchedule schedule in Schedules)
        {
            schedule.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Schedules) + ".");
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 启用企业微信机器人（长连接）。
    /// </summary>
    public bool Enable { get; set; }

    /// <summary>
    /// Gets or sets 机器人 ID，企业微信管理后台「智能机器人」页面获取。
    /// </summary>
    public string BotId { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 机器人 Secret。
    /// </summary>
    public string Secret { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 长连接地址，一般无需修改。
    /// </summary>
    public string WebSocketUrl { get; set; } = DefaultWebSocketUrl;

    /// <summary>
    /// Gets or sets 心跳间隔（秒）。
    /// </summary>
    public int HeartbeatSeconds { get; set; } = 10;

    /// <summary>
    /// Gets or sets 推送会话 ID，为空时由机器人收到的第一条消息自动记录。
    /// </summary>
    public string ChatId { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 推送会话类型，1 为单聊，2 为群聊。
    /// </summary>
    public int ChatType { get; set; } = 1;

    /// <summary>
    /// Gets or sets a value indicating whether 推送任务开始、完成、出错、停滞等状态。
    /// </summary>
    public bool NotifyTaskStatus { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 接收并执行聊天中的指令。
    /// </summary>
    public bool EnableRemoteCommand { get; set; } = true;

    /// <summary>
    /// Gets or sets 允许下发指令的用户 ID 列表，逗号分隔，留空表示不限制。
    /// </summary>
    public string AllowedUserIds { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 用大模型理解自然语言指令。
    /// </summary>
    public bool EnableAiIntent { get; set; }

    /// <summary>
    /// Gets or sets DeepSeek API Key。
    /// </summary>
    public string DeepSeekApiKey { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets DeepSeek 模型名。
    /// </summary>
    public string DeepSeekModel { get; set; } = "deepseek-flash";

    /// <summary>
    /// Gets or sets a value indicating whether 一次「清日常」依次跑完 <see cref="Accounts"/> 中的全部账号。
    /// </summary>
    public bool RotateAccounts { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 启用机器人自带的每日定时清日常。
    /// </summary>
    public bool EnableSchedule { get; set; }

    /// <summary>
    /// Gets 账号轮换列表，每项对应一个配置档案，可单独指定理智作战关卡。
    /// </summary>
    [JsonInclude]
    public ObservableCollection<WeComBotAccount> Accounts { get; private set; } = [];

    /// <summary>
    /// Gets 定时清日常时间列表。
    /// </summary>
    [JsonInclude]
    public ObservableCollection<WeComBotSchedule> Schedules { get; private set; } = [
        new() { Hour = 8, Minute = 0 },
        new() { Hour = 20, Minute = 0 },
    ];

    /// <summary>
    /// Gets 允许下发指令的用户 ID 集合。
    /// </summary>
    [JsonIgnore]
    public string[] AllowedUserList => AllowedUserIds.Split([',', '，', ';', '；', ' '], StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

    public void OnDeserialized()
    {
        BotId ??= string.Empty;
        Secret ??= string.Empty;
        ChatId ??= string.Empty;
        AllowedUserIds ??= string.Empty;
        DeepSeekApiKey ??= string.Empty;
        WebSocketUrl = string.IsNullOrWhiteSpace(WebSocketUrl) ? DefaultWebSocketUrl : WebSocketUrl;
        HeartbeatSeconds = Math.Clamp(HeartbeatSeconds, 5, 600);
        ChatType = ChatType == 2 ? 2 : 1;
        Accounts ??= [];
        Schedules ??= [];
        foreach (var schedule in Schedules)
        {
            schedule.Hour = Math.Clamp(schedule.Hour, 0, 23);
            schedule.Minute = Math.Clamp(schedule.Minute, 0, 59);
        }
    }
}
