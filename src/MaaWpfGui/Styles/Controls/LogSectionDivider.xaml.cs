// <copyright file="LogSectionDivider.xaml.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Controls;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 日志分区分隔线，视觉上对标 <c>hc:Divider</c>（居中标题 + 两侧等长横线）。
/// <para>
/// 不直接使用 <c>hc:Divider</c> 的原因：其模板基于 HandyControl 的 <c>hc:Row</c>/<c>hc:Col</c> 栅格，
/// <c>Row.ArrangeOverride</c> 以 <c>IsVisible</c> 跳过处于不可见链上的 <c>Col</c>。分隔符在窗口
/// 最小化到托盘（<c>Visibility.Hidden</c>）期间动态添加时，模板在隐藏状态下实例化，<c>Col</c>
/// 从未被 <c>Arrange</c>，此后永久渲染为 0 尺寸，窗口恢复后也无法自愈
/// （HandyControl issue #1818 / PR #1819，修复随上游发版后可换回 <c>hc:Divider</c>）。
/// </para>
/// </summary>
public partial class LogSectionDivider : UserControl
{
    public LogSectionDivider()
    {
        InitializeComponent();
        UpdateLineColumns();
    }

    public static readonly DependencyProperty HeaderProperty = DependencyProperty.Register(
        nameof(Header), typeof(string), typeof(LogSectionDivider), new PropertyMetadata(default(string)));

    /// <summary>
    /// Gets or sets 分隔线中间的标题文本，为空时仅显示一条通栏横线。
    /// </summary>
    public string? Header
    {
        get => (string?)GetValue(HeaderProperty);
        set => SetValue(HeaderProperty, value);
    }

    private static void OnHorizontalContentAlignmentChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        ((LogSectionDivider)d).UpdateLineColumns();
    }

    private void UpdateLineColumns()
    {
        StartLineColumn.Width = HorizontalContentAlignment == HorizontalAlignment.Right
            ? new GridLength(20)
            : new GridLength(1, GridUnitType.Star);
        EndLineColumn.Width = HorizontalContentAlignment == HorizontalAlignment.Left
            ? new GridLength(20)
            : new GridLength(1, GridUnitType.Star);
    }
}
