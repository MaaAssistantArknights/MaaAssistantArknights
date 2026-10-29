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

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Styles.Controls;

public class TextBlock : System.Windows.Controls.TextBlock, IWeakEventListener
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

        // 换出的通知集合断开监听，避免旧集合的后续变更继续写入 Inlines；
        // 弱事件订阅避免集合的引用延长元素的生命周期
        if (e.OldValue is INotifyCollectionChanged oldCollection)
        {
            CollectionChangedEventManager.RemoveListener(oldCollection, tb);
        }

        if (e.NewValue is INotifyCollectionChanged newCollection)
        {
            CollectionChangedEventManager.AddListener(newCollection, tb);
        }

        // 值变化时整表快照兜底，非通知集合（如 IReadOnlyList）仅有这一步；
        // 通知集合的后续增删改由 ReceiveWeakEvent 增量同步
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

    bool IWeakEventListener.ReceiveWeakEvent(Type managerType, object sender, EventArgs e)
    {
        if (managerType != typeof(CollectionChangedEventManager)
            || e is not NotifyCollectionChangedEventArgs args)
        {
            return false;
        }

        // 值已切换后旧集合迟到的变更事件直接丢弃
        if (ReferenceEquals(GetBindableInlines(this), sender))
        {
            SyncInlines(args);
        }

        return true;
    }

    private void SyncInlines(NotifyCollectionChangedEventArgs e)
    {
        switch (e.Action)
        {
            case NotifyCollectionChangedAction.Add:
                InsertInlines(e.NewItems, e.NewStartingIndex);
                break;

            case NotifyCollectionChangedAction.Remove:
                RemoveInlines(e.OldItems);
                break;

            case NotifyCollectionChangedAction.Replace:
            case NotifyCollectionChangedAction.Move:
                RemoveInlines(e.OldItems);
                InsertInlines(e.NewItems, e.NewStartingIndex);
                break;

            case NotifyCollectionChangedAction.Reset:
                Inlines.Clear();
                if (GetBindableInlines(this) is IEnumerable<Inline> inlines)
                {
                    foreach (var inline in inlines)
                    {
                        Inlines.Add(inline);
                    }
                }

                break;
        }
    }

    private void InsertInlines(IList items, int startingIndex)
    {
        if (items is null)
        {
            return;
        }

        // InlineCollection 没有按索引插入的公开方法，走 IList 显式实现；
        // 未携带有效索引或索引越界时退化为尾部追加
        var index = startingIndex < 0 || startingIndex > Inlines.Count ? Inlines.Count : startingIndex;
        foreach (Inline inline in items)
        {
            ((IList)Inlines).Insert(index++, inline);
        }
    }

    private void RemoveInlines(IList items)
    {
        if (items is null)
        {
            return;
        }

        foreach (Inline inline in items)
        {
            Inlines.Remove(inline);
        }
    }
}
