// <copyright file="WeComBotIntentParser.cs" company="MaaAssistantArknights">
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
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 用 DeepSeek 把自然语言指令解析成结构化动作。
/// </summary>
public class WeComBotIntentParser
{
    private const string Endpoint = "https://api.deepseek.com/chat/completions";
    private const string SupportedActions = "run_daily / run_roguelike / stop / status / switch_account / describe_tasks / set_task_options";

    /// <summary>
    /// 解析一条消息。
    /// </summary>
    /// <param name="text">用户消息。</param>
    /// <param name="apiKey">DeepSeek API Key。</param>
    /// <param name="model">模型名。</param>
    /// <param name="context">当前账号、运行状态等上下文。</param>
    /// <returns>解析结果。</returns>
    /// <exception cref="InvalidOperationException">未配置 API Key 或返回无法解析时抛出。</exception>
    public async Task<AiIntentPlan> ParseAsync(string text, string apiKey, string model, string context)
    {
        if (string.IsNullOrWhiteSpace(apiKey))
        {
            throw new InvalidOperationException(LocalizationHelper.GetString("WeComBotAiKeyMissing"));
        }

        var payload = new {
            model = string.IsNullOrWhiteSpace(model) ? "deepseek-flash" : model,
            temperature = 0.2,
            response_format = new { type = "json_object" },
            messages = new object[] {
                new {
                    role = "system",
                    content = "你是 MAA（明日方舟小助手）聊天机器人的指令解析器。"
                        + $"把用户的话解析成 JSON：{{\"actions\":[{{\"type\":\"{SupportedActions.Replace(" / ", "|")}\",\"account\":\"账号名或留空\",\"task\":\"任务类型名或留空\",\"settings\":{{}},\"all\":false}}],\"reply\":\"一句中文确认\"}}。"
                        + "run_daily 表示清日常；run_roguelike 表示只跑自动肉鸽；stop 停止；status 查状态；switch_account 切换账号。"
                        + "describe_tasks 表示查看任务选项，task 可留空表示列出全部任务。"
                        + "set_task_options 表示修改任务选项：task 填任务类型名（如 FightTask、InfrastTask、RecruitTask、MallTask、AwardTask、RoguelikeTask、ReclamationTask、StartUpTask），"
                        + "settings 只填需要改的键，键名必须来自上下文里给出的任务设置 JSON，值用 JSON 类型。"
                        + "只有明确要求执行时才给出动作；只是闲聊或提问时 actions 为空。actions 可以是多个，按执行顺序排列。",
                },
                new { role = "user", content = $"{context}\n用户消息：{text}" },
            },
        };

        var headers = new Dictionary<string, string> {
            ["Authorization"] = $"Bearer {apiKey}",
            ["Accept"] = "application/json",
        };
        var response = await Instances.HttpService.PostAsJsonAsync(new Uri(Endpoint), payload, headers).ConfigureAwait(false);
        if (string.IsNullOrWhiteSpace(response))
        {
            throw new InvalidOperationException(LocalizationHelper.GetString("WeComBotAiEmptyResponse"));
        }

        var content = JObject.Parse(response).SelectToken("choices[0].message.content")?.Value<string>();
        if (string.IsNullOrWhiteSpace(content))
        {
            throw new InvalidOperationException(LocalizationHelper.GetString("WeComBotAiEmptyResponse"));
        }

        var plan = JObject.Parse(content);
        var actions = (plan["actions"] as JArray ?? [])
            .OfType<JObject>()
            .Select(item => new AiIntentAction(
                (item.Value<string>("type") ?? string.Empty).Trim().ToLowerInvariant(),
                string.IsNullOrWhiteSpace(item.Value<string>("account")) ? null : item.Value<string>("account")!.Trim(),
                string.IsNullOrWhiteSpace(item.Value<string>("task")) ? null : item.Value<string>("task")!.Trim(),
                item["settings"] as JObject,
                item.Value<bool?>("all") ?? false))
            .Where(action => action.Type.Length > 0)
            .ToList();

        return new AiIntentPlan(actions, plan.Value<string>("reply") ?? string.Empty);
    }
}
