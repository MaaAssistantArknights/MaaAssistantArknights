// <copyright file="TaskSettingsService.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Reflection;
using System.Text;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;

namespace MaaWpfGui.Services.TaskExecution;

/// <summary>
/// 以配置里的 <see cref="BaseTask"/> 对象为唯一数据源，读写「一键长草」每一项任务的全部可设置选项。
/// <para>
/// 运行链路（<see cref="TaskExecutionService"/>）与各设置页都读取 <c>ConfigFactory.CurrentConfig.TaskQueue</c>，
/// 因此这里直接修改任务对象即可生效，不需要模拟界面操作。属性集合通过反射获得，
/// 后续新增任务类型或选项时无需再维护一份映射表。
/// </para>
/// </summary>
public class TaskSettingsService
{
    private static readonly ILogger _logger = Log.ForContext<TaskSettingsService>();
    private static readonly ConcurrentDictionary<Type, PropertyInfo[]> _propertyCache = new();
    private static readonly string[] _excludedProperties = [
        nameof(BaseTask.TaskType),
        nameof(BaseTask.NameOrTaskType),
    ];

    /// <summary>
    /// Gets 当前档案里所有任务及其全部可设置选项，附带枚举类型可选值，便于大模型准确填写。
    /// </summary>
    /// <returns>含 tasks 与 enumOptions 的对象；tasks 每项含 index / type / name / enabled / settings。</returns>
    public JObject DescribeTasks()
    {
        var tasks = new JArray();
        var enumOptions = new JObject();
        var queue = ConfigFactory.CurrentConfig.TaskQueue;
        for (var index = 0; index < queue.Count; ++index)
        {
            var task = queue[index];
            tasks.Add(new JObject {
                ["index"] = index,
                ["type"] = task.GetType().Name,
                ["taskType"] = task.TaskType.ToString(),
                ["name"] = task.Name,
                ["enabled"] = task.IsEnable ?? false,
                ["settings"] = DescribeSettings(task),
            });

            foreach (var property in GetProperties(task.GetType()))
            {
                var type = Nullable.GetUnderlyingType(property.PropertyType) ?? property.PropertyType;
                if (type.IsEnum && !enumOptions.ContainsKey(type.Name))
                {
                    enumOptions[type.Name] = new JArray(Enum.GetNames(type));
                }
            }
        }

        return new JObject {
            ["tasks"] = tasks,
            ["enumOptions"] = enumOptions,
        };
    }

    /// <summary>
    /// Gets 任务队列精简清单，供大模型理解当前有哪些任务及其启用状态。
    /// </summary>
    /// <returns>任务数组，每项含 index / type / taskType / name / enabled。</returns>
    public JArray DescribeTaskList()
    {
        var result = new JArray();
        var queue = ConfigFactory.CurrentConfig.TaskQueue;
        for (var index = 0; index < queue.Count; ++index)
        {
            var task = queue[index];
            result.Add(new JObject {
                ["index"] = index,
                ["type"] = task.GetType().Name,
                ["taskType"] = task.TaskType.ToString(),
                ["name"] = task.Name,
                ["enabled"] = task.IsEnable ?? false,
            });
        }

        return result;
    }

    /// <summary>
    /// 生成适合聊天阅读的任务设置摘要。
    /// </summary>
    /// <param name="filter">任务类型、任务名或任务类型枚举名；为空时只列出任务清单。</param>
    /// <returns>markdown 文本。</returns>
    public string DescribeMarkdown(string? filter = null)
    {
        var queue = ConfigFactory.CurrentConfig.TaskQueue;
        var builder = new StringBuilder();
        builder.Append(LocalizationHelper.GetStringFormat("WeComBotTaskSettingsTitle", ConfigFactory.Root.Current));

        var matched = false;
        for (var index = 0; index < queue.Count; ++index)
        {
            var task = queue[index];
            if (!string.IsNullOrWhiteSpace(filter) && !Matches(task, filter))
            {
                continue;
            }

            matched = true;
            builder.Append('\n')
                .Append(LocalizationHelper.GetStringFormat(
                    "WeComBotTaskSettingsLine",
                    index,
                    task.GetType().Name,
                    task.IsEnable == true ? LocalizationHelper.GetString("WeComBotAccountEnabled") : LocalizationHelper.GetString("WeComBotAccountDisabled")));

            if (string.IsNullOrWhiteSpace(filter))
            {
                continue;
            }

            foreach (var property in GetProperties(task.GetType()))
            {
                var value = DescribeValue(property.GetValue(task));
                builder.Append('\n').Append("  `").Append(property.Name).Append("` = ").Append(value);
            }
        }

        if (!matched)
        {
            builder.Append('\n').Append(LocalizationHelper.GetStringFormat("WeComBotTaskSettingsNotFound", filter));
        }

        return builder.ToString();
    }

