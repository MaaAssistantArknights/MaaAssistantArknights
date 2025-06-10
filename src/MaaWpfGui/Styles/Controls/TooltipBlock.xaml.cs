// <copyright file="TooltipBlock.xaml.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
        DataContext = this;

        Opacity = NormalOpacity;

        MouseEnter += OnMouseEnter;
        MouseLeave += OnMouseLeave;
    }

    public static readonly DependencyProperty TooltipTextProperty = DependencyProperty.Register(nameof(TooltipText), typeof(string), typeof(TooltipBlock), new(string.Empty));

    public static readonly DependencyProperty TooltipMaxWidthProperty = DependencyProperty.Register(nameof(TooltipMaxWidth), typeof(double), typeof(TooltipBlock), new(double.MaxValue));

    public static readonly DependencyProperty NormalOpacityProperty = DependencyProperty.Register(nameof(NormalOpacity), typeof(double), typeof(TooltipBlock), new(0.7, OnOpacityChanged));

    public static readonly DependencyProperty HoverOpacityProperty = DependencyProperty.Register(nameof(HoverOpacity), typeof(double), typeof(TooltipBlock), new(1.0, OnOpacityChanged));

    public static readonly DependencyProperty InitialShowDelayProperty = DependencyProperty.Register(nameof(InitialShowDelay), typeof(int), typeof(TooltipBlock), new(200));

    public string TooltipText
    {
        get => (string)GetValue(TooltipTextProperty);
        set => SetValue(TooltipTextProperty, value);
    }

    public bool TooltipTextEmpty => string.IsNullOrEmpty(TooltipText);

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

    private void AnimateOpacity(double targetOpacity)
    {
        var animation = new DoubleAnimation
        {
            To = targetOpacity,
            Duration = new(TimeSpan.FromMilliseconds(InitialShowDelay)),
            FillBehavior = FillBehavior.HoldEnd,
        };

        BeginAnimation(OpacityProperty, animation);
    }
}
