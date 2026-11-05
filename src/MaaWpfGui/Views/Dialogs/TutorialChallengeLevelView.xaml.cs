// <copyright file="TutorialChallengeLevelView.xaml.cs" company="MaaAssistantArknights">
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

#nullable enable
using System.Windows;
using System.Windows.Input;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// TutorialChallengeLevelView.xaml 的交互逻辑
/// </summary>
public partial class TutorialChallengeLevelView
{
    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeLevelView"/> class.
    /// </summary>
    public TutorialChallengeLevelView()
    {
        InitializeComponent();
    }

    private void DragHandle_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (DataContext is TutorialChallengeLevelViewModel viewModel)
        {
            viewModel.IsStageItemDragging = true;
        }

        // 拖拽松开时鼠标未必还在拖拽手柄上，在窗口层恢复
        PreviewMouseLeftButtonUp -= Window_PreviewMouseLeftButtonUp;
        PreviewMouseLeftButtonUp += Window_PreviewMouseLeftButtonUp;
    }

    /// <summary>
    /// SplitButton 左半边没有独立动作，点击时同样展开下拉。
    /// </summary>
    private void PresetSplitButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is HandyControl.Controls.SplitButton splitButton)
        {
            splitButton.IsDropDownOpen = true;
        }
    }

    private void Window_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (DataContext is TutorialChallengeLevelViewModel viewModel)
        {
            viewModel.IsStageItemDragging = false;
        }

        PreviewMouseLeftButtonUp -= Window_PreviewMouseLeftButtonUp;
    }
}
