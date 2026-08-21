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
/// 点击窗口其他位置关闭，关闭弹层的那次点击不作用于任何元素；点外部/失活关闭后可
/// 立即重新打开，点箭头关闭后的判定窗口内再次点击（如双击的第二击）是同一关闭手势
/// 的延续，不重新打开。机制细节见 <see cref="PopupDismissController"/>：点外部关闭由
/// 其以自管鼠标捕获实现；连击保持在本类箭头上截断——捕获子树外（箭头在弹层独立
/// 窗口之外）的按下仍会照常路由到箭头，判定窗口内的再次点击若不吞掉会随
/// MouseLeftButtonUp 把下拉切换关闭，因此预览阶段吞掉整次点击（并用一次性标志吞掉
/// 随后的 MouseUp）。
/// </summary>
public partial class TreeComboBox : UserControl
{
    // 最近一次下拉打开的时刻与 ｢因点箭头关闭｣ 的时刻（Environment.TickCount64 毫秒）；null 表示从未发生。
    // 不能用 long.MinValue 之类的哨兵值：与 TickCount64 相减会溢出为负，导致判定恒真
    private long? _openedAt;

    private long? _anchorClickClosedAt;

    // 判定窗口内连击吞击时置位：吞掉随后的 MouseUp，避免再次切换开合
    private bool _suppressNextClick;

    // 开合判定控制器；持有引用使存活不依赖 ｢PART_Popup 的事件订阅拴住控制器｣ 这一隐式契约
    private readonly PopupDismissController _dismissController;

    /// <summary>
    /// Initializes a new instance of the <see cref="TreeComboBox"/> class.
    /// </summary>
    public TreeComboBox()
    {
        InitializeComponent();

        // 打开时刻供判定窗口内的连击吞击判定，见 OnTogglePreviewMouseLeftButtonDown
        PART_Popup.Opened += OnPopupOpened;
        _dismissController = new PopupDismissController(
            PART_Popup,
            PART_ToggleBorder,
            onAnchorClickClose: () => _anchorClickClosedAt = Environment.TickCount64);

        // 弹层滚动的 ScrollChanged 就地终止冒泡：Popup 打开期间弹层与外层页面同处一条路由
        // 路径，弹层的滚动事件冒泡到页面滚动订阅者会被误当页面滚动（把页面拉到弹层偏移）
        PART_DropDownScrollViewer.AddHandler(
            ScrollViewer.ScrollChangedEvent,
            new ScrollChangedEventHandler((_, e) => e.Handled = true));
    }

    /// <summary>
    /// 下拉树选中项变化时发生；是否为可选项（非文件夹）及选中后的处理由使用点判定。
    /// </summary>
    public event EventHandler<RoutedPropertyChangedEventArgs<object>>? SelectionChanged;

    /// <summary>
    /// 箭头点击即将打开下拉时发生（仅按钮点击打开的路径触发），供使用点刷新数据源。
    /// </summary>
    public event EventHandler? DropDownOpening;

    /// <summary>
    /// The corner radius property.
    /// </summary>
    public static readonly DependencyProperty CornerRadiusProperty = DependencyProperty.Register(
        nameof(CornerRadius), typeof(CornerRadius), typeof(TreeComboBox), new PropertyMetadata(new CornerRadius(4)));

    /// <summary>
    /// Gets or sets 文本框的圆角，默认四角 4；右侧拼接按钮的使用点覆盖为 4,0,0,4。
    /// </summary>
    public CornerRadius CornerRadius
    {
        get => (CornerRadius)GetValue(CornerRadiusProperty);
        set => SetValue(CornerRadiusProperty, value);
    }

