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
using System;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using MaaWpfGui.Styles.Properties;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.UserControl.TutorialChallenge;

/// <summary>
/// TutorialChallengeSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class TutorialChallengeSettingsUserControl
{
    private bool _rebuildPending;

    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeSettingsUserControl"/> class.
    /// </summary>
    public TutorialChallengeSettingsUserControl()
    {
        InitializeComponent();
        DataContextChanged += OnDataContextChanged;
        Loaded += (_, _) => (DataContext as TutorialChallengeLevelViewModel)?.NotifySettingSectionsLayoutChanged();
    }

    private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
    {
        if (e.OldValue is TutorialChallengeLevelViewModel previous)
        {
            previous.RefreshDividerOffsetsRequested -= OnRefreshDividerOffsetsRequested;
            previous.SandboxSettingSections.CollectionChanged -= OnSectionsChanged;
        }

        if (e.NewValue is TutorialChallengeLevelViewModel current)
        {
            current.RefreshDividerOffsetsRequested += OnRefreshDividerOffsetsRequested;
            current.SandboxSettingSections.CollectionChanged += OnSectionsChanged;
        }

        QueueRebuild();
    }

    private void OnRefreshDividerOffsetsRequested(object? sender, EventArgs e)
    {
        if (SandboxSettingsScrollViewer.IsLoaded)
        {
            ScrollViewerBinding.RefreshDividerOffsets(SandboxSettingsScrollViewer);
        }
    }

    private void OnSectionsChanged(object? sender, NotifyCollectionChangedEventArgs e) => QueueRebuild();

    private void QueueRebuild()
    {
        if (!_rebuildPending)
        {
            _rebuildPending = true;
            Dispatcher.InvokeAsync(RebuildSections, DispatcherPriority.DataBind);
        }
    }

    private void RebuildSections()
    {
        _rebuildPending = false;
        SandboxSettingsContentGrid.Children.Clear();
        SandboxSettingsContentGrid.Children.Add(SandboxAdvancedDivider);
        SandboxSettingsContentGrid.RowDefinitions.Clear();

        if (DataContext is not TutorialChallengeLevelViewModel level)
        {
            return;
        }

        // 分隔条占一行，分区行号由关卡按普通、高级分区的顺序分配。
        SandboxSettingsContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        var template = (DataTemplate)FindResource("SandboxSectionTemplate");
        foreach (var section in level.SandboxSettingSections)
        {
            var container = (Grid)template.LoadContent();
            container.DataContext = section;
            SandboxSettingsContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            SandboxSettingsContentGrid.Children.Add(container);
        }

        SearchHighlightBehavior.Refresh(SandboxSettingsScrollViewer);
        level.NotifySettingSectionsLayoutChanged();
    }
}
