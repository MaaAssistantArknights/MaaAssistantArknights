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
/// 保持手感一致（与 ComboBox 对齐）。唯一用途：弹层刚打开后窗口内的再次点击触发控件
/// （快速双击/宏级连击的第二击及后续）保持打开——Popup 是独立窗口，IsOpen 置 false 的
/// 隐藏立即上屏，即使同帧恢复也会产生可见的关开闪烁，不能靠 ｢关掉再打开｣ 实现。
/// 窗口内 ｢点击触发控件保持、点击外部关闭｣ 的区分机制见 <see cref="PopupDismissController"/>。
/// 窗口需覆盖弹层打开到再次点击的间隔（实际为毫秒级）；过大让连点保持期变长，
/// 过小在低性能机器上可能漏判。
/// 关闭后的重开不受窗口限制：因点触发控件关闭时记录时刻（控制器经 onAnchorClickClose
/// 回调通知触发控件），其判定窗口内对触发控件的再次点击（如双击的第二击）是同一关闭
/// 手势的延续，被触发控件吞掉不重新打开；点外部/失活关闭不记录时刻，下一次打开
/// 立即可用。
/// </summary>
public static class DropDownDismiss
{
    /// <summary>
    /// 打开/关闭后判定同一交互（吞掉重开或保持打开）的时间窗口（毫秒）。
    /// </summary>
    public const int SuppressIntervalMs = 500;
}
