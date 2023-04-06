// <copyright file="App.xaml.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using HandyControl.Data;
using HandyControl.Themes;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        public void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            Hyperlink link = sender as Hyperlink;
            if (!string.IsNullOrEmpty(link.NavigateUri.AbsoluteUri))
            {
                Process.Start(new ProcessStartInfo(link.NavigateUri.AbsoluteUri));
            }
        }

        private void UpdateTheme(SkinType skin)
        {
            SharedResourceDictionary.SharedDictionaries.Clear();
            Theme.GetTheme("HandyTheme", Resources).Skin = skin;
            Current.MainWindow?.OnApplyTemplate();
        }

        private void UpdateTheme(bool syncWithSystem)
        {
            SharedResourceDictionary.SharedDictionaries.Clear();
            Theme.GetTheme("HandyTheme", Resources).SyncWithSystem = syncWithSystem;
            Current.MainWindow?.OnApplyTemplate();
        }

        private void SwitchDarkMode()
        {
            SettingsViewModel.DarkModeType darkModeType =
                Enum.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.DarkMode, SettingsViewModel.DarkModeType.Light.ToString()),
                    out SettingsViewModel.DarkModeType temp)
                    ? temp
                    : SettingsViewModel.DarkModeType.Light;
            switch (darkModeType)
            {
                case SettingsViewModel.DarkModeType.Light:
                    UpdateTheme(skin: SkinType.Default);
                    return;

                case SettingsViewModel.DarkModeType.Dark:
                    UpdateTheme(skin: SkinType.Dark);
                    return;

                case SettingsViewModel.DarkModeType.SyncWithOS:
                    UpdateTheme(syncWithSystem: true);
                    return;
            }
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            ConfigurationHelper.Load();
            LocalizationHelper.Load();
            SwitchDarkMode();

            base.OnStartup(e);
        }
    }
}
