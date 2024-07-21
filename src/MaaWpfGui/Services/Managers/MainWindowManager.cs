// <copyright file="MainWindowManager.cs" company="MaaAssistantArknights">
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
using System.Windows;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Views.UI;

namespace MaaWpfGui.Services.Managers
{
    /// <inheritdoc/>
    public sealed class MainWindowManager : IMainWindowManager
    {
        /// <summary>
        /// Gets the main window object
        /// </summary>
        private static Window MainWindow => Application.Current.MainWindow;

        /// <summary>
        /// Gets or sets a value indicating whether minimize to tray.
        /// </summary>
        private bool ShouldMinimizeToTaskBar { get; set; }


        /// <summary>
        /// Gets or sets a value indicating whether to use tray icon.
        /// </summary>
        private bool ShouldUseTrayIcon { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainWindowManager"/> class.
        /// </summary>
        public MainWindowManager()
        {
            MainWindow.StateChanged += MainWindowStateChanged;

            bool minimizeToTray = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizeToTray, bool.FalseString));
            SetMinimizeToTaskBar(minimizeToTray);

            bool useTrayIcon = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseTray, bool.TrueString));
            SetUseTrayIcon(useTrayIcon);
        }

        /// <inheritdoc/>
        public void Show()
        {
            MainWindow.Show();
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Normal;
            MainWindow.Activate();
        }

        /// <inheritdoc/>
        public void ForceShow()
        {
            ((WindowManager)Instances.WindowManager).ForceShow(MainWindow);
        }

        /// <inheritdoc/>
        public void Collapse()
        {
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Minimized;
        }

        /// <inheritdoc/>
        public void SwitchWindowState()
        {
            if (MainWindow.WindowState == WindowState.Minimized)
            {
                Show();
            }
            else
            {
                Collapse();
            }
        }

        /// <inheritdoc/>
        public WindowState GetWindowState() => MainWindow.WindowState;

        /// <inheritdoc/>
        public void SetMinimizeToTaskBar(bool shouldMinimizeToTaskBar)
        {
            ShouldMinimizeToTaskBar = shouldMinimizeToTaskBar;
        }

        /// <inheritdoc/>
        public void SetUseTrayIcon(bool useTrayIcon)
        {
            ShouldUseTrayIcon = useTrayIcon;
            ((RootView)MainWindow).NotifyIcon.notifyIcon.Visibility = useTrayIcon ? Visibility.Visible : Visibility.Collapsed;
        }

        /// <summary>
        /// Handle the main window's state changed event
        /// </summary>
        /// <param name="sender">The object that triggered the event.</param>
        /// <param name="e">The event arguments.</param>
        private void MainWindowStateChanged(object sender, EventArgs e)
        {
            if (ShouldMinimizeToTaskBar && ShouldUseTrayIcon)
            {
                ChangeVisibility(MainWindow.WindowState != WindowState.Minimized);
            }
        }

        /// <summary>
        /// Change visibility of the main window
        /// </summary>
        /// <param name="visible">A boolean indicating whether the main window should be visible or hidden.</param>
        private static void ChangeVisibility(bool visible)
        {
            if (visible)
            {
                MainWindow.ShowInTaskbar = true;
                MainWindow.Visibility = Visibility.Visible;
            }
            else
            {
                MainWindow.ShowInTaskbar = false;
                MainWindow.Visibility = Visibility.Hidden;
            }

            ((RootView)MainWindow).NotifyIcon.useTrayMenu.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        }

        public Window GetWindowIfVisible()
        {
            if (MainWindow == null ||
                MainWindow.WindowState == WindowState.Minimized ||
                MainWindow.Visibility != Visibility.Visible)
            {
                return null;
            }

            return MainWindow;
        }
    }
}
