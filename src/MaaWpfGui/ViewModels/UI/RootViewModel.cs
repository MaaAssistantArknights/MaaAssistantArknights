// <copyright file="RootViewModel.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Media;
using HandyControl.Tools;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using Microsoft.VisualBasic.Logging;
using Microsoft.WindowsAPICodePack.Taskbar;
using Stylet;

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// The root view model.
    /// </summary>
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        /// <inheritdoc/>
        protected override void OnViewLoaded()
        {
            // 更新直接重启
            if (Instances.VersionUpdateViewModel.CheckAndUpdateNow())
            {
                Bootstrapper.ShutdownAndRestartWithoutArgs();
                return;
            }

            InitViewModels();
            InitProxy();
            if (SettingsViewModel.VersionUpdateDataContext.UpdateNightly && !SettingsViewModel.VersionUpdateDataContext.HasAcknowledgedNightlyWarning)
            {
                MessageBoxHelper.Show(LocalizationHelper.GetString("NightlyWarning"));
            }

            Task.Run(async () =>
            {
                await Instances.AnnouncementViewModel.CheckAndDownloadAnnouncement();
                if (Instances.AnnouncementViewModel.DoNotRemindThisAnnouncementAgain)
                {
                    return;
                }

                if (Instances.AnnouncementViewModel.DoNotShowAnnouncement)
                {
                    return;
                }

                _ = Execute.OnUIThreadAsync(() => Instances.WindowManager.ShowWindow(Instances.AnnouncementViewModel));
            });

            Instances.VersionUpdateViewModel.ShowUpdateOrDownload();
        }

        private static async void InitProxy()
        {
            await Task.Run(Instances.AsstProxy.Init);
        }

        private void InitViewModels()
        {
            Items.Add(Instances.TaskQueueViewModel);
            Items.Add(Instances.CopilotViewModel);
            Items.Add(Instances.RecognizerViewModel);
            Items.Add(Instances.SettingsViewModel);

            Instances.SettingsViewModel.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
            ActiveItem = Instances.TaskQueueViewModel;
        }

        private string _windowTitle = "MAA";

        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        public string WindowTitle
        {
            get => _windowTitle;
            set => SetAndNotify(ref _windowTitle, value);
        }

        private (int Current, int Max)? _taskProgress;

        /// <summary>
        /// Gets or sets the TaskProgress.
        /// 0.0 to 1.0.
        /// 置 0 以隐藏进度条.
        /// </summary>
        public (int Current, int Max)? TaskProgress
        {
            get => _taskProgress;
            set
            {
                SetAndNotify(ref _taskProgress, value);

                Execute.OnUIThreadAsync(() =>
                {
                    if (Application.Current.MainWindow == null || !Application.Current.MainWindow.IsVisible)
                    {
                        return;
                    }

                    try
                    {
                        if (value is null)
                        {
                            TaskbarManager.Instance.SetProgressValue(0, 0);
                        }
                        else
                        {
                            TaskbarManager.Instance.SetProgressValue(value.Value.Current, value.Value.Max);
                        }
                    }
                    catch (Exception e)
                    {
                        // 不知道会不会有异常，先捕获一下
                        Logger.Warning("TaskbarManager Exception: " + e.Message);
                    }
                });
            }
        }

        private bool _windowTitleScrollable = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitleScrollable, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to scroll the window title.
        /// </summary>
        public bool WindowTitleScrollable
        {
            get => _windowTitleScrollable;
            set => SetAndNotify(ref _windowTitleScrollable, value);
        }

        private bool _showCloseButton = !Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideCloseButton, bool.FalseString));

        /// <summary>
        /// Gets or sets a value indicating whether to show close button.
        /// </summary>
        public bool ShowCloseButton
        {
            get => _showCloseButton;
            set => SetAndNotify(ref _showCloseButton, value);
        }

        private bool _isWindowTopMost;

        public bool IsWindowTopMost
        {
            get => _isWindowTopMost;
            set
            {
                if (_isWindowTopMost == value)
                {
                    return;
                }

                SetAndNotify(ref _isWindowTopMost, value);
            }
        }

        private Brush _windowTopMostButtonForeground = (SolidColorBrush)Application.Current.FindResource("PrimaryTextBrush");

        public Brush WindowTopMostButtonForeground
        {
            get => _windowTopMostButtonForeground;
            set => SetAndNotify(ref _windowTopMostButtonForeground, value);
        }

        // UI 绑定的方法
        // ReSharper disable once UnusedMember.Global
        public void ToggleTopMostCommand()
        {
            IsWindowTopMost = !IsWindowTopMost;
            WindowTopMostButtonForeground = IsWindowTopMost
                ? (Brush)Application.Current.FindResource("TitleBrush")
                : (Brush)Application.Current.FindResource("PrimaryTextBrush");
        }

        /// <inheritdoc/>
        protected override void OnClose()
        {
            Bootstrapper.Shutdown();
        }
    }
}
