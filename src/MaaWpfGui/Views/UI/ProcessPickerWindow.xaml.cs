// <copyright file="ProcessPickerWindow.xaml.cs" company="MaaAssistantArknights">
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

using System;
using System.Windows;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// Represents a window that allows the user to select a running process by choosing one of its top-level windows.
/// </summary>
/// <remarks>This window displays a list of currently open top-level windows, enabling users to pick a process
/// based on its window. The selection is available through the SelectedHwnd property after the dialog is closed with a
/// positive result. This class is typically used as a modal dialog in applications that require the user to select a
/// process window.</remarks>
public partial class ProcessPickerWindow
{
    private readonly ProcessPickerDialogViewModel _vm;

    public IntPtr SelectedHwnd { get; private set; } = IntPtr.Zero;

    public ProcessPickerWindow()
    {
        InitializeComponent();
        _vm = new ProcessPickerDialogViewModel();
        DataContext = _vm;
        _ = _vm.LoadWindows();
    }

    private void BtnRefresh_Click(object sender, RoutedEventArgs e)
    {
        _ = _vm.LoadWindows();
    }

    private void BtnOk_Click(object sender, RoutedEventArgs e)
    {
        if (_vm.Selected != null)
        {
            SelectedHwnd = _vm.Selected.Hwnd;
            DialogResult = true;
            Close();
            return;
        }

        DialogResult = false;
        Close();
    }
}
