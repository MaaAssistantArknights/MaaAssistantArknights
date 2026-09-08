// <copyright file="ConnectSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows;

namespace MaaWpfGui.Views.UserControl.Settings;

/// <summary>
/// ConnectSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class ConnectSettingsUserControl : System.Windows.Controls.UserControl
{
    public static readonly DependencyProperty ShowAdvancedConnectSettingsProperty = DependencyProperty.Register(
        nameof(ShowAdvancedConnectSettings), typeof(bool), typeof(ConnectSettingsUserControl), new PropertyMetadata(true));

    public bool ShowAdvancedConnectSettings
    {
        get => (bool)GetValue(ShowAdvancedConnectSettingsProperty);
        set => SetValue(ShowAdvancedConnectSettingsProperty, value);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ConnectSettingsUserControl"/> class.
    /// </summary>
    public ConnectSettingsUserControl()
    {
        InitializeComponent();
    }
}
