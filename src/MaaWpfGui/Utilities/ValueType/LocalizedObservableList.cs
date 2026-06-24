// <copyright file="LocalizedObservableList.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// 支持语言热切换的本地化可观察列表。
/// 每个条目存储 Value 和对应的本地化 key，Display 在初始化和语言切换时自动从 key 获取。
/// 实现 IEnumerable + INotifyCollectionChanged + INotifyPropertyChanged 以支持 WPF ItemsSource 绑定，
/// 内部 Items 的增删和 item 属性变更会转发给绑定方。
/// </summary>
/// <typeparam name="TValue">条目的值类型</typeparam>
public class LocalizedObservableList<TValue> : IEnumerable<GenericCombinedData<TValue>>, INotifyCollectionChanged
{
    private readonly (TValue Value, string LocalizationKey, string? SecondaryKey)[] _entries;

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表。
    /// </summary>
    /// <param name="entries">(值, 本地化key) 数组</param>
    public LocalizedObservableList(params (TValue Value, string LocalizationKey)[] entries)
    {
        _entries = entries.Select(e => (e.Value, e.LocalizationKey, (string?)null)).ToArray();
        Items = new(entries.Select(e => new GenericCombinedData<TValue>
        {
            Display = FormatDisplay(e.LocalizationKey, null),
            Value = e.Value,
        }));

        // 转发内部集合变更，使 WPF ItemsSource 绑定能感知 Items.Add/Remove
        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表，支持部分条目有附加本地化 key（如 "{主key} ({附加key})" 格式）。
    /// </summary>
    /// <param name="entries">(值, 主本地化key, 附加本地化key或null) 数组</param>
    public LocalizedObservableList(params (TValue Value, string LocalizationKey, string? SecondaryKey)[] entries)
    {
        _entries = entries;
        Items = new(entries.Select(e => new GenericCombinedData<TValue>
        {
            Display = FormatDisplay(e.LocalizationKey, e.SecondaryKey),
            Value = e.Value,
        }));

        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    private static string FormatDisplay(string key, string? secondaryKey)
    {
        var primary = LocalizationHelper.TryGetString(key, out var value) ? value : key;
        return secondaryKey != null
            ? $"{primary} ({LocalizationHelper.GetString(secondaryKey)})"
            : primary;
    }

    /// <summary>
    /// Gets the observable collection of items.
    /// </summary>
    public ObservableCollection<GenericCombinedData<TValue>> Items { get; }

    /// <summary>
    /// 刷新所有条目的 Display 文本（语言切换时调用）。
    /// </summary>
    public void RefreshLocalization()
    {
        foreach (var (item, entry) in Items.Zip(_entries))
        {
            item.Display = FormatDisplay(entry.LocalizationKey, entry.SecondaryKey);
        }
    }

    public IEnumerator<GenericCombinedData<TValue>> GetEnumerator() => Items.GetEnumerator();

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

    /// <summary>
    /// 内部 Items 变更时转发事件，使绑定到本对象的 WPF ItemsControl 能感知增删。
    /// </summary>
    private void OnItemsCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        => CollectionChanged?.Invoke(this, e);

    /// <inheritdoc/>
    public event NotifyCollectionChangedEventHandler? CollectionChanged;
}
