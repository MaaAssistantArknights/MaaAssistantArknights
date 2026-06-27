// <copyright file="EasterEggDialogView.xaml.cs" company="MaaAssistantArknights">
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
/// 彩蛋弹窗（非阻塞 HandyControl Dialog）。带标题、内容和单个确认按钮，
/// 用户点击确认后通过 <see cref="ConfirmClicked"/> 事件通知调用方。
/// </summary>
public partial class EasterEggDialogView : System.Windows.Controls.Border
{
    /// <summary>
    /// Initializes a new instance of the <see cref="EasterEggDialogView"/> class.
    /// </summary>
    /// <param name="caption">标题</param>
    /// <param name="message">提示内容</param>
    /// <param name="confirmText">确认按钮文本</param>
    public EasterEggDialogView(string caption, string message, string confirmText)
    {
        InitializeComponent();
        CaptionText.Text = caption;
        MessageText.Text = message;
        ConfirmButton.Content = confirmText;
    }

    /// <summary>
    /// 用户点击确认按钮时触发。
    /// </summary>
    public event RoutedEventHandler ConfirmClicked
    {
        add => ConfirmButton.Click += value;
        remove => ConfirmButton.Click -= value;
    }
}
