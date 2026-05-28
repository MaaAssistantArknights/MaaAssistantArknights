// <copyright file="SettingsView.xaml.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Helper;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// 设置界面。
/// </summary>
public partial class SettingsView
{
    public SettingsView()
    {
        InitializeComponent();

        Instances.SettingsViewModel.RefreshDividerOffsetsRequested += (_, _) =>
        {
            ScrollViewerBinding.RefreshDividerOffsets(SettingsScrollViewer);
        };

        // 初始化时将焦点移到左侧列表，避免搜索栏抢焦点
        Loaded += (_, _) => MasterListBox.Focus();
    }
}
