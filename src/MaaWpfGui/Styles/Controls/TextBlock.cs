// <copyright file="TextBlock.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Styles.Controls;

public class TextBlock : System.Windows.Controls.TextBlock
{
    static TextBlock()
    {
        DefaultStyleKeyProperty.OverrideMetadata(typeof(TextBlock), new FrameworkPropertyMetadata(typeof(TextBlock)));
        ForegroundProperty.OverrideMetadata(typeof(TextBlock), new FrameworkPropertyMetadata(OnForegroundChanged));
        RainbowAnimationBehavior.IsActiveProperty.OverrideMetadata(typeof(TextBlock), new FrameworkPropertyMetadata(true));
    }

    public TextBlock()
    {
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
    }

    private static void OnLoaded(object sender, RoutedEventArgs e) => RainbowAnimationBehavior.RefreshAnimation((TextBlock)sender);

    private static void OnUnloaded(object sender, RoutedEventArgs e) => RainbowAnimationBehavior.StopAnimation((TextBlock)sender);

    private static void OnForegroundChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        // 动画只改变画刷内部的 Transform；相同画刷的子属性通知无需处理。
        if (d is TextBlock { IsLoaded: true } element && !ReferenceEquals(e.OldValue, e.NewValue))
        {
            RainbowAnimationBehavior.RefreshAnimation(element);
        }
    }

    public static readonly DependencyProperty ForegroundKeyProperty = DependencyProperty.Register(nameof(ForegroundKey), typeof(string), typeof(TextBlock), new PropertyMetadata(ThemeHelper.DefaultKey, OnForegroundKeyChanged));

    private static void OnForegroundKeyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var element = (TextBlock)d;
        if (e.NewValue != null)
        {
            element.ApplyForegroundKey((string)e.NewValue);
        }
    }

    public string ForegroundKey
    {
        get {
            return (string)GetValue(ForegroundKeyProperty);
        }

        set {
            SetValue(ForegroundKeyProperty, value);
        }
    }

    private void ApplyForegroundKey(string value)
    {
        // 彩虹资源使用 x:Shared="False"，预先查找会额外创建一个不会使用的画刷。
        // 其余主题 Brush 仍用 TryFindResource，以支持 MergedDictionaries。
        if (value == UiLogColor.Rainbow || TryFindResource(value) is Brush)
        {
            SetResourceReference(ForegroundProperty, value);
            return;
        }

        var brush = ThemeHelper.String2Brush(value);
        if (ThemeHelper.SimilarToBackground(brush.Color))
        {
            SetResourceReference(ForegroundProperty, ThemeHelper.DefaultKey);
            return;
        }

        SetValue(ForegroundProperty, brush);
    }

    public static readonly DependencyProperty BindableInlinesProperty =
        DependencyProperty.RegisterAttached(
            "BindableInlines",
            typeof(IEnumerable<Inline>),
            typeof(TextBlock),
            new PropertyMetadata(null, OnBindableInlinesChanged));

    public static void SetBindableInlines(DependencyObject element, IEnumerable<Inline> value)
        => element.SetValue(BindableInlinesProperty, value);

    public static IEnumerable<Inline> GetBindableInlines(DependencyObject element)
        => (IEnumerable<Inline>)element.GetValue(BindableInlinesProperty);

    private static void OnBindableInlinesChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not TextBlock tb)
        {
            return;
        }

        tb.Inlines.Clear();
        if (e.NewValue is not IEnumerable<Inline> inlines)
        {
            return;
        }

        foreach (var inline in inlines)
        {
            tb.Inlines.Add(inline);
        }
    }
}
