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
        public static readonly DependencyProperty TargetPropertyProperty =
            DependencyProperty.RegisterAttached(
                "TargetProperty",
                typeof(DependencyProperty),
                typeof(HoverDelayHelper),
                new(null, OnPropertyChanged));

        public static readonly DependencyProperty TargetValueProperty =
            DependencyProperty.RegisterAttached(
                "TargetValue",
                typeof(double),
                typeof(HoverDelayHelper),
                new(0.0));

        public static readonly DependencyProperty DefaultValueProperty =
            DependencyProperty.RegisterAttached(
                "DefaultValue",
                typeof(double),
                typeof(HoverDelayHelper),
                new(0.0));

        public static readonly DependencyProperty AnimationDurationProperty =
            DependencyProperty.RegisterAttached(
                "AnimationDuration",
                typeof(TimeSpan),
                typeof(HoverDelayHelper),
                new(TimeSpan.FromMilliseconds(300)));

        public static readonly DependencyProperty AccelerationRatioProperty =
            DependencyProperty.RegisterAttached(
                "AccelerationRatio",
                typeof(double),
                typeof(HoverDelayHelper),
                new(0.3));

        public static readonly DependencyProperty DecelerationRatioProperty =
            DependencyProperty.RegisterAttached(
                "DecelerationRatio",
                typeof(double),
                typeof(HoverDelayHelper),
                new(0.3));

        private static readonly DependencyProperty _hoverTimerProperty =
            DependencyProperty.RegisterAttached(
                "_hoverTimer",
                typeof(DispatcherTimer),
                typeof(HoverDelayHelper),
                new PropertyMetadata(null));

        public static readonly DependencyProperty DelayProperty =
            DependencyProperty.RegisterAttached(
                "Delay",
                typeof(int),
                typeof(HoverDelayHelper),
                new PropertyMetadata(500, OnPropertyChanged));

        // Getter/Setter 方法
        public static DependencyProperty GetTargetProperty(DependencyObject obj) => (DependencyProperty)obj.GetValue(TargetPropertyProperty);

        public static void SetTargetProperty(DependencyObject obj, DependencyProperty value) => obj.SetValue(TargetPropertyProperty, value);

        public static double GetTargetValue(DependencyObject obj) => (double)obj.GetValue(TargetValueProperty);

        public static void SetTargetValue(DependencyObject obj, double value) => obj.SetValue(TargetValueProperty, value);

        public static double GetDefaultValue(DependencyObject obj) => (double)obj.GetValue(DefaultValueProperty);

        public static void SetDefaultValue(DependencyObject obj, double value) => obj.SetValue(DefaultValueProperty, value);

        public static TimeSpan GetAnimationDuration(DependencyObject obj) => (TimeSpan)obj.GetValue(AnimationDurationProperty);

        public static void SetAnimationDuration(DependencyObject obj, TimeSpan value) => obj.SetValue(AnimationDurationProperty, value);

        public static double GetAccelerationRatio(DependencyObject obj) => (double)obj.GetValue(AccelerationRatioProperty);

        public static void SetAccelerationRatio(DependencyObject obj, double value) => obj.SetValue(AccelerationRatioProperty, value);

        public static double GetDecelerationRatio(DependencyObject obj) => (double)obj.GetValue(DecelerationRatioProperty);

        public static void SetDecelerationRatio(DependencyObject obj, double value) => obj.SetValue(DecelerationRatioProperty, value);

        public static int GetDelay(DependencyObject obj) => (int)obj.GetValue(DelayProperty);

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

            var targetProperty = GetTargetProperty(element);
            if (targetProperty != null)
            {
                element.SetValue(targetProperty, GetDefaultValue(element));
            }

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

                var targetProperty = GetTargetProperty(element);
                if (targetProperty != null)
                {
                    AnimateToValue(element, targetProperty, GetTargetValue(element));
                }
            };
            element.SetValue(_hoverTimerProperty, timer);
            timer.Start();
        }

        private static void OnMouseLeave(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)sender;
            var timer = (DispatcherTimer)element.GetValue(_hoverTimerProperty);
            timer?.Stop();

            var targetProperty = GetTargetProperty(element);
            if (targetProperty != null)
            {
                AnimateToValue(element, targetProperty, GetDefaultValue(element));
            }
        }

        private static void AnimateToValue(UIElement element, DependencyProperty property, double value)
        {
            var animation = new DoubleAnimation
            {
                To = value,
                Duration = GetAnimationDuration(element),
                AccelerationRatio = GetAccelerationRatio(element),
                DecelerationRatio = GetDecelerationRatio(element),
            };

            element.BeginAnimation(property, animation);
        }
    }
}
