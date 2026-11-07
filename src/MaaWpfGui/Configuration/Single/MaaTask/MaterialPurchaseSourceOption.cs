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
using System.Collections.Generic;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public enum MaterialPurchaseSource
{
    GreenTickets,
    ParametricModels,
    PremiumVoucher,
    AdvancedVoucher,
    EventShop,
}

public record MaterialPurchaseSourceOption(MaterialPurchaseSource Source, bool IsEnabled)
{
    public static List<MaterialPurchaseSourceOption> Defaults(bool green = false, bool parametric = false) =>
    [
        new(MaterialPurchaseSource.GreenTickets, green),
        new(MaterialPurchaseSource.ParametricModels, parametric),
        new(MaterialPurchaseSource.PremiumVoucher, false),
        new(MaterialPurchaseSource.AdvancedVoucher, false),
        new(MaterialPurchaseSource.EventShop, false),
    ];
}
