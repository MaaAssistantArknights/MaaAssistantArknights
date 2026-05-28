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
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// TooltipBlock.xaml 的交互逻辑
/// </summary>
public partial class TooltipBlock : UserControl
{
    public TooltipBlock()
    {
        InitializeComponent();

        Opacity = NormalOpacity;
    }

    public static readonly DependencyProperty PathDateProperty = DependencyProperty.Register(nameof(PathDate), typeof(Geometry), typeof(TooltipBlock), new FrameworkPropertyMetadata(null));

    public static readonly DependencyProperty PathStrokeThicknessProperty = DependencyProperty.Register(nameof(PathStrokeThickness), typeof(double), typeof(TooltipBlock), new(1d));

    public static readonly DependencyProperty TextBlockTextProperty = DependencyProperty.Register(nameof(TextBlockText), typeof(string), typeof(TooltipBlock), new("?"));

    public static readonly DependencyProperty TooltipTextProperty = DependencyProperty.Register(nameof(TooltipText), typeof(string), typeof(TooltipBlock), new PropertyMetadata(string.Empty, OnTooltipTextChanged));

    public static readonly DependencyProperty TooltipTextEmptyProperty = DependencyProperty.Register(nameof(TooltipTextEmpty), typeof(bool), typeof(TooltipBlock), new PropertyMetadata(true));

    public static readonly DependencyProperty TooltipMaxWidthProperty = DependencyProperty.Register(nameof(TooltipMaxWidth), typeof(double), typeof(TooltipBlock), new(double.MaxValue));

    public static readonly DependencyProperty NormalOpacityProperty = DependencyProperty.Register(nameof(NormalOpacity), typeof(double), typeof(TooltipBlock), new(0.7, OnOpacityChanged));

    public static readonly DependencyProperty HoverOpacityProperty = DependencyProperty.Register(nameof(HoverOpacity), typeof(double), typeof(TooltipBlock), new(1.0, OnOpacityChanged));

    public static readonly DependencyProperty InitialShowDelayProperty = DependencyProperty.Register(nameof(InitialShowDelay), typeof(int), typeof(TooltipBlock), new(200));

    public static readonly DependencyProperty CustomToolTipProperty = DependencyProperty.Register(nameof(CustomToolTip), typeof(object), typeof(TooltipBlock), new PropertyMetadata(null, OnCustomToolTipChanged));

    public static readonly DependencyProperty IsToolTipEnabledProperty = DependencyProperty.Register(nameof(IsToolTipEnabled), typeof(bool), typeof(TooltipBlock), new PropertyMetadata(false));

    public Geometry? PathDate
    {
        get => (Geometry?)GetValue(PathDateProperty);
        set => SetValue(PathDateProperty, value);
    }

    public double PathStrokeThickness
    {
        get => (double)GetValue(PathStrokeThicknessProperty);
        set => SetValue(PathStrokeThicknessProperty, value);
    }

    public bool TextBlockTextEmpty => string.IsNullOrEmpty(TextBlockText);

    public string TextBlockText
    {
        get => (string)GetValue(TextBlockTextProperty);
        set => SetValue(TextBlockTextProperty, value);
    }

    public string TooltipText
    {
        get => (string)GetValue(TooltipTextProperty);
        set => SetValue(TooltipTextProperty, value);
    }

    public bool TooltipTextEmpty
    {
        get => (bool)GetValue(TooltipTextEmptyProperty);
        private set => SetValue(TooltipTextEmptyProperty, value);
    }

    private static void OnTooltipTextChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is TooltipBlock block)
        {
            block.TooltipTextEmpty = string.IsNullOrEmpty((string?)e.NewValue);
            block.UpdateIsToolTipEnabled();
        }
    }

    private static void OnCustomToolTipChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not TooltipBlock block)
        {
            return;
        }

        if (block.PART_Border is { } border)
        {
            border.ToolTip = e.NewValue ?? block.DefaultToolTipContent;
        }

        block.UpdateIsToolTipEnabled();
    }

    private void UpdateIsToolTipEnabled()
    {
        IsToolTipEnabled = !TooltipTextEmpty || CustomToolTip != null;
    }

    public double TooltipMaxWidth
    {
        get => (double)GetValue(TooltipMaxWidthProperty);
        set => SetValue(TooltipMaxWidthProperty, value);
    }

    public double NormalOpacity
    {
        get => (double)GetValue(NormalOpacityProperty);
        set => SetValue(NormalOpacityProperty, value);
    }

    public double HoverOpacity
    {
        get => (double)GetValue(HoverOpacityProperty);
        set => SetValue(HoverOpacityProperty, value);
    }

    public int InitialShowDelay
    {
        get => (int)GetValue(InitialShowDelayProperty);
        set => SetValue(InitialShowDelayProperty, value);
    }

    /// <summary>
    /// Gets or sets 自定义 ToolTip 内容。设置后将覆盖默认的 TooltipText TextBlock；设为 null 时恢复默认。
    /// </summary>
    public object? CustomToolTip
    {
        get => GetValue(CustomToolTipProperty);
        set => SetValue(CustomToolTipProperty, value);
    }

    public bool IsToolTipEnabled
    {
        get => (bool)GetValue(IsToolTipEnabledProperty);
        private set => SetValue(IsToolTipEnabledProperty, value);
    }

    private static void OnOpacityChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is TooltipBlock { IsMouseOver: false } tooltipBlock)
        {
            tooltipBlock.Opacity = tooltipBlock.NormalOpacity;
        }
    }

    private void OnMouseEnter(object sender, EventArgs e)
    {
        AnimateOpacity(HoverOpacity);
    }

    private void OnMouseLeave(object sender, EventArgs e)
    {
        AnimateOpacity(NormalOpacity);
    }

    private void OnToolTipOpening(object element, ToolTipEventArgs args)
    {
        if (IsEnabled)
        {
            return;
        }

        AnimateOpacity(HoverOpacity);
    }

    private void OnToolTipClosing(object element, ToolTipEventArgs args)
    {
        if (IsEnabled)
        {
            return;
        }

        AnimateOpacity(NormalOpacity);
    }

    private void AnimateOpacity(double targetOpacity)
    {
        var animation = new DoubleAnimation {
            To = targetOpacity,
            Duration = new(TimeSpan.FromMilliseconds(InitialShowDelay)),
            FillBehavior = FillBehavior.HoldEnd,
        };

        BeginAnimation(OpacityProperty, animation);
    }
}
