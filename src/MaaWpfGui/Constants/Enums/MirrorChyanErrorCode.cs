// <copyright file="MirrorChyanErrorCode.cs" company="MaaAssistantArknights">
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

public enum MirrorChyanErrorCode
{
    /// <summary>
    /// 请求成功
    /// </summary>
    Success = 0,

    /// <summary>
    /// 参数不正确，请参考集成文档
    /// </summary>
    InvalidParams = 1001,

    /// <summary>
    /// CDK 已过期
    /// </summary>
    KeyExpired = 7001,

    /// <summary>
    /// CDK 错误
    /// </summary>
    KeyInvalid = 7002,

    /// <summary>
    /// CDK 今日下载次数已达上限
    /// </summary>
    ResourceQuotaExhausted = 7003,

    /// <summary>
    /// CDK 类型和待下载的资源不匹配
    /// </summary>
    KeyMismatched = 7004,

    /// <summary>
    /// 对应架构和系统下的资源不存在
    /// </summary>
    ResourceNotFound = 8001,

    /// <summary>
    /// 错误的系统参数
    /// </summary>
    InvalidOs = 8002,

    /// <summary>
    /// 错误的架构参数
    /// </summary>
    InvalidArch = 8003,

    /// <summary>
    /// 错误的更新通道参数
    /// </summary>
    InvalidChannel = 8004,

    /// <summary>
    /// 未区分的业务错误，以响应体 JSON 的 msg 为准
    /// </summary>
    Undivided = 1,
}