    /// <summary>
    /// The text property.
    /// </summary>
    public static readonly DependencyProperty TextProperty = DependencyProperty.Register(
        nameof(Text), typeof(string), typeof(TreeComboBox),
        new FrameworkPropertyMetadata(string.Empty, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

    /// <summary>
    /// Gets or sets 文本框显示的文本。双向绑定：内部文本框实时同步本属性，
    /// 到绑定源的回写时机由使用点绑定的 UpdateSourceTrigger 决定。
    /// </summary>
    public string Text
    {
        get => (string)GetValue(TextProperty);
        set => SetValue(TextProperty, value);
    }

    /// <summary>
    /// The placeholder property.
    /// </summary>
    public static readonly DependencyProperty PlaceholderProperty = DependencyProperty.Register(
        nameof(Placeholder), typeof(string), typeof(TreeComboBox), new PropertyMetadata(string.Empty));

    /// <summary>
    /// Gets or sets 文本框的占位提示文本。
    /// </summary>
    public string Placeholder
    {
        get => (string)GetValue(PlaceholderProperty);
        set => SetValue(PlaceholderProperty, value);
    }

    /// <summary>
    /// The items source property.
    /// </summary>
    public static readonly DependencyProperty ItemsSourceProperty = DependencyProperty.Register(
        nameof(ItemsSource), typeof(IEnumerable), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉树的数据源。
    /// </summary>
    public IEnumerable ItemsSource
    {
        get => (IEnumerable)GetValue(ItemsSourceProperty);
        set => SetValue(ItemsSourceProperty, value);
    }

    /// <summary>
    /// The item template property.
    /// </summary>
    public static readonly DependencyProperty ItemTemplateProperty = DependencyProperty.Register(
        nameof(ItemTemplate), typeof(DataTemplate), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉树的项模板（通常为 HierarchicalDataTemplate）。
    /// </summary>
    public DataTemplate ItemTemplate
    {
        get => (DataTemplate)GetValue(ItemTemplateProperty);
        set => SetValue(ItemTemplateProperty, value);
    }

    /// <summary>
    /// The item container style property.
    /// </summary>
    public static readonly DependencyProperty ItemContainerStyleProperty = DependencyProperty.Register(
        nameof(ItemContainerStyle), typeof(Style), typeof(TreeComboBox), new PropertyMetadata(null));

    /// <summary>
    /// Gets or sets 下拉树项的容器样式（TreeViewItem）。
    /// </summary>
    public Style ItemContainerStyle
    {
        get => (Style)GetValue(ItemContainerStyleProperty);
        set => SetValue(ItemContainerStyleProperty, value);
    }

    /// <summary>
    /// The is open property.
    /// </summary>
    public static readonly DependencyProperty IsOpenProperty = DependencyProperty.Register(
        nameof(IsOpen), typeof(bool), typeof(TreeComboBox),
        new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

    /// <summary>
    /// Gets or sets 下拉是否展开。一般由控件内部管理；需要外部主动控制开合时可双向绑定。
    /// </summary>
    public bool IsOpen
    {
        get => (bool)GetValue(IsOpenProperty);
        set => SetValue(IsOpenProperty, value);
    }

    /// <summary>
    /// The popup width property.
    /// </summary>
    public static readonly DependencyProperty PopupWidthProperty = DependencyProperty.Register(
        nameof(PopupWidth), typeof(double), typeof(TreeComboBox), new PropertyMetadata(double.NaN));

    /// <summary>
    /// Gets or sets 下拉 Popup 的宽度，NaN（默认）表示按内容自适应。
    /// </summary>
    public double PopupWidth
    {
        get => (double)GetValue(PopupWidthProperty);
        set => SetValue(PopupWidthProperty, value);
    }

    /// <summary>
    /// The popup min width property.
    /// </summary>
    public static readonly DependencyProperty PopupMinWidthProperty = DependencyProperty.Register(
        nameof(PopupMinWidth), typeof(double), typeof(TreeComboBox), new PropertyMetadata(double.NaN));

    /// <summary>
    /// Gets or sets 下拉 Popup 的最小宽度，NaN（默认）表示不限制。
    /// </summary>
    public double PopupMinWidth
    {
        get => (double)GetValue(PopupMinWidthProperty);
        set => SetValue(PopupMinWidthProperty, value);
    }

    /// <summary>
    /// The popup placement target property.
    /// </summary>
    public static readonly DependencyProperty PopupPlacementTargetProperty = DependencyProperty.Register(
        nameof(PopupPlacementTarget), typeof(UIElement), typeof(TreeComboBox),
        new PropertyMetadata(null, OnPopupPlacementTargetChanged));

    private static void OnPopupPlacementTargetChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var ctl = (TreeComboBox)d;
        ctl.PART_Popup.PlacementTarget = e.NewValue as UIElement ?? ctl;
    }

    /// <summary>
    /// Gets or sets 下拉 Popup 的定位目标，null（默认）表示本控件。
    /// </summary>
    public UIElement? PopupPlacementTarget
    {
        get => (UIElement?)GetValue(PopupPlacementTargetProperty);
        set => SetValue(PopupPlacementTargetProperty, value);
    }

    /// <summary>
    /// The popup placement rectangle property.
    /// </summary>
    public static readonly DependencyProperty PopupPlacementRectangleProperty = DependencyProperty.Register(
        nameof(PopupPlacementRectangle), typeof(Rect), typeof(TreeComboBox), new PropertyMetadata(Rect.Empty));

    /// <summary>
    /// Gets or sets 下拉 Popup 的定位矩形（相对于定位目标的偏移）。
    /// </summary>
    public Rect PopupPlacementRectangle
    {
        get => (Rect)GetValue(PopupPlacementRectangleProperty);
        set => SetValue(PopupPlacementRectangleProperty, value);
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

    private void OnTogglePreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        // 下拉着且刚打开：判定窗口内的连击，吞掉整次点击保持打开（含随后的 MouseUp）
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
        // 判定窗口内连击的同一次点击：吞掉 MouseUp，避免保持打开又被切换关闭
        if (_suppressNextClick)
        {
            _suppressNextClick = false;
            return;
        }

        // 因点箭头关闭后的判定窗口内再次点击（如双击的第二击）是同一关闭手势的
        // 延续：不视为重新打开；窗口外的点击正常展开
        if (_anchorClickClosedAt is { } closedAt
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
