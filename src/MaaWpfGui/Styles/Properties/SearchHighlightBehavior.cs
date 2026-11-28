// <copyright file="SearchHighlightBehavior.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 搜索高亮附加行为。
/// 挂在 ScrollViewer 上，通过 SearchText 驱动关键词高亮；
/// 通过 SectionFilter 接收集合，设置其中每个 <see cref="ISearchableSettingSection.IsVisibleInSearch"/> 过滤分区。
/// 每次搜索实时遍历视觉树，只搜索当前可见的控件，确保不会搜索到被条件隐藏的子区域。
/// </summary>
public static class SearchHighlightBehavior
{
    #region 附加属性

    /// <summary>
    /// 搜索关键词，绑定到 ViewModel 的 SearchText。
    /// </summary>
    public static readonly DependencyProperty SearchTextProperty =
        DependencyProperty.RegisterAttached(
            "SearchText",
            typeof(string),
            typeof(SearchHighlightBehavior),
            new PropertyMetadata(string.Empty, OnSearchInputChanged));

    public static string GetSearchText(DependencyObject obj) => (string)obj.GetValue(SearchTextProperty);

    public static void SetSearchText(DependencyObject obj, string value) => obj.SetValue(SearchTextProperty, value);

    /// <summary>
    /// 可过滤的分区集合，绑定到 ViewModel 的分区列表。
    /// 搜索时会设置每个 <see cref="ISearchableSettingSection"/> 条目的 IsVisibleInSearch 属性。
    /// </summary>
    public static readonly DependencyProperty SectionFilterProperty =
        DependencyProperty.RegisterAttached(
            "SectionFilter",
            typeof(IList),
            typeof(SearchHighlightBehavior),
            new PropertyMetadata(null, OnSearchInputChanged));

    public static IList? GetSectionFilter(DependencyObject obj) => (IList?)obj.GetValue(SectionFilterProperty);

    public static void SetSectionFilter(DependencyObject obj, IList? value) => obj.SetValue(SectionFilterProperty, value);

    #endregion

    private static readonly ConditionalWeakTable<ScrollViewer, SearchState> _states = new();

    internal static void Refresh(ScrollViewer viewer) => GetState(viewer).QueueSearch(resetExpansion: true);

    private static SearchState GetState(ScrollViewer viewer) => _states.GetValue(viewer, static key => new SearchState(key));

