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
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using HandyControl.Data;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 过渡方向的坐标轴。
/// </summary>
public enum TransitionOrientation
{
    /// <summary>
    /// 横向，索引增大自右侧滑入。
    /// </summary>
    Horizontal,

    /// <summary>
    /// 纵向，索引增大自底部滑入。
    /// </summary>
    Vertical,
}

/// <summary>
/// 带优先级仲裁的 <see cref="HandyControl.Controls.TransitioningContentControl"/>。
/// 所有过渡触发（内容变化、模板应用、模式变化）统一合并到一个延迟提交点，
/// 按提交时刻的最终方向播放且仅播放一次；嵌套使用时通过 <see cref="TransitionPriority"/>
/// 参与全局仲裁，优先级更高的过渡播放期间本控件让位，避免多层动画叠加错位。
/// 优先级数值越小越高，同级互不压制。
/// </summary>
public class TransitioningContentControl : HandyControl.Controls.TransitioningContentControl
{
    // 原速档基准时长（ms），与 HandyControl 内置过渡一致（内置资源为私有，无法读取，取常量）；
    // 快速档减半、无动画档为零，设置页导航的平滑滚动时长也按此基准同比缩放
    internal const double NormalDurationMilliseconds = 400;

    // 全局过渡时长，由设置（GuiSettingsUserControlModel）在启动和档位切换时同步，
    // 动画在播放时刻现建，改档立即生效
    public static TimeSpan TransitionDuration { get; set; } = TimeSpan.FromMilliseconds(NormalDurationMilliseconds);

    private static readonly PropertyPath TranslateXPath = new("(UIElement.RenderTransform).(TransformGroup.Children)[3].(TranslateTransform.X)");

    private static readonly PropertyPath TranslateYPath = new("(UIElement.RenderTransform).(TransformGroup.Children)[3].(TranslateTransform.Y)");

    private static readonly PropertyPath OpacityPath = new("(UIElement.Opacity)");

    // 最近一次放行的过渡及其截止时刻（毫秒，Environment.TickCount64，抗系统时钟跳变），
    // 优先级更低的过渡请求在此期间让位；更高优先级可抢占；同级互不压制；
    // 控件用弱引用持有，避免静态字段 root 住最后一次播放过渡的控件及其整棵视觉树
    private static readonly WeakReference<TransitioningContentControl?> _grantedControl = new(null);
    private static int _grantedPriority = int.MaxValue;
    private static long _grantedUntilTick = long.MinValue;

    private static readonly object GrantGate = new();

    private bool _playPending;

    // 延迟提交期间由显式方向（索引推导）指定的过渡模式；null 表示用 TransitionMode 绑定值
    private TransitionMode? _pendingMode;

    // 初始化/构建期（模板应用、绑定解析、内容设置）会触发多次 RequestTransition，
    // 期间全部静默丢弃，Loaded 后再响应真实切换
    private bool _suppressFirstTransition = true;

