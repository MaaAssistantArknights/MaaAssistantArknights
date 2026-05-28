// <copyright file="SearchHighlightBehavior.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Animation;
using MaaWpfGui.ViewModels.Items;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 搜索高亮附加行为。
/// 挂在 ScrollViewer 上，通过 SearchText 驱动关键词高亮；
/// 通过 SectionFilter 接收集合，设置其中每个 <see cref="SettingItemViewModel.IsVisibleInSearch"/> 过滤分区。
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
            new PropertyMetadata(string.Empty, OnSearchTextChanged));

    public static string GetSearchText(DependencyObject obj) => (string)obj.GetValue(SearchTextProperty);

    public static void SetSearchText(DependencyObject obj, string value) => obj.SetValue(SearchTextProperty, value);

    /// <summary>
    /// 可过滤的 SettingItemViewModel 集合，绑定到 ViewModel 的 Settings。
    /// 搜索时会设置每个条目的 IsVisibleInSearch 属性。
    /// </summary>
    public static readonly DependencyProperty SectionFilterProperty =
        DependencyProperty.RegisterAttached(
            "SectionFilter",
            typeof(IList),
            typeof(SearchHighlightBehavior),
            new PropertyMetadata(null));

    public static IList GetSectionFilter(DependencyObject obj) => (IList)obj.GetValue(SectionFilterProperty);

    public static void SetSectionFilter(DependencyObject obj, IList value) => obj.SetValue(SectionFilterProperty, value);

    #endregion

    // 当前高亮过的 TextBlock，用于统一清除
    private static readonly List<TextBlock> _highlightedTextBlocks = [];

    // 搜索时被强行展开的 Expander（原本是收起的），清空搜索时还原
    private static readonly List<Expander> _expandedBySearch = [];

    // 当前附加的 Adorner 列表
    private static readonly List<RainbowBorderAdorner> _adorners = [];

    private static void OnSearchTextChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not ScrollViewer scrollViewer)
        {
            return;
        }

        var contentGrid = scrollViewer.Content as Grid;
        if (contentGrid == null)
        {
            return;
        }

        var searchText = (string)e.NewValue;
        var sectionFilter = GetSectionFilter(scrollViewer);

        if (string.IsNullOrEmpty(searchText))
        {
            ClearSearch(sectionFilter);
        }
        else
        {
            // 先展开所有 Expander，等布局更新后再搜索
            ExpandAllSections(contentGrid);
            contentGrid.Dispatcher.BeginInvoke(
                () => PerformSearch(contentGrid, searchText, sectionFilter),
                System.Windows.Threading.DispatcherPriority.Loaded);
        }
    }

    private static void ExpandAllSections(Grid contentGrid)
    {
        foreach (var child in contentGrid.Children)
        {
            if (child is Grid sectionGrid)
            {
                foreach (var sectionChild in sectionGrid.Children)
                {
                    if (sectionChild is Expander expander && !expander.IsExpanded)
                    {
                        _expandedBySearch.Add(expander);
                        expander.IsExpanded = true;
                    }
                }
            }
        }
    }

    private static void PerformSearch(Grid contentGrid, string searchText, IList sectionFilter)
    {
        ClearHighlights();

        foreach (var child in contentGrid.Children)
        {
            if (child is not Grid sectionGrid)
            {
                continue;
            }

            var rowIndex = Grid.GetRow(sectionGrid);
            var setting = FindSettingByRowIndex(sectionFilter, rowIndex);
            if (setting == null)
            {
                continue;
            }

            // 在分区 Display 名称中搜索
            bool found = setting.Display.Contains(searchText, StringComparison.OrdinalIgnoreCase);

            // 遍历当前可见的子控件，高亮匹配项
            var matchedBlocks = new List<TextBlock>();
            CollectVisibleMatches(sectionGrid, searchText, matchedBlocks);

            if (matchedBlocks.Count > 0)
            {
                found = true;
                foreach (var tb in matchedBlocks)
                {
                    tb.FontWeight = FontWeights.Bold;
                    AddRainbowAdorner(tb);
                    _highlightedTextBlocks.Add(tb);
                }
            }

            setting.IsVisibleInSearch = found;
        }
    }

    private static void ClearSearch(IList sectionFilter)
    {
        ClearHighlights();
        RestoreExpandedSections();

        foreach (var item in sectionFilter)
        {
            if (item is SettingItemViewModel setting)
            {
                setting.IsVisibleInSearch = true;
            }
        }
    }

    private static SettingItemViewModel? FindSettingByRowIndex(IList sectionFilter, int rowIndex)
    {
        foreach (var item in sectionFilter)
        {
            if (item is SettingItemViewModel setting && setting.Value == rowIndex)
            {
                return setting;
            }
        }

        return null;
    }

    /// <summary>
    /// 还原搜索时被强行展开的 Expander。
    /// </summary>
    private static void RestoreExpandedSections()
    {
        foreach (var expander in _expandedBySearch)
        {
            expander.IsExpanded = false;
        }

        _expandedBySearch.Clear();
    }

    private static void ClearHighlights()
    {
        // 移除所有 Adorner
        foreach (var adorner in _adorners)
        {
            adorner.Remove();
        }

        _adorners.Clear();

        foreach (var tb in _highlightedTextBlocks)
        {
            tb.ClearValue(TextBlock.FontWeightProperty);
        }

        _highlightedTextBlocks.Clear();
    }

    /// <summary>
    /// 为 TextBlock 添加彩虹流光边框 Adorner。
    /// </summary>
    private static void AddRainbowAdorner(TextBlock tb)
    {
        var layer = AdornerLayer.GetAdornerLayer(tb);
        if (layer == null)
        {
            return;
        }

        // 每个 Adorner 创建独立的画刷和动画
        var brush = CreateRainbowBrush();

        var adorner = new RainbowBorderAdorner(tb, brush);
        layer.Add(adorner);
        _adorners.Add(adorner);
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
            var layer = AdornerLayer.GetAdornerLayer(AdornedElement);
            layer?.Remove(this);
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
