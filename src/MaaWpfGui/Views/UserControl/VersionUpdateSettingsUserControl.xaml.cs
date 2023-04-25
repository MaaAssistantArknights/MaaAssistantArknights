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
                timer.IsEnabled = false;
            };
        }

        private readonly DispatcherTimer timer = new DispatcherTimer { Interval = new TimeSpan(0, 0, 0, 0, 1500), };

        private void EasterEggs(object sender, MouseButtonEventArgs e)
        {
            if (!timer.IsEnabled)
            {
                timer.IsEnabled = true;
                var growinfo = new GrowlInfo
                {
                    IsCustom = true,
                    Message = LocalizationHelper.GetString("BuyWineOnAprilFoolsDay"),
                    IconKey = "HangoverGeometry",
                    IconBrushKey = "PallasBrush",
                };

                Growl.Info(growinfo);
            }
        }
    }
}
