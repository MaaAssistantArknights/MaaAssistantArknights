// <copyright file="NotifyIcon.xaml.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using System.Timers;
using System.Windows;
using System.Windows.Controls;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Views.UI
{
    /// <summary>
    /// 托盘图标。
    /// </summary>
    public partial class NotifyIcon
    {
        private static readonly ILogger _logger = Log.ForContext<NotifyIcon>();
        private readonly int _menuItemNum;
        private static Timer _clickTimer;
        private static bool _canClick = true;

        [DllImport("user32.dll")]
        private static extern uint GetDoubleClickTime();

        public NotifyIcon()
        {
            InitializeComponent();

            uint doubleClickTime = GetDoubleClickTime();
            _clickTimer = new(doubleClickTime);
            _clickTimer.AutoReset = false;
            _clickTimer.Elapsed += (s, e) =>
            {
                _canClick = true;
            };

            InitIcon();
            if (notifyIcon.ContextMenu is not null)
            {
                _menuItemNum = notifyIcon.ContextMenu.Items.Count;
            }
        }

        private void InitIcon()
        {
            notifyIcon.Icon = AppIcon.GetIcon();
            notifyIcon.Visibility = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseTray, bool.TrueString)) ? Visibility.Visible : Visibility.Collapsed;

            notifyIcon.Click += NotifyIcon_MouseClick;
            notifyIcon.MouseDoubleClick += NotifyIcon_MouseClick;

            startMenu.Click += StartTask;
            stopMenu.Click += StopTask;
            forceShowMenu.Click += ForceShow;
            hideTrayMenu.Click += HideTray;
            restartMenu.Click += App_restart;
            exitMenu.Click += App_exit;

            foreach (var lang in LocalizationHelper.SupportedLanguages)
            {
                if (lang.Key == SettingsViewModel.PallasLangKey)
                {
                    continue;
                }

                var langMenu = new MenuItem() { Header = lang.Value };
                langMenu.Click += (_, _) =>
                {
                    SettingsViewModel.GuiSettings.Language = lang.Key;
                };

                switchLangMenu.Items.Add(langMenu);
            }
        }

        // 不知道是干嘛的，先留着
        // ReSharper disable once UnusedMember.Local
        private void AddMenuItemOnFirst(string text, Action action)
        {
            var menuItem = new MenuItem { Header = text };
            menuItem.Click += (_, _) => { action?.Invoke(); };
            if (notifyIcon.ContextMenu is null)
            {
                return;
            }

            if (notifyIcon.ContextMenu.Items.Count == _menuItemNum)
            {
                notifyIcon.ContextMenu.Items.Insert(0, menuItem);
            }
            else
            {
                notifyIcon.ContextMenu.Items[0] = menuItem;
            }
        }

        private static void NotifyIcon_MouseClick(object sender, RoutedEventArgs e)
        {
            if (!_canClick)
            {
                return;
            }

            _clickTimer.Start();
            _canClick = false;
            Instances.MainWindowManager?.SwitchWindowState();
        }

        private static void StartTask(object sender, RoutedEventArgs e)
        {
            // taskQueueViewModel意外为null了是不是也可以考虑Log一下
            // 先放个log点方便跟踪
            Instances.TaskQueueViewModel?.LinkStart();
            _logger.Information("Tray service task started.");
        }

        private static void StopTask(object sender, RoutedEventArgs e)
        {
            Instances.TaskQueueViewModel?.Stop();
            _logger.Information("Tray service task stop.");
        }

        private static void ForceShow(object sender, RoutedEventArgs e)
        {
            Instances.MainWindowManager?.ForceShow();
            _logger.Information("WindowManager force show.");
        }

        private static void HideTray(object sender, RoutedEventArgs e)
        {
            Instances.MainWindowManager?.Show();

            SettingsViewModel.GuiSettings.UseTray = !SettingsViewModel.GuiSettings.UseTray;
            _logger.Information("Use tray icon: {0}", SettingsViewModel.GuiSettings.UseTray);
        }

        private static void App_restart(object sender, RoutedEventArgs e)
        {
            if (Instances.TaskQueueViewModel.ConfirmExit())
            {
                Bootstrapper.ShutdownAndRestartWithoutArgs();
            }
        }

        private static void App_exit(object sender, RoutedEventArgs e)
        {
            if (Instances.TaskQueueViewModel.ConfirmExit())
            {
                Bootstrapper.Shutdown();
            }
        }

        private static void App_show(object sender, RoutedEventArgs e)
        {
            Instances.MainWindowManager?.Show();
        }
    }
}
