// <copyright file="RunOwner.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.States;

#nullable enable

namespace MaaWpfGui.Constants.Enums;

/// <summary>
/// 当前运行轮次的发起入口归属，由各开始入口在启动运行时经 <see cref="RunningState.BeginRun"/> 声明，
/// 回到空闲时自动清零。手动停止的结束脚本发射条件与停止按钮提示按此判定，
/// 使跨页停止时也能正确识别实际运行的任务（如 copilot 须双开关同时开启）。
/// </summary>
public enum RunOwner
{
    /// <summary>
    /// 未在运行，或入口不属于任何任务语境（如连接测试）。
    /// </summary>
    None,

    /// <summary>
    /// 主任务队列（一键长草）。
    /// </summary>
    TaskQueue,

    /// <summary>
    /// 自动战斗。
    /// </summary>
    Copilot,

    /// <summary>
    /// 小工具页的小游戏。
    /// </summary>
    MiniGame,

    /// <summary>
    /// 小工具页的识别工具（公招计算、仓库识别、干员识别、牛牛抽卡、查看截图）。
    /// </summary>
    Toolbox,
}
