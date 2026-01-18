// <copyright file="TextDialogUserControl.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows;

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// TextDialogUserControl.xaml 的交互逻辑
/// </summary>
public partial class TextDialogUserControl
{
    /// <summary>
    /// 获取输入的文本
    /// </summary>
    public string InputText { get; private set; } = string.Empty;

    /// <summary>
    /// Initializes a new instance of the <see cref="TextDialogUserControl"/> class.
    /// </summary>
    public TextDialogUserControl()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TextDialogUserControl"/> class.
    /// </summary>
    /// <param name="title">标题</param>
    /// <param name="prompt">提示文本</param>
    /// <param name="defaultText">默认文本</param>
    public TextDialogUserControl(string title, string prompt, string defaultText = "")
    {
        InitializeComponent();
        Title = title;
        PromptTextBlock.Text = prompt;
        InputTextBox.Text = defaultText;
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        // 自动聚焦到输入框并选中文本
        InputTextBox.Focus();
        InputTextBox.SelectAll();
    }

    private void BtnOk_Click(object sender, RoutedEventArgs e)
    {
        InputText = InputTextBox.Text;
        DialogResult = true;
        Close();
    }
}
