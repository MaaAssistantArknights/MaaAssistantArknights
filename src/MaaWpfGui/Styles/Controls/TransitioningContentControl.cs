// <copyright file="TransitioningContentControl.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using HandyControl.Data;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 带优先级仲裁的 <see cref="HandyControl.Controls.TransitioningContentControl"/>。
/// 所有过渡触发（内容变化、模板应用、模式变化）统一合并到一个延迟提交点，
/// 按提交时刻的最终方向播放且仅播放一次；嵌套使用时通过 <see cref="TransitionPriority"/>
/// 参与全局仲裁，优先级更高的过渡播放期间本控件让位，避免多层动画叠加错位。
/// 优先级数值越小越高，同级互不压制。
/// </summary>
public class TransitioningContentControl : HandyControl.Controls.TransitioningContentControl
{
    // 与 HandyControl 内置过渡 Storyboard 的时长保持一致（内置资源为私有，无法读取，此处取常量）
    private static readonly TimeSpan TransitionDuration = TimeSpan.FromMilliseconds(400);

    private static readonly PropertyPath TranslateXPath = new("(UIElement.RenderTransform).(TransformGroup.Children)[3].(TranslateTransform.X)");

    private static readonly PropertyPath TranslateYPath = new("(UIElement.RenderTransform).(TransformGroup.Children)[3].(TranslateTransform.Y)");

    private static readonly PropertyPath OpacityPath = new("(UIElement.Opacity)");

    // 最近一次放行的过渡及其截止时刻，优先级更低的过渡请求在此期间让位
    private static int _grantedPriority = int.MaxValue;
    private static DateTime _grantedUntil = DateTime.MinValue;

    private static readonly object GrantGate = new();

    private bool _playPending;

    // 实例首次触发发生在初始化或随外层切入的构建期（且可能早于外层登记仲裁），
    // 此时不应播入场动画
    private bool _suppressFirstTransition = true;

    public TransitioningContentControl()
    {
        // TransitionMode 变化走基类私有回调无法重写，这里通过属性描述器监听
        DependencyPropertyDescriptor.FromProperty(TransitionModeProperty, typeof(HandyControl.Controls.TransitioningContentControl))
            .AddValueChanged(this, OnTransitionModeValueChanged);
    }

    public static readonly DependencyProperty TransitionPriorityProperty = DependencyProperty.Register(
        nameof(TransitionPriority), typeof(int), typeof(TransitioningContentControl), new PropertyMetadata(0));

    /// <summary>
    /// Gets or sets 过渡优先级，数值越小越高；更高优先级的过渡播放期间，本控件的过渡被跳过。
    /// </summary>
    public int TransitionPriority
    {
        get => (int)GetValue(TransitionPriorityProperty);
        set => SetValue(TransitionPriorityProperty, value);
    }

    public override void OnApplyTemplate()
    {
        // 压掉基类的同步过渡，统一延迟提交
        SuppressBaseTransition(base.OnApplyTemplate);
        RequestTransition();
    }

    protected override void OnContentChanged(object oldContent, object newContent)
    {
        if (newContent is null)
        {
            base.OnContentChanged(oldContent, newContent);
            return;
        }

        SuppressBaseTransition(() => base.OnContentChanged(oldContent, newContent));
        RequestTransition();
    }

    private void OnTransitionModeValueChanged(object? sender, EventArgs e)
    {
        // 终止基类按（可能已过时的）模式值启动的动画，统一延迟提交
        StopActiveTransition();
        RequestTransition();
    }

    private void RequestTransition()
    {
        if (_playPending)
        {
            // 同一次切换引发的多个触发（内容、方向等）合并为一次播放
            return;
        }

        _playPending = true;

        // 提交点晚于全部绑定更新（DispatcherPriority.DataBinding）、早于渲染（Render），
        // 保证播放时方向为最终值，且首帧即动画起点
        Dispatcher.BeginInvoke(PlayPendingTransition, System.Windows.Threading.DispatcherPriority.Normal);
    }

