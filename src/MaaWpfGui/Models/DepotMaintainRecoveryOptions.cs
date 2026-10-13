// <copyright file="DepotMaintainRecoveryOptions.cs" company="MaaAssistantArknights">
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

using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

internal static class DepotMaintainRecoveryOptions
{
    // Keep the exact wire serializer independent of WPF so both switches and
    // disabled saved allowances can be verified without starting a game.
    internal static JObject Serialize(bool taskMedicine, bool planMedicine, int medicineCount,
        bool taskStone, bool planStone, int stoneCount, bool expiring, int expireDays) => new() {
        ["medicine"] = taskMedicine && planMedicine ? medicineCount : 0,
        ["stone"] = taskStone && planStone ? stoneCount : 0,
        ["medicine_expire_days"] = expiring ? expireDays : 0,
    };
}
