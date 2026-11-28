// <copyright file="DepotMaintainTaskUserControl.xaml.cs" company="MaaAssistantArknights">
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

/// <summary>
/// DepotMaintainTaskUserControl.xaml 的交互逻辑
/// </summary>
public partial class DepotMaintainTaskUserControl : System.Windows.Controls.UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DepotMaintainTaskUserControl"/> class.
    /// </summary>
    public DepotMaintainTaskUserControl()
    {
        InitializeComponent();
    }

    /// <summary>
    /// SplitButton 左半边点击时也展开下拉，避免用户只能点右侧箭头。
    /// </summary>
    private void PresetSplitButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is HandyControl.Controls.SplitButton splitButton)
        {
            splitButton.IsDropDownOpen = true;
        }
    }
}
