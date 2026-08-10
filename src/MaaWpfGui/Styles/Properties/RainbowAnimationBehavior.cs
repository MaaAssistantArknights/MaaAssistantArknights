// <copyright file="RainbowAnimationBehavior.cs" company="MaaAssistantArknights">
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
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 为 RainbowFlowBrush 资源提供流光动画的附加行为。
/// 在控件 Loaded 时自动为 Foreground 中的 LinearGradientBrush 启动 TranslateTransform 动画。
/// </summary>
public static class RainbowAnimationBehavior
{
    /// <summary>
    /// 是否启用彩虹流光动画。设为 true 时，控件 Loaded 后自动为 Foreground 启动动画。
    /// </summary>
    public static readonly DependencyProperty IsActiveProperty =
        DependencyProperty.RegisterAttached(
            "IsActive",
            typeof(bool),
            typeof(RainbowAnimationBehavior),
            new PropertyMetadata(false, OnIsActiveChanged));

    public static bool GetIsActive(DependencyObject obj) => (bool)obj.GetValue(IsActiveProperty);

    public static void SetIsActive(DependencyObject obj, bool value) => obj.SetValue(IsActiveProperty, value);

    private static void OnIsActiveChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not FrameworkElement element)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            element.Loaded += OnElementLoaded;
        }
        else
        {
            element.Loaded -= OnElementLoaded;
        }
    }

    private static void OnElementLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement element)
        {
            return;
        }

        Brush? brush = element switch
        {
            TextBlock tb => tb.Foreground,
            Control ctrl => ctrl.Foreground,
            _ => null,
        };

        if (brush is LinearGradientBrush linearBrush
            && linearBrush.Transform is TranslateTransform translate)
        {
            var anim = new DoubleAnimation
            {
                From = 0,
                To = 4000,
                Duration = new Duration(System.TimeSpan.FromSeconds(20)),
                RepeatBehavior = RepeatBehavior.Forever,
            };

            translate.BeginAnimation(TranslateTransform.XProperty, anim);
        }
    }
}
