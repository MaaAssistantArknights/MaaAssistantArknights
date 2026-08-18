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
using System.ComponentModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
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

    // 标记某个 ComboBox 是否已被 MakeComboBoxSearchable 处理过。
    // Loaded 事件会在元素每次进入可视化树时触发（如跨实例依赖引起视觉树重建），
    // 不加此标记会重复挂载事件处理器，且第二次进入时 ItemsSource 已被替换为 ICollectionView，
    // 再次赋给 CollectionViewSource.Source 会抛 ArgumentException。
    private static readonly DependencyPropertyKey IsSearchableInitializedPropertyKey =
        DependencyProperty.RegisterAttachedReadOnly(
            "IsSearchableInitialized",
            typeof(bool),
            typeof(ComboBoxExtensions),
            new PropertyMetadata(false));

    // 承载原始 ItemsSource 及其绑定关系的附加属性。
    // 初始化时把 ComboBox 的 ItemsSource 绑定整体迁移到此属性，由 WPF 绑定系统接管源属性通知：
    // 源集合被整体替换（如语言切换重建，PropertyChanged 通知）时，属性变更回调会重建
    // 独立 CollectionView 并回填 ItemsSource，避免本地赋值断开绑定导致的通知失效。
    private static readonly DependencyProperty OriginItemsSourceProperty =
        DependencyProperty.RegisterAttached(
            "OriginItemsSource",
            typeof(object),
            typeof(ComboBoxExtensions),
            new PropertyMetadata(null, OnOriginItemsSourceChanged));

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

        // 已处理过则直接返回，避免 Loaded 重复触发导致事件处理器重复挂载
        if ((bool)targetComboBox.GetValue(IsSearchableInitializedPropertyKey.DependencyProperty))
        {
            return;
        }

        targetComboBox.SetValue(IsSearchableInitializedPropertyKey, true);

        // 必须在下方迁移 ItemsSource 之前关闭 currency 同步：ItemsSource 换成独立 CollectionView 后，
        // Selector 的 IsSynchronizedWithCurrentItem=null 默认语义（ItemsSource 为 CollectionView 时同步）
        // 会把选中项强制对齐到 view 的当前项，而新建 view 的当前项固定在第一位，
        // TwoWay 的 Text/SelectedValue 绑定随之把 ｢列表第一项｣ 写回源属性（如开局干员、DropId）。
        targetComboBox.IsSynchronizedWithCurrentItem = false;

        // 为每个 ComboBox 创建独立的 CollectionView，避免多个控件共享默认视图导致搜索过滤互相干扰。
        // 注意不能直接把 ItemsSource 替换为独立视图：给依赖属性赋本地值会断开原有绑定，
        // 且 CollectionViewSource.View 无变更通知（只是快照），源集合被整体替换（PropertyChanged）时无法生效。
        // 因此把原始 ItemsSource 连同其绑定整体迁移到附加属性 OriginItemsSource 上，
        // 源属性更新时由 WPF 绑定系统触发属性变更回调，在回调中重建独立视图并回填 ItemsSource。
        if (targetComboBox.ItemsSource is not ICollectionView)
        {
            var bindingExpression = targetComboBox.GetBindingExpression(ItemsControl.ItemsSourceProperty);
            if (bindingExpression is { ParentBinding: { } parentBinding })
            {
                // 有绑定：整体迁移（保留相对绑定的 DataContext 语义），源属性替换时自动触发回调。
                // 不能先 ClearBinding 再迁移：清除绑定会让 ItemsSource 瞬时回落到 null，Selector 随即
                // 清空选中项，TwoWay 的 SelectedValue/SelectedItem 把空值写回绑定源（如各 Plan 的 DropId），
                // 表现为展开后指定材料被改成不选择。SetBinding 求值立即触发变更回调，回调中对
                // ItemsSource 赋本地值会自然移除其上的原绑定，全程不经过 null。
                BindingOperations.SetBinding(targetComboBox, OriginItemsSourceProperty, parentBinding);
            }
            else if (targetComboBox.ItemsSource is { } itemsSource)
            {
                // 无绑定：直接保存当前集合，SetValue 触发回调回填独立视图
                targetComboBox.SetValue(OriginItemsSourceProperty, itemsSource);
            }
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

                // 选中后清除搜索过滤，避免残留 filter 导致后续列表内容丢失
                targetComboBox.Items.Filter = null;
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

    private static void OnOriginItemsSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is ComboBox comboBox)
        {
            UpdateItemsSourceFromOrigin(comboBox);
        }
    }

    private static void UpdateItemsSourceFromOrigin(ComboBox comboBox)
    {
        var originSource = comboBox.GetValue(OriginItemsSourceProperty);
        comboBox.ItemsSource = originSource switch
        {
            null => null,
            ICollectionView view => view,
            _ => new CollectionViewSource { Source = originSource }.View,
        };

        comboBox.Items.IsLiveFiltering = true;
    }
}
