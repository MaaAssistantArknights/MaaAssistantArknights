// <copyright file="AnnouncementViewModel.cs" company="MaaAssistantArknights">
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
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The view model of version update.
    /// </summary>
    // 通过 container.Get<AnnouncementViewModel>(); 实例化或获取实例
    // ReSharper disable once ClassNeverInstantiated.Global
    public class AnnouncementViewModel : Screen
    {
        private string _announcementInfo = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AnnouncementInfo, string.Empty);

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
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.AnnouncementInfo, value);
            }
        }

        private bool _doNotRemindThisAnnouncementAgain = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.DoNotRemindThisAnnouncementAgain, bool.FalseString));

        public bool DoNotRemindThisAnnouncementAgain
        {
            get => _doNotRemindThisAnnouncementAgain;
            set
            {
                SetAndNotify(ref _doNotRemindThisAnnouncementAgain, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.DoNotRemindThisAnnouncementAgain, value.ToString());
            }
        }

        private bool _doNotShowAnnouncement = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.DoNotShowAnnouncement, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to show the update.
        /// </summary>
        public bool DoNotShowAnnouncement
        {
            get => _doNotShowAnnouncement;
            set
            {
                SetAndNotify(ref _doNotShowAnnouncement, value);
                ConfigurationHelper.SetGlobalValue(ConfigurationKeys.DoNotShowAnnouncement, value.ToString());
            }
        }

        /// <summary>
        /// 检查更新
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task CheckAndDownloadAnnouncement()
        {
            const string Path = "announcements/wpf.md";
            const string Url = MaaUrls.MaaApi + Path;

            using var response = await ETagCache.FetchResponseWithEtag(Url, string.IsNullOrEmpty(AnnouncementInfo));

            if (response == null ||
                response.StatusCode == System.Net.HttpStatusCode.NotModified ||
                response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                return;
            }

            var body = await HttpResponseHelper.GetStringAsync(response);
            if (!string.IsNullOrEmpty(body))
            {
                AnnouncementInfo = body;
                DoNotRemindThisAnnouncementAgain = false;
            }

            ETagCache.Set(response);
            ETagCache.Save();
        }
    }
}
