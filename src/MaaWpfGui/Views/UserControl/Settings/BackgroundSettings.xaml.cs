// <copyright file="BackgroundSettings.xaml.cs" company="MaaAssistantArknights">
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

using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UserControl.Settings;

namespace MaaWpfGui.Views.UserControl.Settings;

/// <summary>
/// BackgroundSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class BackgroundSettingsUserControl : System.Windows.Controls.UserControl
{
    private bool _lostFocus;

    /// <summary>
    /// Initializes a new instance of the <see cref="BackgroundSettingsUserControl"/> class.
    /// </summary>
    public BackgroundSettingsUserControl()
    {
        InitializeComponent();
    }

    private void BackgroundTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is BackgroundSettingsUserControlModel viewModel && e.NewValue is BackgroundImageItem imageItem && !imageItem.IsFolder)
        {
            viewModel.OnBackgroundImageSelected(imageItem);
        }
    }

    private async void BackgroundImagePopup_LostFocus(object sender, RoutedEventArgs e)
    {
        _lostFocus = true;
        await Task.Delay(500);
        _lostFocus = false;
    }

    private void BackgroundDropdownBorder_MouseUp(object sender, RoutedEventArgs e)
    {
        if (_lostFocus)
        {
            return;
        }

        if (DataContext is BackgroundSettingsUserControlModel viewModel)
        {
            viewModel.ToggleBackgroundImagePopup();
        }
    }
}
