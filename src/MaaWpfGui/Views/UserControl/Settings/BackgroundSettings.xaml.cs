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

using System;
using System.Windows;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UserControl.Settings;

namespace MaaWpfGui.Views.UserControl.Settings;

/// <summary>
/// BackgroundSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class BackgroundSettingsUserControl : System.Windows.Controls.UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="BackgroundSettingsUserControl"/> class.
    /// </summary>
    public BackgroundSettingsUserControl()
    {
        InitializeComponent();
    }

    // 下拉展开前刷新背景图目录列表（与旧实现 ｢打开前刷新数据源｣ 的时机对齐）
    private void BackgroundImageTreeBox_DropDownOpening(object sender, EventArgs e)
    {
        (DataContext as BackgroundSettingsUserControlModel)?.LoadBackgroundImageItems();
    }

    private void BackgroundImageTreeBox_SelectionChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (e.NewValue is BackgroundImageItem { IsFolder: false } imageItem)
        {
            (DataContext as BackgroundSettingsUserControlModel)?.OnBackgroundImageSelected(imageItem);
            BackgroundImageTreeBox.CloseDropDown();
        }
    }
}
