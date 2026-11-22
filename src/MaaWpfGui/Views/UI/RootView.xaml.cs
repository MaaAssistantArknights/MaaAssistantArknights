// <copyright file="RootView.xaml.cs" company="MaaAssistantArknights">
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

using System;
using System.Windows;
using System.Windows.Shell;

namespace MaaWpfGui.Views.UI;

public partial class RootView
{
    private readonly Thickness _normalResizeBorderThickness;

    public RootView()
    {
        InitializeComponent();
        _normalResizeBorderThickness = WindowChrome.GetWindowChrome(this)?.ResizeBorderThickness ?? default;
    }

    /// <inheritdoc/>
    protected override void OnStateChanged(EventArgs e)
    {
        base.OnStateChanged(e);

        // HandyControls 3.6.0 运行中最大化时不会清零 ResizeBorderThickness，标题栏最顶部一条被判为 HTTOP，
        // 按住该处无法拖动还原窗口（上游 3.7.0 已修复，见 ghost1372/HandyControls#234）
        if (WindowChrome.GetWindowChrome(this) is { } chrome)
        {
            chrome.ResizeBorderThickness = WindowState == WindowState.Maximized ? default : _normalResizeBorderThickness;
        }
    }
}
