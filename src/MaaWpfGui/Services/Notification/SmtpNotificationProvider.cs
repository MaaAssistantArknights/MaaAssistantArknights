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

using System.Text.RegularExpressions;
using System.Threading.Tasks;
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
    public class SmtpNotificationProvider : IExternalNotificationProvider
    {
        private readonly ILogger _logger = Log.ForContext<SmtpNotificationProvider>();

        public async Task<bool> SendAsync(string title, string content)
        {
            var smtpServer = SettingsViewModel.ExternalNotificationDataContext.SmtpServer;
            var smtpPortValid = int.TryParse(SettingsViewModel.ExternalNotificationDataContext.SmtpPort, out var smtpPort);
            var smtpUser = SettingsViewModel.ExternalNotificationDataContext.SmtpUser;
            var smtpPassword = SettingsViewModel.ExternalNotificationDataContext.SmtpPassword;
            var smtpUseSsl = SettingsViewModel.ExternalNotificationDataContext.SmtpUseSsl;
            var smtpRequiresAuthentication = SettingsViewModel.ExternalNotificationDataContext.SmtpRequireAuthentication;

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

            var emailFrom = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpFrom, string.Empty);
            var emailTo = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpTo, string.Empty);

            title = title.Replace("\r", string.Empty).Replace("\n", string.Empty);
            content = content.Replace("\r", string.Empty).Replace("\n", string.Empty);

            var email = Email
                .From(emailFrom)
                .To(emailTo)
                .Subject($"[MAA] {title}")
                .UsingTemplate(EmailTemplate, new
                {
                    Title = title,
                    Content = content,
                    Hello = LocalizationHelper.GetString("ExternalNotificationEmailTemplateHello"),
                    FooterLineOne = LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineOne"),
                    FooterLineTwo = LocalizationHelper.GetString("ExternalNotificationEmailTemplateFooterLineTwo"),
                    LinkTextOfficialSite = LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkOfficialSite"),
                    LinkTextCopilotSite = LocalizationHelper.GetString("ExternalNotificationEmailTemplateLinkCopilotSite"),
                });

            var sendResult = await email.SendAsync();

            if (sendResult.Successful)
            {
                _logger.Information($"Successfully sent Email notification to {emailTo}");
                return true;
            }

            _logger.Warning($"Failed to send Email notification to {emailTo}, {sendResult.ErrorMessages}");
            return false;
        }

        private const string EmailTemplate = """

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
     <p>{{ Hello }}</p>
   </div>
 
   <hr />
 
   <div class="notification">
     <h1>{{ Title }}</h1>
     <p>{{ Content }}</p>
   </div>
 
   <hr />
 
   <div class="footer">
     <p>
       {{ FooterLineOne }}
     </p>
     <p>{{ FooterLineTwo }}</p>
     <p>
       <a class="space" href="https://github.com/MaaAssistantArknights">
         GitHub
       </a>
       <a class="space" href="https://space.bilibili.com/3493274731940507">
         Bilibili
       </a>
       <a class="space" href="https://maa.plus">{{ LinkTextOfficialSite }}</a>
       <a class="space" href="https://prts.plus">{{ LinkTextCopilotSite }}</a>
     </p>
   </div>
 </html>

 """;
    }
}
