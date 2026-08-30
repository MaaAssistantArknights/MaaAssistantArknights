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
/// <para>每个条目存储 <see cref="GenericCombinedData{TValueType}.Value"/> 和对应的本地化 key，
/// <see cref="GenericCombinedData{TValueType}.Display"/> 在初始化和语言切换时自动从 key 获取。</para>
/// <para>实现 <see cref="IEnumerable{T}"/> + <see cref="INotifyCollectionChanged"/> 以支持 WPF
/// <c>ItemsSource</c> 绑定。增删操作请使用本类型提供的 <see cref="Add"/>/<see cref="Remove"/> 等
/// 方法，它们会同步维护内部本地化 key 表，确保 <see cref="RefreshLocalization"/> 能正确刷新所有条目
/// （包括运行时动态添加的）。不要直接操作 <see cref="Items"/>，那样会导致 key 表与条目脱节。</para>
/// <para>item 自身的属性变更（如 <see cref="GenericCombinedData{TValueType}.Display"/>）由 WPF
/// 数据绑定直接监听，本类型无需转发。</para>
/// </summary>
/// <typeparam name="TValue">条目的值类型</typeparam>
public class LocalizedObservableList<TValue> : IEnumerable<GenericCombinedData<TValue>>, INotifyCollectionChanged
{
    // 与 Items 一一对应的本地化 key 表；增删操作必须同步维护两者。
    private readonly List<(TValue Value, string LocalizationKey, string? SecondaryKey)> _entries;

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表。
    /// </summary>
    /// <param name="entries">(值, 本地化key) 数组</param>
    public LocalizedObservableList(params (TValue Value, string LocalizationKey)[] entries)
    {
        _entries = entries.Select(e => (e.Value, e.LocalizationKey, (string?)null)).ToList();
        Items = new(entries.Select(e => CreateItem(e.Value, e.LocalizationKey, null)));

        // 转发内部集合变更，使 WPF ItemsSource 绑定能感知增删
        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表，支持部分条目有附加本地化 key（如 "{主key} ({附加key})" 格式）。
    /// </summary>
    /// <param name="entries">(值, 主本地化key, 附加本地化key或null) 数组</param>
    public LocalizedObservableList(params (TValue Value, string LocalizationKey, string? SecondaryKey)[] entries)
    {
        _entries = [.. entries];
        Items = new(entries.Select(e => CreateItem(e.Value, e.LocalizationKey, e.SecondaryKey)));

        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表，支持部分条目初始为不可选（配合 <see cref="GenericCombinedData{TValue}.IsEnabled"/>）。
    /// </summary>
    /// <param name="entries">(值, 本地化key, 是否可选) 数组</param>
    public LocalizedObservableList(params (TValue Value, string LocalizationKey, bool IsEnabled)[] entries)
    {
        _entries = entries.Select(e => (e.Value, e.LocalizationKey, (string?)null)).ToList();
        Items = new(entries.Select(e => CreateItem(e.Value, e.LocalizationKey, null, e.IsEnabled)));

        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    private static string FormatDisplay(string key, string? secondaryKey)
    {
        var primary = LocalizationHelper.TryGetString(key, out var value) ? value : key;
        return secondaryKey != null
            ? $"{primary} ({LocalizationHelper.GetString(secondaryKey)})"
            : primary;
    }

    private GenericCombinedData<TValue> CreateItem(TValue value, string localizationKey, string? secondaryKey, bool isEnabled = true)
        => new()
        {
            Display = FormatDisplay(localizationKey, secondaryKey),
            Value = value,
            IsEnabled = isEnabled,
        };

    /// <summary>
    /// Gets 内部可观察集合。仅供 WPF 绑定与只读访问，不要直接增删——
    /// 否则本地化 key 表不会同步更新，语言切换后该项 Display 不会被刷新。
    /// 增删请使用 <see cref="Add"/>/<see cref="Remove"/> 等方法。
    /// </summary>
    public ObservableCollection<GenericCombinedData<TValue>> Items { get; }

    /// <summary>
    /// 刷新所有条目的 Display 文本（语言切换时调用）。
    /// <see cref="_entries"/> 与 <see cref="Items"/> 始终一一对应（含运行时动态添加的项），
    /// 因此索引遍历能覆盖全部条目。
    /// </summary>
    public void RefreshLocalization()
    {
        for (int i = 0; i < Items.Count && i < _entries.Count; i++)
        {
            var entry = _entries[i];
            Items[i].Display = FormatDisplay(entry.LocalizationKey, entry.SecondaryKey);
        }
    }

    /// <summary>
    /// 添加一个带本地化 key 的条目。同步扩展 <see cref="_entries"/>，确保语言切换时该项也会被刷新。
    /// </summary>
    /// <param name="value">业务值</param>
    /// <param name="localizationKey">本地化 key</param>
    /// <param name="secondaryKey">附加本地化 key（可选），非空时 Display 显示为 "{主key} ({附加key})"</param>
    public void Add(TValue value, string localizationKey, string? secondaryKey = null)
    {
        _entries.Add((value, localizationKey, secondaryKey));
        Items.Add(CreateItem(value, localizationKey, secondaryKey));
    }

    /// <summary>
    /// 移除第一个值匹配的条目（同步移除对应的本地化 key）。
    /// </summary>
    /// <param name="value">要移除的业务值</param>
    /// <returns>如果成功移除则返回 true，否则返回 false</returns>
    public bool Remove(TValue value)
    {
        int index = _entries.FindIndex(e => EqualityComparer<TValue>.Default.Equals(e.Value, value));
        if (index < 0)
        {
            return false;
        }

        RemoveAt(index);
        return true;
    }

    /// <summary>
    /// 移除指定索引处的条目（同步移除对应的本地化 key）。
    /// </summary>
    /// <param name="index">要移除的从零开始的索引</param>
    public void RemoveAt(int index)
    {
        _entries.RemoveAt(index);
        Items.RemoveAt(index);
    }

    /// <summary>
    /// 在指定索引处插入一个带本地化 key 的条目（同步扩展 <see cref="_entries"/>）。
    /// </summary>
    /// <param name="index">从零开始的插入位置</param>
    /// <param name="value">业务值</param>
    /// <param name="localizationKey">本地化 key</param>
    /// <param name="secondaryKey">附加本地化 key（可选）</param>
    public void Insert(int index, TValue value, string localizationKey, string? secondaryKey = null)
    {
        _entries.Insert(index, (value, localizationKey, secondaryKey));
        Items.Insert(index, CreateItem(value, localizationKey, secondaryKey));
    }

    /// <summary>
    /// 移除所有条目（同步清空 <see cref="_entries"/>）。
    /// </summary>
    public void Clear()
    {
        _entries.Clear();
        Items.Clear();
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
