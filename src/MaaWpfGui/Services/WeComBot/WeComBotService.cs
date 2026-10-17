// <copyright file="WeComBotService.cs" company="MaaAssistantArknights">
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
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.Services.TaskExecution;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 企业微信机器人常驻服务：维护长连接、推送任务状态、执行聊天指令。
/// </summary>
public class WeComBotService : IDisposable
{
    private static readonly ILogger _logger = Log.ForContext<WeComBotService>();
    private readonly SemaphoreSlim _lifecycleLock = new(1, 1);
    private readonly WeComBotCommandService _commands;
    private WeComBotClient? _client;
    private string _clientSignature = string.Empty;
    private string _lastErrorLog = string.Empty;
    private DateTimeOffset _lastErrorLogAt;
    private bool _disposed;

    /// <summary>
    /// Initializes a new instance of the <see cref="WeComBotService"/> class.
    /// </summary>
    public WeComBotService()
    {
        _commands = new WeComBotCommandService();

        RunningState.Instance.StateChanged += OnRunningStateChanged;
        Instances.NotificationService.MessagePublished += OnMessagePublished;
        Instances.NotificationService.ErrorLogPublished += OnErrorLogPublished;
    }

    private static Configuration.Global.WeComBot Settings => ConfigFactory.Root.WeComBot;

    /// <summary>
    /// Gets a value indicating whether 长连接当前在线。
    /// </summary>
    public bool IsConnected => _client?.IsConnected ?? false;

    /// <summary>
    /// 依据当前设置启停长连接；设置变化后调用即可。
    /// </summary>
    /// <returns>异步任务。</returns>
    public async Task ApplySettingsAsync()
    {
        var settings = Settings;
        var signature = $"{settings.BotId}|{settings.Secret}|{settings.WebSocketUrl}|{settings.HeartbeatSeconds}";
        var shouldRun = settings.Enable
            && !string.IsNullOrWhiteSpace(settings.BotId)
            && !string.IsNullOrWhiteSpace(settings.Secret);

        await _lifecycleLock.WaitAsync().ConfigureAwait(false);
        try
        {
            if (!shouldRun)
            {
                await StopClientAsync().ConfigureAwait(false);
                if (settings.Enable)
                {
                    _logger.Warning("WeCom bot is enabled but BotId or Secret is empty");
                }

                return;
            }

            if (_client is not null && _clientSignature == signature)
            {
                return;
            }

            await StopClientAsync().ConfigureAwait(false);

            var client = new WeComBotClient(settings.BotId, settings.Secret, settings.WebSocketUrl, settings.HeartbeatSeconds);
            client.MessageReceived += OnFrameAsync;
            _client = client;
            _clientSignature = signature;
            client.Start();
            _logger.Information("WeCom bot service started");
        }
        finally
        {
            _lifecycleLock.Release();
        }
    }

    /// <summary>
    /// 主动推送一条 markdown 消息到已记录的会话。
    /// </summary>
    /// <param name="content">markdown 正文。</param>
    /// <returns>推送成功返回 <c>true</c>。</returns>
    public async Task<bool> PushAsync(string content)
    {
        var client = _client;
        var settings = Settings;
        if (client is null)
        {
            _logger.Warning("WeCom bot push skipped: service is not running");
            return false;
        }

        return await client.SendMarkdownAsync(settings.ChatId, settings.ChatType, content).ConfigureAwait(false);
    }

