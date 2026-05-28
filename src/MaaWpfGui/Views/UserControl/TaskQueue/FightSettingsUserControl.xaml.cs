// <copyright file="FightSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows;
using System.Windows.Input;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Views.UserControl.TaskQueue;

/// <summary>
/// ParamSettingUserControl.xaml 的交互逻辑
/// </summary>
public partial class FightSettingsUserControl : System.Windows.Controls.UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="FightSettingsUserControl"/> class.
    /// </summary>
    public FightSettingsUserControl()
    {
        InitializeComponent();
    }

    private void DragHandle_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        TaskQueueViewModel.FightTask.IsStageItemDragging = true;

        // 拖拽松开时可能不经过 Button，在 Window 层兜底恢复
        var window = Window.GetWindow(this);
        window?.PreviewMouseLeftButtonUp += Window_PreviewMouseLeftButtonUp;
    }

    private void DragHandle_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        TaskQueueViewModel.FightTask.IsStageItemDragging = false;
    }

    private void Window_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        TaskQueueViewModel.FightTask.IsStageItemDragging = false;
        if (sender is Window window)
        {
            window.PreviewMouseLeftButtonUp -= Window_PreviewMouseLeftButtonUp;
        }
    }
}
