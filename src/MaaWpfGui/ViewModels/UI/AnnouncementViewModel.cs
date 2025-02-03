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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using HandyControl.Controls;
using HandyControl.Tools.Command;
using MaaWpfGui.Configuration;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MdXaml;
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
        public class AnnouncementSection
        {
            public string Title { get; set; }

            public string Content { get; set; }
        }

        public AnnouncementViewModel()
        {
            UpdateScrollStateCommand = new RelayCommand<ScrollViewer>(scrollViewer =>
            {
                if (scrollViewer == null)
                {
                    return;
                }

                // 计算是否滚动到底部
                IsScrolledToBottom |= scrollViewer.VerticalOffset >= scrollViewer.ScrollableHeight - 10;
            });
            AnnouncementSections = new(
                ParseAnnouncementInfo(AnnouncementInfo));
            SelectedAnnouncementSection = AnnouncementSections.FirstOrDefault();
        }

        private static IEnumerable<AnnouncementSection> ParseAnnouncementInfo(string markdown)
        {
            var sections = markdown.Split(["### "], StringSplitOptions.RemoveEmptyEntries)
                .Select(section =>
                {
                    var lines = section.Split('\n', StringSplitOptions.RemoveEmptyEntries);
                    return new AnnouncementSection
                    {
                        Title = lines.FirstOrDefault(),
                        Content = string.Join("\n", lines.Skip(1)),
                    };
                }).ToList();

            sections.Insert(0, new AnnouncementSection
            {
                Title = "ALL~ the Announcement",
                Content = markdown,
            });

            return sections;
        }

        public ICommand UpdateScrollStateCommand { get; }

        private bool _isScrolledToBottom;

        public bool IsScrolledToBottom
        {
            get => _isScrolledToBottom;
            set => SetAndNotify(ref _isScrolledToBottom, value);
        }

        private ObservableCollection<AnnouncementSection> _announcementSections;

        public ObservableCollection<AnnouncementSection> AnnouncementSections
        {
            get => _announcementSections;
            set => SetAndNotify(ref _announcementSections, value);
        }

        private AnnouncementSection _selectedAnnouncementSection;

        public AnnouncementSection SelectedAnnouncementSection
        {
            get => _selectedAnnouncementSection;
            set => SetAndNotify(ref _selectedAnnouncementSection, value);
        }

        private string _announcementInfo = ConfigFactory.Root.AnnouncementInfo.Info;

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
                ConfigFactory.Root.AnnouncementInfo.Info = value;
            }
        }

        private bool _doNotRemindThisAnnouncementAgain = ConfigFactory.Root.AnnouncementInfo.DoNotShowAgain;

        public bool DoNotRemindThisAnnouncementAgain
        {
            get => _doNotRemindThisAnnouncementAgain;
            set
            {
                SetAndNotify(ref _doNotRemindThisAnnouncementAgain, value);
                ConfigFactory.Root.AnnouncementInfo.DoNotShowAgain = value;
            }
        }

        private bool _doNotShowAnnouncement = ConfigFactory.Root.AnnouncementInfo.DoNotShow;

        /// <summary>
        /// Gets or sets a value indicating whether to show the update.
        /// </summary>
        public bool DoNotShowAnnouncement
        {
            get => _doNotShowAnnouncement;
            set
            {
                SetAndNotify(ref _doNotShowAnnouncement, value);
                ConfigFactory.Root.AnnouncementInfo.DoNotShow = value;
            }
        }

        /// <summary>
        /// 检查更新
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task CheckAndDownloadAnnouncement()
        {
            string path = "announcements/wpf";
            if (SettingsViewModel.GuiSettings.Language is not ("zh-cn" or "zh-tw"))
            {
                path += "_en";
            }

            string url = MaaUrls.MaaApi + path + ".md";

            using var response = await ETagCache.FetchResponseWithEtag(url, string.IsNullOrEmpty(AnnouncementInfo));

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

        public void Close()
        {
            RequestClose();
        }
    }
}
