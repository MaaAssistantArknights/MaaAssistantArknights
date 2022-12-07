// <copyright file="MainWindowManager.cs" company="MaaAssistantArknights">
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
using System.Windows;

namespace MaaWpfGui
{
    /// <inheritdoc/>
    public class MainWindowManager : IMainWindowManager
    {
        /// <summary>
        /// Gets the main window object
        /// </summary>
        protected static Window MainWindow => Application.Current.MainWindow;

        /// <summary>
        /// Gets or sets a value indicating whether whether minimize to tray.
        /// </summary>
        protected bool ShouldMinimizeToTaskbar { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainWindowManager"/> class.
        /// </summary>
        public MainWindowManager()
        {
            MainWindow.StateChanged += MainWindow_StateChanged;
        }

        /// <inheritdoc/>
        public virtual void Show()
        {
            MainWindow.Show();
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Normal;
            MainWindow.Activate();
        }

        /// <inheritdoc/>
        public virtual void Collapse()
        {
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Minimized;
        }

        /// <inheritdoc/>
        public virtual void SwitchWindowState()
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
        public virtual WindowState GetWindowState() => MainWindow.WindowState;

        /// <inheritdoc/>
        public virtual void SetMinimizeToTaskbar(bool shouldMinimizeToTaskbar)
        {
            ShouldMinimizeToTaskbar = shouldMinimizeToTaskbar;
        }

        /// <summary>
        /// Handle the main window's state changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected virtual void MainWindow_StateChanged(object sender, EventArgs e)
        {
            if (ShouldMinimizeToTaskbar)
            {
                ChangeVisibility(MainWindow.WindowState != WindowState.Minimized);
            }
        }

        /// <summary>
        /// Change visibility of the main window
        /// </summary>
        /// <param name="visible"></param>
        protected virtual void ChangeVisibility(bool visible)
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
        }
    }
}
