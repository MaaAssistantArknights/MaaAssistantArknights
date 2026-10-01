// <copyright file="OperCardBorder.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Windows;
using System.Windows.Automation.Peers;
using System.Windows.Controls;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 干员识别卡片容器：自动化树在本节点截断（卡内装饰子元素不参与）。
/// 与 <see cref="AutomationSuppressedListBox"/> 配合，滚动生成新卡片时不再为卡内几十个
/// 装饰元素创建自动化节点，降低滚动与页签切换的自动化事件开销。
/// </summary>
public class OperCardBorder : Border
{
    /// <summary>
    /// 返回子树截断的 peer：只暴露本卡片节点，不展开卡内元素。
    /// </summary>
    /// <returns>截断子树的 <see cref="AutomationPeer"/>。</returns>
    protected override AutomationPeer OnCreateAutomationPeer()
        => new LeafAutomationPeer(this);

        /// <summary>
        /// 子树截断的 peer：<see cref="GetChildrenCore"/> 返回 <see langword="null"/>，自动化事件不深入卡内。
        /// </summary>
    private sealed class LeafAutomationPeer(FrameworkElement owner) : FrameworkElementAutomationPeer(owner)
    {
        /// <summary>
        /// 返回 <see langword="null"/> 以在自动化树中截断子元素。
        /// </summary>
        /// <returns><see langword="null"/>。</returns>
        protected override List<AutomationPeer>? GetChildrenCore() => null;
    }
}
