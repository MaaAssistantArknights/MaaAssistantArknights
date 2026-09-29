// <copyright file="SkillSpecializationIcon.xaml.cs" company="MaaAssistantArknights">
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
/// 干员技能的专精等级角标：品字排布的三个圆点，按专精等级逐步点亮——专 1 亮上圆、
/// 专 2 加亮右下圆、专 3 全亮。颜色走主题资源
/// <c>OperBox.MasteryOnBrush</c>/<c>OperBox.MasteryOffBrush</c>，随浅深主题切换。
/// <para>
/// 使用点只需给 <see cref="Level"/> 一个 0~3 的专精等级，三个圆点的亮灭由本控件推导
/// （<see cref="TopOn"/>/<see cref="BottomRightOn"/>/<see cref="BottomLeftOn"/> 为只读属性，
/// 仅供模板绑定，不可外部赋值）；绘制细节（尺寸、坐标、阴影）封装在 XAML 内，
/// 控件不含外边距，间距（如列表内 2,0）由使用点通过 <see cref="FrameworkElement.Margin"/> 指定。
/// </para>
/// </summary>
public partial class SkillSpecializationIcon : UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="SkillSpecializationIcon"/> class.
    /// </summary>
    public SkillSpecializationIcon()
    {
        InitializeComponent();
    }

    /// <summary>
    /// The level property.
    /// </summary>
    public static readonly DependencyProperty LevelProperty = DependencyProperty.Register(
        nameof(Level), typeof(int), typeof(SkillSpecializationIcon), new PropertyMetadata(0, OnLevelChanged));

    private static readonly DependencyPropertyKey TopOnPropertyKey = DependencyProperty.RegisterReadOnly(
        nameof(TopOn), typeof(bool), typeof(SkillSpecializationIcon), new PropertyMetadata(false));

    /// <summary>
    /// The top on property.
    /// </summary>
    public static readonly DependencyProperty TopOnProperty = TopOnPropertyKey.DependencyProperty;

    private static readonly DependencyPropertyKey BottomRightOnPropertyKey = DependencyProperty.RegisterReadOnly(
        nameof(BottomRightOn), typeof(bool), typeof(SkillSpecializationIcon), new PropertyMetadata(false));

    /// <summary>
    /// The bottom right on property.
    /// </summary>
    public static readonly DependencyProperty BottomRightOnProperty = BottomRightOnPropertyKey.DependencyProperty;

    private static readonly DependencyPropertyKey BottomLeftOnPropertyKey = DependencyProperty.RegisterReadOnly(
        nameof(BottomLeftOn), typeof(bool), typeof(SkillSpecializationIcon), new PropertyMetadata(false));

    /// <summary>
    /// The bottom left on property.
    /// </summary>
    public static readonly DependencyProperty BottomLeftOnProperty = BottomLeftOnPropertyKey.DependencyProperty;

    /// <summary>
    /// Gets or sets 技能专精等级（0~3），决定三个圆点的亮灭：0 全灭、1 亮上圆、2 加亮右下圆、3 全亮。
    /// </summary>
    public int Level
    {
        get => (int)GetValue(LevelProperty);
        set => SetValue(LevelProperty, value);
    }

    /// <summary>
    /// Gets a value indicating whether 上圆是否点亮（专精 1 级起）。
    /// </summary>
    public bool TopOn => (bool)GetValue(TopOnProperty);

    /// <summary>
    /// Gets a value indicating whether 右下圆是否点亮（专精 2 级起）。
    /// </summary>
    public bool BottomRightOn => (bool)GetValue(BottomRightOnProperty);

    /// <summary>
    /// Gets a value indicating whether 左下圆是否点亮（专精 3 级）。
    /// </summary>
    public bool BottomLeftOn => (bool)GetValue(BottomLeftOnProperty);

    private static void OnLevelChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var icon = (SkillSpecializationIcon)d;
        int level = (int)e.NewValue;

        // 三个点亮标记同源（Level），不支持外部单独赋值，避免互相矛盾的状态
        icon.SetValue(TopOnPropertyKey, level >= 1);
        icon.SetValue(BottomRightOnPropertyKey, level >= 2);
        icon.SetValue(BottomLeftOnPropertyKey, level >= 3);
    }
}
