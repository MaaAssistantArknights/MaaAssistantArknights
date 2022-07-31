// <copyright file="AboutUserControl.xaml.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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

using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;

namespace MeoAsstGui
{
    /// <summary>
    /// PenguinReportSettingsUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class AboutUserControl : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AboutUserControl"/> class.
        /// </summary>
        public AboutUserControl()
        {
            InitializeComponent();
        }

        private void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            Hyperlink link = sender as Hyperlink;

            // 激活的是当前默认的浏览器
            Process.Start(new ProcessStartInfo(link.NavigateUri.AbsoluteUri));
        }
    }
}
