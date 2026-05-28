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
using System.Windows.Media;
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

    // 缓存高亮画刷
    private static Brush? _highlightBrush;

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
            PerformSearch(contentGrid, searchText, sectionFilter);
        }
    }

    private static void PerformSearch(Grid contentGrid, string searchText, IList sectionFilter)
    {
        ClearHighlights();

        var highlightBrush = GetHighlightBrush();

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

            // 先展开 Expander，让所有子控件变为可见，再遍历搜索
            if (!found)
            {
                ExpandForSearch(sectionGrid);
            }

            // 遍历当前可见的子控件，高亮匹配项
            var matchedBlocks = new List<TextBlock>();
            CollectVisibleMatches(sectionGrid, searchText, matchedBlocks);

            if (matchedBlocks.Count > 0)
            {
                found = true;
                foreach (var tb in matchedBlocks)
                {
                    tb.Background = highlightBrush;
                    _highlightedTextBlocks.Add(tb);
                }
            }

            setting.IsVisibleInSearch = found;

            // 命中时确保 Expander 展开
            if (found)
            {
                ExpandForSearch(sectionGrid);
            }
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
    /// 展开 section 中的 Expander 以便搜索。
    /// 如果 Expander 原本是收起的，记录下来以便搜索结束后还原。
    /// </summary>
    private static void ExpandForSearch(Grid sectionGrid)
    {
        foreach (var child in sectionGrid.Children)
        {
            if (child is Expander expander && !expander.IsExpanded)
            {
                _expandedBySearch.Add(expander);
                expander.IsExpanded = true;
                return;
            }
        }
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

    private static Brush GetHighlightBrush()
    {
        return _highlightBrush ??= (Brush)Application.Current.TryFindResource("SettingsSearchHighlightBrush")
            ?? new SolidColorBrush(Color.FromArgb(0x80, 0xFF, 0xD5, 0x4F));
    }

    private static void ClearHighlights()
    {
        foreach (var tb in _highlightedTextBlocks)
        {
            tb.ClearValue(TextBlock.BackgroundProperty);
        }

        _highlightedTextBlocks.Clear();
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
}