    public TransitioningContentControl()
    {
        // 基类会在 TransitionMode 变化 / Loaded / IsVisibleChanged 时自行 StartTransition，
        // 与自研的延迟提交动画叠加会造成回弹、初始化时还会让内容短暂不可见；
        // 用空 Storyboard 接管 TransitionStoryboard 使基类动画变为空操作，统一由 PlayPendingTransition 驱动
        TransitionStoryboard = new Storyboard();

        // 初始化期间会先后触发绑定解析、OnApplyTemplate 等多个 RequestTransition，
        // 若只压掉第一次播放，后续那次会在初始化末尾以最终模式播一遍入场动画，把内容短暂隐藏；
        // 因此整个初始化窗口内静默丢弃请求，Loaded 后再放行，并顺带清除可能残留的动画
        Loaded += (_, _) => {
            _suppressFirstTransition = false;
            StopActiveTransition();
        };
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

    public static readonly DependencyProperty TransitionIndexProperty = DependencyProperty.Register(
        nameof(TransitionIndex), typeof(int), typeof(TransitioningContentControl), new PropertyMetadata(-1, OnTransitionIndexChanged));

    /// <summary>
    /// Gets or sets 用于推导过渡方向的索引：值增大时新内容沿正方向滑入（横向自右、纵向自下），减小时反向。
    /// 从未选中（-1）进入时淡入；回到负值（如取消选择）不过渡。
    /// 直接绑定页签或列表的选中索引即可，无需在 ViewModel 中维护过渡模式。
    /// </summary>
    public int TransitionIndex
    {
        get => (int)GetValue(TransitionIndexProperty);
        set => SetValue(TransitionIndexProperty, value);
    }

    public static readonly DependencyProperty TransitionOrientationProperty = DependencyProperty.Register(
        nameof(TransitionOrientation), typeof(TransitionOrientation), typeof(TransitioningContentControl), new PropertyMetadata(TransitionOrientation.Horizontal));

    /// <summary>
    /// Gets or sets 过渡方向的坐标轴，默认横向。
    /// </summary>
    public TransitionOrientation TransitionOrientation
    {
        get => (TransitionOrientation)GetValue(TransitionOrientationProperty);
        set => SetValue(TransitionOrientationProperty, value);
    }

    public static readonly DependencyProperty TransitionFadeProperty = DependencyProperty.Register(
        nameof(TransitionFade), typeof(bool), typeof(TransitioningContentControl), new PropertyMetadata(false));

    /// <summary>
    /// Gets or sets 在平移动画中叠加淡入，柔化进入内容被自身可视区切割的边缘。
    /// 伪全屏（内容基本铺满、可显示区域大）页面默认禁用；面积受限、动画期间内容会超出
    /// 自身可显示区域被切割的页面应开启。
    /// </summary>
    public bool TransitionFade
    {
        get => (bool)GetValue(TransitionFadeProperty);
        set => SetValue(TransitionFadeProperty, value);
    }

    private int _lastTransitionIndex = -1;

    private static void OnTransitionIndexChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var ctl = (TransitioningContentControl)d;
        ctl.UpdateTransitionModeByIndex((int)e.NewValue);
    }

    private void UpdateTransitionModeByIndex(int newIndex)
    {
        var oldIndex = _lastTransitionIndex;
        _lastTransitionIndex = newIndex;
        if (newIndex == oldIndex)
        {
            return;
        }

        if (newIndex < 0)
        {
            // 取消选择：不过渡
            return;
        }

        TransitionMode mode;
        if (oldIndex < 0)
        {
            // 从未选中进入：没有上一项位置可推导方向，淡入即可
            mode = TransitionMode.Fade;
        }
        else
        {
            // 同方向连续切换时模式值可能不变，直接内部强制触发
            var forward = newIndex > oldIndex;
            var vertical = TransitionOrientation == TransitionOrientation.Vertical;
            mode = (forward, vertical) switch {
                (true, false) => TransitionMode.Right2Left,
                (false, false) => TransitionMode.Left2Right,
                (true, true) => TransitionMode.Bottom2Top,
                (false, true) => TransitionMode.Top2Bottom,
            };
            if (TransitionFade)
            {
                mode = (forward, vertical) switch {
                    (true, false) => TransitionMode.Right2LeftWithFade,
                    (false, false) => TransitionMode.Left2RightWithFade,
                    (true, true) => TransitionMode.Bottom2TopWithFade,
                    (false, true) => TransitionMode.Top2BottomWithFade,
                };
            }
        }

        // 显式指定方向而非写回 TransitionMode：直接写会清掉 TransitionMode 的绑定
        // （如常规/高级切换的 ContentTransitionMode），导致后续绑定变化不再触发过渡
        RequestTransition(mode);
    }

    public override void OnApplyTemplate()
    {
        base.OnApplyTemplate();
        RequestTransition();
    }

    protected override void OnContentChanged(object oldContent, object newContent)
    {
        if (newContent is null)
        {
            base.OnContentChanged(oldContent, newContent);
            return;
        }

        base.OnContentChanged(oldContent, newContent);
        RequestTransition();
    }

