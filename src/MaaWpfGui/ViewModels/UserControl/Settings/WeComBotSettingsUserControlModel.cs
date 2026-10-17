// <copyright file="WeComBotSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Timers;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Serilog;
using Stylet;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 企业微信机器人
/// </summary>
public class WeComBotSettingsUserControlModel : PropertyChangedBase
{
    // 逐字符编辑凭据时不必每敲一个字就重连，改动停下来后再应用。
    private const int ApplyDelayMilliseconds = 1500;

    private const int StatusRefreshMilliseconds = 3000;

    private static readonly ILogger _logger = Log.ForContext<WeComBotSettingsUserControlModel>();
    private readonly Timer _applyTimer = new(ApplyDelayMilliseconds) { AutoReset = false };
    private readonly Timer _statusTimer = new(StatusRefreshMilliseconds) { AutoReset = true };
    private bool _lastConnected;

    static WeComBotSettingsUserControlModel() => Instance = new();

    private WeComBotSettingsUserControlModel()
    {
        _applyTimer.Elapsed += (_, _) => _ = ApplySettingsAsync();
        _statusTimer.Elapsed += (_, _) => RefreshConnectionState();
        _statusTimer.Start();
        LocalizationHelper.LanguageChanged += OnLanguageChanged;
    }

    /// <summary>
    /// Gets 单例。
    /// </summary>
    public static WeComBotSettingsUserControlModel Instance { get; }

    private static WeComBot Settings => ConfigFactory.Root.WeComBot;

    /// <summary>
    /// Gets 会话类型选项。
    /// </summary>
    public LocalizedObservableList<int> ChatTypeList { get; } = new(
        (1, "WeComBotChatTypeSingle"),
        (2, "WeComBotChatTypeGroup"));

    /// <summary>
    /// Gets 账号轮换列表，直接反映配置对象。
    /// </summary>
    public ObservableCollection<WeComBotAccount> Accounts => Settings.Accounts;

    /// <summary>
    /// Gets 定时清日常时间列表。
    /// </summary>
    public ObservableCollection<WeComBotSchedule> Schedules => Settings.Schedules;

    public bool Enable
    {
        get => Settings.Enable;
        set {
            if (Settings.Enable == value)
            {
                return;
            }

            Settings.Enable = value;
            NotifyOfPropertyChange();
            ApplyNow();
        }
    }

    public string BotId
    {
        get => Settings.BotId;
        set {
            if (Settings.BotId == value)
            {
                return;
            }

            Settings.BotId = value;
            NotifyOfPropertyChange();
            QueueApply();
        }
    }

    public string Secret
    {
        get => Settings.Secret;
        set {
            if (Settings.Secret == value)
            {
                return;
            }

            Settings.Secret = value;
            NotifyOfPropertyChange();
            QueueApply();
        }
    }

    public string WebSocketUrl
    {
        get => Settings.WebSocketUrl;
        set {
            if (Settings.WebSocketUrl == value)
            {
                return;
            }

            Settings.WebSocketUrl = value;
            NotifyOfPropertyChange();
            QueueApply();
        }
    }

    public int HeartbeatSeconds
    {
        get => Settings.HeartbeatSeconds;
        set {
            if (Settings.HeartbeatSeconds == value)
            {
                return;
            }

            Settings.HeartbeatSeconds = value;
            NotifyOfPropertyChange();
            QueueApply();
        }
    }

    public string ChatId
    {
        get => Settings.ChatId;
        set {
            if (Settings.ChatId == value)
            {
                return;
            }

            Settings.ChatId = value;
            NotifyOfPropertyChange();
        }
    }

    public int ChatType
    {
        get => Settings.ChatType;
        set {
            if (Settings.ChatType == value || value == 0)
            {
                return;
            }

            Settings.ChatType = value;
            NotifyOfPropertyChange();
        }
    }

    public bool NotifyTaskStatus
    {
        get => Settings.NotifyTaskStatus;
        set {
            if (Settings.NotifyTaskStatus == value)
            {
                return;
            }

            Settings.NotifyTaskStatus = value;
            NotifyOfPropertyChange();
        }
    }

    public bool EnableRemoteCommand
    {
        get => Settings.EnableRemoteCommand;
        set {
            if (Settings.EnableRemoteCommand == value)
            {
                return;
            }

            Settings.EnableRemoteCommand = value;
            NotifyOfPropertyChange();
        }
    }

    public string AllowedUserIds
    {
        get => Settings.AllowedUserIds;
        set {
            if (Settings.AllowedUserIds == value)
            {
                return;
            }

            Settings.AllowedUserIds = value;
            NotifyOfPropertyChange();
        }
    }

    public bool EnableAiIntent
    {
        get => Settings.EnableAiIntent;
        set {
            if (Settings.EnableAiIntent == value)
            {
                return;
            }

            Settings.EnableAiIntent = value;
            NotifyOfPropertyChange();
        }
    }

    public string DeepSeekApiKey
    {
        get => Settings.DeepSeekApiKey;
        set {
            if (Settings.DeepSeekApiKey == value)
            {
                return;
            }

            Settings.DeepSeekApiKey = value;
            NotifyOfPropertyChange();
        }
    }

    public string DeepSeekModel
    {
        get => Settings.DeepSeekModel;
        set {
            if (Settings.DeepSeekModel == value)
            {
                return;
            }

            Settings.DeepSeekModel = value;
            NotifyOfPropertyChange();
        }
    }

    public bool RotateAccounts
    {
        get => Settings.RotateAccounts;
        set {
            if (Settings.RotateAccounts == value)
            {
                return;
            }

            Settings.RotateAccounts = value;
            NotifyOfPropertyChange();
        }
    }

