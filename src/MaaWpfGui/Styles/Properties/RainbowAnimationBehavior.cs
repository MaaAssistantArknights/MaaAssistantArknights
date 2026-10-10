// <copyright file="RainbowAnimationBehavior.cs" company="MaaAssistantArknights">
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

using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;
using MaaTextBlock = MaaWpfGui.Styles.Controls.TextBlock;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 为 RainbowFlowBrush 资源提供流光动画的附加行为。
/// 使用固定宽度的绝对坐标渐变，让不同字形段共享连续、匀速的流光。
/// </summary>
public static class RainbowAnimationBehavior
{
    private const double Speed = 160;

    private static readonly DoubleAnimation _defaultAnimation = CreateAnimation(280);

    private static readonly DependencyProperty AnimationStateProperty =
        DependencyProperty.RegisterAttached(
            "AnimationState",
            typeof(AnimationState),
            typeof(RainbowAnimationBehavior),
            new PropertyMetadata(null));

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
            if (element is not MaaTextBlock)
            {
                element.Loaded += OnElementLoaded;
                element.Unloaded += OnElementUnloaded;
            }

            if (element.IsLoaded)
            {
                StartAnimation(element);
            }
        }
        else
        {
            if (element is not MaaTextBlock)
            {
                element.Loaded -= OnElementLoaded;
                element.Unloaded -= OnElementUnloaded;
            }

            StopAnimation(element);
        }
    }

    internal static void RefreshAnimation(FrameworkElement element)
    {
        if (!element.IsLoaded || !GetIsActive(element))
        {
            return;
        }

        if (element.GetValue(AnimationStateProperty) is AnimationState state)
        {
            state.UpdateAnimation();
        }
        else
        {
            StartAnimation(element);
        }
    }

    private static void OnElementLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement element)
        {
            return;
        }

        StartAnimation(element);
    }

    private static void OnElementUnloaded(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            StopAnimation(element);
        }
    }

    private static void StartAnimation(FrameworkElement element)
    {
        if (element.GetValue(AnimationStateProperty) is AnimationState)
        {
            return;
        }

        var foregroundProperty = element switch {
            TextBlock => TextBlock.ForegroundProperty,
            Control => Control.ForegroundProperty,
            _ => null,
        };

        if (foregroundProperty == null || (element is MaaTextBlock && !IsFlowBrush(element.GetValue(foregroundProperty))))
        {
            return;
        }

        var state = new AnimationState(element, foregroundProperty);
        element.SetValue(AnimationStateProperty, state);
        state.Start();
    }

    internal static void StopAnimation(FrameworkElement element)
    {
        if (element.GetValue(AnimationStateProperty) is AnimationState state)
        {
            state.Stop();
            element.ClearValue(AnimationStateProperty);
        }
    }

    private static bool IsFlowBrush(object? brush) => brush is LinearGradientBrush {
        MappingMode: BrushMappingMode.Absolute,
        SpreadMethod: GradientSpreadMethod.Repeat,
        IsFrozen: false,
        Transform: TranslateTransform { IsFrozen: false },
    } gradient && gradient.EndPoint.X > gradient.StartPoint.X && gradient.EndPoint.Y == gradient.StartPoint.Y;

    private static DoubleAnimation CreateAnimation(double width)
    {
        var animation = new DoubleAnimation {
            From = 0,
            To = width,
            Duration = new Duration(TimeSpan.FromSeconds(width / Speed)),
            RepeatBehavior = new RepeatBehavior(TimeSpan.FromSeconds(120)),
        };

        // 流光不需要跟随高刷新率显示器逐帧更新；冻结模板可供多个时钟共用。
        Timeline.SetDesiredFrameRate(animation, 30);
        animation.Freeze();
        return animation;
    }

    private sealed class AnimationState
    {
        private readonly FrameworkElement _element;
        private readonly DependencyProperty _foregroundProperty;
        private readonly DependencyPropertyDescriptor? _foregroundDescriptor;
        private LinearGradientBrush? _animatedBrush;
        private AnimationClock? _clock;

        public AnimationState(FrameworkElement element, DependencyProperty foregroundProperty)
        {
            _element = element;
            _foregroundProperty = foregroundProperty;

            // 自定义 TextBlock 使用属性元数据回调，只为显式启用行为的原生控件注册监听。
            if (element is not MaaTextBlock)
            {
                _foregroundDescriptor = DependencyPropertyDescriptor.FromProperty(foregroundProperty, element.GetType());
            }
        }

        public void Start()
        {
            _foregroundDescriptor?.AddValueChanged(_element, OnForegroundChanged);
            UpdateAnimation();
        }

        public void Stop()
        {
            _foregroundDescriptor?.RemoveValueChanged(_element, OnForegroundChanged);
            StopBrushAnimation();
        }

        private void OnForegroundChanged(object? sender, EventArgs e) => UpdateAnimation();

        public void UpdateAnimation()
        {
            var brush = _element.GetValue(_foregroundProperty);
            if (ReferenceEquals(brush, _animatedBrush))
            {
                return;
            }

            StopBrushAnimation();
            if (!IsFlowBrush(brush))
            {
                if (_foregroundDescriptor == null)
                {
                    _element.ClearValue(AnimationStateProperty);
                }

                return;
            }

            var linearBrush = (LinearGradientBrush)brush;
            var width = linearBrush.EndPoint.X - linearBrush.StartPoint.X;

            // 流光资源使用 x:Shared="False"，每个控件拥有独立画刷。
            // 仅修改 Transform，保留 Foreground 上的绑定和 DynamicResource 表达式。
            _animatedBrush = linearBrush;
            var translate = (TranslateTransform)_animatedBrush.Transform;
            var animation = width == _defaultAnimation.To ? _defaultAnimation : CreateAnimation(width);
            _clock = animation.CreateClock();
            _clock.Completed += OnAnimationCompleted;
            translate.ApplyAnimationClock(TranslateTransform.XProperty, _clock);
        }

        private void OnAnimationCompleted(object? sender, EventArgs e)
        {
            if (!ReferenceEquals(sender, _clock) || _animatedBrush?.Transform is not TranslateTransform translate)
            {
                return;
            }

            // 保留结束时的颜色位置，并移除动画时钟，停止后续逐帧更新。
            var finalOffset = translate.X;
            translate.BeginAnimation(TranslateTransform.XProperty, null);
            translate.X = finalOffset;
            _clock = null;
        }

        private void StopBrushAnimation()
        {
            if (_animatedBrush?.Transform is TranslateTransform translate)
            {
                translate.BeginAnimation(TranslateTransform.XProperty, null);
            }

            _animatedBrush = null;
            _clock = null;
        }
    }
}
