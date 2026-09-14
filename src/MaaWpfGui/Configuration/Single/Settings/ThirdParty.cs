// <copyright file="ThirdParty.cs" company="MaaAssistantArknights">
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
using System;
using MaaWpfGui.Models;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 三方服务设置：与企鹅物流、一图流等外部网站的数据交互
/// </summary>
public partial class ThirdParty : NotifyPropertyChangedWithValue
{
    public bool ReportToPenguin { get; set; } = true;

    public string PenguinId { get; set; } = string.Empty;

    public bool ReportToYituliu { get; set; } = true;

    /// <summary>
    /// 一图流第三方 OpenAPI Token，用于读取干员练度数据
    /// </summary>
    public string YituliuOpenApiToken { get; set; } = string.Empty;

    /// <summary>
    /// 干员识别改为从一图流 OpenAPI 获取，而不是连接模拟器本地识别
    /// </summary>
    public bool OperBoxUseYituliuApi { get; set; }
}
