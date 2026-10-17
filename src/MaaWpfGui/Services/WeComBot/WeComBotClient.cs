// <copyright file="WeComBotClient.cs" company="MaaAssistantArknights">
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
using System.Collections.Concurrent;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 企业微信智能机器人长连接客户端。
/// <para>
/// 协议见 <see href="https://developer.work.weixin.qq.com/document/path/101463"/>：
/// 连接后发送 <c>aibot_subscribe</c> 订阅，用 <c>ping</c>/<c>pong</c> 保活，
/// 收 <c>aibot_msg_callback</c>，用 <c>aibot_respond_msg</c> 被动回复或 <c>aibot_send_msg</c> 主动推送。
/// </para>
/// </summary>
public sealed class WeComBotClient : IDisposable
{
    private static readonly ILogger _logger = Log.ForContext<WeComBotClient>();
    private static readonly TimeSpan SubscribeTimeout = TimeSpan.FromSeconds(20);
    private static readonly TimeSpan RequestTimeout = TimeSpan.FromSeconds(15);
    private static readonly TimeSpan ReplyTimeout = TimeSpan.FromSeconds(8);
    private static readonly TimeSpan PingTimeout = TimeSpan.FromSeconds(10);

    private readonly ConcurrentDictionary<string, TaskCompletionSource<JObject>> _pending = new();
    private readonly SemaphoreSlim _sendLock = new(1, 1);
    private readonly CancellationTokenSource _stopSource = new();
    private readonly string _botId;
    private readonly string _secret;
    private readonly string _url;
    private readonly int _heartbeatSeconds;
    private ClientWebSocket? _socket;
    private Task? _runTask;
    private int _requestCounter;
    private bool _disposed;

    /// <summary>
    /// Initializes a new instance of the <see cref="WeComBotClient"/> class.
    /// </summary>
    /// <param name="botId">机器人 ID。</param>
    /// <param name="secret">机器人 Secret。</param>
    /// <param name="url">长连接地址。</param>
    /// <param name="heartbeatSeconds">心跳间隔（秒）。</param>
    public WeComBotClient(string botId, string secret, string url, int heartbeatSeconds)
    {
        _botId = botId;
        _secret = secret;
        _url = url;
        _heartbeatSeconds = Math.Clamp(heartbeatSeconds, 5, 600);
    }

    /// <summary>
    /// 收到机器人消息回调等业务帧时触发，参数为整帧 JSON。
    /// </summary>
    public event Func<JObject, Task>? MessageReceived;

    /// <summary>
    /// Gets a value indicating whether 长连接当前已建立并订阅成功。
    /// </summary>
    public bool IsConnected { get; private set; }

