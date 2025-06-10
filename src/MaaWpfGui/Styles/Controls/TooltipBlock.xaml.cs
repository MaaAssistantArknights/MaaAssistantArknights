// <copyright file="TooltipBlock.xaml.cs" company="MaaAssistantArknights">
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
using System.Windows.Controls;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// TooltipBlock.xaml 的交互逻辑
/// </summary>
public partial class TooltipBlock : UserControl
{
    public TooltipBlock()
    {
        InitializeComponent();
        DataContext = this;
    }

    public static readonly DependencyProperty TooltipTextProperty = DependencyProperty.Register(nameof(TooltipText), typeof(string), typeof(TooltipBlock), new PropertyMetadata(string.Empty));

    public static readonly DependencyProperty TooltipMaxWidthProperty = DependencyProperty.Register(nameof(TooltipMaxWidth), typeof(double), typeof(TooltipBlock), new PropertyMetadata(double.MaxValue));

    public string TooltipText
    {
        get => (string)GetValue(TooltipTextProperty);
        set => SetValue(TooltipTextProperty, value);
    }

    public double TooltipMaxWidth
    {
        get => (double)GetValue(TooltipMaxWidthProperty);
        set => SetValue(TooltipMaxWidthProperty, value);
    }
}
