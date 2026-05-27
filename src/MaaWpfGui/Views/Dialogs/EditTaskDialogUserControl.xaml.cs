// <copyright file="EditTaskDialogUserControl.xaml.cs" company="MaaAssistantArknights">
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

using System;
using System.Windows;

namespace MaaWpfGui.Views.Dialogs;

public enum EditTaskAction
{
    Confirm,
    CopyTask,
    DeleteTask,
}

/// <summary>
/// EditTaskDialogUserControl.xaml 的交互逻辑。
/// </summary>
public partial class EditTaskDialogUserControl
{
    private readonly Action<string>? _onApplyName;

    public string InputText { get; private set; } = string.Empty;

    public EditTaskAction ActionResult { get; private set; }

    public EditTaskDialogUserControl()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    public EditTaskDialogUserControl(string prompt, string defaultText = "", Action<string>? onApplyName = null)
    {
        InitializeComponent();
        PromptTextBlock.Text = prompt;
        InputTextBox.Text = defaultText;
        _onApplyName = onApplyName;
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        InputTextBox.Focus();
        InputTextBox.SelectAll();
    }

    private void BtnApply_Click(object sender, RoutedEventArgs e)
    {
        _onApplyName?.Invoke(InputTextBox.Text);
    }

    private void BtnConfirm_Click(object sender, RoutedEventArgs e)
    {
        InputText = InputTextBox.Text;
        ActionResult = EditTaskAction.Confirm;
        DialogResult = true;
        Close();
    }

    private void BtnCancel_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = false;
        Close();
    }

    private void BtnCopyTask_Click(object sender, RoutedEventArgs e)
    {
        InputText = InputTextBox.Text;
        ActionResult = EditTaskAction.CopyTask;
        DialogResult = true;
        Close();
    }

    private void BtnDeleteTask_Click(object sender, RoutedEventArgs e)
    {
        InputText = InputTextBox.Text;
        ActionResult = EditTaskAction.DeleteTask;
        DialogResult = true;
        Close();
    }
}
