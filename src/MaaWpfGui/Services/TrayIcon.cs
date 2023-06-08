// <copyright file="TrayIcon.cs" company="MaaAssistantArknights">
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

using System;
using System.Drawing;
using System.Windows.Forms;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services
{
    /// <summary>
    /// 托盘图标。
    /// </summary>
    public class TrayIcon
    {
        private readonly NotifyIcon _notifyIcon = new NotifyIcon();
        private static readonly ILogger _logger = Log.ForContext<TrayIcon>();

        /// <summary>
        /// Initializes a new instance of the <see cref="TrayIcon"/> class.
        /// </summary>
        public TrayIcon()
        {
            InitIcon();
            this.SetVisible(true);
        }

        private void InitIcon()
        {
            this._notifyIcon.Text = "MaaAssistantArknights";
            this._notifyIcon.Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
            _notifyIcon.MouseClick += NotifyIcon_MouseClick;
            _notifyIcon.MouseDoubleClick += OnNotifyIconDoubleClick;

            MenuItem startMenu = new MenuItem(LocalizationHelper.GetString("Farming"));
            startMenu.Click += StartTask;
            MenuItem stopMenu = new MenuItem(LocalizationHelper.GetString("Stop"));
            stopMenu.Click += StopTask;

            MenuItem switchLangMenu = new MenuItem(LocalizationHelper.GetString("SwitchLanguage"));

            foreach (var lang in LocalizationHelper.SupportedLanguages)
            {
                if (lang.Key == SettingsViewModel.PallasLangKey)
                {
                    continue;
                }

                var langMenu = new MenuItem(lang.Value);
                langMenu.Click += (sender, e) =>
                {
                    Instances.SettingsViewModel.Language = lang.Key;
                };
                switchLangMenu.MenuItems.Add(langMenu);
            }

            MenuItem forceShowMenu = new MenuItem(LocalizationHelper.GetString("ForceShow"));
            forceShowMenu.Click += ForceShow;

            MenuItem exitMenu = new MenuItem(LocalizationHelper.GetString("Exit"));
            exitMenu.Click += App_exit;
            MenuItem[] menuItems = new MenuItem[] { startMenu, stopMenu, switchLangMenu, forceShowMenu, exitMenu };
            this._notifyIcon.ContextMenu = new ContextMenu(menuItems);
        }

        private void NotifyIcon_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left)
            {
                return;
            }

            Instances.MainWindowManager.SwitchWindowState();
        }

        private void StartTask(object sender, EventArgs e)
        {
            // taskQueueViewModel意外为null了是不是也可以考虑Log一下
            // 先放个log点方便跟踪
            Instances.TaskQueueViewModel?.LinkStart();
            _logger.Information("Tray service task started.");
        }

        private void StopTask(object sender, EventArgs e)
        {
            Instances.TaskQueueViewModel?.Stop();
            _logger.Information("Tray service task stop.");
        }

        private void ForceShow(object sender, EventArgs e)
        {
            Instances.MainWindowManager.ForceShow();
            _logger.Information("WindowManager Forceshow.");
        }

        private void App_exit(object sender, EventArgs e)
        {
            System.Windows.Application.Current.MainWindow.Close();
        }

        private void App_show(object sender, EventArgs e)
        {
            Instances.MainWindowManager.Show();
        }

        private void OnNotifyIconDoubleClick(object sender, EventArgs e)
        {
            Instances.MainWindowManager.Show();
        }

        /// <summary>
        /// Sets visibility.
        /// </summary>
        /// <param name="visible">Whether it is visible.</param>
        public void SetVisible(bool visible)
        {
            _notifyIcon.Visible = visible;
        }

        /// <summary>
        /// Closes this instance.
        /// </summary>
        public void Close()
        {
            _notifyIcon.Icon = null;
            _notifyIcon.Visible = false;
            _notifyIcon.Dispose();
        }
    }
}
