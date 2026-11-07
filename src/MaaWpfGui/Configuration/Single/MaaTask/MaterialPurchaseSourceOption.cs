// <copyright file="MaterialPurchaseSourceOption.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using System.Linq;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public enum MaterialPurchaseSource
{
    GreenTickets,
    ParametricModels,
    PremiumVoucher,
    AdvancedVoucher,
}

// 未知来源被配置容错逻辑移除后，不能回落成绿票。
public record MaterialPurchaseSourceOption(MaterialPurchaseSource Source = (MaterialPurchaseSource)(-1), bool IsEnabled = false)
{
    public static List<MaterialPurchaseSourceOption> Normalize(IEnumerable<MaterialPurchaseSourceOption> sources) =>
        sources.Where(option => Enum.IsDefined(option.Source)).DistinctBy(option => option.Source).ToList();

    public static List<MaterialPurchaseSourceOption> Defaults(bool green = false, bool parametric = false) =>
    [
        new(MaterialPurchaseSource.GreenTickets, green),
        new(MaterialPurchaseSource.AdvancedVoucher, false),
        new(MaterialPurchaseSource.PremiumVoucher, false),
        new(MaterialPurchaseSource.ParametricModels, parametric),
    ];
}
