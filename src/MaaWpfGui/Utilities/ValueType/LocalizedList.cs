// <copyright file="LocalizedList.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// 支持语言热切换的本地化列表。
/// 每个条目存储 Value 和对应的本地化 key，Display 在初始化和语言切换时自动从 key 获取。
/// 实现 IEnumerable 以支持 WPF ItemsSource 绑定。
/// </summary>
/// <typeparam name="TValue">条目的值类型</typeparam>
public class LocalizedList<TValue> : IEnumerable<GenericCombinedData<TValue>>
{
    private readonly (TValue Value, string LocalizationKey)[] _entries;

    /// <summary>
    /// Initializes a new instance of the <see cref="LocalizedList{TValue}"/> class.
    /// 初始化本地化列表。
    /// </summary>
    /// <param name="entries">(值, 本地化key) 数组</param>
    public LocalizedList(params (TValue Value, string LocalizationKey)[] entries)
    {
        _entries = entries;
        Items = new(entries.Select(e => new GenericCombinedData<TValue>
        {
            Display = LocalizationHelper.GetString(e.LocalizationKey),
            Value = e.Value,
        }));
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
            item.Display = LocalizationHelper.GetString(entry.LocalizationKey);
        }
    }

    public IEnumerator<GenericCombinedData<TValue>> GetEnumerator() => Items.GetEnumerator();

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
}
