// <copyright file="DropDownDismiss.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 下拉/菜单类弹层开合交互的判定窗口时长，MenuButton、TreeComboBox、SplitButton 下拉共用，
/// 保持手感一致（与 ComboBox 对齐）。两个用途：
/// 1) 弹层刚打开后窗口内的再次点击触发控件（快速双击/宏级连击的第二击及后续）保持打开——
///    Popup 是独立窗口，IsOpen 置 false 的隐藏立即上屏，即使同帧恢复也会产生可见的关开闪烁，
///    不能靠 ｢关掉再打开｣ 实现；
/// 2) 弹层刚关闭后窗口内的再次点击触发控件是关闭操作的延续（或关闭后的立即再点），
///    吞掉以免立刻重开，保证 ｢关闭后再打开｣ 有间隔。
/// 窗口内 ｢点击触发控件保持、点击外部关闭｣ 的区分由 <see cref="PopupDismissController"/>
/// 通过自管鼠标捕获实现。窗口需覆盖关闭到触发事件传播的间隔（实际为毫秒级）；
/// 过大让连点保持期变长，过小在低性能机器上可能漏判。
/// </summary>
public static class DropDownDismiss
{
    /// <summary>
    /// 打开/关闭后判定同一交互（吞掉重开或保持打开）的时间窗口（毫秒）。
    /// </summary>
    public const int SuppressIntervalMs = 500;
}
