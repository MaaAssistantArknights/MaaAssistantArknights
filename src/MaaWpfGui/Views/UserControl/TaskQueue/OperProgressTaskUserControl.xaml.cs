// <copyright file="OperProgressTaskUserControl.xaml.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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

namespace MaaWpfGui.Views.UserControl.TaskQueue;

public partial class OperProgressTaskUserControl : System.Windows.Controls.UserControl
{
    public OperProgressTaskUserControl() => InitializeComponent();

    /// <summary>
    /// Opens the "Read from..." dropdown when the main SplitButton area is clicked.
    /// HandyControl only anchors the dropdown to the narrow arrow, so without this
    /// the main button area appears dead.
    /// </summary>
    private void OpenImportMenu(object sender, RoutedEventArgs e)
    {
        ReadFromSplitButton.IsDropDownOpen = true;
    }
}
