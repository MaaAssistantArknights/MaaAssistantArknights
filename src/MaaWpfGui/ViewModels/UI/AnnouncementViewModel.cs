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
