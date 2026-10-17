// <copyright file="AiIntentAction.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 大模型解析出的单个动作。
/// </summary>
/// <param name="Type">动作类型，取值见 <see cref="WeComBotIntentParser"/>。</param>
/// <param name="Account">动作作用的账号名或配置名，可为空。</param>
/// <param name="Task">动作作用的任务类型名，可为空。</param>
/// <param name="Settings">要写入任务选项的键值对，可为空。</param>
/// <param name="All">是否作用于所有同名任务。</param>
public sealed record AiIntentAction(
    string Type,
    string? Account = null,
    string? Task = null,
    JObject? Settings = null,
    bool All = false);
