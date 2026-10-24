// <copyright file="CultivationSupplyPlanItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

public class CultivationSupplyPlanItemViewModel(CultivationSupplyTask.Plan plan) : PropertyChangedBase
{
    public string ItemId => plan.ItemId;

    public string Name => ItemListHelper.GetItemName(ItemId) ?? ItemId;

    public string Stage => plan.Stage;

    public int TargetCount => plan.TargetCount;

    public string InventoryDisplay
    {
        get {
            var inventory = Instances.ToolboxViewModel?.GetDepotSnapshot();
            var current = inventory is null ? "--" : CultivationSupplyItemHelper.GetEffectiveCount(ItemId, inventory).ToString("N0");
            return LocalizationHelper.GetStringFormat("CultivationSupplyTargetInventory", current, TargetCount.ToString("N0"));
        }
    }

    public void RefreshDisplay()
    {
        NotifyOfPropertyChange(nameof(Name));
        NotifyOfPropertyChange(nameof(InventoryDisplay));
    }
}
