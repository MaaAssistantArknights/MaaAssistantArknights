// <copyright file="App.xaml.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Documents;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.WineCompat;
using MaaWpfGui.WineCompat.FontConfig;
using Serilog;

namespace MaaWpfGui
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        private static readonly ILogger _logger = Log.ForContext<App>();

        public void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            Hyperlink link = sender as Hyperlink;
            if (!string.IsNullOrEmpty(link?.NavigateUri?.AbsoluteUri))
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = link.NavigateUri.AbsoluteUri,
                    UseShellExecute = true,
                });
            }
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            if (WineRuntimeInformation.IsRunningUnderWine && MaaDesktopIntegration.Availabile)
            {
                // override buintin font map as early as possible
                FontConfigIntegration.Install();
            }

            base.OnStartup(e);
        }
    }
}
