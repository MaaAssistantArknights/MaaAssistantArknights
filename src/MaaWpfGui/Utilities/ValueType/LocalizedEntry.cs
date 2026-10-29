// <copyright file="LocalizedEntry.cs" company="MaaAssistantArknights">
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
using System;

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// <see cref="LocalizedObservableList{TValue}"/> 的条目定义。
/// <para>(值, 本地化key) 二元组可隐式转换为本类型；附加 key、悬浮提示、可选状态通过
/// <see cref="WithSecondaryKey"/>/<see cref="WithTip"/>/<see cref="WithEnabled"/> 链式补充，
/// 二元组上也可直接调用同名扩展方法（见 <see cref="LocalizedEntryExtensions"/>）。</para>
/// </summary>
/// <typeparam name="TValue">条目的值类型</typeparam>
/// <param name="Value">业务值</param>
/// <param name="LocalizationKey">本地化 key</param>
public readonly record struct LocalizedEntry<TValue>(TValue Value, string LocalizationKey)
{
    /// <summary>
    /// Gets 附加本地化 key，显示时 Display 为 "{主key} ({附加key})"。
    /// </summary>
    public string? SecondaryKey { get; init; }

    /// <summary>
    /// Gets 附加 key 的显示条件。为 null 时恒显示；否则每次刷新文本时重新求值，返回 true 才显示。
    /// </summary>
    public Func<bool>? ShowSecondary { get; init; }

    /// <summary>
    /// Gets 悬浮提示本地化 key，填入 <see cref="GenericCombinedData{TValueType}.Tip"/>。
    /// </summary>
    public string? TipKey { get; init; }

    /// <summary>
    /// Gets a value indicating whether 条目初始可选，供 <see cref="GenericCombinedData{TValueType}.IsEnabled"/> 置灰。
    /// </summary>
    public bool IsEnabled { get; init; } = true;

    /// <summary>
    /// 由 (值, 本地化key) 二元组构造条目。
    /// </summary>
    /// <param name="entry">(值, 本地化key)</param>
    public static implicit operator LocalizedEntry<TValue>((TValue Value, string LocalizationKey) entry)
        => new(entry.Value, entry.LocalizationKey);

    /// <summary>
    /// 设置附加本地化 key 及其显示条件。
    /// </summary>
    /// <param name="secondaryKey">附加本地化 key</param>
    /// <param name="showSecondary">显示条件，为 null 时恒显示；条件依赖的状态变化后需由调用方触发列表的 RefreshLocalization</param>
    /// <returns>设置后的新条目</returns>
    public LocalizedEntry<TValue> WithSecondaryKey(string secondaryKey, Func<bool>? showSecondary = null)
        => this with { SecondaryKey = secondaryKey, ShowSecondary = showSecondary };

    /// <summary>
    /// 设置悬浮提示本地化 key。
    /// </summary>
    /// <param name="tipKey">悬浮提示本地化 key</param>
    /// <returns>设置后的新条目</returns>
    public LocalizedEntry<TValue> WithTip(string tipKey)
        => this with { TipKey = tipKey };

    /// <summary>
    /// 设置条目初始是否可选。
    /// </summary>
    /// <param name="isEnabled">是否可选</param>
    /// <returns>设置后的新条目</returns>
    public LocalizedEntry<TValue> WithEnabled(bool isEnabled)
        => this with { IsEnabled = isEnabled };
}

/// <summary>
/// 让 (值, 本地化key) 二元组直接链式调用 <see cref="LocalizedEntry{TValue}"/> 的设置方法。
/// </summary>
public static class LocalizedEntryExtensions
{
    /// <summary>
    /// 由二元组构造条目并设置附加本地化 key 及其显示条件。
    /// </summary>
    /// <typeparam name="TValue">条目的值类型</typeparam>
    /// <param name="entry">(值, 本地化key)</param>
    /// <param name="secondaryKey">附加本地化 key</param>
    /// <param name="showSecondary">显示条件，为 null 时恒显示；条件依赖的状态变化后需由调用方触发列表的 RefreshLocalization</param>
    /// <returns>构造出的条目</returns>
    public static LocalizedEntry<TValue> WithSecondaryKey<TValue>(this (TValue Value, string LocalizationKey) entry, string secondaryKey, Func<bool>? showSecondary = null)
        => ((LocalizedEntry<TValue>)entry).WithSecondaryKey(secondaryKey, showSecondary);

    /// <summary>
    /// 由二元组构造条目并设置悬浮提示本地化 key。
    /// </summary>
    /// <typeparam name="TValue">条目的值类型</typeparam>
    /// <param name="entry">(值, 本地化key)</param>
    /// <param name="tipKey">悬浮提示本地化 key</param>
    /// <returns>构造出的条目</returns>
    public static LocalizedEntry<TValue> WithTip<TValue>(this (TValue Value, string LocalizationKey) entry, string tipKey)
        => ((LocalizedEntry<TValue>)entry).WithTip(tipKey);

    /// <summary>
    /// 由二元组构造条目并设置初始是否可选。
    /// </summary>
    /// <typeparam name="TValue">条目的值类型</typeparam>
    /// <param name="entry">(值, 本地化key)</param>
    /// <param name="isEnabled">是否可选</param>
    /// <returns>构造出的条目</returns>
    public static LocalizedEntry<TValue> WithEnabled<TValue>(this (TValue Value, string LocalizationKey) entry, bool isEnabled)
        => ((LocalizedEntry<TValue>)entry).WithEnabled(isEnabled);
}
