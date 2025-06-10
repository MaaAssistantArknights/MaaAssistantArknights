// <copyright file="FadeBehavior.cs" company="MaaAssistantArknights">
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
using System.Windows.Media.Animation;

namespace MaaWpfGui.Extensions.UIBehaviors;

public static class FadeBehavior
{
    #region IsActive 属性

    public static readonly DependencyProperty IsActiveProperty =
        DependencyProperty.RegisterAttached("IsActive", typeof(bool), typeof(FadeBehavior),
            new(false, OnIsActiveChanged));

    public static bool GetIsActive(DependencyObject obj) => (bool)obj.GetValue(IsActiveProperty);

    public static void SetIsActive(DependencyObject obj, bool value) => obj.SetValue(IsActiveProperty, value);

    #endregion

    #region Duration 属性（新增）

    public static readonly DependencyProperty DurationProperty =
        DependencyProperty.RegisterAttached("Duration", typeof(TimeSpan), typeof(FadeBehavior),
            new(TimeSpan.FromSeconds(0.3)));

    public static TimeSpan GetDuration(DependencyObject obj) => (TimeSpan)obj.GetValue(DurationProperty);

    public static void SetDuration(DependencyObject obj, TimeSpan value) => obj.SetValue(DurationProperty, value);

    #endregion

    private static void OnIsActiveChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not FrameworkElement element)
        {
            return;
        }

        // 获取动画持续时间（使用附加属性或默认值）
        var duration = GetDuration(d);

        var storyboard = new Storyboard();
        var animation = new DoubleAnimation
        {
            Duration = new(duration),
            FillBehavior = FillBehavior.HoldEnd,
        };

        if ((bool)e.NewValue)
        {
            // 显示动画
            animation.From = 0;
            animation.To = 1;
            element.Visibility = Visibility.Visible;
        }
        else
        {
            // 隐藏动画
            animation.From = 1;
            animation.To = 0;
            animation.Completed += (s, _) =>
            {
                // 动画完成后才真正隐藏元素
                if (element.Opacity == 0)
                {// 确保没有其他动画改变透明度
                    element.Visibility = Visibility.Collapsed;
                }
            };
        }

        Storyboard.SetTarget(animation, element);
        Storyboard.SetTargetProperty(animation, new PropertyPath(UIElement.OpacityProperty));
        storyboard.Children.Add(animation);
        storyboard.Begin();
    }
}
