// <copyright file="SettingsView.xaml.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using MaaWpfGui.Helper;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Views.UI;

public partial class SettingsView
{
    private readonly HashSet<TextBlock> _highlightedTextBlocks = [];

    public SettingsView()
    {
        InitializeComponent();

        Instances.SettingsViewModel.RefreshDividerOffsetsRequested += (_, _) =>
        {
            ScrollViewerBinding.RefreshDividerOffsets(SettingsScrollViewer);
        };

        Instances.SettingsViewModel.SearchRequested += OnSearchRequested;

        // 初始化时将焦点移到左侧列表，避免搜索栏抢焦点
        Loaded += (_, _) => MasterListBox.Focus();
    }

    private static Brush GetHighlightBrush()
    {
        return (Brush)Application.Current.TryFindResource("SettingsSearchHighlightBrush")
            ?? new SolidColorBrush(Color.FromArgb(0x80, 0xFF, 0xD5, 0x4F));
    }

    private void OnSearchRequested(object? sender, string searchText)
    {
        if (string.IsNullOrEmpty(searchText))
        {
            PerformClearSearch();
        }
        else
        {
            PerformSearch(searchText);
        }
    }

    private void PerformSearch(string searchText)
    {
        RestoreAllHighlights();

        var highlightBrush = GetHighlightBrush();
        var vm = Instances.SettingsViewModel;
        var contentGrid = SettingsScrollViewer.Content as Grid;
        if (contentGrid == null)
        {
            return;
        }

        foreach (var setting in vm.Settings)
        {
            bool found = setting.Display.Contains(searchText, StringComparison.OrdinalIgnoreCase);

            var sectionGrid = FindSectionGridByIndex(contentGrid, setting.Value);
            if (sectionGrid != null)
            {
                foreach (var tb in FindVisualChildren<TextBlock>(sectionGrid))
                {
                    var text = tb.Text ?? string.Empty;
                    if (text.Contains(searchText, StringComparison.OrdinalIgnoreCase))
                    {
                        found = true;
                        tb.Background = highlightBrush;
                        _highlightedTextBlocks.Add(tb);
                    }
                }

                foreach (var cc in FindVisualChildren<ContentControl>(sectionGrid))
                {
                    if (cc.Content is string content
                        && content.Contains(searchText, StringComparison.OrdinalIgnoreCase))
                    {
                        found = true;
                    }
                }

                foreach (var fe in FindVisualChildren<FrameworkElement>(sectionGrid))
                {
                    var title = HandyControl.Controls.TitleElement.GetTitle(fe);
                    if (!string.IsNullOrEmpty(title)
                        && title.Contains(searchText, StringComparison.OrdinalIgnoreCase))
                    {
                        found = true;
                    }
                }
            }

            setting.IsVisibleInSearch = found;
        }
    }

    private void PerformClearSearch()
    {
        RestoreAllHighlights();

        var vm = Instances.SettingsViewModel;
        foreach (var setting in vm.Settings)
        {
            setting.IsVisibleInSearch = true;
        }
    }

    private void RestoreAllHighlights()
    {
        foreach (var tb in _highlightedTextBlocks)
        {
            tb.ClearValue(TextBlock.BackgroundProperty);
        }

        _highlightedTextBlocks.Clear();
    }

    private static Grid? FindSectionGridByIndex(Grid contentGrid, int index)
    {
        foreach (UIElement child in contentGrid.Children)
        {
            if (child is Grid grid && Grid.GetRow(grid) == index)
            {
                return grid;
            }
        }

        return null;
    }

    private static IEnumerable<T> FindVisualChildren<T>(DependencyObject parent)
        where T : DependencyObject
    {
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(parent); i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is T t)
            {
                yield return t;
            }

            foreach (var descendant in FindVisualChildren<T>(child))
            {
                yield return descendant;
            }
        }
    }
}
