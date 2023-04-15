// <copyright file="ThemeHelper.cs" company="MaaAssistantArknights">
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
using System.Windows.Media;
using HandyControl.Themes;
using HandyControl.Tools;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper
{
    public static class ThemeHelper
    {
        private static void SystemEvents_UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e)
        {
            ThemeManager.Current.ApplicationTheme = ThemeManager.GetSystemTheme(isSystemTheme: false);
            ThemeManager.Current.AccentColor = ThemeManager.Current.GetAccentColorFromSystem();
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToLightMode()
        {
            SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
            ThemeManager.Current.UsingWindowsAppTheme = false;
            ThemeManager.Current.ApplicationTheme = ApplicationTheme.Light;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToDarkMode()
        {
            SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
            ThemeManager.Current.UsingWindowsAppTheme = false;
            ThemeManager.Current.ApplicationTheme = ApplicationTheme.Dark;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToSyncWithOSMode()
        {
            ThemeManager.Current.UsingWindowsAppTheme = true;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
            SystemEvents.UserPreferenceChanged += SystemEvents_UserPreferenceChanged;
        }
    }
}
