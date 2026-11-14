// <copyright file="GuideHighlight.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Windows;

namespace MaaWpfGui.Helper;

/// <summary>
/// 将设置指引的目标控件注册到全局遮罩层，由遮罩挖孔显示目标并展示文字提示。
/// </summary>
public static class GuideHighlight
{
    private static readonly DependencyProperty OverlayProperty = DependencyProperty.RegisterAttached(
        "Overlay",
        typeof(GuideSpotlightOverlay),
        typeof(GuideHighlight),
        new PropertyMetadata(null));

    private static readonly List<FrameworkElement> Pending = [];

    /// <summary>
    /// 标识目标控件是否需要显示引导高亮。
    /// </summary>
    public static readonly DependencyProperty IsActiveProperty = DependencyProperty.RegisterAttached(
        "IsActive",
        typeof(bool),
        typeof(GuideHighlight),
        new PropertyMetadata(false, OnIsActiveChanged));

    /// <summary>
    /// 标识遮罩提示中显示的文字。
    /// </summary>
    public static readonly DependencyProperty TextProperty = DependencyProperty.RegisterAttached(
        "Text",
        typeof(string),
        typeof(GuideHighlight),
        new PropertyMetadata(string.Empty, OnTextChanged));

    /// <summary>
    /// 标识提示文字相对目标控件的位置。
    /// </summary>
    public static readonly DependencyProperty PlacementProperty = DependencyProperty.RegisterAttached(
        "Placement",
        typeof(GuideHighlightPlacement),
        typeof(GuideHighlight),
        new PropertyMetadata(GuideHighlightPlacement.Top, OnPlacementChanged));

    /// <summary>
    /// 标识引导高亮的呈现方式。
    /// </summary>
    public static readonly DependencyProperty ModeProperty = DependencyProperty.RegisterAttached(
        "Mode",
        typeof(GuideHighlightMode),
        typeof(GuideHighlight),
        new PropertyMetadata(GuideHighlightMode.Spotlight, OnModeChanged));

    /// <summary>
    /// 获取目标控件是否显示引导高亮。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <returns>是否显示。</returns>
    public static bool GetIsActive(DependencyObject element) => (bool)element.GetValue(IsActiveProperty);

    /// <summary>
    /// 设置目标控件是否显示引导高亮。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <param name="value">是否显示。</param>
    public static void SetIsActive(DependencyObject element, bool value) => element.SetValue(IsActiveProperty, value);

    /// <summary>
    /// 获取提示文字。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <returns>提示文字。</returns>
    public static string GetText(DependencyObject element) => (string)element.GetValue(TextProperty);

    /// <summary>
    /// 设置提示文字。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <param name="value">提示文字。</param>
    public static void SetText(DependencyObject element, string value) => element.SetValue(TextProperty, value);

    /// <summary>
    /// 获取提示文字位置。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <returns>提示文字位置。</returns>
    public static GuideHighlightPlacement GetPlacement(DependencyObject element) =>
        (GuideHighlightPlacement)element.GetValue(PlacementProperty);

    /// <summary>
    /// 设置提示文字位置。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <param name="value">提示文字位置。</param>
    public static void SetPlacement(DependencyObject element, GuideHighlightPlacement value) =>
        element.SetValue(PlacementProperty, value);

    /// <summary>
    /// 获取引导高亮的呈现方式。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <returns>呈现方式。</returns>
    public static GuideHighlightMode GetMode(DependencyObject element) =>
        (GuideHighlightMode)element.GetValue(ModeProperty);

    /// <summary>
    /// 设置引导高亮的呈现方式。
    /// </summary>
    /// <param name="element">目标控件。</param>
    /// <param name="value">呈现方式。</param>
    public static void SetMode(DependencyObject element, GuideHighlightMode value) =>
        element.SetValue(ModeProperty, value);

    internal static void NotifyOverlayReady(GuideSpotlightOverlay overlay)
    {
        if (Pending.Count == 0)
        {
            return;
        }

        FrameworkElement[] snapshot = Pending.ToArray();
        Pending.Clear();
        foreach (var element in snapshot)
        {
            if (GetIsActive(element) && element.IsLoaded)
            {
                Attach(element, overlay);
            }
        }
    }

