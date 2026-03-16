// <copyright file="ItemSelectionDialogView.xaml.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// ItemSelectionDialogView.xaml 的交互逻辑
/// </summary>
public partial class ItemSelectionDialogView
{
    /// <summary>
    /// Initializes a new instance of the <see cref="ItemSelectionDialogView"/> class.
    /// </summary>
    /// <param name="availableItems">可选项</param>
    /// <param name="windowTitle">窗口标题（可选）</param>
    /// <param name="promptMessage">提示信息（可选）</param>
    public ItemSelectionDialogView(IEnumerable<string> availableItems, string? windowTitle = null, string? promptMessage = null)
    {
        InitializeComponent();
        DataContext = new ItemSelectionDialogViewModel(availableItems, windowTitle, promptMessage);
    }

    /// <summary>
    /// Gets the selected item.
    /// </summary>
    public string? SelectedItem => (DataContext as ItemSelectionDialogViewModel)?.SelectedItem;

    private void OnConfirmClick(object sender, RoutedEventArgs e)
    {
        if (!string.IsNullOrEmpty(SelectedItem))
        {
            DialogResult = true;
            Close();
        }
        else
        {
            MessageBox.Show(
                Helper.LocalizationHelper.GetString("PleaseSelectItem"),
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
