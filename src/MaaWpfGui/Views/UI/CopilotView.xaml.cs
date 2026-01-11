// <copyright file="CopilotView.xaml.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// Interaction logic for CopilotView.xaml
/// </summary>
public partial class CopilotView
{
    public CopilotView()
    {
        InitializeComponent();
    }

    private void FileTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is CopilotViewModel viewModel && e.NewValue is CopilotFileItem fileItem && !fileItem.IsFolder)
        {
            viewModel.OnFileSelected(fileItem);
        }
    }

    private bool _lostFocus = false;

    private async void Popup_LostFocus(object sender, RoutedEventArgs e)
    {
        _lostFocus = true;
        await Task.Delay(500);
        _lostFocus = false;
    }

    private void Border_MouseUp(object sender, RoutedEventArgs e)
    {
        if (_lostFocus)
        {
            return;
        }

        if (DataContext is CopilotViewModel viewModel)
        {
            viewModel.ToggleFilePopup();
        }
    }
}
