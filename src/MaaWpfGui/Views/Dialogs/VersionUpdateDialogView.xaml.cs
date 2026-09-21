// <copyright file="VersionUpdateDialogView.xaml.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using MaaWpfGui.ViewModels.Dialogs;

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// 更新日志渲染后需要做 inline 级后处理（贡献者头像与用户名合并为原子元素防止拆行），
/// MdXaml 经绑定生成 Document 的时机在 View 侧可观测，故订阅 Document 变化转发给 VM。
/// </summary>
public partial class VersionUpdateDialogView
{
    public VersionUpdateDialogView()
    {
        InitializeComponent();
        DependencyPropertyDescriptor.FromProperty(FlowDocumentScrollViewer.DocumentProperty, typeof(FlowDocumentScrollViewer))
            .AddValueChanged(ChangelogViewer, OnChangelogDocumentChanged);
    }

    private void OnChangelogDocumentChanged(object? sender, EventArgs e)
    {
        (DataContext as VersionUpdateDialogViewModel)?.MergeAvatarUsernameIntoAtoms();
    }
}
