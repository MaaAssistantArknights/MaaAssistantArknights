// <copyright file="AutomationSuppressedListBox.cs" company="MaaAssistantArknights">
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
using System.Windows.Automation.Peers;
using System.Windows.Controls;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 不参与 UI 自动化树的列表：OnCreateAutomationPeer 返回 null 时整个子树（含全部条目）从自动化树摘除。
/// 虚拟化滚动生成新行、页签切换布局时 WPF 会对自动化树逐节点发事件，环境存在 UIA 客户端时
/// 逐节点同步通知会明显拖慢滚动与切换；该列表以头像视觉为主，读屏价值有限。
/// </summary>
public class AutomationSuppressedListBox : ListBox
{
    /// <summary>
    /// 返回 <see langword="null"/> 使本列表及全部子元素不进入自动化树。
    /// </summary>
    /// <returns><see langword="null"/>。</returns>
    protected override AutomationPeer? OnCreateAutomationPeer() => null;
}
