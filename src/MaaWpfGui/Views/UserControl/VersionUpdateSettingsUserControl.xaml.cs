// <copyright file="VersionUpdateSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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
using System.Windows.Input;
using System.Windows.Threading;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Views.UserControl
{
    /// <summary>
    /// OtherCombatSettingsUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class VersionUpdateSettingsUserControl : System.Windows.Controls.UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VersionUpdateSettingsUserControl"/> class.
        /// </summary>
        public VersionUpdateSettingsUserControl()
        {
            InitializeComponent();
            timer.Tick += (s, e1) =>
            {
                count = 0;
                timer.IsEnabled = false;
            };
        }

        private const int ClickCount = 1;
        private int count = 0;
        private readonly DispatcherTimer timer = new DispatcherTimer { Interval = new TimeSpan(0, 0, 0, 2, 0), };

        private void EasterEggs(object sender, MouseButtonEventArgs e)
        {
            count += 1;
            timer.IsEnabled = true;
            if (count % ClickCount == 0)
            {
                count = 0;
                timer.IsEnabled = false;

                var growinfo = new GrowlInfo
                {
                    IsCustom = true,
                    Message = LocalizationHelper.GetString("HelloWorld"),
                    IconKey = "InfoGeometry",
                    IconBrushKey = "PallasBrush",
                };

                switch (new Random().Next(0, 5))
                {
                    case 1:
                        growinfo.Message = LocalizationHelper.GetString("BuyWineOnAprilFoolsDay");
                        growinfo.IconKey = "HangoverGeometry";
                        break;
                    case 2:
                        growinfo.Message = LocalizationHelper.GetString("Burping");
                        growinfo.IconKey = "DrunkAndStaggeringGeometry";
                        break;
                    case 3:
                        growinfo.Message = LocalizationHelper.GetString("DrunkAndStaggering");
                        growinfo.IconKey = "DrunkAndStaggeringGeometry";
                        break;
                    case 4:
                        growinfo.Message = LocalizationHelper.GetString("Hangover");
                        growinfo.IconKey = "HangoverGeometry";
                        break;
                    default:
                        // case 0
                        growinfo.IconBrushKey = "InfoBrush";
                        break;
                }

                Growl.Info(growinfo);
            }
        }
    }
}
