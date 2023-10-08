// <copyright file="AnnouncementViewModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows.Documents;
using System.Windows.Input;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Markdig;
using Markdig.Wpf;
using Stylet;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of version update.
    /// </summary>
    // 通过 container.Get<TaskQueueViewModel>(); 实例化或获取实例
    // ReSharper disable once ClassNeverInstantiated.Global
    public class AnnouncementViewModel : Screen
    {
        private string _announcementInfo = ConfigurationHelper.GetValue(ConfigurationKeys.AnnouncementInfo, string.Empty);

        /// <summary>
        /// Gets the announcement info.
        /// </summary>
        // ReSharper disable once MemberCanBePrivate.Global
        public string AnnouncementInfo
        {
            get => _announcementInfo;
            private set
            {
                SetAndNotify(ref _announcementInfo, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.AnnouncementInfo, value);
            }
        }

        private bool _doNotRemindThisAnnouncementAgain = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.DoNotRemindThisAnnouncementAgain, false.ToString()));

        public bool DoNotRemindThisAnnouncementAgain
        {
            get => _doNotRemindThisAnnouncementAgain;
            set
            {
                SetAndNotify(ref _doNotRemindThisAnnouncementAgain, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.DoNotRemindThisAnnouncementAgain, value.ToString());
            }
        }

        public FlowDocument AnnouncementInfoDoc => Markdig.Wpf.Markdown.ToFlowDocument(AnnouncementInfo,
            new MarkdownPipelineBuilder().UseSupportedExtensions().Build());

        /// <summary>
        /// 检查更新
        /// </summary>
        public async Task CheckAndDownloadAnnouncement()
        {
            const string Path = "announcements/wpf.md";
            const string Url = MaaUrls.MaaApi + Path;

            var response = await ETagCache.FetchResponseWithEtag(Url, string.IsNullOrEmpty(AnnouncementInfo));

            if (response == null ||
                response.StatusCode == System.Net.HttpStatusCode.NotModified ||
                response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                return;
            }

            try
            {
                var body = await response.Content.ReadAsStringAsync();
                if (!string.IsNullOrEmpty(body))
                {
                    AnnouncementInfo = body;
                    DoNotRemindThisAnnouncementAgain = false;
                }
            }
            catch
            {
                return;
            }

            ETagCache.Set(response);
            ETagCache.Save();
        }

        /// <summary>
        /// The event handler of opening hyperlink.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        // xaml 里用到了
        // ReSharper disable once UnusedMember.Global
        // ReSharper disable once UnusedParameter.Global
        public void OpenHyperlink(object sender, ExecutedRoutedEventArgs e)
        {
            Process.Start(e.Parameter.ToString());
        }
    }
}
