// <copyright file="WeComBotCommandService.cs" company="MaaAssistantArknights">
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
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.TaskExecution;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;

namespace MaaWpfGui.Services.WeComBot;

/// <summary>
/// 把企业微信聊天里的文本翻译成 MAA 动作，并给出面向聊天的回复文案。
/// </summary>
public class WeComBotCommandService
{
    private static readonly ILogger _logger = Log.ForContext<WeComBotCommandService>();
    private static readonly string[] _dailyKeywords = [
        "清日常", "清体力", "一键长草", "长草", "开始清理", "开始执行", "link start", "start", "maa", "日常",
    ];

    private static readonly string[] _statusKeywords = ["状态", "进度", "status"];

    private static readonly string[] _helpKeywords = ["帮助", "help", "菜单"];

    private static readonly string[] _stopKeywords = ["停止", "stop", "结束", "停下", "取消", "终止", "别跑了"];

    private readonly WeComBotIntentParser _intentParser = new();

    private static TaskExecutionService Execution => Instances.TaskExecutionService;

    private static TaskSettingsService TaskSettings => Instances.TaskSettingsService;

    /// <summary>
    /// 处理一条聊天消息。
    /// </summary>
    /// <param name="text">消息正文。</param>
    /// <returns>回复给聊天窗口的 markdown 文案。</returns>
    public async Task<string> HandleAsync(string text)
    {
        var content = text.Trim();
        if (content.Length == 0)
        {
            return LocalizationHelper.GetString("WeComBotHelp");
        }

        var lowered = content.ToLowerInvariant();
        var account = FindAccount(content);

        // 任务选项类指令：可以查看或修改任意一项「一键长草」任务的设置
        if (IsSettingsIntent(content))
        {
            if (ConfigFactory.Root.WeComBot.EnableAiIntent)
            {
                return await HandleByAiAsync(content).ConfigureAwait(false);
            }

            var description = TaskSettings.DescribeMarkdown(FindTaskFilter(content));
            return IsChangeIntent(content)
                ? description + "\n" + LocalizationHelper.GetString("WeComBotTaskSettingsNeedAi")
                : description;
        }

        // 复杂指令（含时间、先后顺序等）交给大模型解析
        if (ConfigFactory.Root.WeComBot.EnableAiIntent && IsComplex(content))
        {
            return await HandleByAiAsync(content).ConfigureAwait(false);
        }

        if (content.Contains("肉鸽", StringComparison.Ordinal))
        {
            return await StartRoguelikeAsync(account).ConfigureAwait(false);
        }

        if (_dailyKeywords.Any(keyword => lowered.Contains(keyword, StringComparison.Ordinal)))
        {
            return StartDaily(account);
        }

        if (content.Contains("账号", StringComparison.Ordinal) || content.Contains("账户", StringComparison.Ordinal))
        {
            return DescribeAccounts();
        }

        if (content.StartsWith("切换", StringComparison.Ordinal) || account is not null)
        {
            var name = content.StartsWith("切换", StringComparison.Ordinal)
                ? content["切换".Length..].Trim()
                : content;
            return SwitchAccount(string.IsNullOrEmpty(name) ? content : name);
        }

        if (_stopKeywords.Any(keyword => lowered.Contains(keyword, StringComparison.Ordinal)))
        {
            var stopped = await Execution.StopAsync().ConfigureAwait(false);
            return LocalizationHelper.GetString(stopped ? "WeComBotStopping" : "WeComBotNothingRunning");
        }

        if (_statusKeywords.Any(keyword => lowered.Contains(keyword, StringComparison.Ordinal)))
        {
            return DescribeStatus();
        }

        if (_helpKeywords.Any(keyword => lowered.Contains(keyword, StringComparison.Ordinal)))
        {
            return LocalizationHelper.GetString("WeComBotHelp");
        }

        if (ConfigFactory.Root.WeComBot.EnableAiIntent)
        {
            return await HandleByAiAsync(content).ConfigureAwait(false);
        }

        return LocalizationHelper.GetString("WeComBotHelp");
    }

    /// <summary>
    /// 依次对所有账号执行一次清日常，用于定时或聊天触发。
    /// </summary>
    /// <param name="account">指定账号，为空表示按设置轮换全部账号。</param>
    /// <returns>异步任务。</returns>
    public async Task RunDailyAsync(string? account = null)
    {
        if (account is not null)
        {
            if (!Execution.SwitchConfiguration(account))
            {
                _logger.Warning("WeCom bot daily run skipped: unknown configuration {Configuration}", account);
                return;
            }

            await Execution.StartTaskQueueAsync().ConfigureAwait(false);
            return;
        }

        await Execution.StartDailyAsync().ConfigureAwait(false);
    }