    /// <inheritdoc/>
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        RunningState.Instance.StateChanged -= OnRunningStateChanged;
        Instances.NotificationService.MessagePublished -= OnMessagePublished;
        Instances.NotificationService.ErrorLogPublished -= OnErrorLogPublished;
        _client?.Dispose();
        _client = null;
        _lifecycleLock.Dispose();
    }

    private static void FireAndForget(Task task, string action)
    {
        _ = task.ContinueWith(
            completed => _logger.Debug(completed.Exception, "WeCom bot {Action} failed", action),
            CancellationToken.None,
            TaskContinuationOptions.OnlyOnFaulted,
            TaskScheduler.Default);
    }

    private async Task StopClientAsync()
    {
        var client = _client;
        _client = null;
        _clientSignature = string.Empty;
        if (client is null)
        {
            return;
        }

        client.MessageReceived -= OnFrameAsync;
        await client.StopAsync().ConfigureAwait(false);
        client.Dispose();
        _logger.Information("WeCom bot service stopped");
    }

    private async Task OnFrameAsync(JObject frame)
    {
        if (frame.Value<string>("cmd") != "aibot_msg_callback")
        {
            return;
        }

        var body = frame["body"] as JObject ?? new JObject();
        var chatId = body.Value<string>("chatid") ?? body.SelectToken("from.userid")?.Value<string>() ?? string.Empty;
        var chatType = body.Value<string>("chattype") == "group" ? 2 : 1;
        var text = body.SelectToken("text.content")?.Value<string>()?.Trim() ?? string.Empty;
        var userId = body.SelectToken("from.userid")?.Value<string>() ?? string.Empty;
        var responseUrl = body.Value<string>("response_url") ?? string.Empty;
        var requestId = frame.SelectToken("headers.req_id")?.Value<string>() ?? string.Empty;

        RememberChat(chatId, chatType);

        if (!Settings.EnableRemoteCommand)
        {
            return;
        }

        var allowed = Settings.AllowedUserList;
        if (allowed.Length > 0 && !allowed.Contains(userId))
        {
            _logger.Warning("WeCom bot command from unauthorized user {UserId} ignored", userId);
            await ReplyAsync(requestId, responseUrl, LocalizationHelper.GetString("WeComBotPermissionDenied")).ConfigureAwait(false);
            return;
        }

        _logger.Information("WeCom bot command from user {UserId}", userId);
        var reply = await _commands.HandleAsync(text).ConfigureAwait(false);
        await ReplyAsync(requestId, responseUrl, reply).ConfigureAwait(false);
    }

    private void RememberChat(string chatId, int chatType)
    {
        if (string.IsNullOrEmpty(chatId))
        {
            return;
        }

        var settings = Settings;
        if (settings.ChatId == chatId && settings.ChatType == chatType)
        {
            return;
        }

        // 已记录推送目标后不再被其他聊天覆盖，避免陌生人把任务通知改发到自己那里
        if (!string.IsNullOrEmpty(settings.ChatId) && settings.ChatId != chatId)
        {
            _logger.Warning("WeCom bot ignored chat switch from {OldChat} to {NewChat}", settings.ChatId, chatId);
            return;
        }

        Execute.OnUIThread(() => {
            settings.ChatId = chatId;
            settings.ChatType = chatType;
            WeComBotSettingsUserControlModel.Instance.NotifyChatRemembered();
        });
        _logger.Information("WeCom bot remembered chat {ChatId} (type {ChatType})", chatId, chatType);
    }

    private async Task ReplyAsync(string requestId, string responseUrl, string content)
    {
        if (string.IsNullOrEmpty(content))
        {
            return;
        }

        if (!string.IsNullOrEmpty(responseUrl) && await ReplyViaUrlAsync(responseUrl, content).ConfigureAwait(false))
        {
            return;
        }

        var client = _client;
        if (client is not null && await client.ReplyMarkdownAsync(requestId, content).ConfigureAwait(false))
        {
            return;
        }

        await PushAsync(content).ConfigureAwait(false);
    }

    private async Task<bool> ReplyViaUrlAsync(string responseUrl, string content)
    {
        if (!Uri.TryCreate(responseUrl, UriKind.Absolute, out var uri))
        {
            return false;
        }

        var body = new {
            msgtype = "markdown",
            markdown = new { content },
        };

        try
        {
            var response = await Instances.HttpService.PostAsJsonAsync(uri, body, uriPartial: UriPartial.Authority).ConfigureAwait(false);
            if (string.IsNullOrWhiteSpace(response))
            {
                return false;
            }

            var code = JObject.Parse(response).Value<int?>("errcode");
            return code is null or 0;
        }
        catch (Exception ex)
        {
            _logger.Warning("WeCom bot response_url reply failed: {Message}", ex.Message);
            return false;
        }
    }

    private void OnMessagePublished(NotificationSource source, NotificationMessage message)
    {
        if (!Settings.NotifyTaskStatus || _client is null)
        {
            return;
        }

        var key = message.Kind switch {
            NotificationKind.TaskError => "WeComBotTaskError",
            NotificationKind.TaskComplete => "WeComBotTaskFinished",
            NotificationKind.Stalled => "WeComBotTaskStalled",
            _ => string.Empty,
        };
        if (key.Length == 0)
        {
            return;
        }

        var content = LocalizationHelper.GetStringFormat(key, CurrentAccountDisplay(), message.Title, message.Content);
        if (message.Kind == NotificationKind.TaskComplete
            && !message.Content.Contains(LocalizationHelper.GetString("TaskErrorSummaryTitle"), StringComparison.Ordinal))
        {
            content += "\n" + LocalizationHelper.GetString("WeComBotNoError");
        }

        FireAndForget(PushAsync(content), "task status push");
    }

    private void OnErrorLogPublished(NotificationSource source, string content)
    {
        if (!Settings.NotifyTaskStatus || _client is null)
        {
            return;
        }

        var now = DateTimeOffset.Now;
        if (content == _lastErrorLog && now - _lastErrorLogAt < TimeSpan.FromSeconds(10))
        {
            return;
        }

        _lastErrorLog = content;
        _lastErrorLogAt = now;
        FireAndForget(
            PushAsync(LocalizationHelper.GetStringFormat("WeComBotErrorLog", CurrentAccountDisplay(), content)),
            "error log push");
    }

    private void OnRunningStateChanged(object? sender, RunningState.RunningStateChangedEventArgs e)
    {
        if (!Settings.NotifyTaskStatus || _client is null)
        {
            return;
        }

        if (e.OldState.Idle && !e.NewState.Idle)
        {
            FireAndForget(
                PushAsync(LocalizationHelper.GetStringFormat("WeComBotTaskStarted", CurrentAccountDisplay())),
                "run start push");
        }
        else if (!e.OldState.Stopping && e.NewState.Stopping)
        {
            FireAndForget(PushAsync(LocalizationHelper.GetString("WeComBotTaskStopping")), "run stop push");
        }
    }

    private static string CurrentAccountDisplay()
    {
        var current = ConfigFactory.Root.Current;
        var name = ConfigFactory.Root.WeComBot.Accounts
            .FirstOrDefault(account => account.Configuration == current)?.Name;
        return string.IsNullOrWhiteSpace(name) ? current : $"{name}（{current}）";
    }
}
