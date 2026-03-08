// <copyright file="ComboBoxExtensions.cs" company="MaaAssistantArknights">
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

using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Serilog;

namespace MaaWpfGui.Extensions;

/// <summary>
/// <seealso cref="ComboBox"/> Extensions
/// </summary>
public static class ComboBoxExtensions
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "ComboBoxExtensions");
    private const string InputTag = "TextInput";
    private const string SelectionTag = "Selection";

    /// <summary>
    /// Make <seealso cref="ComboBox"/> searchable
    /// </summary>
    /// <param name="targetComboBox">Target <seealso cref="ComboBox"/></param>
    public static void MakeComboBoxSearchable(this ComboBox targetComboBox)
    {
        if (targetComboBox?.Template.FindName("PART_EditableTextBox", targetComboBox) is not TextBox targetTextBox)
        {
            return;
        }

        targetComboBox.Items.IsLiveFiltering = true;
        targetComboBox.StaysOpenOnEdit = true;
        targetComboBox.IsEditable = true;
        targetComboBox.IsTextSearchEnabled = false;

        targetComboBox.Tag = SelectionTag;

        targetTextBox.PreviewKeyDown += (_, ev) =>
        {
            if (ev.Key is Key.Enter or Key.Return or Key.Tab)
            {
                return;
            }

            if (targetComboBox.Tag is SelectionTag)
            {
                var text = targetComboBox.SelectedItem?.ToString() ?? string.Empty;
                _logger.Debug("Switching to input mode with text: {Text}", text);
                targetComboBox.SelectedItem = null;
                targetTextBox.Text = text;
                targetTextBox.Select(text.Length, 0);
            }

            // switch to input mode
            targetComboBox.Tag = InputTag;
            targetComboBox.IsDropDownOpen = true;
        };

        targetTextBox.TextChanged += (_, _) =>
        {
            if (targetComboBox.Tag is SelectionTag)
            {
                return;
            }

            var searchTerm = targetTextBox.Text;
            _logger.Debug("Searching for: {SearchTerm}", searchTerm);

            // 如果文字完全匹配某个选项，恢复完整列表
            object exactMatchItem = targetComboBox.ItemsSource.Cast<object>().FirstOrDefault(obj => string.Equals(obj?.ToString(), searchTerm, StringComparison.CurrentCultureIgnoreCase));

            if (exactMatchItem != null)
            {
                targetComboBox.Items.Filter = null;
                targetComboBox.SelectedItem = exactMatchItem;
                targetComboBox.Dispatcher.BeginInvoke(new Action(() =>
                {
                    targetComboBox.UpdateLayout();
                    if (targetComboBox.ItemContainerGenerator.ContainerFromItem(exactMatchItem) is FrameworkElement element)
                    {
                        element.BringIntoView();
                    }
                }), System.Windows.Threading.DispatcherPriority.Background);
            }
            else
            {
                targetComboBox.Items.Filter = item => item?.ToString()?.Contains(searchTerm, StringComparison.CurrentCultureIgnoreCase) ?? false;
            }
        };

        targetComboBox.SelectionChanged += (_, _) =>
        {
            if (targetComboBox.SelectedItem != null)
            {
                targetComboBox.Tag = SelectionTag;
            }

            targetComboBox.Dispatcher.BeginInvoke(new Action(() =>
            {
                targetTextBox.Select(targetTextBox.Text.Length, 0);
            }), System.Windows.Threading.DispatcherPriority.Background);
        };

        targetComboBox.DropDownOpened += (_, _) =>
        {
            targetComboBox.Items.Filter = null;
            targetComboBox.Dispatcher.BeginInvoke(new Action(() =>
            {
                targetTextBox.Select(targetTextBox.Text.Length, 0);
            }), System.Windows.Threading.DispatcherPriority.Background);
        };
    }
}
