// <copyright file="InfrastRoomType.cs" company="MaaAssistantArknights">
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
namespace MaaWpfGui.Constants.Enums;

public enum InfrastRoomType
{
    // 显式指定枚举值，保证序列化/配置兼容性稳定
    /// <summary>
    /// 贸易站
    /// </summary>
    Trade = 0,

    /// <summary>
    /// 制造站
    /// </summary>
    Mfg = 1,

    /// <summary>
    /// 控制中心
    /// </summary>
    Control = 2,

    /// <summary>
    /// 发电站
    /// </summary>
    Power = 3,

    /// <summary>
    /// 会客室
    /// </summary>
    Reception = 4,

    /// <summary>
    /// 办公室(+速公招那个
    /// </summary>
    Office = 5,

    /// <summary>
    /// 宿舍
    /// </summary>
    Dorm = 6,

    /// <summary>
    /// 加工站(合精英材料
    /// </summary>
    Processing = 7,

    /// <summary>
    /// 训练室
    /// </summary>
    Training = 8,
}
