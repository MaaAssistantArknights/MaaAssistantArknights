// <copyright file="IMainWindowManager.cs" company="MaaAssistantArknights">
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

using System.Windows;

namespace MaaWpfGui
{
    /// <summary>
    /// Manager of the MAA main window
    /// </summary>
    public interface IMainWindowManager
    {
        /// <summary>
        /// Show the main window
        /// </summary>
        void Show();

        /// <summary>
        /// Collapse the main window
        /// </summary>
        void Collapse();

        /// <summary>
        /// Show the main window if it collapsed and vice versa.
        /// </summary>
        void SwitchWindowState();

        /// <summary>
        /// Get the current window state of the main window
        /// </summary>
        /// <returns>WindowState</returns>
        WindowState GetWindowState();

        /// <summary>
        /// Sets whether to minimize to taskbar.
        /// </summary>
        /// <param name="shouldMinimizeToTaskbar">Whether to minimize to taskbar.</param>
        void SetMinimizeToTaskbar(bool shouldMinimizeToTaskbar);
    }
}
