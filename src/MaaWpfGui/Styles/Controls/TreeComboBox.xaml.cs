// <copyright file="TreeComboBox.xaml.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 文本框 + 箭头按钮 + 树形下拉的组合控件，整体用法类似 ComboBox：
/// 文本框显示/输入路径等文本，点箭头在下方弹出树（数据源与项模板由使用点提供），
/// 选中叶子项后经 <see cref="SelectionChanged"/> 交给使用点回填文本并调用 <see cref="CloseDropDown"/> 关闭。
/// 开合交互与 ComboBox 对齐——点一次打开、再点一次关闭、快速双击/连击保持打开、
/// 点击窗口其他位置关闭。点外部关闭由 <see cref="PopupDismissController"/> 以自管鼠标捕获实现；
/// 连击保持则在箭头上截断：捕获子树外（箭头在弹层独立窗口之外）的按下仍会照常路由到箭头，
/// 判定窗口内的再次点击若不吞掉会随 MouseLeftButtonUp 把下拉切换关闭，因此预览阶段吞掉整次点击。
/// 刚关闭的判定窗口内的点击吞掉重开（保证关闭后再打开有间隔）。
/// </summary>
public partial class TreeComboBox : UserControl
{
    // 最近一次下拉打开/关闭的时刻（Environment.TickCount64 毫秒）；null 表示从未发生。
    // 不能用 long.MinValue 之类的哨兵值：与 TickCount64 相减会溢出为负，导致判定恒真
    private long? _openedAt;

    private long? _closedAt;

    // 预览按下已吞掉点击（判定窗口内的连击）时置位，吞掉随后的 MouseUp，避免再次切换关闭
    private bool _suppressNextClick;

    /// <summary>
    /// Initializes a new instance of the <see cref="TreeComboBox"/> class.
    /// </summary>
    public TreeComboBox()
    {
        InitializeComponent();
        _ = new PopupDismissController(PART_Popup, PART_ToggleBorder);

        // 弹层滚动的 ScrollChanged 就地终止冒泡：Popup 打开期间弹层与外层页面同处一条路由
        // 路径，弹层的滚动事件冒泡到页面滚动订阅者会被误当页面滚动（把页面拉到弹层偏移）
        PART_DropDownScrollViewer.AddHandler(
            ScrollViewer.ScrollChangedEvent,
            new ScrollChangedEventHandler((_, e) => e.Handled = true));
    }

    /// <summary>
    /// 文本框内容变化（树选中项经使用点回填，或用户输入）时发生。
    /// </summary>
    public event EventHandler? TextChanged;

    /// <summary>
    /// 下拉树选中项变化时发生；是否为可选项（非文件夹）及选中后的处理由使用点判定。
    /// </summary>
    public event EventHandler<RoutedPropertyChangedEventArgs<object>>? SelectionChanged;

    /// <summary>
    /// 箭头点击即将打开下拉时发生（仅按钮点击打开的路径触发），供使用点刷新数据源。
    /// </summary>
    public event EventHandler? DropDownOpening;

    /// <summary>
    /// Gets or sets 文本框的圆角，默认四角 4；右侧拼接按钮的使用点覆盖为 4,0,0,4。
    /// </summary>
    public CornerRadius CornerRadius
    {
        get => (CornerRadius)GetValue(CornerRadiusProperty);
        set => SetValue(CornerRadiusProperty, value);
    }

    public static readonly DependencyProperty CornerRadiusProperty = DependencyProperty.Register(
        nameof(CornerRadius), typeof(CornerRadius), typeof(TreeComboBox), new PropertyMetadata(new CornerRadius(4)));

    /// <summary>
    /// Gets or sets 文本框显示的文本。双向绑定：用户输入按 LostFocus 回写绑定源。
    /// </summary>
    public string Text
    {
        get => (string)GetValue(TextProperty);
        set => SetValue(TextProperty, value);
    }

    public static readonly DependencyProperty TextProperty = DependencyProperty.Register(
        nameof(Text), typeof(string), typeof(TreeComboBox),
        new FrameworkPropertyMetadata(string.Empty, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnTextChanged));

