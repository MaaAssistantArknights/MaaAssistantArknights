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
/// 首次搜索时预建可搜索文本索引，后续搜索复用缓存，避免重复遍历视觉树。
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

    #region 可搜索文本索引

    /// <summary>
    /// 索引条目：文本内容 + 对应 TextBlock 引用（用于高亮）。
    /// ContentControl / TitleElement 的文本无对应 TextBlock，TextBlock 引用为 null。
    /// </summary>
    private sealed class TextEntry(string text, TextBlock? textBlock)
    {
        public string Text { get; } = text;

        public TextBlock? TextBlock { get; } = textBlock;
    }

    // section Grid → 可搜索文本索引，首次搜索时建立，Unloaded 时清除
    private static readonly Dictionary<Grid, List<TextEntry>> _searchIndex = [];

    // 当前高亮过的 TextBlock，用于统一清除
    private static readonly List<TextBlock> _highlightedTextBlocks = [];

    // 缓存高亮画刷
    private static Brush? _highlightBrush;

    #endregion

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

            // 在分区 Display 名称或预建索引中搜索
            bool found = setting.Display.Contains(searchText, StringComparison.OrdinalIgnoreCase);

            // 始终遍历索引以高亮匹配的 TextBlock（无论 Display 是否已匹配）
            var entries = GetOrCreateSearchIndex(sectionGrid);
            foreach (var entry in entries)
            {
                if (entry.Text.Contains(searchText, StringComparison.OrdinalIgnoreCase))
                {
                    found = true;

                    if (entry.TextBlock != null)
                    {
                        entry.TextBlock.Background = highlightBrush;
                        _highlightedTextBlocks.Add(entry.TextBlock);
                    }
                }
            }

            setting.IsVisibleInSearch = found;
        }
    }

    private static void ClearSearch(IList sectionFilter)
    {
        ClearHighlights();

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

    #region 索引管理

    /// <summary>
    /// 获取或建立某个 section Grid 的可搜索文本索引。
    /// 首次调用时遍历视觉树一次，缓存结果；后续直接复用。
    /// </summary>
    private static List<TextEntry> GetOrCreateSearchIndex(Grid sectionGrid)
    {
        if (_searchIndex.TryGetValue(sectionGrid, out var cached))
        {
            return cached;
        }

        sectionGrid.Unloaded += OnSectionGridUnloaded;

        var entries = new List<TextEntry>();
        CollectSearchableEntries(sectionGrid, entries);
        _searchIndex[sectionGrid] = entries;
        return entries;
    }

    private static void OnSectionGridUnloaded(object sender, RoutedEventArgs e)
    {
        if (sender is Grid grid)
        {
            grid.Unloaded -= OnSectionGridUnloaded;
            _searchIndex.Remove(grid);
        }
    }

    /// <summary>
    /// 递归收集视觉树中所有可搜索文本：TextBlock.Text、ContentControl string Content、TitleElement Title。
    /// </summary>
    private static void CollectSearchableEntries(DependencyObject parent, List<TextEntry> entries)
    {
        var childCount = VisualTreeHelper.GetChildrenCount(parent);
        for (int i = 0; i < childCount; i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);

            if (child is TextBlock tb && !string.IsNullOrEmpty(tb.Text))
            {
                entries.Add(new TextEntry(tb.Text, tb));
            }
            else if (child is ContentControl cc && cc.Content is string content && !string.IsNullOrEmpty(content))
            {
                entries.Add(new TextEntry(content, null));
            }

            if (child is FrameworkElement fe)
            {
                var title = HandyControl.Controls.TitleElement.GetTitle(fe);
                if (!string.IsNullOrEmpty(title))
                {
                    entries.Add(new TextEntry(title, null));
                }
            }

            CollectSearchableEntries(child, entries);
        }
    }

    #endregion
}