    private static bool IsComplex(string text)
    {
        if (text.Length >= 12)
        {
            return true;
        }

        string[] hints = ["等", "再", "之后", "然后", "先", "明天", "今天", "提醒", "分钟后", "小时后", "点"];
        return hints.Any(hint => text.Contains(hint, StringComparison.Ordinal))
            || text.Any(char.IsDigit);
    }

    private static string? FindAccount(string text)
    {
        foreach (var item in ConfigFactory.Root.WeComBot.Accounts)
        {
            if (!string.IsNullOrWhiteSpace(item.Name) && text.Contains(item.Name, StringComparison.Ordinal))
            {
                return item.Configuration;
            }
        }

        return null;
    }

    private string StartDaily(string? account)
    {
        if (Execution.IsRunning)
        {
            return LocalizationHelper.GetString("WeComBotBusy");
        }

        _ = Task.Run(() => RunDailyAsync(account));
        var bot = ConfigFactory.Root.WeComBot;
        return account is null && bot.RotateAccounts && bot.Accounts.Count > 0
            ? LocalizationHelper.GetStringFormat("WeComBotDailyAllAccounts", bot.Accounts.Count(account => account.Enable))
            : LocalizationHelper.GetString("WeComBotDailyStarted");
    }

    private async Task<string> StartRoguelikeAsync(string? configuration)
    {
        if (configuration is not null && !Execution.SwitchConfiguration(configuration))
        {
            return LocalizationHelper.GetStringFormat("WeComBotAccountNotFound", configuration);
        }

        var tasks = ConfigFactory.CurrentConfig.TaskQueue.OfType<RoguelikeTask>().ToList();
        if (tasks.Count == 0)
        {
            return LocalizationHelper.GetString("WeComBotRoguelikeMissing");
        }

        if (Execution.IsRunning)
        {
            return LocalizationHelper.GetString("WeComBotBusy");
        }

        _ = Task.Run(() => Execution.StartTasksAsync(tasks));
        return LocalizationHelper.GetString("WeComBotRoguelikeStarted");
    }

    private string SwitchAccount(string name)
    {
        var configuration = ResolveConfiguration(name);
        if (configuration is null)
        {
            return LocalizationHelper.GetStringFormat("WeComBotAccountNotFound", name);
        }

        if (Execution.IsRunning)
        {
            return LocalizationHelper.GetString("WeComBotBusy");
        }

        return Execution.SwitchConfiguration(configuration)
            ? LocalizationHelper.GetStringFormat("WeComBotAccountSwitched", configuration)
            : LocalizationHelper.GetStringFormat("WeComBotAccountNotFound", name);
    }

    private string? ResolveConfiguration(string name)
    {
        if (string.IsNullOrWhiteSpace(name))
        {
            return null;
        }

        foreach (var account in ConfigFactory.Root.WeComBot.Accounts)
        {
            if (string.Equals(account.Name, name, StringComparison.OrdinalIgnoreCase)
                || string.Equals(account.Configuration, name, StringComparison.OrdinalIgnoreCase))
            {
                return account.Configuration;
            }
        }

        return ConfigFactory.ConfigurationExists(name) ? name : null;
    }

    private string DescribeAccounts()
    {
        var builder = new StringBuilder(LocalizationHelper.GetString("WeComBotAccountListTitle"));
        var accounts = ConfigFactory.Root.WeComBot.Accounts;
        if (accounts.Count == 0)
        {
            builder.Append('\n').Append(LocalizationHelper.GetString("WeComBotAccountListEmpty"));
        }
        else
        {
            foreach (var account in accounts)
            {
                builder.Append('\n')
                    .Append(LocalizationHelper.GetStringFormat(
                        "WeComBotAccountListLine",
                        string.IsNullOrWhiteSpace(account.Name) ? account.Configuration : account.Name,
                        account.Configuration,
                        string.IsNullOrWhiteSpace(account.Stage) ? LocalizationHelper.GetString("WeComBotAccountStageDefault") : account.Stage,
                        account.Enable ? LocalizationHelper.GetString("WeComBotAccountEnabled") : LocalizationHelper.GetString("WeComBotAccountDisabled")));
            }
        }

        builder.Append('\n')
            .Append(LocalizationHelper.GetStringFormat("WeComBotAccountListCurrent", ConfigFactory.Root.Current));
        return builder.ToString();
    }

    private string DescribeStatus()
    {
        var state = Execution.IsRunning
            ? LocalizationHelper.GetString("WeComBotStatusRunning")
            : LocalizationHelper.GetString("WeComBotStatusIdle");
        return LocalizationHelper.GetStringFormat(
            "WeComBotStatus",
            state,
            ConfigFactory.Root.Current,
            string.IsNullOrWhiteSpace(Execution.CurrentStage) ? LocalizationHelper.GetString("WeComBotAccountStageDefault") : Execution.CurrentStage,
            ConfigFactory.Root.WeComBot.ChatType == 2 ? LocalizationHelper.GetString("WeComBotChatTypeGroup") : LocalizationHelper.GetString("WeComBotChatTypeSingle"));
    }

