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
public class RemoteControlProgressReporter
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

        /// <summary>
        /// Gets or sets 该任务项已结束的 core 任务链数量。
        /// </summary>
        public int ChainsDone { get; set; }

        /// <summary>
        /// Gets or sets 该任务项预期要执行的 core 任务链数量（下发时确认的 id 数量）。
        /// 库存保持等任务项会展开成多条链，需全部结束才可收尾。
        /// </summary>
        public int ChainsExpected { get; set; }

        public bool Ended { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 是否已发送过 TASK_END，避免多链任务项重复上报。
        /// </summary>
        public bool HasSentEnd { get; set; }

        /// <summary>
        /// Gets or sets 按关卡聚合的战斗统计（按首次结算顺序）。
        /// 一次任务项可能对应多条战斗链（如库存保持的多个计划），各链统计按关卡合并。
        /// </summary>
        public List<StageResult> StageResults { get; } = [];

        /// <summary>
        /// Gets or sets 最近一次公招识别的各标签组合保底星级（键由 <see cref="BuildTagKey"/> 规范化），
        /// 待该槽位实际选中标签后按选中集合取出对应值。
        /// </summary>
        public Dictionary<string, int> PendingCombinations { get; set; } = new(StringComparer.Ordinal);

        /// <summary>
        /// Gets or sets 各公招槽位最终选中标签组合的保底星级（按槽位顺序）。
        /// </summary>
        public List<int> MinLevels { get; } = [];

        /// <summary>
        /// Gets or sets 进图前识别的最后一次理智快照，随每轮战斗刷新；仅战斗链会产生。
        /// </summary>
        public SanitySnapshot? Sanity { get; set; }
    }

    /// <summary>
    /// 进图前识别的理智快照（<c>SanityBeforeStage</c>），战斗链每轮进图前刷新，收尾时取最后一次。
    /// </summary>
    private sealed class SanitySnapshot
    {
        public int Current { get; set; }

        public int Max { get; set; }

        public DateTimeOffset ReportTime { get; set; }
    }

    private sealed class DropItem
    {
        [Newtonsoft.Json.JsonProperty("id")]
        public string Id { get; set; } = string.Empty;

        [Newtonsoft.Json.JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 本次新增数量（同一关卡多次结算累加）
        /// </summary>
        [Newtonsoft.Json.JsonProperty("count")]
        public int Count { get; set; }
    }

    /// <summary>
    /// 单个关卡的战斗统计。
    /// </summary>
    private sealed class StageResult
    {
        /// <summary>
        /// Gets or sets 关卡编号。
        /// </summary>
        [Newtonsoft.Json.JsonProperty("stage")]
        public string Stage { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 该关卡完成的战斗次数。
        /// </summary>
        [Newtonsoft.Json.JsonProperty("times")]
        public int Times { get; set; }

        /// <summary>
        /// Gets or sets 该关卡刷到的掉落（按物品 id 合并、跨链累加）。
        /// </summary>
        [Newtonsoft.Json.JsonProperty("drops")]
        public List<DropItem> Drops { get; } = [];
    }

    /// <summary>
    /// 单条战斗链的临时统计：一次任务项可能下发多条战斗链（如库存保持的多个计划），
    /// 每条链独立累计，链结束时并入任务项的 <see cref="ItemResult.StageResults"/>。
    /// </summary>
    private sealed class ChainStat
    {
        public string Stage { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 各次结算界面报出的连战次数之和。
        /// </summary>
        public int CurTimesSum { get; set; }

        /// <summary>
        /// Gets or sets FightTimes 回调报出的累计完成次数。
        /// </summary>
        public int FightTimes { get; set; }

        public Dictionary<string, DropItem> Drops { get; } = new(StringComparer.Ordinal);
    }

    private readonly object Gate = new();

    private readonly Dictionary<int, ItemResult> Items = new();

    /// <summary>
    /// 按 core 任务链 id 索引的在途战斗链统计。
    /// </summary>
    private readonly Dictionary<int, ChainStat> Chains = new();

    private string _remoteTaskId = string.Empty;

    private long _seq;

    private bool _progressReportEnabled;

    /// <summary>
    /// 由获取任务端点的响应驱动（<c>progressReport</c> 字段，每轮轮询刷新）：
    /// 仅当服务端声明支持后才会发送中间汇报，缺省关闭，与未升级的服务端完全兼容。
    /// </summary>
    public void SetEnabled(bool enabled)
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
    public void BeginRun(string remoteTaskId, string? typeFilter = null)
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
    public void OnTaskStatusChanged(int taskId, TaskItemStatus status)
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
                        if (item.ChainsExpected == 0)
                        {
                            // 下发时确认的 core 任务链数量：库存保持等任务项会展开成多条链（仓库识别 + 每个计划）
                            var expected = Instances.TaskQueueViewModel.TaskItemViewModels
                                .ElementAtOrDefault(index)?.TaskIds.Count(id => id > 0) ?? 0;
                            item.ChainsExpected = Math.Max(expected, 1);
                        }

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

                        // 该链结束：先把链内统计并入任务项，再判断是否整体收尾
                        MergeChain(taskId, item);

                        item.Started = true;
                        if (item.ChainsExpected == 0)
                        {
                            item.ChainsExpected = 1;
                        }

                        ++item.ChainsDone;

                        // Error 一旦出现则不被后续 Completed 覆盖
                        if (result == "FAILED" || item.Result != "FAILED")
                        {
                            item.Result = result;
                        }

                        // 多链任务项（如库存保持）需等全部链结束才收尾，避免用部分数据提前上报
                        var complete = item.ChainsDone >= item.ChainsExpected;
                        shouldSendEnd = complete && !item.HasSentEnd;
                        if (complete)
                        {
                            item.Ended = true;
                            item.HasSentEnd = true;
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
    /// 归集战斗链的关卡结算信息（来自 StageDrops 回调），链结束时并入任务项。
    /// </summary>
    /// <param name="taskId">当前战斗链 id。</param>
    /// <param name="stageCode">本次结算的关卡编号。</param>
    /// <param name="curTimes">结算界面识别到的连战次数；未识别到时为负值。</param>
    /// <param name="drops">回调解析出的掉落列表（<c>Add</c> 为本次结算新增数量）。</param>
    public void AppendDrops(int taskId, string? stageCode, int curTimes, IReadOnlyList<(string ItemId, string ItemName, int Total, int Add)> drops)
    {
        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            // 该链不属于任何被跟踪的任务项（如库存保持的仓库识别链）
            return;
        }

        lock (Gate)
        {
            var chain = EnsureChain(taskId);

            if (!string.IsNullOrWhiteSpace(stageCode))
            {
                chain.Stage = stageCode.Trim();
            }

            if (curTimes > 0)
            {
                chain.CurTimesSum += curTimes;
            }

            foreach (var (itemId, itemName, _, add) in drops)
            {
                if (string.IsNullOrEmpty(itemId))
                {
                    continue;
                }

                if (chain.Drops.TryGetValue(itemId, out var existing))
                {
                    // addQuantity 是本次结算新增量，跨结算累加即为该关卡本次刷取的总量
                    existing.Count += add;
                }
                else
                {
                    chain.Drops[itemId] = new DropItem { Id = itemId, Name = itemName, Count = add };
                }
            }
        }
    }

    /// <summary>
    /// 归集战斗次数（来自 FightTimes 回调的 <c>times_finished</c>）。
    /// </summary>
    /// <param name="taskId">当前战斗链 id。</param>
    /// <param name="timesFinished">该链已完成的战斗次数。</param>
    public void NoteFightTimes(int taskId, int timesFinished)
    {
        if (timesFinished <= 0)
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
            var chain = EnsureChain(taskId);

            // 回调随每次战斗推进重复到达，取链内最大值
            chain.FightTimes = Math.Max(chain.FightTimes, timesFinished);
        }
    }

    /// <summary>
    /// 记录进图前识别的理智快照（<c>SanityBeforeStage</c>），随每轮战斗刷新，任务项收尾时取最后一次。
    /// </summary>
    /// <param name="taskId">当前任务链 id。</param>
    /// <param name="sanityCurrent">识别到的当前理智。</param>
    /// <param name="sanityMax">理智上限。</param>
    /// <param name="reportTime">识别时刻。</param>
    public void NoteSanity(int taskId, int sanityCurrent, int sanityMax, DateTimeOffset reportTime)
    {
        if (sanityMax <= 0)
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
            EnsureItem(index).Sanity = new SanitySnapshot { Current = sanityCurrent, Max = sanityMax, ReportTime = reportTime };
        }
    }

    private ChainStat EnsureChain(int taskId)
    {
        if (!Chains.TryGetValue(taskId, out var chain))
        {
            chain = new ChainStat();
            Chains[taskId] = chain;
        }

        return chain;
    }

    /// <summary>
    /// 把已结束的战斗链并入任务项（按关卡合并次数与掉落），并移除在途统计。
    /// </summary>
    private void MergeChain(int taskId, ItemResult item)
    {
        if (!Chains.Remove(taskId, out var chain) || string.IsNullOrEmpty(chain.Stage))
        {
            return;
        }

        // 两个来源择其大者：连战次数按结算累加，FightTimes 为链内累计
        var times = Math.Max(chain.CurTimesSum, chain.FightTimes);

        var entry = item.StageResults.FirstOrDefault(s => string.Equals(s.Stage, chain.Stage, StringComparison.Ordinal));
        if (entry is null)
        {
            entry = new StageResult { Stage = chain.Stage };
            item.StageResults.Add(entry);
        }

        // 同一关卡可能由多条链刷取（如库存保持的不同计划），次数累加、掉落按物品合并
        entry.Times += times;
        foreach (var (itemId, drop) in chain.Drops)
        {
            var existing = entry.Drops.FirstOrDefault(d => string.Equals(d.Id, itemId, StringComparison.Ordinal));
            if (existing is null)
            {
                entry.Drops.Add(new DropItem { Id = drop.Id, Name = drop.Name, Count = drop.Count });
            }
            else
            {
                existing.Count += drop.Count;
            }
        }
    }

    /// <summary>
    /// 记录公招识别结果中各标签组合的保底星级（<c>RecruitResult.result[].level</c>），
    /// 暂存到对应任务项，待该槽位实际选中标签后由 <see cref="CommitRecruitSelection"/> 取出选中集合对应的值。
    /// </summary>
    /// <param name="taskId">当前任务链 id。</param>
    /// <param name="combinations">识别结果中的组合数组，每项含 <c>tags</c> 与 <c>level</c>。</param>
    public void NoteRecruitResult(int taskId, JArray? combinations)
    {
        if (combinations is null || combinations.Count == 0)
        {
            return;
        }

        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            return;
        }

        var map = new Dictionary<string, int>(StringComparer.Ordinal);
        foreach (var combination in combinations.OfType<JObject>())
        {
            if (combination["tags"] is not JArray tags || tags.Count == 0)
            {
                continue;
            }

            var level = combination["level"]?.Value<int>() ?? 0;
            if (level > 0)
            {
                map[BuildTagKey(tags)] = level;
            }
        }

        if (map.Count == 0)
        {
            return;
        }

        lock (Gate)
        {
            EnsureItem(index).PendingCombinations = map;
        }
    }

    /// <summary>
    /// 公招槽位实际选中标签后调用：按选中标签集合取出对应保底星级；
    /// 跳过、刷新或选中集合无对应组合的槽位不会计入。
    /// </summary>
    /// <param name="taskId">当前任务链 id。</param>
    /// <param name="selectedTags">本次实际选中的标签。</param>
    public void CommitRecruitSelection(int taskId, JArray? selectedTags)
    {
        if (selectedTags is null || selectedTags.Count == 0)
        {
            return;
        }

        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            return;
        }

        var key = BuildTagKey(selectedTags);
        lock (Gate)
        {
            if (!Items.TryGetValue(index, out var item))
            {
                return;
            }

            if (item.PendingCombinations.TryGetValue(key, out var minLevel) && minLevel > 0)
            {
                item.MinLevels.Add(minLevel);
            }

            // 一个槽位只对应一次选择，消费后清空，避免影响后续槽位
            item.PendingCombinations = new Dictionary<string, int>(StringComparer.Ordinal);
        }
    }

    /// <summary>
    /// 把标签数组规范化为与顺序无关的查找键。
    /// </summary>
    private string BuildTagKey(JArray tags)
    {
        var names = tags.Select(token => token?.ToString() ?? string.Empty)
            .Where(name => name.Length > 0)
            .OrderBy(name => name, StringComparer.Ordinal);
        return string.Join('\n', names);
    }

    /// <summary>
    /// 无标签招募的保底星级：不选任何标签时不会出现 1/2 星，保底 3 星。
    /// </summary>
    private const int NoTagMinLevel = 3;

    /// <summary>
    /// 公招槽位开始确认招募时调用：若该槽位到确认时仍未选中任何标签，
    /// 说明走的是"无标签直接招募"，按无标签保底口径记入 <see cref="NoTagMinLevel"/>。
    /// </summary>
    /// <param name="taskId">当前任务链 id。</param>
    public void NoteRecruitConfirmed(int taskId)
    {
        var index = FindIndexByTaskId(taskId);
        if (index < 0)
        {
            return;
        }

        lock (Gate)
        {
            // 选中标签时 CommitRecruitSelection 已消费并清空缓存；仍有缓存即表示本槽位未选标签
            if (!Items.TryGetValue(index, out var item) || item.PendingCombinations.Count == 0)
            {
                return;
            }

            item.MinLevels.Add(NoTagMinLevel);
            item.PendingCombinations = new Dictionary<string, int>(StringComparer.Ordinal);
        }
    }

    /// <summary>
    /// 运行收尾时调用：未正常结束的项标记 STOPPED，发送 ALL_COMPLETED 汇总并复位状态。
    /// </summary>
    public void CompleteRun()
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

    private void SendTaskEnd(int index, ItemResult item)
    {
        var payload = ItemPayload(index, item);
        payload["result"] = item.Result;

        // 战斗类：按关卡给出「次数 + 该关卡掉落」，一次运行涉及多个关卡时逐项列出
        if (item.StageResults.Count > 0)
        {
            payload["stages"] = JToken.FromObject(item.StageResults);
        }

        // 战斗类：附上进图前识别的最后一次理智快照与回满预估
        if (item.Sanity is { Max: > 0 } sanity)
        {
            payload["sanity"] = BuildSanityPayload(sanity);
        }

        if (item.MinLevels.Count > 0)
        {
            payload["minLevel"] = JToken.FromObject(item.MinLevels);
        }

        Send("TASK_END", payload);
    }

    /// <summary>
    /// 按自然回复速率（6 分钟 1 点）由识别时刻推算回满时刻；<c>recoverMinutes</c> 在发送时计算，避免长任务中途的推算漂移。
    /// </summary>
    private static JObject BuildSanityPayload(SanitySnapshot sanity)
    {
        var fullAt = sanity.Current < sanity.Max
            ? sanity.ReportTime.AddMinutes((sanity.Max - sanity.Current) * 6)
            : sanity.ReportTime;
        var minutesLeft = sanity.Current < sanity.Max
            ? Math.Max(0, (int)Math.Ceiling((fullAt - DateTimeOffset.Now).TotalMinutes))
            : 0;
        return new JObject
        {
            ["current"] = sanity.Current,
            ["max"] = sanity.Max,
            ["recoverFullAt"] = fullAt.ToString("yyyy-MM-dd'T'HH:mm:sszzz"),
            ["recoverMinutes"] = minutesLeft,
        };
    }

    private JObject ItemPayload(int index, ItemResult item)
    {
        return new JObject
        {
            ["index"] = index,
            ["name"] = item.Name,
            ["type"] = item.Type,
        };
    }

    private List<object> Snapshot()
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
                if (p.Value.StageResults.Count > 0)
                {
                    entry["stages"] = JToken.FromObject(p.Value.StageResults);
                }

                return (object)entry;
            })
            .ToList();
    }

    private int FindIndexByTaskId(int taskId)
    {
        return Instances.TaskQueueViewModel.TaskItemViewModels
            .FirstOrDefault(i => i.TaskIds.Contains(taskId))?.Index ?? -1;
    }

    private ItemResult EnsureItem(int index)
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
    private void Send(string eventName, JObject? extra = null, string? explicitTaskId = null)
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

    private async Task DispatchAsync(string remoteTaskId, string payload)
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
