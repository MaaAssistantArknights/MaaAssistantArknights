// <copyright file="VersionUpdateSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Serilog;

namespace MaaWpfGui.Views.UserControl.Settings
{
    /// <summary>
    /// VersionUpdateSettingsUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class VersionUpdateSettingsUserControl : System.Windows.Controls.UserControl
    {
        private static readonly ILogger _logger = Log.ForContext<VersionUpdateSettingsUserControl>();

        /// <summary>
        /// Initializes a new instance of the <see cref="VersionUpdateSettingsUserControl"/> class.
        /// </summary>
        public VersionUpdateSettingsUserControl()
        {
            InitializeComponent();
            _timer.Tick += (s, e1) =>
            {
                _easterEggsCount = 0;
                _timer.IsEnabled = false;
            };
        }

        private readonly DispatcherTimer _timer = new()
        {
            Interval = new(0, 0, 6),
        };

        private void MaaVersionClick(object sender, MouseButtonEventArgs e)
        {
            CopyToClipboardAsync($"UI Version: {VersionUpdateSettingsUserControlModel.UiVersion}\nCore Version: {VersionUpdateSettingsUserControlModel.CoreVersion}\nBuild Time: {VersionUpdateSettingsUserControlModel.BuildDateTimeCurrentCultureString}");
            EasterEggs();
        }

        private void CoreVersionClick(object sender, MouseButtonEventArgs e)
        {
            CopyToClipboardAsync("Core Version: " + VersionUpdateSettingsUserControlModel.CoreVersion);
            EasterEggs();
        }

        private void UiVersionClick(object sender, MouseButtonEventArgs e)
        {
            CopyToClipboardAsync("UI Version: " + VersionUpdateSettingsUserControlModel.UiVersion);
        }

        private void ResourceVersionClick(object sender, MouseButtonEventArgs e)
        {
            CopyToClipboardAsync($"Resource Version: {SettingsViewModel.VersionUpdateSettings.ResourceVersion}\nResource Time: {SettingsViewModel.VersionUpdateSettings.ResourceDateTimeCurrentCultureString}");
        }

        private static void CopyToClipboardAsync(string text)
        {
            if (string.IsNullOrEmpty(text))
            {
                return;
            }

            try
            {
                var clipboardThread = new Thread(() =>
                {
                    try
                    {
                        System.Windows.Forms.Clipboard.Clear();
                        System.Windows.Forms.Clipboard.SetDataObject(text, true);
                    }
                    catch (Exception e)
                    {
                        _logger.Error("Clipboard operation failed: " + e.Message);
                    }
                });

                clipboardThread.SetApartmentState(ApartmentState.STA);
                clipboardThread.Start();
            }
            catch (Exception e)
            {
                _logger.Error("Clipboard operation failed: " + e.Message);
            }
        }

        private int _easterEggsCount;

        private void EasterEggs()
        {
            if (!_timer.IsEnabled)
            {
                _timer.IsEnabled = true;
            }

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.VersionClick);

            if (_easterEggsCount < 5)
            {
                ++_easterEggsCount;
                return;
            }

            MessageBoxInfo info = new()
            {
                Message = LocalizationHelper.GetString("EasterEggsRules"),
                Caption = LocalizationHelper.GetString("EmployeeGuidelines"),
                IconKey = "EasterEggsRulesIcon",
                IconBrushKey = "PrimaryTextBrush",
                ConfirmContent = LocalizationHelper.GetString("ConfirmExitText"),
            };
            MessageBoxHelper.Show(info);

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.Rules);

            /*
            var growlInfo = new GrowlInfo
            {
                IsCustom = true,
                Message = LocalizationHelper.GetString("BuyWineOnAprilFoolsDay"),
                IconKey = "EasterEggsRulesIcon",
                IconBrushKey = "TextIconBrush",
            };

            Growl.Info(growlInfo);
            */
        }
    }
}