    private async Task<string> HandleByAiAsync(string text)
    {
        var bot = ConfigFactory.Root.WeComBot;
        AiIntentPlan plan;
        try
        {
            plan = await _intentParser.ParseAsync(
                text,
                bot.DeepSeekApiKey,
                bot.DeepSeekModel,
                BuildContext(IsSettingsIntent(text))).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "WeCom bot intent parsing failed");
            return LocalizationHelper.GetStringFormat("WeComBotAiUnavailable", ex.Message);
        }

        var results = new List<string>();
        foreach (var action in plan.Actions)
        {
            switch (action.Type)
            {
                case "run_daily":
                    if (Execution.IsRunning)
                    {
                        return LocalizationHelper.GetString("WeComBotBusy");
                    }

                    _ = Task.Run(() => RunDailyAsync(action.Account));
                    break;
                case "run_roguelike":
                    _ = StartRoguelikeAsync(action.Account);
                    break;
                case "stop":
                    _ = Execution.StopAsync();
                    break;
                case "switch_account":
                    if (action.Account is not null)
                    {
                        Execution.SwitchConfiguration(ResolveConfiguration(action.Account) ?? action.Account);
                    }

                    break;
                case "status":
                    results.Add(DescribeStatus());
                    break;
                case "describe_tasks":
                    results.Add(TaskSettings.DescribeMarkdown(action.Task));
                    break;
                case "set_task_options":
                    var request = new JObject {
                        ["task"] = action.Task,
                        ["all"] = action.All,
                        ["settings"] = action.Settings ?? new JObject(),
                    };
                    var (success, message) = TaskSettings.Apply(request);
                    results.Add(message);
                    if (!success)
                    {
                        return string.Join('\n', results);
                    }

                    break;
                default:
                    _logger.Information("WeCom bot ignored unknown AI action {Type}", action.Type);
                    break;
            }
        }

        if (results.Count > 0)
        {
            return string.Join('\n', results);
        }

        return string.IsNullOrWhiteSpace(plan.Reply)
            ? LocalizationHelper.GetString("WeComBotDailyStarted")
            : LocalizationHelper.GetStringFormat("WeComBotAiReply", plan.Reply);
    }

    private static bool IsSettingsIntent(string text)
    {
        // 必须同时出现「动作词」和「设置对象」，避免把「开启清日常」这类启动指令误判成改设置
        string[] nouns = ["设置", "选项", "配置", "肉鸽", "基建", "公招", "招募", "信用", "商店", "奖励", "邮件", "理智", "作战", "关卡", "唤醒"];
        return IsChangeIntent(text)
            && nouns.Any(noun => text.Contains(noun, StringComparison.Ordinal));
    }

    private static bool IsChangeIntent(string text)
    {
        string[] verbs = ["设置", "选项", "改成", "改为", "修改", "配置", "开启", "关闭", "关掉", "启用", "禁用", "打开"];
        return verbs.Any(verb => text.Contains(verb, StringComparison.Ordinal));
    }

    private static string? FindTaskFilter(string text)
    {
        (string Hint, string Task)[] hints = [
            ("肉鸽", "Roguelike"),
            ("基建", "Infrast"),
            ("公招", "Recruit"),
            ("招募", "Recruit"),
            ("信用", "Mall"),
            ("商店", "Mall"),
            ("奖励", "Award"),
            ("邮件", "Award"),
            ("理智", "Fight"),
            ("作战", "Fight"),
            ("关卡", "Fight"),
            ("唤醒", "StartUp"),
            ("启动", "StartUp"),
            ("数据", "UserDataUpdate"),
        ];
        return hints.Where(hint => text.Contains(hint.Hint, StringComparison.Ordinal))
            .Select(hint => hint.Task)
            .FirstOrDefault();
    }

    private static string BuildContext(bool includeTasks)
    {
        var context = LocalizationHelper.GetStringFormat(
            "WeComBotAiContext",
            string.Join('、', ConfigFactory.Root.WeComBot.Accounts.Select(account => account.Name)),
            ConfigFactory.Root.Current,
            Execution.IsRunning ? LocalizationHelper.GetString("WeComBotStatusRunning") : LocalizationHelper.GetString("WeComBotStatusIdle"));
        context += "\n任务队列：" + TaskSettings.DescribeTaskList().ToString(Formatting.None);
        if (includeTasks)
        {
            context += "\n完整设置：" + TaskSettings.DescribeTasks().ToString(Formatting.None);
        }

        return context;
    }
}