    private static void OnSearchInputChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is ScrollViewer viewer)
        {
            if (e.Property == SectionFilterProperty && e.OldValue is IList previous)
            {
                ShowAllSections(previous);
            }

            GetState(viewer).QueueSearch(resetExpansion: e.Property == SectionFilterProperty);
        }
    }

    private static void ShowAllSections(IList filter)
    {
        foreach (var section in filter.OfType<ISearchableSettingSection>())
        {
            section.IsVisibleInSearch = true;
        }
    }

    private sealed class SearchState
    {
        private readonly ScrollViewer _viewer;
        private readonly HashSet<Expander> _expandedBySearch = [];
        private readonly List<(TextBlock Text, RainbowBorderAdorner? Adorner)> _highlights = [];
        private DispatcherOperation? _pendingSearch;

        public SearchState(ScrollViewer viewer)
        {
            _viewer = viewer;
            viewer.Loaded += (_, _) => QueueSearch();
            viewer.Unloaded += (_, _) => {
                _pendingSearch?.Abort();
                _pendingSearch = null;
                ClearHighlights();
                RestoreExpansion();
            };
        }

        public void QueueSearch(bool resetExpansion = false)
        {
            if (resetExpansion)
            {
                RestoreExpansion();
            }

            if (_viewer.IsLoaded)
            {
                _pendingSearch ??= _viewer.Dispatcher.InvokeAsync(Search, DispatcherPriority.Loaded);
            }
        }

        private void Search()
        {
            _pendingSearch = null;
            ClearHighlights();
            if (GetSectionFilter(_viewer) is not { } filter)
            {
                return;
            }

            var searchText = GetSearchText(_viewer);
            ShowAllSections(filter);
            if (string.IsNullOrEmpty(searchText) || _viewer.Content is not Grid content)
            {
                RestoreExpansion();
                return;
            }

            foreach (var section in content.Children.OfType<Grid>())
            {
                foreach (var expander in section.Children.OfType<Expander>().Where(item => !item.IsExpanded))
                {
                    _expandedBySearch.Add(expander);
                    expander.SetCurrentValue(Expander.IsExpandedProperty, true);
                }
            }

            // 先生成刚展开的分区内容，再统一写回结果，让导航按最终布局同步滚动位置。
            content.UpdateLayout();
            foreach (var grid in content.Children.OfType<Grid>())
            {
                var row = Grid.GetRow(grid);
                var section = filter.OfType<ISearchableSettingSection>().FirstOrDefault(item => item.GridRow == row);
                if (section == null)
                {
                    continue;
                }

                var matches = new List<TextBlock>();
                CollectVisibleMatches(grid, searchText, matches);
                foreach (var text in matches)
                {
                    text.SetCurrentValue(TextBlock.FontWeightProperty, FontWeights.Bold);
                    RainbowBorderAdorner? adorner = null;
                    if (AdornerLayer.GetAdornerLayer(text) is { } layer)
                    {
                        adorner = new RainbowBorderAdorner(text, CreateRainbowBrush());
                        layer.Add(adorner);
                    }

                    _highlights.Add((text, adorner));
                }

                section.IsVisibleInSearch = matches.Count > 0 || section.Display.Contains(searchText, StringComparison.OrdinalIgnoreCase);
            }
        }

        private void ClearHighlights()
        {
            foreach (var (text, adorner) in _highlights)
            {
                adorner?.Remove();
                text.InvalidateProperty(TextBlock.FontWeightProperty);
            }

            _highlights.Clear();
        }

        private void RestoreExpansion()
        {
            foreach (var expander in _expandedBySearch)
            {
                expander.SetCurrentValue(Expander.IsExpandedProperty, false);
            }

            _expandedBySearch.Clear();
        }
    }

    /// <summary>
    /// 创建带流光动画的独立彩虹画刷实例。
    /// </summary>
    private static LinearGradientBrush CreateRainbowBrush()
    {
        // 尝试从资源获取基础渐变色，复制一份
        var translate = new TranslateTransform();
        var brush = new LinearGradientBrush
        {
            StartPoint = new Point(0, 0),
            EndPoint = new Point(1, 0),
            SpreadMethod = GradientSpreadMethod.Repeat,
            Transform = translate,
        };

        if (Application.Current.TryFindResource("RainbowFlowBrush") is LinearGradientBrush source)
        {
            foreach (var stop in source.GradientStops)
            {
                brush.GradientStops.Add(new GradientStop(stop.Color, stop.Offset));
            }
        }
        else
        {
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0xFF, 0x00, 0x00), 0.000));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0xFF, 0xA5, 0x00), 0.143));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0xFF, 0xD7, 0x00), 0.286));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0x00, 0xC8, 0x00), 0.429));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0x1E, 0x90, 0xFF), 0.571));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0x8A, 0x2B, 0xE2), 0.714));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0xFF, 0x00, 0xFF), 0.857));
            brush.GradientStops.Add(new GradientStop(Color.FromRgb(0xFF, 0x00, 0x00), 1.000));
        }

        // 每个实例独立的动画，随机起始偏移避免完全同步
        var anim = new DoubleAnimation
        {
            From = 0,
            To = 4000,
            Duration = new Duration(TimeSpan.FromSeconds(20)),
            RepeatBehavior = RepeatBehavior.Forever,
        };

        translate.BeginAnimation(TranslateTransform.XProperty, anim);

        return brush;
    }

    #region 实时视觉树遍历

    /// <summary>
    /// 实时遍历 sectionGrid 中当前可见的子控件，收集匹配搜索词的 TextBlock。
    /// 跳过 Visibility 为 Collapsed/Hidden 的控件及其子树，确保不搜索到被条件隐藏的区域。
    /// </summary>
    private static void CollectVisibleMatches(DependencyObject parent, string searchText, List<TextBlock> matchedBlocks)
    {
        var childCount = VisualTreeHelper.GetChildrenCount(parent);
        for (int i = 0; i < childCount; i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);

            // 跳过不可见的控件及其整棵子树
            if (child is FrameworkElement { Visibility: not Visibility.Visible })
            {
                continue;
            }

            if (child is TextBlock tb && !string.IsNullOrEmpty(tb.Text)
                && tb.Text.Contains(searchText, StringComparison.OrdinalIgnoreCase))
            {
                matchedBlocks.Add(tb);
            }

            // 递归进入子树
            CollectVisibleMatches(child, searchText, matchedBlocks);
        }
    }

    #endregion

    #region 彩虹边框 Adorner

    /// <summary>
    /// 彩虹流光边框 Adorner，在目标控件外围叠加一个带圆角的 Border。
    /// 自动同步目标控件的 Visibility，控件隐藏时 Adorner 也隐藏。
    /// </summary>
    private sealed class RainbowBorderAdorner : Adorner
    {
        private readonly Border _border;

        public RainbowBorderAdorner(UIElement adornedElement, Brush borderBrush)
            : base(adornedElement)
        {
            IsHitTestVisible = false;

            _border = new Border
            {
                BorderBrush = borderBrush,
                BorderThickness = new Thickness(2),
                CornerRadius = new CornerRadius(4),
                Background = Brushes.Transparent,
            };

            AddVisualChild(_border);

            // 监听目标控件的 Visibility 变化
            adornedElement.IsVisibleChanged += OnAdornedElementVisibilityChanged;
        }

        protected override int VisualChildrenCount => 1;

        protected override Visual GetVisualChild(int index) => _border;

        public void Remove()
        {
            AdornedElement.IsVisibleChanged -= OnAdornedElementVisibilityChanged;
            (VisualTreeHelper.GetParent(this) as AdornerLayer)?.Remove(this);
        }

        private void OnAdornedElementVisibilityChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            Visibility = (bool)e.NewValue ? Visibility.Visible : Visibility.Collapsed;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            const int Padding = 4;
            _border.Arrange(new Rect(
                new Point(-Padding, -Padding),
                new Size(finalSize.Width + (2 * Padding), finalSize.Height + (2 * Padding))));
            return finalSize;
        }
    }

    #endregion
}
