// <copyright file="RemoteControlProgressReporter.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Services.RemoteControl;

/// <summary>
/// 远程控制逐任务进度汇报聚合器。
/// <para>远程下发的一次任务运行期间，把任务列表里每一项的开始/结束转成中间汇报发送到汇报任务端点：</para>
/// <para>中间汇报复用现有报文 {user, device, task, status, payload}，status 固定为 "RUNNING"，payload 为 UTF-8 JSON 文本，</para>
/// <para>事件依次为 QUEUED（计划）、TASK_START / TASK_END（逐任务）、ALL_COMPLETED（收尾汇总）。</para>
/// </summary>
public static class RemoteControlProgressReporter
{
    private sealed class ItemResult
    {
        public string Name { get; set; } = string.Empty;

        public string Type { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 项级结果：PENDING（待执行）/ SKIPPED / SUCCESS / FAILED / STOPPED
        /// </summary>
        public string Result { get; set; } = "SKIPPED";

        public bool Started { get; set; }

        public bool Ended { get; set; }

        public List<DropItem> Drops { get; } = [];
    }

    private sealed class DropItem
    {
        [Newtonsoft.Json.JsonProperty("id")]
        public string Id { get; set; } = string.Empty;

        [Newtonsoft.Json.JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 累计数量（回调里 stats.quantity 的最新值）
        /// </summary>
        [Newtonsoft.Json.JsonProperty("count")]
        public int Count { get; set; }

        /// <summary>
        /// Gets or sets 本次新增数量（多次关卡结算累加）
        /// </summary>
        [Newtonsoft.Json.JsonProperty("add")]
        public int Add { get; set; }
    }

    private static readonly object Gate = new();

    private static readonly Dictionary<int, ItemResult> Items = new();

    private static string _remoteTaskId = string.Empty;

    private static long _seq;

    private static bool _progressReportEnabled;

    /// <summary>
    /// 由获取任务端点的响应驱动（<c>progressReport</c> 字段，每轮轮询刷新）：
    /// 仅当服务端声明支持后才会发送中间汇报，缺省关闭，与未升级的服务端完全兼容。
    /// </summary>
    public static void SetEnabled(bool enabled)
    {
        lock (Gate)
        {
            if (_progressReportEnabled == enabled)
            {
                return;
            }

            Log.Logger.Information("RemoteControl progress report {State} by server declaration.", enabled ? "enabled" : "disabled");
            _progressReportEnabled = enabled;
        }
    }

    /// <summary>
    /// 远程任务开始执行前调用：记录任务列表快照并发送 QUEUED 计划汇报。
    /// </summary>
    /// <param name="remoteTaskId">远程下发的任务 id。</param>
    /// <param name="typeFilter">单模块执行（LinkStart-XXX）时的配置任务类型名；全量 LinkStart 传 null。</param>
    public static void BeginRun(string remoteTaskId, string? typeFilter = null)
    {
        lock (Gate)
        {
            if (!_progressReportEnabled)
            {
                return;
            }

            _remoteTaskId = remoteTaskId;
            _seq = 0;
            Items.Clear();

            var queue = ConfigFactory.CurrentConfig.TaskQueue;
            for (var index = 0; index < queue.Count; index++)
            {
                var task = queue[index];
                var enabled = typeFilter is null
                    ? task.IsEnable == true
                    : string.Equals(task.TaskType.ToString(), typeFilter, StringComparison.OrdinalIgnoreCase);
                Items[index] = new ItemResult
                {
                    Name = task.NameOrTaskType,
                    Type = task.TaskType.ToString(),
                    Result = enabled ? "PENDING" : "SKIPPED",
                };
            }
        }

        Send("QUEUED", new JObject
        {
            ["plan"] = JToken.FromObject(Snapshot()),
        });
    }