    private static void OnIsActiveChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not FrameworkElement element)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            element.Loaded -= OnLoaded;
            element.Loaded += OnLoaded;
            element.Unloaded -= OnUnloaded;
            element.Unloaded += OnUnloaded;
            element.IsVisibleChanged -= OnIsVisibleChanged;
            element.IsVisibleChanged += OnIsVisibleChanged;
            if (element.IsLoaded)
            {
                Apply(element);
            }
        }
        else
        {
            element.Loaded -= OnLoaded;
            element.Unloaded -= OnUnloaded;
            element.IsVisibleChanged -= OnIsVisibleChanged;
            Remove(element);
        }
    }

    private static void OnLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement element && GetIsActive(element))
        {
            Apply(element);
        }
    }

    private static void OnUnloaded(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            Remove(element);
        }
    }

    /// <summary>
    /// 目标进出可视区（折叠、切页、展开高级设置等）会改变 ｢该高亮谁｣，让遮罩重新解析目标。
    /// </summary>
    /// <param name="sender">状态发生变化的元素。</param>
    /// <param name="e">可见性变化事件参数。</param>
    private static void OnIsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            RefreshTarget(element);
        }
    }

    private static void OnTextChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) => RefreshTarget(d);

    private static void OnPlacementChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) => RefreshTarget(d);

    private static void OnModeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) => RefreshTarget(d);

    /// <summary>
    /// 附加属性变化后让遮罩重绘。遮罩在渲染时直接读附加属性，这里只触发重绘，不同步任何副本。
    /// </summary>
    /// <param name="d">状态发生变化的元素。</param>
    private static void RefreshTarget(DependencyObject d)
    {
        if (d is FrameworkElement element && element.GetValue(OverlayProperty) is GuideSpotlightOverlay overlay)
        {
            overlay.Refresh();
        }
    }

    private static void Apply(FrameworkElement element)
    {
        if (element.GetValue(OverlayProperty) is GuideSpotlightOverlay existing)
        {
            // 遮罩已卸载、或已被新实例顶替时丢弃这条旧引用，否则目标会一直挂在失效实例上，
            // 后续的 Refresh 只作用在旧遮罩，目标永远不会注册到当前遮罩，高亮随之消失
            if (existing.IsLoaded && ReferenceEquals(existing, GuideSpotlightOverlay.Current))
            {
                existing.Refresh();
                return;
            }

            element.ClearValue(OverlayProperty);
        }

        if (GuideSpotlightOverlay.Current is not { } overlay)
        {
            if (!Pending.Contains(element))
            {
                Pending.Add(element);
            }

            return;
        }

        Attach(element, overlay);
    }

    /// <summary>
    /// 遮罩卸载时断开目标与它的引用：清掉附加属性，目标仍处于激活且已加载状态时挂回待注册列表，
    /// 等下一个遮罩就绪后由 <see cref="NotifyOverlayReady"/> 重新接管。
    /// </summary>
    /// <param name="element">仍指向该遮罩的目标控件。</param>
    internal static void Detach(FrameworkElement element)
    {
        element.ClearValue(OverlayProperty);
        if (GetIsActive(element) && element.IsLoaded && !Pending.Contains(element))
        {
            Pending.Add(element);
        }
    }

    private static void Attach(FrameworkElement element, GuideSpotlightOverlay overlay)
    {
        overlay.RegisterTarget(element);
        element.SetValue(OverlayProperty, overlay);
        Pending.Remove(element);
    }

    private static void Remove(FrameworkElement element)
    {
        Pending.Remove(element);
        if (element.GetValue(OverlayProperty) is not GuideSpotlightOverlay overlay)
        {
            return;
        }

        overlay.UnregisterTarget(element);
        element.ClearValue(OverlayProperty);
    }

    /// <summary>
    /// 提示文字相对目标控件的位置。
    /// </summary>
    public enum GuideHighlightPlacement
    {
        /// <summary>
        /// 上方居中。
        /// </summary>
        Top,

        /// <summary>
        /// 下方居中。
        /// </summary>
        Bottom,

        /// <summary>
        /// 左侧垂直居中。
        /// </summary>
        Left,

        /// <summary>
        /// 右侧垂直居中。
        /// </summary>
        Right,
    }

    /// <summary>
    /// 引导高亮的呈现方式。
    /// </summary>
    public enum GuideHighlightMode
    {
        /// <summary>
        /// 全局半透明遮罩挖孔，文字直接显示在遮罩上。
        /// </summary>
        Spotlight,

        /// <summary>
        /// 不绘制遮罩，只在目标控件外围绘制选择框。
        /// </summary>
        Frame,
    }
}
