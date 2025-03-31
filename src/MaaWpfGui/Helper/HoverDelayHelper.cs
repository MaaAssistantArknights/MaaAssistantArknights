// <copyright file="HoverDelayHelper.cs" company="MaaAssistantArknights">
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

using System;
using System.Windows;
using System.Windows.Media.Animation;
using System.Windows.Threading;


namespace MaaWpfGui.Helper
{
    public static class HoverDelayHelper
    {
        // 是否已完成悬停计时（只读，供外部绑定）
        public static readonly DependencyPropertyKey IsHoveredPropertyKey =
            DependencyProperty.RegisterAttachedReadOnly(
                "IsHovered",
                typeof(bool),
                typeof(HoverDelayHelper),
                new(false, OnPropertyChanged));

        public static readonly DependencyProperty IsHoveredProperty = IsHoveredPropertyKey.DependencyProperty;

        public static bool GetIsHovered(DependencyObject obj) => (bool)obj.GetValue(IsHoveredProperty);

        private static readonly DependencyProperty _hoverTimerProperty =
            DependencyProperty.RegisterAttached(
                "_hoverTimer",
                typeof(DispatcherTimer),
                typeof(HoverDelayHelper),
                new(null));

        public static readonly DependencyProperty DelayProperty =
            DependencyProperty.RegisterAttached(
                "Delay",
                typeof(int),
                typeof(HoverDelayHelper),
                new(-1, OnPropertyChanged));

        // Getter/Setter 方法
        public static int GetDelay(DependencyObject obj)
        {
            int value = (int)obj.GetValue(DelayProperty);
            return value == -1 ? 0 : value;
        }

        public static void SetDelay(DependencyObject obj, int value) => obj.SetValue(DelayProperty, value);

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is not UIElement element)
            {
                return;
            }

            element.MouseEnter -= OnMouseEnter;
            element.MouseLeave -= OnMouseLeave;
            var timer = (DispatcherTimer)element.GetValue(_hoverTimerProperty);
            timer?.Stop();

            element.MouseEnter += OnMouseEnter;
            element.MouseLeave += OnMouseLeave;
        }

        private static void OnMouseEnter(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)sender;
            var delay = GetDelay(element);
            var timer = (DispatcherTimer)element.GetValue(_hoverTimerProperty);

            timer?.Stop();

            timer = new() { Interval = TimeSpan.FromMilliseconds(delay) };
            timer.Tick += (s, args) =>
            {
                timer.Stop();
                if (!element.IsMouseOver)
                {
                    return;
                }

                element.SetValue(IsHoveredPropertyKey, true);
            };
            element.SetValue(_hoverTimerProperty, timer);
            timer.Start();
        }

        private static void OnMouseLeave(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)sender;
            var timer = (DispatcherTimer)element.GetValue(_hoverTimerProperty);
            timer?.Stop();

            element.SetValue(IsHoveredPropertyKey, false);
        }
    }
}
