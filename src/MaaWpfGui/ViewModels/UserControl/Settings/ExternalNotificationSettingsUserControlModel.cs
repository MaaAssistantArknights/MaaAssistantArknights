// <copyright file="ExternalNotificationSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 外部通知
/// </summary>
public class ExternalNotificationSettingsUserControlModel : PropertyChangedBase
{
    public static ExternalNotificationSettingsUserControlModel Instance { get; } = new();

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public static void ExternalNotificationSendTest()
    {
        ExternalNotificationService.Send(
            LocalizationHelper.GetString("ExternalNotificationSendTestTitle"),
            LocalizationHelper.GetString("ExternalNotificationSendTestContent"),
            true);
    }

    public static readonly List<string> ExternalNotificationProviders =
        [
            "ServerChan",
            "Telegram",
            "Discord",
            "SMTP",
            "Bark",
            "Qmsg",
        ];

    public static List<string> ExternalNotificationProvidersShow => ExternalNotificationProviders;

    private object[] _enabledExternalNotificationProviders =
        ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationEnabled, string.Empty)
        .Split(',')
        .Where(s => ExternalNotificationProviders.Contains(s.ToString()))
        .Distinct()
        .ToArray();

    public object[] EnabledExternalNotificationProviders
    {
        get => _enabledExternalNotificationProviders;
        set
        {
            SetAndNotify(ref _enabledExternalNotificationProviders, value);
            var validProviders = value
                .Where(provider => ExternalNotificationProviders.Contains(provider.ToString() ?? string.Empty))
                .Select(provider => provider.ToString())
                .Distinct();

            var config = string.Join(",", validProviders);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationEnabled, config);
            UpdateExternalNotificationProvider();
            NotifyOfPropertyChange(nameof(EnabledExternalNotificationProviderCount));
        }
    }

    public string[] EnabledExternalNotificationProviderList => EnabledExternalNotificationProviders
        .Select(s => s.ToString() ?? string.Empty)
        .ToArray();

    public int EnabledExternalNotificationProviderCount => EnabledExternalNotificationProviders.Length;

    #region External Enable

    private bool _serverChanEnabled = false;

    public bool ServerChanEnabled
    {
        get => _serverChanEnabled;
        set => SetAndNotify(ref _serverChanEnabled, value);
    }

    private bool _telegramEnabled = false;

    public bool TelegramEnabled
    {
        get => _telegramEnabled;
        set => SetAndNotify(ref _telegramEnabled, value);
    }

    private bool _discordEnabled = false;

    public bool DiscordEnabled
    {
        get => _discordEnabled;
        set => SetAndNotify(ref _discordEnabled, value);
    }

    private bool _smtpEnabled = false;

    public bool SmtpEnabled
    {
        get => _smtpEnabled;
        set => SetAndNotify(ref _smtpEnabled, value);
    }

    private bool _barkEnabled = false;

    public bool BarkEnabled
    {
        get => _barkEnabled;
        set => SetAndNotify(ref _barkEnabled, value);
    }

    private bool _qmsgEnabled = false;

    public bool QmsgEnabled
    {
        get => _qmsgEnabled;
        set => SetAndNotify(ref _qmsgEnabled, value);
    }

    public void UpdateExternalNotificationProvider()
    {
        ServerChanEnabled = _enabledExternalNotificationProviders.Contains("ServerChan");
        TelegramEnabled = _enabledExternalNotificationProviders.Contains("Telegram");
        DiscordEnabled = _enabledExternalNotificationProviders.Contains("Discord");
        SmtpEnabled = _enabledExternalNotificationProviders.Contains("SMTP");
        BarkEnabled = _enabledExternalNotificationProviders.Contains("Bark");
        QmsgEnabled = _enabledExternalNotificationProviders.Contains("Qmsg");
    }

    #endregion External Enable

    #region External Notification Config

    private string _serverChanSendKey = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationServerChanSendKey, string.Empty));

    public string ServerChanSendKey
    {
        get => _serverChanSendKey;
        set
        {
            SetAndNotify(ref _serverChanSendKey, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationServerChanSendKey, value);
        }
    }

    private string _barkSendKey = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkSendKey, string.Empty));

    public string BarkSendKey
    {
        get => _barkSendKey;
        set
        {
            SetAndNotify(ref _barkSendKey, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationBarkSendKey, value);
        }
    }

    private string _barkServer = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkServer, "https://api.day.app"));

    public string BarkServer
    {
        get => _barkServer;
        set
        {
            SetAndNotify(ref _barkServer, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationBarkServer, value);
        }
    }

    private string _smtpServer = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpServer, string.Empty));

    public string SmtpServer
    {
        get => _smtpServer;
        set
        {
            SetAndNotify(ref _smtpServer, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpServer, value);
        }
    }

    private string _smtpPort = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpPort, string.Empty));

    public string SmtpPort
    {
        get => _smtpPort;
        set
        {
            SetAndNotify(ref _smtpPort, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpPort, value);
        }
    }

    private string _smtpUser = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpUser, string.Empty));

    public string SmtpUser
    {
        get => _smtpUser;
        set
        {
            SetAndNotify(ref _smtpUser, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpUser, value);
        }
    }

    private string _smtpPassword = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpPassword, string.Empty));

    public string SmtpPassword
    {
        get => _smtpPassword;
        set
        {
            SetAndNotify(ref _smtpPassword, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpPassword, value);
        }
    }

    private string _smtpFrom = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpFrom, string.Empty));

    public string SmtpFrom
    {
        get => _smtpFrom;
        set
        {
            SetAndNotify(ref _smtpFrom, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpFrom, value);
        }
    }

    private string _smtpTo = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpTo, string.Empty));

    public string SmtpTo
    {
        get => _smtpTo;
        set
        {
            SetAndNotify(ref _smtpTo, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpTo, value);
        }
    }

    private bool _smtpUseSsl = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpUseSsl, "false"));

    public bool SmtpUseSsl
    {
        get => _smtpUseSsl;
        set
        {
            SetAndNotify(ref _smtpUseSsl, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpUseSsl, value.ToString());
        }
    }

    private bool _smtpRequireAuthentication = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication, "false"));

    public bool SmtpRequireAuthentication
    {
        get => _smtpRequireAuthentication;
        set
        {
            SetAndNotify(ref _smtpRequireAuthentication, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication, value.ToString());
        }
    }

    private string _discordBotToken = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordBotToken, string.Empty));

    public string DiscordBotToken
    {
        get => _discordBotToken;
        set
        {
            SetAndNotify(ref _discordBotToken, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationDiscordBotToken, value);
        }
    }

    private string _discordUserId = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordUserId, string.Empty));

    public string DiscordUserId
    {
        get => _discordUserId;
        set
        {
            SetAndNotify(ref _discordUserId, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationDiscordUserId, value);
        }
    }

    private string _telegramBotToken = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramBotToken, string.Empty));

    public string TelegramBotToken
    {
        get => _telegramBotToken;
        set
        {
            SetAndNotify(ref _telegramBotToken, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationTelegramBotToken, value);
        }
    }

    private string _telegramChatId = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramChatId, string.Empty));

    public string TelegramChatId
    {
        get => _telegramChatId;
        set
        {
            SetAndNotify(ref _telegramChatId, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationTelegramChatId, value);
        }
    }

    private string _qmsgServer = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgServer, string.Empty));

    public string QmsgServer
    {
        get => _qmsgServer;
        set
        {
            SetAndNotify(ref _qmsgServer, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationQmsgServer, value);
        }
    }

    private string _qmsgKey = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgKey, string.Empty));

    public string QmsgKey
    {
        get => _qmsgKey;
        set
        {
            SetAndNotify(ref _qmsgKey, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationQmsgKey, value);
        }
    }

    private string _qmsgUser = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgUser, string.Empty));

    public string QmsgUser
    {
        get => _qmsgUser;
        set
        {
            SetAndNotify(ref _qmsgUser, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationQmsgUser, value);
        }
    }

    private string _qmsgBot = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgBot, string.Empty));

    public string QmsgBot
    {
        get => _qmsgBot;
        set
        {
            SetAndNotify(ref _qmsgBot, value);
            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ExternalNotificationQmsgBot, value);
        }
    }

    #endregion External Notification Config
}
