// <copyright file="TutorialChallengeLevelView.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows;
using System.Windows.Controls;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.Views.UserControl.TutorialChallenge;
using Growl = HandyControl.Controls.Growl;

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// TutorialChallengeLevelView.xaml 的交互逻辑
/// </summary>
public partial class TutorialChallengeLevelView
{
    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeLevelView"/> class.
    /// </summary>
    public TutorialChallengeLevelView()
    {
        InitializeComponent();
        DataContextChanged += OnLevelChanged;
        Closed += (_, _) => Growl.Unregister(TutorialGrowlPanel);
    }

    private void OnLevelChanged(object sender, DependencyPropertyChangedEventArgs e)
    {
        SandboxTabs.Items.Clear();
        if (e.NewValue is not TutorialChallengeLevelViewModel level)
        {
            return;
        }

        if (level.ShowTaskQueuePanel)
        {
            AddTab("TaskQueue.Farming", new TutorialChallengeTaskQueueUserControl { DataContext = level });
        }

        if (level.ShowSettingsPanel)
        {
            AddTab("Settings.Title", new TutorialChallengeSettingsUserControl { DataContext = level });
        }

        if (SandboxTabs.HasItems)
        {
            SandboxTabs.SetCurrentValue(TabControl.SelectedIndexProperty, 0);
        }
    }

    private void AddTab(string headerKey, FrameworkElement page)
    {
        var tab = new TabItem { Content = page };
        tab.SetResourceReference(HeaderedContentControl.HeaderProperty, headerKey);
        SandboxTabs.Items.Add(tab);
    }
}