    public bool EnableSchedule
    {
        get => Settings.EnableSchedule;
        set {
            if (Settings.EnableSchedule == value)
            {
                return;
            }

            Settings.EnableSchedule = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// Gets 连接状态文案。
    /// </summary>
    public string StatusText => LocalizationHelper.GetString(
        Instances.WeComBotService.IsConnected ? "WeComBotConnected" : "WeComBotDisconnected");

    /// <summary>
    /// 立即按当前设置重建长连接。
    /// </summary>
    /// <returns>异步任务。</returns>
    public async Task ApplySettingsAsync()
    {
        var service = Instances.WeComBotService;
        if (service is null)
        {
            return;
        }

        await service.ApplySettingsAsync();
        RefreshConnectionState();
    }

    /// <summary>
    /// 发送一条测试消息。
    /// </summary>
    [UsedImplicitly]
    public void SendTestMessage()
    {
        if (string.IsNullOrWhiteSpace(Settings.ChatId))
        {
            GrowlHelper.Warning(LocalizationHelper.GetString("WeComBotChatIdMissing"));
            return;
        }

        if (!(Instances.WeComBotService?.IsConnected ?? false))
        {
            GrowlHelper.Warning(LocalizationHelper.GetString("WeComBotNotConnected"));
            return;
        }

        var content = LocalizationHelper.GetString("WeComBotTestMessage");
        _ = PushTestMessageAsync(content);
    }

    /// <summary>
    /// 机器人记下新会话后刷新界面上的会话 ID / 类型。
    /// </summary>
    public void NotifyChatRemembered() => Execute.OnUIThread(() => {
        NotifyOfPropertyChange(nameof(ChatId));
        NotifyOfPropertyChange(nameof(ChatType));
    });

    /// <summary>
    /// 添加一个账号轮换项。
    /// </summary>
    [UsedImplicitly]
    public void AddAccount()
    {
        var index = Settings.Accounts.Count + 1;
        var name = $"{LocalizationHelper.GetString("WeComBotDefaultAccountName")}{index}";
        if (!ConfigFactory.ConfigurationExists(name))
        {
            ConfigFactory.AddConfiguration(name);
        }

        EnsureProfileListed(name);
        Settings.Accounts.Add(new WeComBotAccount {
            Name = name,
            Configuration = name,
            Enable = true,
        });
        NotifyOfPropertyChange(nameof(Accounts));
    }

    /// <summary>
    /// 确保新建的配置档案出现在「配置档案」下拉列表里。
    /// </summary>
    /// <param name="name">档案名。</param>
    public void EnsureProfileListed(string name)
    {
        if (string.IsNullOrWhiteSpace(name)
            || Instances.SettingsViewModel.ConfigurationList.Any(item => item.Value == name))
        {
            return;
        }

        Execute.OnUIThread(() => Instances.SettingsViewModel.ConfigurationList.Add(
            new CombinedData { Display = name, Value = name }));
    }

    /// <summary>
    /// 删除一个账号轮换项。
    /// </summary>
    /// <param name="account">要删除的账号。</param>
    [UsedImplicitly]
    public void RemoveAccount(WeComBotAccount account)
    {
        Settings.Accounts.Remove(account);
        NotifyOfPropertyChange(nameof(Accounts));
    }

    /// <summary>
    /// 打开该账号的一键长草配置弹窗。
    /// </summary>
    /// <param name="account">要编辑的账号。</param>
    [UsedImplicitly]
    public void EditAccount(WeComBotAccount account)
    {
        if (account is null)
        {
            return;
        }

        Instances.WindowManager.ShowDialog(new ViewModels.Dialogs.WeComBotAccountEditorViewModel(account));
    }

    /// <summary>
    /// 添加一条定时清日常时间。
    /// </summary>
    [UsedImplicitly]
    public void AddSchedule()
    {
        Settings.Schedules.Add(new WeComBotSchedule { Hour = 8, Minute = 0, Enable = true });
        NotifyOfPropertyChange(nameof(Schedules));
    }

    /// <summary>
    /// 删除一条定时清日常时间。
    /// </summary>
    /// <param name="schedule">要删除的定时项。</param>
    [UsedImplicitly]
    public void RemoveSchedule(WeComBotSchedule schedule)
    {
        Settings.Schedules.Remove(schedule);
        NotifyOfPropertyChange(nameof(Schedules));
    }

    private async Task PushTestMessageAsync(string content)
    {
        var result = await Instances.WeComBotService.PushAsync(content);
        _logger.Information("WeCom bot test message sent: {Result}", result);
        Execute.OnUIThread(() => {
            if (result)
            {
                GrowlHelper.Success(LocalizationHelper.GetString("WeComBotTestMessageSent"));
                return;
            }

            GrowlHelper.Error(LocalizationHelper.GetString("WeComBotTestMessageFailed"));
        });
    }

    private void QueueApply()
    {
        _applyTimer.Stop();
        _applyTimer.Start();
    }

    private void ApplyNow()
    {
        _applyTimer.Stop();
        _ = ApplySettingsAsync();
    }

    private void RefreshConnectionState()
    {
        var connected = Instances.WeComBotService?.IsConnected ?? false;
        if (connected == _lastConnected)
        {
            return;
        }

        _lastConnected = connected;
        Execute.OnUIThread(() => NotifyOfPropertyChange(nameof(StatusText)));
    }

    private void OnLanguageChanged()
    {
        ChatTypeList.RefreshLocalization();
        NotifyOfPropertyChange(nameof(StatusText));
    }
}
