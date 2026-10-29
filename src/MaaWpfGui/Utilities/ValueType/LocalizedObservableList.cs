// <copyright file="LocalizedObservableList.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// 支持语言热切换的本地化可观察列表。
/// <para>每个条目由 <see cref="LocalizedEntry{TValue}"/> 描述，
/// <see cref="GenericCombinedData{TValueType}.Display"/> 与 <see cref="GenericCombinedData{TValueType}.Tip"/>
/// 在初始化和 <see cref="RefreshLocalization"/> 时从 key 获取。条目直接写 (值, 本地化key) 二元组，
/// 需要附加 key、悬浮提示或初始不可选时在二元组后链式调用 <c>WithSecondaryKey</c>/<c>WithTip</c>/<c>WithEnabled</c>。</para>
/// <para>实现 <see cref="IEnumerable{T}"/> + <see cref="INotifyCollectionChanged"/> 以支持 WPF
/// <c>ItemsSource</c> 绑定。增删操作请使用本类型提供的 <see cref="Add(LocalizedEntry{TValue})"/>/<see cref="Remove"/> 等
/// 方法，它们会同步维护内部条目表，确保 <see cref="RefreshLocalization"/> 能正确刷新所有条目
/// （包括运行时动态添加的）。不要直接操作 <see cref="Items"/>，那样会导致条目表与 Items 脱节。</para>
/// <para>item 自身的属性变更（如 <see cref="GenericCombinedData{TValueType}.Display"/>）由 WPF
/// 数据绑定直接监听，本类型无需转发。</para>
/// </summary>
/// <typeparam name="TValue">条目的值类型</typeparam>
public class LocalizedObservableList<TValue> : IEnumerable<GenericCombinedData<TValue>>, INotifyCollectionChanged
{
    // 与 Items 一一对应的条目表；增删操作必须同步维护两者。
    private readonly List<LocalizedEntry<TValue>> _entries;

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedObservableList{TValue}"/> class.
    /// 初始化本地化列表。
    /// </summary>
    /// <param name="entries">条目数组，元素可直接写 (值, 本地化key) 二元组</param>
    public LocalizedObservableList(params LocalizedEntry<TValue>[] entries)
    {
        _entries = [.. entries];
        Items = new(entries.Select(CreateItem));

        // 转发内部集合变更，使 WPF ItemsSource 绑定能感知增删
        Items.CollectionChanged += OnItemsCollectionChanged;
    }

    private static string FormatDisplay(LocalizedEntry<TValue> entry)
    {
        var primary = LocalizationHelper.TryGetString(entry.LocalizationKey, out var value) ? value : entry.LocalizationKey;
        return entry.SecondaryKey != null && (entry.ShowSecondary?.Invoke() ?? true)
            ? $"{primary} ({LocalizationHelper.GetString(entry.SecondaryKey)})"
            : primary;
    }

    private static string? FormatTip(LocalizedEntry<TValue> entry)
        => entry.TipKey != null ? LocalizationHelper.GetString(entry.TipKey) : null;

    private static GenericCombinedData<TValue> CreateItem(LocalizedEntry<TValue> entry)
        => new()
        {
            Display = FormatDisplay(entry),
            Value = entry.Value,
            IsEnabled = entry.IsEnabled,
            Tip = FormatTip(entry),
        };

    /// <summary>
    /// Gets 内部可观察集合。仅供 WPF 绑定与只读访问，不要直接增删——
    /// 否则条目表不会同步更新，刷新时该项 Display 不会被更新。
    /// 增删请使用 <see cref="Add(LocalizedEntry{TValue})"/>/<see cref="Remove"/> 等方法。
    /// </summary>
    public ObservableCollection<GenericCombinedData<TValue>> Items { get; }

    /// <summary>
    /// 刷新所有条目的 Display 与 Tip 文本，并重新求值附加 key 的显示条件。
    /// 语言切换时，以及显示条件依赖的状态变化后调用。
    /// <see cref="_entries"/> 与 <see cref="Items"/> 始终一一对应（含运行时动态添加的项），
    /// 因此索引遍历能覆盖全部条目。
    /// </summary>
    public void RefreshLocalization()
    {
        for (int i = 0; i < Items.Count && i < _entries.Count; i++)
        {
            var entry = _entries[i];
            Items[i].Display = FormatDisplay(entry);
            Items[i].Tip = FormatTip(entry);
        }
    }

    /// <summary>
    /// 添加一个条目。同步扩展 <see cref="_entries"/>，确保刷新时该项也会被更新。
    /// </summary>
    /// <param name="entry">条目，可直接写 (值, 本地化key) 二元组</param>
    public void Add(LocalizedEntry<TValue> entry)
    {
        // 先创建 item：ShowSecondary 回调抛异常时两表都未改动，避免错位
        var item = CreateItem(entry);
        _entries.Add(entry);
        Items.Add(item);
    }

    /// <summary>
    /// 添加一个只带本地化 key 的条目。
    /// </summary>
    /// <param name="value">业务值</param>
    /// <param name="localizationKey">本地化 key</param>
    public void Add(TValue value, string localizationKey) => Add(new LocalizedEntry<TValue>(value, localizationKey));

    /// <summary>
    /// 移除第一个值匹配的条目（同步移除条目表中的对应项）。
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
    /// 移除指定索引处的条目（同步移除条目表中的对应项）。
    /// </summary>
    /// <param name="index">要移除的从零开始的索引</param>
    public void RemoveAt(int index)
    {
        _entries.RemoveAt(index);
        Items.RemoveAt(index);
    }

    /// <summary>
    /// 在指定索引处插入一个条目（同步扩展 <see cref="_entries"/>）。
    /// </summary>
    /// <param name="index">从零开始的插入位置</param>
    /// <param name="entry">条目，可直接写 (值, 本地化key) 二元组</param>
    public void Insert(int index, LocalizedEntry<TValue> entry)
    {
        var item = CreateItem(entry);
        _entries.Insert(index, entry);
        Items.Insert(index, item);
    }

    /// <summary>
    /// 在指定索引处插入一个只带本地化 key 的条目。
    /// </summary>
    /// <param name="index">从零开始的插入位置</param>
    /// <param name="value">业务值</param>
    /// <param name="localizationKey">本地化 key</param>
    public void Insert(int index, TValue value, string localizationKey) => Insert(index, new LocalizedEntry<TValue>(value, localizationKey));

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
