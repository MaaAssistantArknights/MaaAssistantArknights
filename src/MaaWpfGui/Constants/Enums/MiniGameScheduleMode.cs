// <copyright file="MiniGameScheduleMode.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums;

/// <summary>
/// 牛杂任务的执行计划模式。
/// </summary>
public enum MiniGameScheduleMode
{
    /// <summary>
    /// 不启用计划，每次运行都执行。
    /// </summary>
    None,

    /// <summary>
    /// 周计划，按游戏内星期决定是否执行。
    /// </summary>
    Weekly,

    /// <summary>
    /// 月计划，按游戏内日期决定是否执行。
    /// </summary>
    Monthly,
}