    /// <summary>
    /// 按补丁修改任务选项。
    /// </summary>
    /// <param name="request">
    /// 形如 <c>{"task":"RoguelikeTask","settings":{"Theme":"Sami"},"all":false}</c> 的对象，
    /// 也可带 <c>index</c> 精确指定队列位置。
    /// </param>
    /// <returns>是否全部成功，以及面向聊天的结果说明。</returns>
    public (bool Success, string Message) Apply(JObject request)
    {
        var settings = request["settings"] as JObject;
        if (settings is null || settings.Count == 0)
        {
            return (false, LocalizationHelper.GetString("WeComBotTaskSettingsEmptyPatch"));
        }

        var targets = ResolveTargets(request);
        if (targets.Count == 0)
        {
            return (false, LocalizationHelper.GetStringFormat("WeComBotTaskSettingsNotFound", request.Value<string>("task") ?? string.Empty));
        }

        var changed = new JArray();
        foreach (var task in targets)
        {
            var changes = ApplyToTask(task, settings, out var error);
            if (error is not null)
            {
                return (false, error);
            }

            changed.Add(new JObject {
                ["type"] = task.GetType().Name,
                ["changes"] = changes,
            });

            // 让已打开的设置页从任务对象重新取值，避免界面显示旧的选项
            Execute.OnUIThread(() => Instances.TaskQueueViewModel.RefreshTaskModel(task));
        }

        return (true, LocalizationHelper.GetStringFormat(
            "WeComBotTaskSettingsApplied",
            changed.ToString(Newtonsoft.Json.Formatting.None)));
    }

    private static bool Matches(BaseTask task, string filter) =>
        string.Equals(task.GetType().Name, filter, StringComparison.OrdinalIgnoreCase)
        || string.Equals(task.TaskType.ToString(), filter, StringComparison.OrdinalIgnoreCase)
        || (!string.IsNullOrEmpty(task.Name) && string.Equals(task.Name, filter, StringComparison.OrdinalIgnoreCase))
        || task.GetType().Name.Contains(filter, StringComparison.OrdinalIgnoreCase);

    private static JObject DescribeSettings(BaseTask task)
    {
        var result = new JObject();
        foreach (var property in GetProperties(task.GetType()))
        {
            try
            {
                var value = property.GetValue(task);
                result[property.Name] = value is null ? JValue.CreateNull() : JToken.FromObject(value);
            }
            catch (Exception ex)
            {
                _logger.Debug(ex, "Task settings read failed: {Property}", property.Name);
            }
        }

        result[nameof(BaseTask.IsEnable)] = task.IsEnable;
        return result;
    }

    private static string DescribeValue(object? value)
    {
        if (value is null)
        {
            return "null";
        }

        try
        {
            var token = JToken.FromObject(value);
            var text = token.ToString(Newtonsoft.Json.Formatting.None);
            return text.Length > 120 ? text[..120] + "…" : text;
        }
        catch (Exception)
        {
            return value.ToString() ?? string.Empty;
        }
    }

    private static PropertyInfo[] GetProperties(Type type) => _propertyCache.GetOrAdd(type, static taskType => taskType
        .GetProperties(BindingFlags.Public | BindingFlags.Instance)
        .Where(property => property.CanWrite && property.GetIndexParameters().Length == 0)
        .Where(property => !_excludedProperties.Contains(property.Name))
        .Where(property => property.Name != nameof(BaseTask.IsEnable))
        .OrderBy(property => property.Name, StringComparer.Ordinal)
        .ToArray());

    private static JArray ApplyToTask(BaseTask task, JObject settings, out string? error)
    {
        var changes = new JArray();
        foreach (var pair in settings.Properties())
        {
            var property = task.GetType()
                .GetProperties(BindingFlags.Public | BindingFlags.Instance)
                .FirstOrDefault(item => item.CanWrite
                    && item.GetIndexParameters().Length == 0
                    && !_excludedProperties.Contains(item.Name)
                    && string.Equals(item.Name, pair.Name, StringComparison.OrdinalIgnoreCase));
            if (property is null)
            {
                error = LocalizationHelper.GetStringFormat("WeComBotTaskSettingsUnknownKey", task.GetType().Name, pair.Name);
                return changes;
            }

            try
            {
                var value = pair.Value?.ToObject(property.PropertyType);
                property.SetValue(task, value);
                changes.Add(new JObject { ["property"] = property.Name, ["value"] = pair.Value?.DeepClone() });
            }
            catch (Exception ex)
            {
                _logger.Warning(ex, "Task settings write failed: {Task}.{Property}", task.GetType().Name, pair.Name);
                error = LocalizationHelper.GetStringFormat("WeComBotTaskSettingsInvalidValue", property.Name, pair.Value?.ToString() ?? "null");
                return changes;
            }
        }

        error = null;
        return changes;
    }

    private static System.Collections.Generic.List<BaseTask> ResolveTargets(JObject request)
    {
        var queue = ConfigFactory.CurrentConfig.TaskQueue;
        var index = request.Value<int?>("index");
        if (index is >= 0 && index < queue.Count)
        {
            return [queue[index.Value]];
        }

        var filter = request.Value<string>("task");
        if (string.IsNullOrWhiteSpace(filter))
        {
            return [];
        }

        var matched = queue.Where(task => Matches(task, filter)).ToList();
        return request.Value<bool?>("all") == true ? matched : matched.Take(1).ToList();
    }
}