    private static void OnTextChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var ctl = (TreeComboBox)d;
        ctl.TextChanged?.Invoke(ctl, EventArgs.Empty);
    }

    /// <summary>
    /// Gets or sets 文本框的占位提示文本。
    /// </summary>
    public string Placeholder
    {
        get => (string)GetValue(PlaceholderProperty);
        set => SetValue(PlaceholderProperty, value);
    }

    public static readonly DependencyProperty PlaceholderProperty = DependencyProperty.Register(
        nameof(Placeholder), typeof(string), typeof(TreeComboBox), new PropertyMetadata(string.Empty));

    /// <summary>
    /// Gets or sets 下拉树的数据源。
    /// </summary>
    public IEnumerable ItemsSource
    {
        get => (IEnumerable)GetValue(ItemsSourceProperty);
        set => SetValue(ItemsSourceProperty, value);
    }

    public static readonly DependencyProperty ItemsSourceProperty = DependencyProperty.Register(
        nameof(ItemsSource), typeof(IEnumerable), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉树的项模板（通常为 HierarchicalDataTemplate）。
    /// </summary>
    public DataTemplate ItemTemplate
    {
        get => (DataTemplate)GetValue(ItemTemplateProperty);
        set => SetValue(ItemTemplateProperty, value);
    }

    public static readonly DependencyProperty ItemTemplateProperty = DependencyProperty.Register(
        nameof(ItemTemplate), typeof(DataTemplate), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉树项的容器样式（TreeViewItem）。
    /// </summary>
    public Style ItemContainerStyle
    {
        get => (Style)GetValue(ItemContainerStyleProperty);
        set => SetValue(ItemContainerStyleProperty, value);
    }

    public static readonly DependencyProperty ItemContainerStyleProperty = DependencyProperty.Register(
        nameof(ItemContainerStyle), typeof(Style), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉是否展开。一般由控件内部管理；需要外部主动控制开合时可双向绑定。
    /// </summary>
    public bool IsOpen
    {
        get => (bool)GetValue(IsOpenProperty);
        set => SetValue(IsOpenProperty, value);
    }

    public static readonly DependencyProperty IsOpenProperty = DependencyProperty.Register(
        nameof(IsOpen), typeof(bool), typeof(TreeComboBox),
        new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

    /// <summary>
    /// Gets or sets 下拉 Popup 的宽度，NaN（默认）表示按内容自适应。
    /// </summary>
    public double PopupWidth
    {
        get => (double)GetValue(PopupWidthProperty);
        set => SetValue(PopupWidthProperty, value);
    }

    public static readonly DependencyProperty PopupWidthProperty = DependencyProperty.Register(
        nameof(PopupWidth), typeof(double), typeof(TreeComboBox), new PropertyMetadata(double.NaN));

    /// <summary>
    /// Gets or sets 下拉 Popup 的最小宽度，NaN（默认）表示不限制。
    /// </summary>
    public double PopupMinWidth
    {
        get => (double)GetValue(PopupMinWidthProperty);
        set => SetValue(PopupMinWidthProperty, value);
    }

    public static readonly DependencyProperty PopupMinWidthProperty = DependencyProperty.Register(
        nameof(PopupMinWidth), typeof(double), typeof(TreeComboBox), new PropertyMetadata(double.NaN));

    /// <summary>
    /// Gets or sets 下拉 Popup 的定位目标，null（默认）表示本控件。
    /// </summary>
    public UIElement? PopupPlacementTarget
    {
        get => (UIElement?)GetValue(PopupPlacementTargetProperty);
        set => SetValue(PopupPlacementTargetProperty, value);
    }

    public static readonly DependencyProperty PopupPlacementTargetProperty = DependencyProperty.Register(
        nameof(PopupPlacementTarget), typeof(UIElement), typeof(TreeComboBox),
        new PropertyMetadata(null, OnPopupPlacementTargetChanged));

    /// <summary>
    /// Gets or sets 下拉 Popup 的定位矩形（相对于定位目标的偏移）。
    /// </summary>
    public Rect PopupPlacementRectangle
    {
        get => (Rect)GetValue(PopupPlacementRectangleProperty);
        set => SetValue(PopupPlacementRectangleProperty, value);
    }

    public static readonly DependencyProperty PopupPlacementRectangleProperty = DependencyProperty.Register(
        nameof(PopupPlacementRectangle), typeof(Rect), typeof(TreeComboBox), new PropertyMetadata(Rect.Empty));

    private static void OnPopupPlacementTargetChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var ctl = (TreeComboBox)d;
        ctl.PART_Popup.PlacementTarget = e.NewValue as UIElement ?? ctl;
    }

    /// <summary>
    /// 关闭下拉（选中可选项后由使用点调用）。
    /// </summary>
    public void CloseDropDown() => SetCurrentValue(IsOpenProperty, false);

    private void OnTreeViewSelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        SelectionChanged?.Invoke(this, e);
    }

    private void OnPopupOpened(object? sender, EventArgs e) => _openedAt = Environment.TickCount64;

    private void OnPopupClosed(object? sender, EventArgs e) => _closedAt = Environment.TickCount64;

    private void OnTogglePreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        // 下拉着且刚打开：判定窗口内的连击，吞掉整次点击保持打开
        if (IsOpen
            && _openedAt is { } openedAt
            && Environment.TickCount64 - openedAt < DropDownDismiss.SuppressIntervalMs)
        {
            _suppressNextClick = true;
            e.Handled = true;
        }
    }

    private void OnToggleMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (_suppressNextClick)
        {
            _suppressNextClick = false;
            return;
        }

        // 下拉刚关闭，本次点击即关闭操作的一部分：不再重新打开
        if (_closedAt is { } closedAt
            && Environment.TickCount64 - closedAt < DropDownDismiss.SuppressIntervalMs)
        {
            return;
        }

        if (!IsOpen)
        {
            // 与旧实现 ｢打开前刷新数据源｣ 的时机对齐：刷新完成后才展开下拉
            DropDownOpening?.Invoke(this, EventArgs.Empty);
        }

        SetCurrentValue(IsOpenProperty, !IsOpen);
    }
}