    /// <summary>
    /// 启动长连接，断线自动重连（指数退避）。
    /// </summary>
    public void Start()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        _runTask ??= Task.Run(() => RunAsync(_stopSource.Token));
    }

    /// <summary>
    /// 关闭长连接并等待后台循环退出。
    /// </summary>
    /// <returns>异步任务。</returns>
    public async Task StopAsync()
    {
        if (!_stopSource.IsCancellationRequested)
        {
            await _stopSource.CancelAsync().ConfigureAwait(false);
        }

        Abort();

        var runTask = _runTask;
        _runTask = null;
        if (runTask is not null)
        {
            try
            {
                await runTask.ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                _logger.Debug(ex, "WeCom bot client stopped with error");
            }
        }
    }

    /// <summary>
    /// 主动推送 markdown 消息到指定会话。
    /// </summary>
    /// <param name="chatId">会话 ID。</param>
    /// <param name="chatType">会话类型，1 为单聊，2 为群聊。</param>
    /// <param name="content">markdown 正文。</param>
    /// <returns>发送成功返回 <c>true</c>。</returns>
    public async Task<bool> SendMarkdownAsync(string chatId, int chatType, string content)
    {
        if (string.IsNullOrEmpty(chatId))
        {
            _logger.Warning("WeCom bot push skipped: chat id is empty");
            return false;
        }

        var body = new JObject {
            ["chatid"] = chatId,
            ["msgtype"] = "markdown",
            ["markdown"] = new JObject { ["content"] = content },
        };
        if (chatType != 0)
        {
            body["chat_type"] = chatType;
        }

        var response = await RequestAsync(new JObject {
            ["cmd"] = "aibot_send_msg",
            ["body"] = body,
        }, RequestTimeout).ConfigureAwait(false);

        return IsSuccess(response, "push message");
    }

    /// <summary>
    /// 用消息回调的 <c>req_id</c> 被动回复 markdown，长连接模式下的推荐回复方式。
    /// </summary>
    /// <param name="callbackRequestId">消息回调帧里的 req_id。</param>
    /// <param name="content">markdown 正文。</param>
    /// <returns>回复成功返回 <c>true</c>。</returns>
    public async Task<bool> ReplyMarkdownAsync(string callbackRequestId, string content)
    {
        if (string.IsNullOrEmpty(callbackRequestId))
        {
            return false;
        }

        var payload = new JObject {
            ["cmd"] = "aibot_respond_msg",
            ["headers"] = new JObject { ["req_id"] = callbackRequestId },
            ["body"] = new JObject {
                ["msgtype"] = "markdown",
                ["markdown"] = new JObject { ["content"] = content },
            },
        };

        var response = await RequestAsync(payload, ReplyTimeout, callbackRequestId).ConfigureAwait(false);
        return IsSuccess(response, "reply message");
    }

    /// <inheritdoc/>
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        if (!_stopSource.IsCancellationRequested)
        {
            _stopSource.Cancel();
        }

        Abort();
        _stopSource.Dispose();
        _sendLock.Dispose();
    }

    private static JObject? ParseFrame(string raw)
    {
        try
        {
            return JObject.Parse(raw);
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "WeCom bot frame is not valid json");
            return null;
        }
    }

    private static bool IsSuccess(JObject? response, string action)
    {
        if (response is null)
        {
            _logger.Warning("WeCom bot {Action} failed: no response", action);
            return false;
        }

        if (response.Value<int?>("errcode") is 0)
        {
            _logger.Information("WeCom bot {Action} succeeded", action);
            return true;
        }

        _logger.Warning("WeCom bot {Action} failed: {Response}", action, response.ToString(Newtonsoft.Json.Formatting.None));
        return false;
    }

    private async Task RunAsync(CancellationToken token)
    {
        var backoffSeconds = 3;
        while (!token.IsCancellationRequested)
        {
            try
            {
                using var socket = new ClientWebSocket();
                socket.Options.KeepAliveInterval = Timeout.InfiniteTimeSpan;
                _socket = socket;

                await socket.ConnectAsync(new Uri(_url), token).ConfigureAwait(false);
                if (!await SubscribeAsync(socket, token).ConfigureAwait(false))
                {
                    throw new InvalidOperationException("subscribe rejected");
                }

                IsConnected = true;
                backoffSeconds = 3;
                _logger.Information("WeCom bot long connection established");

                using var sessionSource = CancellationTokenSource.CreateLinkedTokenSource(token);
                var heartbeatTask = HeartbeatLoopAsync(socket, sessionSource);
                try
                {
                    await ReceiveLoopAsync(socket, sessionSource.Token).ConfigureAwait(false);
                }
                finally
                {
                    await sessionSource.CancelAsync().ConfigureAwait(false);
                    await WaitQuietlyAsync(heartbeatTask).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                break;
            }
            catch (Exception ex)
            {
                _logger.Warning("WeCom bot connection error: {Message}", ex.Message);
            }
            finally
            {
                IsConnected = false;
                _socket = null;
                FailPendingRequests();
            }

            if (token.IsCancellationRequested)
            {
                break;
            }

            // 半死连接可能连 CloseAsync 都会卡住，直接掐掉 TCP 保证能重连。
            try
            {
                await Task.Delay(TimeSpan.FromSeconds(backoffSeconds), token).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                break;
            }

            backoffSeconds = Math.Min(60, backoffSeconds * 2);
        }

        IsConnected = false;
        _logger.Information("WeCom bot long connection loop exited");
    }

    private async Task<bool> SubscribeAsync(ClientWebSocket socket, CancellationToken token)
    {
        var requestId = NextRequestId();
        var payload = new JObject {
            ["cmd"] = "aibot_subscribe",
            ["headers"] = new JObject { ["req_id"] = requestId },
            ["body"] = new JObject { ["bot_id"] = _botId, ["secret"] = _secret },
        };

        await SendRawAsync(socket, payload, token).ConfigureAwait(false);

        var deadline = DateTimeOffset.UtcNow + SubscribeTimeout;
        while (DateTimeOffset.UtcNow < deadline)
        {
            var remaining = deadline - DateTimeOffset.UtcNow;
            var frame = await ReceiveFrameAsync(socket, remaining, token).ConfigureAwait(false);
            if (frame is null)
            {
                break;
            }

            if (frame.SelectToken("headers.req_id")?.Value<string>() == requestId)
            {
                var code = frame.Value<int?>("errcode") ?? -1;
                _logger.Information("WeCom bot subscribe result: {Code} {Message}", code, frame.Value<string>("errmsg") ?? string.Empty);
                return code == 0;
            }

            await HandleFrameAsync(frame).ConfigureAwait(false);
        }

        return false;
    }

    private async Task ReceiveLoopAsync(ClientWebSocket socket, CancellationToken token)
    {
        while (!token.IsCancellationRequested && socket.State == WebSocketState.Open)
        {
            var frame = await ReceiveFrameAsync(socket, Timeout.InfiniteTimeSpan, token).ConfigureAwait(false);
            if (frame is null)
            {
                throw new WebSocketException("remote closed the connection");
            }

            await HandleFrameAsync(frame).ConfigureAwait(false);
        }
    }

    private async Task HeartbeatLoopAsync(ClientWebSocket socket, CancellationTokenSource sessionSource)
    {
        var token = sessionSource.Token;
        while (!token.IsCancellationRequested)
        {
            try
            {
                await Task.Delay(TimeSpan.FromSeconds(_heartbeatSeconds), token).ConfigureAwait(false);
                var reply = await RequestCoreAsync(
                    socket,
                    new JObject { ["cmd"] = "ping" },
                    PingTimeout,
                    requestId: null,
                    token: token).ConfigureAwait(false);

                if (reply is null)
                {
                    _logger.Warning("WeCom bot heartbeat timed out, rebuilding the connection");
                    await sessionSource.CancelAsync().ConfigureAwait(false);
                    socket.Abort();
                    return;
                }
            }
            catch (OperationCanceledException)
            {
                return;
            }
            catch (Exception ex)
            {
                _logger.Warning("WeCom bot heartbeat failed: {Message}", ex.Message);
                await sessionSource.CancelAsync().ConfigureAwait(false);
                Abort();
                return;
            }
        }
    }

    private async Task HandleFrameAsync(JObject frame)
    {
        var requestId = frame.SelectToken("headers.req_id")?.Value<string>();
        if (!string.IsNullOrEmpty(requestId) && _pending.TryRemove(requestId, out var pending))
        {
            pending.TrySetResult(frame);
            return;
        }

        var command = frame.Value<string>("cmd") ?? string.Empty;
        if (command.Length == 0)
        {
            return;
        }

        if (command == "ping")
        {
            if (_socket is { } socket && socket.State == WebSocketState.Open)
            {
                await SendRawAsync(socket, new JObject { ["cmd"] = "pong", ["headers"] = new JObject { ["req_id"] = NextRequestId() } }, _stopSource.Token).ConfigureAwait(false);
            }

            return;
        }

        if (command == "aibot_event_callback"
            && frame.SelectToken("body.event.eventtype")?.Value<string>() == "disconnected_event")
        {
            // 同一机器人只允许一条长连接，被新连接顶掉后必须重建。
            _logger.Warning("WeCom bot connection was replaced by a newer one, reconnecting");
            Abort();
            return;
        }

        _logger.Debug("WeCom bot received frame: {Command}", command);
        if (MessageReceived is { } handler)
        {
            try
            {
                await handler(frame).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "WeCom bot message handler failed");
            }
        }
    }

    private async Task<JObject?> RequestAsync(JObject payload, TimeSpan timeout, string? requestId = null)
    {
        var socket = _socket;
        if (socket is null || socket.State != WebSocketState.Open)
        {
            _logger.Warning("WeCom bot request skipped: connection is not open");
            return null;
        }

        return await RequestCoreAsync(socket, payload, timeout, requestId, _stopSource.Token).ConfigureAwait(false);
    }

    private async Task<JObject?> RequestCoreAsync(
        ClientWebSocket socket,
        JObject payload,
        TimeSpan timeout,
        string? requestId,
        CancellationToken token)
    {
        var id = string.IsNullOrEmpty(requestId) ? NextRequestId() : requestId;
        payload["headers"] = new JObject { ["req_id"] = id };

        var completion = new TaskCompletionSource<JObject>(TaskCreationOptions.RunContinuationsAsynchronously);
        _pending[id] = completion;
        try
        {
            await SendRawAsync(socket, payload, token).ConfigureAwait(false);
            return await completion.Task.WaitAsync(timeout, token).ConfigureAwait(false);
        }
        catch (TimeoutException)
        {
            _logger.Warning("WeCom bot request timed out: {Payload}", payload.Value<string>("cmd"));
            return null;
        }
        catch (OperationCanceledException)
        {
            return null;
        }
        finally
        {
            _pending.TryRemove(id, out _);
        }
    }

    private async Task SendRawAsync(ClientWebSocket socket, JObject payload, CancellationToken token)
    {
        var bytes = Encoding.UTF8.GetBytes(payload.ToString(Newtonsoft.Json.Formatting.None));
        await _sendLock.WaitAsync(token).ConfigureAwait(false);
        try
        {
            await socket.SendAsync(bytes, WebSocketMessageType.Text, true, token).ConfigureAwait(false);
        }
        finally
        {
            _sendLock.Release();
        }
    }

    private static async Task<JObject?> ReceiveFrameAsync(ClientWebSocket socket, TimeSpan timeout, CancellationToken token)
    {
        using var timeoutSource = CancellationTokenSource.CreateLinkedTokenSource(token);
        if (timeout != Timeout.InfiniteTimeSpan)
        {
            timeoutSource.CancelAfter(timeout);
        }

        var buffer = new byte[8192];
        using var stream = new System.IO.MemoryStream();
        while (true)
        {
            WebSocketReceiveResult result;
            try
            {
                result = await socket.ReceiveAsync(new ArraySegment<byte>(buffer), timeoutSource.Token).ConfigureAwait(false);
            }
            catch (OperationCanceledException) when (!token.IsCancellationRequested)
            {
                return null;
            }
            catch (WebSocketException)
            {
                return null;
            }

            if (result.MessageType == WebSocketMessageType.Close)
            {
                return null;
            }

            stream.Write(buffer, 0, result.Count);
            if (result.EndOfMessage)
            {
                break;
            }
        }

        return ParseFrame(Encoding.UTF8.GetString(stream.ToArray()));
    }

    private static async Task WaitQuietlyAsync(Task task)
    {
        try
        {
            await task.ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "WeCom bot background loop ended with error");
        }
    }

    private void FailPendingRequests()
    {
        foreach (var pair in _pending)
        {
            if (_pending.TryRemove(pair.Key, out var pending))
            {
                pending.TrySetResult(null!);
            }
        }
    }

    private void Abort()
    {
        var socket = _socket;
        if (socket is null)
        {
            return;
        }

        try
        {
            socket.Abort();
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "WeCom bot socket abort failed");
        }
    }

    private string NextRequestId() => $"{Interlocked.Increment(ref _requestCounter):x}-{Guid.NewGuid():N}";
}
