// <copyright file="EmulatorPathSelectionWindow.xaml.cs" company="MaaAssistantArknights">
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

#nullable enable

using System.Collections.Generic;
using System.Windows;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// EmulatorPathSelectionWindow.xaml 的交互逻辑
/// </summary>
public partial class EmulatorPathSelectionWindow
{
    /// <summary>
    /// Initializes a new instance of the <see cref="EmulatorPathSelectionWindow"/> class.
    /// </summary>
    /// <param name="paths">可选路径列表</param>
    public EmulatorPathSelectionWindow(List<string> paths)
    {
        InitializeComponent();
        DataContext = new EmulatorPathSelectionDialogViewModel(paths);
    }

    /// <summary>
    /// Gets the selected path.
    /// </summary>
    public string? SelectedPath => (DataContext as EmulatorPathSelectionDialogViewModel)?.SelectedPath;

    private void OnConfirmClick(object sender, RoutedEventArgs e)
    {
        if (!string.IsNullOrEmpty(SelectedPath))
        {
            DialogResult = true;
            Close();
        }
        else
        {
            MessageBox.Show(
                Helper.LocalizationHelper.GetString("PleaseSelectPath"),
                Helper.LocalizationHelper.GetString("Tip"),
                MessageBoxButton.OK,
                MessageBoxImage.Warning);
        }
    }

    private void OnCancelClick(object sender, RoutedEventArgs e)
    {
        DialogResult = false;
        Close();
    }
}