    private void PlayPendingTransition()
    {
        _playPending = false;

        if (_suppressFirstTransition)
        {
            // 初始化/构建期不播入场动画，之后由用户切换正常触发
            _suppressFirstTransition = false;
            return;
        }

        if (!IsVisible)
        {
            return;
        }

        if (!TryAcquireTransitionGrant())
        {
            // 更高优先级的过渡播放中：让位，内容直接就位
            return;
        }

        if (GetContentPresenter() is FrameworkElement presenter)
        {
            CreateTransitionStoryboard()?.Begin(presenter);
        }
    }

    private bool TryAcquireTransitionGrant()
    {
        lock (GrantGate)
        {
            var now = DateTime.UtcNow;
            if (TransitionPriority > _grantedPriority && now < _grantedUntil)
            {
                return false;
            }

            _grantedPriority = TransitionPriority;
            _grantedUntil = now + TransitionDuration;
            return true;
        }
    }

    private void SuppressBaseTransition(Action baseAction)
    {
        // 临时置为 Collapsed，基类的 StartTransition 会因不可见而跳过，随后恢复原可见性
        var visibility = Visibility;
        Visibility = Visibility.Collapsed;
        baseAction();
        Visibility = visibility;
    }

    private void StopActiveTransition()
    {
        // 终止基类已启动的过渡 Storyboard，使内容回到最终位置
        if (GetContentPresenter() is not FrameworkElement presenter)
        {
            return;
        }

        presenter.BeginAnimation(UIElement.OpacityProperty, null);
        if (presenter.RenderTransform is TransformGroup { } group
            && group.Children.Count > 3
            && group.Children[3] is TranslateTransform translate)
        {
            translate.BeginAnimation(TranslateTransform.XProperty, null);
            translate.BeginAnimation(TranslateTransform.YProperty, null);
        }
    }

    // 控件自身的 ControlTemplate 应用之前就可能收到模式变化（如 Style 的 Setter），
    // 此时没有视觉子级，不能直接按索引取
    private FrameworkElement? GetContentPresenter()
    {
        return VisualTreeHelper.GetChildrenCount(this) > 0
            ? VisualTreeHelper.GetChild(this, 0) as FrameworkElement
            : null;
    }

    private Storyboard? CreateTransitionStoryboard()
    {
        // 与 HandyControl 内置过渡保持一致：位移 50px，CubicEase EaseOut；
        // 仅覆盖项目用到的模式，Custom 模式交由基类 TransitionStoryboard，此处不处理
        var storyboard = new Storyboard();
        var duration = new Duration(TransitionDuration);
        var easing = new CubicEase { EasingMode = EasingMode.EaseOut };

        void AddAnimation(double from, double to, PropertyPath path)
        {
            var animation = new DoubleAnimation(from, to, duration) { EasingFunction = easing };
            Storyboard.SetTargetProperty(animation, path);
            storyboard.Children.Add(animation);
        }

        var mode = TransitionMode;
        switch (mode)
        {
            case TransitionMode.Right2Left:
                AddAnimation(50, 0, TranslateXPath);
                break;
            case TransitionMode.Left2Right:
                AddAnimation(-50, 0, TranslateXPath);
                break;
            case TransitionMode.Bottom2Top:
                AddAnimation(50, 0, TranslateYPath);
                break;
            case TransitionMode.Top2Bottom:
                AddAnimation(-50, 0, TranslateYPath);
                break;
            case TransitionMode.Fade:
                AddAnimation(0, 1, OpacityPath);
                break;
            case TransitionMode.Right2LeftWithFade:
                AddAnimation(50, 0, TranslateXPath);
                AddAnimation(0, 1, OpacityPath);
                break;
            case TransitionMode.Left2RightWithFade:
                AddAnimation(-50, 0, TranslateXPath);
                AddAnimation(0, 1, OpacityPath);
                break;
            case TransitionMode.Bottom2TopWithFade:
                AddAnimation(50, 0, TranslateYPath);
                AddAnimation(0, 1, OpacityPath);
                break;
            case TransitionMode.Top2BottomWithFade:
                AddAnimation(-50, 0, TranslateYPath);
                AddAnimation(0, 1, OpacityPath);
                break;
            default:
                return null;
        }

        return storyboard;
    }
}
