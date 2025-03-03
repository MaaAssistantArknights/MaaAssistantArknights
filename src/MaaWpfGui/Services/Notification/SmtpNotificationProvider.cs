// <copyright file="SmtpNotificationProvider.cs" company="MaaAssistantArknights">
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

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using FluentEmail.Core;
using FluentEmail.Liquid;
using FluentEmail.MailKitSmtp;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Microsoft.Extensions.Options;
using Serilog;

namespace MaaWpfGui.Services.Notification
{
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

            Email.DefaultSender = new MailKitSender(new SmtpClientOptions
            {
                Server = smtpServer,
                Port = smtpPort,
                User = smtpUser,
                Password = smtpPassword,
                RequiresAuthentication = smtpRequiresAuthentication,
                UseSsl = smtpUseSsl,
                UsePickupDirectory = false,
            });

            Email.DefaultRenderer = new LiquidRenderer(new OptionsWrapper<LiquidRendererOptions>(new LiquidRendererOptions()));

            var emailFrom = SettingsViewModel.ExternalNotificationSettings.SmtpFrom;
            var emailTo = SettingsViewModel.ExternalNotificationSettings.SmtpTo;

            title = title.Replace("\r", string.Empty).Replace("\n", string.Empty);
            content = content.Replace("\r", string.Empty).Replace("\n", "<br/>");

            var email = Email
                .From(emailFrom)
                .To(emailTo)
                .Subject($"[MAA] {title}")
                .Body(_emailTemplate.Replace("{title}", title).Replace("{content}", content), true);

            var sendResult = await email.SendAsync();

            if (sendResult.Successful)
            {
                _logger.Information($"Successfully sent Email notification to {emailTo}");
                return true;
            }

            _logger.Warning($"Failed to send Email notification to {emailTo}, {sendResult.ErrorMessages}");
            return false;
        }

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
            <p>{{LocalizationHelper.GetString("ExternalNotificationEmailTemplateHello")}}</p>
        </div>
          
        <hr />
          
        <div class="notification">
            <h1>{title}</h1>
            <p>{content}</p>
        </div>
          
        <hr />
          
        <div class="footer">
            <p>
            {{LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineOne")}}
            </p>
            <p>{{LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineTwo")}}</p>
            <p>
            <a class="space" href="https://github.com/MaaAssistantArknights">
                GitHub
            </a>
            <a class="space" href="https://space.bilibili.com/3493274731940507">
                Bilibili
            </a>
            <a class="space" href="https://maa.plus">{{LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkOfficialSite")}}</a>
            <a class="space" href="https://prts.plus">{{LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkCopilotSite")}}</a>
            </p>
        </div>
        </html>
        """;
    }
}
