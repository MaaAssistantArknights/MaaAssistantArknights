// <copyright file="GlobalConfigTip.xaml.cs" company="MaaAssistantArknights">
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

using System.Windows.Controls;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 全局配置提示控件。当配置列表多于一项时显示提示文本。
/// 用法：&lt;controls:GlobalConfigTip /&gt;
/// </summary>
public partial class GlobalConfigTip : UserControl
{
    public GlobalConfigTip()
    {
        InitializeComponent();
    }
}