    /// <summary>
    /// 订阅 <see cref="Main.AsstProxy.OnTaskStatusChanged"/> 后由任务链回调驱动；仅在远程任务运行期间生效。
    /// </summary>
    /// <param name="taskId">Core 任务链 id。</param>
    /// <param name="status">链状态。</param>
    public static void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            return;
        }

        switch (status)
        {
            case TaskItemStatus.InProgress:
                {
                    ItemResult item;
                    bool firstStart;
                    lock (Gate)
                    {
                        item = EnsureItem(index);
                        firstStart = !item.Started;
                        item.Started = true;
                        if (item.Result == "SKIPPED")
                        {
                            item.Result = "PENDING";
                        }
                    }

                    if (firstStart)
                    {
                        Send("TASK_START", ItemPayload(index, item));
                    }

                    break;
                }

            case TaskItemStatus.Completed:
            case TaskItemStatus.Error:
                {
                    var result = status == TaskItemStatus.Error ? "FAILED" : "SUCCESS";
                    ItemResult item;
                    bool shouldSendEnd;
                    lock (Gate)
                    {
                        item = EnsureItem(index);
                        item.Started = true;

                        // 首次终态必发；Error 一旦出现，后续 Completed 不再重复发送
                        shouldSendEnd = !item.Ended || (item.Result != "FAILED" && result == "FAILED");
                        item.Ended = true;
                        if (result == "FAILED" || item.Result != "FAILED")
                        {
                            item.Result = result;
                        }
                    }

                    if (shouldSendEnd)
                    {
                        SendTaskEnd(index, item);
                    }

                    break;
                }
        }
    }

    /// <summary>
    /// 归集关卡结算掉落明细（来自 StageDrops 回调），附加到对应任务项的 TASK_END 汇报里。
    /// </summary>
    /// <param name="taskId">当前任务链 id。</param>
    /// <param name="drops">回调解析出的掉落列表。</param>
    public static void AppendDrops(int taskId, IReadOnlyList<(string ItemId, string ItemName, int Total, int Add)> drops)
    {
        if (drops.Count == 0)
        {
            return;
        }

        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            return;
        }

