// <copyright file="GameSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Views.UserControl.Settings
{
    /// <summary>
    /// GameSettingsUserControl.xaml 的交互逻辑
    /// </summary>
    public partial class GameSettingsUserControl : System.Windows.Controls.UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GameSettingsUserControl"/> class.
        /// </summary>
        public GameSettingsUserControl()
        {
            InitializeComponent();
        }

        private void StartsWithScript_Drop(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                return;
            }

            // Note that you can have more than one file.
            var files = (string[])e.Data.GetData(DataFormats.FileDrop);
            SettingsViewModel.GameSettings.StartsWithScript = files?[0] ?? string.Empty;
        }

        private void EndsWithScript_Drop(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                return;
            }

            // Note that you can have more than one file.
            var files = (string[])e.Data.GetData(DataFormats.FileDrop);
            SettingsViewModel.GameSettings.EndsWithScript = files?[0] ?? string.Empty;
        }

        private void TextBox_PreviewDragOver(object sender, DragEventArgs e)
        {
            e.Handled = true;
        }
    }
}
