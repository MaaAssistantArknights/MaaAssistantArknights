// <copyright file="TutorialChallengeSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using MaaWpfGui.Styles.Properties;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.UserControl.TutorialChallenge;

/// <summary>
/// TutorialChallengeSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class TutorialChallengeSettingsUserControl
{
    private TutorialChallengeLevelViewModel? _attachedViewModel;

    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeSettingsUserControl"/> class.
    /// </summary>
    public TutorialChallengeSettingsUserControl()
    {
        InitializeComponent();
        DataContextChanged += OnDataContextChanged;
    }

    private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
    {
        if (_attachedViewModel != null)
        {
            _attachedViewModel.RefreshDividerOffsetsRequested -= OnRefreshDividerOffsetsRequested;
            _attachedViewModel.SandboxSettingSections.CollectionChanged -= OnSandboxSettingSectionsChanged;
            _attachedViewModel = null;
        }

        if (e.NewValue is TutorialChallengeLevelViewModel viewModel)
        {
            _attachedViewModel = viewModel;
            viewModel.RefreshDividerOffsetsRequested += OnRefreshDividerOffsetsRequested;
            viewModel.SandboxSettingSections.CollectionChanged += OnSandboxSettingSectionsChanged;
            RebuildSettingSectionGrids(viewModel);
        }
        else
        {
            RebuildSettingSectionGrids(null);
        }
    }

    private void OnRefreshDividerOffsetsRequested(object? sender, System.EventArgs e)
        => ScrollViewerBinding.RefreshDividerOffsets(SandboxSettingsScrollViewer);

    private void OnSandboxSettingSectionsChanged(object? sender, NotifyCollectionChangedEventArgs e)
        => RebuildSettingSectionGrids(DataContext as TutorialChallengeLevelViewModel);

    /// <summary>
    /// 按当前分区列表重建右侧内容 Grid 的分区子元素：每个分区生成一个 Grid 容器，
    /// 行号与折叠显隐由绑定跟随分区模型，容器内 ContentPresenter 套用本控件资源里的分区模板。
    /// 结构与真实设置页同构——分区是内容 Grid 的直接子元素，ScrollViewerBinding 才能逐分区量出
    /// header 偏移供左侧导航联动；高级分隔条行由 <see cref="SandboxAdvancedDivider"/> 占位。
    /// RowDefinitions 按分区数动态补齐（分区数 + 分隔条行），XAML 不预设固定行数。
    /// </summary>
    /// <param name="viewModel">当前关卡 VM，null 时清空分区子元素。</param>
    private void RebuildSettingSectionGrids(TutorialChallengeLevelViewModel? viewModel)
    {
        SandboxSettingsContentGrid.Children.Clear();
        SandboxSettingsContentGrid.Children.Add(SandboxAdvancedDivider);
        SandboxSettingsContentGrid.RowDefinitions.Clear();

        if (viewModel == null)
        {
            return;
        }

        // +1 给高级分隔条所在行；分区行号见 AssignSectionGridRows
        var rowCount = System.Math.Max(viewModel.SandboxSettingSections.Count + 1, 1);
        for (var i = 0; i < rowCount; i++)
        {
            SandboxSettingsContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        }

        var sectionTemplate = (DataTemplate)FindResource("SandboxSettingSectionTemplate");
        foreach (var section in viewModel.SandboxSettingSections)
        {
            var container = new Grid {
                Margin = new Thickness(0, 5, 0, 0),
            };
            container.SetBinding(Grid.RowProperty, new Binding(nameof(TutorialChallengeSettingSection.GridRow)) {
                Source = section,
            });
            container.SetBinding(VisibilityProperty, new Binding(nameof(TutorialChallengeSettingSection.IsVisible)) {
                Source = section,
                Converter = new BooleanToVisibilityConverter(),
            });
            container.Children.Add(new ContentPresenter {
                Content = section,
                ContentTemplate = sectionTemplate,
            });
            SandboxSettingsContentGrid.Children.Add(container);
        }

        // 容器重建后左侧导航选中态需要按新布局纠偏（含首次进关无选中的初始落位）
        viewModel.NotifySettingSectionsLayoutChanged();
    }
}