        lock (Gate)
        {
            var item = EnsureItem(index);
            foreach (var (itemId, itemName, total, add) in drops)
            {
                if (string.IsNullOrEmpty(itemId))
                {
                    continue;
                }

                var existing = item.Drops.FirstOrDefault(d => d.Id == itemId);
                if (existing is null)
                {
                    item.Drops.Add(new DropItem { Id = itemId, Name = itemName, Count = total, Add = add });
                }
                else
                {
                    // stats.quantity 是累计值，直接刷新；add 逐次累加
                    existing.Count = total;
                    existing.Add += add;
                }
            }
        }
    }

    /// <summary>
    /// 运行收尾时调用：未正常结束的项标记 STOPPED，发送 ALL_COMPLETED 汇总并复位状态。
    /// </summary>
    public static void CompleteRun()
    {
        string remoteTaskId;
        List<object> summary;
        bool enabled;
        lock (Gate)
        {
            if (string.IsNullOrEmpty(_remoteTaskId))
            {
                return;
            }

            foreach (var item in Items.Values)
            {
                if (item.Started && !item.Ended)
                {
                    item.Result = "STOPPED";
                }
                else if (!item.Started && item.Result == "PENDING")
                {
                    item.Result = "STOPPED";
                }
            }

            remoteTaskId = _remoteTaskId;
            enabled = _progressReportEnabled;
            summary = Snapshot();
            _remoteTaskId = string.Empty;
            Items.Clear();
        }

        // 服务端中途关闭进度汇报时不再补发汇总，仅复位状态。
        // 开关在锁内随任务 id 一并捕获：轮询线程可能在复位后切换声明，
        // 收尾决策必须基于本次运行期间最后观察到的状态，而非锁外实时值
        if (!enabled)
        {
            return;
        }

        // 汇总必须带显式 taskId 发送：此时运行状态已复位，Send 的活跃检查不再适用
        Send("ALL_COMPLETED", new JObject
        {
            ["summary"] = JToken.FromObject(summary),
        }, remoteTaskId);
    }

    private static void SendTaskEnd(int index, ItemResult item)
    {
        var payload = ItemPayload(index, item);
        payload["result"] = item.Result;
        if (item.Drops.Count > 0)
        {
            payload["drops"] = JToken.FromObject(item.Drops);
        }

        Send("TASK_END", payload);
    }

    private static JObject ItemPayload(int index, ItemResult item)
    {
        return new JObject
        {
            ["index"] = index,
            ["name"] = item.Name,
            ["type"] = item.Type,
        };
    }

    private static List<object> Snapshot()
    {
        return Items.OrderBy(p => p.Key)
            .Select(p =>
            {
                var entry = new JObject
                {
                    ["index"] = p.Key,
                    ["name"] = p.Value.Name,
                    ["type"] = p.Value.Type,
                    ["result"] = p.Value.Result,
                };
                if (p.Value.Drops.Count > 0)
                {
                    entry["drops"] = JToken.FromObject(p.Value.Drops);
                }

                return (object)entry;
            })
            .ToList();
    }

    private static int FindIndexByTaskId(int taskId)
    {
        return Instances.TaskQueueViewModel.TaskItemViewModels
            .FirstOrDefault(i => i.TaskIds.Contains(taskId))?.Index ?? -1;
    }

    private static ItemResult EnsureItem(int index)
    {
        if (Items.TryGetValue(index, out var item))
        {
            return item;
        }

        var queue = ConfigFactory.CurrentConfig.TaskQueue;
        var name = index >= 0 && index < queue.Count ? queue[index].NameOrTaskType : string.Empty;
        var type = index >= 0 && index < queue.Count ? queue[index].TaskType.ToString() : string.Empty;
        return Items[index] = new ItemResult { Name = name, Type = type, Result = "PENDING", Started = true };
    }

    /// <summary>
    /// seq 在锁内按调用顺序分配后再异步发送，保证服务端可按 seq 恢复时序。
    /// </summary>
    private static void Send(string eventName, JObject? extra = null, string? explicitTaskId = null)
    {
        long seq;
        string remoteTaskId;
        string payload;

        lock (Gate)
        {
            if (!_progressReportEnabled)
            {
                return;
            }

            // explicitTaskId 供 ALL_COMPLETED 在状态复位后使用；常规中间汇报仍要求运行中
            remoteTaskId = explicitTaskId ?? _remoteTaskId;
            if (string.IsNullOrEmpty(remoteTaskId))
            {
                return;
            }

            seq = ++_seq;
            var payloadObj = new JObject
            {
                ["seq"] = seq,
                ["event"] = eventName,
            };
            if (extra is not null)
            {
                payloadObj.Merge(extra);
            }

            payload = payloadObj.ToString(Newtonsoft.Json.Formatting.None);
        }

        _ = DispatchAsync(remoteTaskId, payload);
    }

    private static async Task DispatchAsync(string remoteTaskId, string payload)
    {
        try
        {
            var endpoint = SettingsViewModel.RemoteControlSettings.RemoteControlReportStatusUri;
            if (!RemoteControlService.IsEndpointValid(endpoint))
            {
                return;
            }

            var uid = SettingsViewModel.RemoteControlSettings.RemoteControlUserIdentity;
            var did = SettingsViewModel.RemoteControlSettings.RemoteControlDeviceIdentity;
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(endpoint), new
            {
                user = uid,
                device = did,
                task = remoteTaskId,
                status = "RUNNING",
                payload,
            });
            if (response == null)
            {
                Log.Logger.Warning("RemoteControl progress report failed.");
            }
        }
        catch (Exception ex)
        {
            Log.Logger.Error(ex, "RemoteControl progress report raises error.");
        }
    }
}