    protected override void OnPropertyChanged(DependencyPropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);
        if (e.Property == TransitionModeProperty)
        {
            // TransitionMode 变化走基类私有回调无法重写，这里在属性变更回调中监听；
            // 基类动画已由空 TransitionStoryboard 接管为空操作，这里只需延迟提交自研动画
            RequestTransition();
        }
    }

    private void RequestTransition()
    {
        RequestTransition(null);
    }

    private void RequestTransition(TransitionMode? mode)
    {
        if (_suppressFirstTransition)
        {
            // 初始化/构建期不播入场动画，静默丢弃
            return;
        }

        if (TransitionDuration <= TimeSpan.Zero)
        {
            // 无动画档：清掉可能仍在播放的动画，内容直接就位
            StopActiveTransition();
            return;
        }

        if (_playPending)
        {
            // 同一次切换引发的多个触发（内容、方向、模式等）合并为一次播放；
            // 显式指定的方向（索引推导）优先于隐式模式（TransitionMode 绑定）
            if (mode.HasValue)
            {
                _pendingMode = mode;
            }

            return;
        }

        _playPending = true;
        _pendingMode = mode;

        // 提交点早于渲染（Render）：布局失效会把渲染消息先排到 Render 优先级，
        // 若在 Render 提交，内容会先以最终位置渲染一帧、再回跳到动画起点（回弹）；
        // 用 DataBind 提交则动画在渲染前开始，首帧即动画起点。
        // 方向此时已确定：模式在触发前已同步设置（UpdateTransitionModeByIndex / 绑定），无需等全部绑定
        Dispatcher.BeginInvoke(PlayPendingTransition, System.Windows.Threading.DispatcherPriority.DataBind);
    }

    private void PlayPendingTransition()
    {
        _playPending = false;
        var mode = _pendingMode ?? TransitionMode;
        _pendingMode = null;

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
            // 位移模式依赖 RenderTransform 的 TransformGroup[3]，与 StopActiveTransition 的既有判断对称：
            // 取不到时退化为淡入而非 Begin 抛 InvalidOperationException
            var effectiveMode = RequiresTranslate(mode) && !HasTranslateTransform(presenter)
                ? TransitionMode.Fade
                : mode;
            CreateTransitionStoryboard(effectiveMode)?.Begin(presenter);
        }
    }

    private bool TryAcquireTransitionGrant()
    {
        lock (GrantGate)
        {
            var now = Environment.TickCount64;
            _grantedControl.TryGetTarget(out var granted);
            if (now < _grantedUntilTick && granted != this)
            {
                // 窗口内已有其他控件的过渡在播放：
                // 更低优先级让位；同级互不压制；更高优先级可抢占。
                // 抢占只影响后续请求的准入，无法撤回已开始的动画——低优先级先提交、高优先级随后到
                // 时两者会叠加播放；现有接入点外层过渡随选中项变更同步入队（DataBind 优先级），
                // 总先于内层子树 realize（Render 优先级）提交，天然满足高优先级在先，新接入点需保持此顺序
                if (TransitionPriority > _grantedPriority)
                {
                    return false;
                }
            }

            _grantedControl.SetTarget(this);
            _grantedPriority = TransitionPriority;
            _grantedUntilTick = now + (long)TransitionDuration.TotalMilliseconds;
            return true;
        }
    }

    // 非 Fade 模式都依赖 TransformGroup[3] 的位移
    private static bool RequiresTranslate(TransitionMode mode) => mode != TransitionMode.Fade;

    private static bool HasTranslateTransform(FrameworkElement element) =>
        element.RenderTransform is TransformGroup group
        && group.Children.Count > 3
        && group.Children[3] is TranslateTransform;

    private void StopActiveTransition()
    {
        // 清除内容上可能残留的过渡动画（基类动画已由空 TransitionStoryboard 接管为空操作），
        // 使内容回到最终位置
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

    private Storyboard? CreateTransitionStoryboard(TransitionMode mode)
    {
        // 与 HandyControl 内置过渡保持一致：位移 50px，CubicEase EaseOut；
        // 仅覆盖项目用到的模式，未列出的模式（含 Custom，基类动画已被空 Storyboard 架空）不播动画
        var storyboard = new Storyboard();
        var duration = new Duration(TransitionDuration);
        var easing = new CubicEase { EasingMode = EasingMode.EaseOut };

        void AddAnimation(double from, double to, PropertyPath path)
        {
            var animation = new DoubleAnimation(from, to, duration) { EasingFunction = easing };
            Storyboard.SetTargetProperty(animation, path);
            storyboard.Children.Add(animation);
        }

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
