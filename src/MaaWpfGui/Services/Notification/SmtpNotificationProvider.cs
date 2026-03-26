// <copyright file="SmtpNotificationProvider.cs" company="MaaAssistantArknights">
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

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using MailKit.Net.Smtp;
using MimeKit;
using Serilog;

namespace MaaWpfGui.Services.Notification;

/// <inheritdoc />
public partial class SmtpNotificationProvider : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<SmtpNotificationProvider>();

    [GeneratedRegex(@"\[(.*?)\]\[(.*?)\]([\s\S]*?)(?=\n\[|$)")]
    private static partial Regex ContentRegex();

    private static string ProcessContent(string content)
    {
        var matches = ContentRegex().Matches(content);
        if (matches.Count == 0)
        {
            return content;
        }

        var resultContent = new StringBuilder(content);

        string timeRgbColor = GetRgbColor(UiLogColor.Trace);
        if (timeRgbColor == null)
        {
            return content;
        }

        foreach (Match match in matches)
        {
            string time = match.Groups[1].Value;
            string colorCode = match.Groups[2].Value;
            string contentText = match.Groups[3].Value;

            string rgbColor = GetRgbColor(colorCode);
            if (rgbColor == null)
            {
                continue;
            }

            string replacement = $"<span style='color: {timeRgbColor};'>{time}  </span><span style='color: {rgbColor};'>{contentText}</span>";
            resultContent.Replace(match.Value, replacement);
        }

        return resultContent.ToString();

        static string GetRgbColor(string resourceKey)
        {
            return Application.Current.Resources[resourceKey] is SolidColorBrush brush
                ? $"rgb({brush.Color.R}, {brush.Color.G}, {brush.Color.B})"
                : null;
        }
    }

    public async Task<bool> SendAsync(string title, string content)
    {
        content = ProcessContent(content);

        var smtpServer = SettingsViewModel.ExternalNotificationSettings.SmtpServer;
        var smtpPortValid = int.TryParse(SettingsViewModel.ExternalNotificationSettings.SmtpPort, out var smtpPort);
        var smtpUser = SettingsViewModel.ExternalNotificationSettings.SmtpUser;
        var smtpPassword = SettingsViewModel.ExternalNotificationSettings.SmtpPassword;
        var smtpUseSsl = SettingsViewModel.ExternalNotificationSettings.SmtpUseSsl;
        var smtpRequiresAuthentication = SettingsViewModel.ExternalNotificationSettings.SmtpRequireAuthentication;

        if (string.IsNullOrEmpty(smtpServer) || smtpPortValid is false)
        {
            _logger.Error("Failed to send Email notification, invalid SMTP configuration");
            return false;
        }

        var emailFrom = SettingsViewModel.ExternalNotificationSettings.SmtpFrom;
        var emailTo = SettingsViewModel.ExternalNotificationSettings.SmtpTo;

        if (string.IsNullOrWhiteSpace(emailFrom) || string.IsNullOrWhiteSpace(emailTo))
        {
            _logger.Error("Failed to send Email notification, sender or recipient is empty");
            return false;
        }

        if (smtpRequiresAuthentication && (string.IsNullOrWhiteSpace(smtpUser) || string.IsNullOrWhiteSpace(smtpPassword)))
        {
            _logger.Error("Failed to send Email notification, authentication is enabled but credentials are incomplete");
            return false;
        }

        title = title.Replace("\r", string.Empty).Replace("\n", string.Empty);
        content = content.Replace("\r", string.Empty).Replace("\n", "<br/>");
        string body = BuildEmailBody(title, content);

        try
        {
            var message = new MimeMessage();
            message.From.Add(MailboxAddress.Parse(emailFrom));
            message.To.AddRange(InternetAddressList.Parse(emailTo));
            message.Subject = title;
            message.Body = new BodyBuilder { HtmlBody = body }.ToMessageBody();

            using var client = new SmtpClient();
            await client.ConnectAsync(smtpServer, smtpPort, smtpUseSsl);

            if (smtpRequiresAuthentication)
            {
                await client.AuthenticateAsync(smtpUser, smtpPassword);
            }

            await client.SendAsync(message);
            await client.DisconnectAsync(true);
            _logger.Information("Successfully sent Email notification to {EmailTo}", emailTo);
            return true;
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to send Email notification to {EmailTo}", emailTo);
        }

        return false;
    }

    private static string BuildEmailBody(string title, string content) =>
        _emailTemplate
            .Replace("{greeting}", LocalizationHelper.GetString("ExternalNotificationEmailTemplateHello"))
            .Replace("{title}", title)
            .Replace("{content}", content)
            .Replace("{footerLineOne}", LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineOne"))
            .Replace("{footerLineTwo}", LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineTwo"))
            .Replace("{officialSite}", LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkOfficialSite"))
            .Replace("{copilotSite}", LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkCopilotSite"));

    private static readonly string _emailTemplate =
    $$"""
    <html lang="zh">
    <style>
        .title {
        font-size: xx-large;
        font-weight: bold;
        color: black;
        text-align: center;
        }
      
        .heading {
        font-size: large;
        }
      
        .notification h1 {
        font-size: large;
        font-weight: bold;
        }
      
        .notification p {
        font-size: medium;
        }
      
        .footer {
        font-size: small;
        }
      
        .space {
        padding-left: 0.5rem;
        padding-right: 0.5rem;
        }
    </style>
      
    <h1 class="title">Maa Assistant Arknights</h1>
      
    <div class="heading">
        <p>{greeting}</p>
    </div>
      
    <hr />
      
    <div class="notification">
        <h1>{title}</h1>
        <p>{content}</p>
    </div>
      
    <hr />
      
    <div class="footer">
        <p>
        {footerLineOne}
        </p>
        <p>{footerLineTwo}</p>
        <p>
        <a class="space" href="https://github.com/MaaAssistantArknights">
            GitHub
        </a>
        <a class="space" href="https://space.bilibili.com/3493274731940507">
            Bilibili
        </a>
        <a class="space" href="https://maa.plus">{officialSite}</a>
        <a class="space" href="https://prts.plus">{copilotSite}</a>
        </p>
    </div>
    </html>
    """;
}
