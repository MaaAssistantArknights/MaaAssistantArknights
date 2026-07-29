// <copyright file="DepotPlanItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using JetBrains.Annotations;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

public class DepotPlanItemViewModel(string stage, string dropId, string? dropName = null, int dropCount = 0, bool useMedicine = false, int medicineCount = 0, bool useStone = false, int stoneCount = 0, int taskId = 0) : PropertyChangedBase
{
    public DepotPlanItemViewModel()
        : this(string.Empty, string.Empty, null, 0, false, 0, false, 0, 0)
    {
    }

    public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

    public int Index
    {
        get; set {
            field = value;
            NotifyOfPropertyChange(nameof(Title));
        }
    }

    public string Title => $"{Index + 1}: {DepotMaintainTaskUserControlModel.Instance.StageListSource.FirstOrDefault(i => i.Value == Stage)?.Display ?? Stage} - {DropName} x{DropCount.FormatNumber(false)}";

    /// <summary>
    /// 增删 plan 或语言切换后刷新 Title 显示（序号/关卡名/材料名）。
    /// </summary>
    public void RefreshTitle() => NotifyOfPropertyChange(nameof(Title));

    public string Stage
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            NotifyOfPropertyChange(nameof(Title));
        }
    } = stage;

    /// <summary>
    /// Gets or sets 指定掉落材料 ID。
    /// </summary>
    public string DropId
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            NotifyOfPropertyChange(nameof(Title));
        }
    } = dropId;

    /// <summary>
    /// Gets or sets 指定掉落材料名称。
    /// </summary>
    public string DropName
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            NotifyOfPropertyChange(nameof(Title));
        }
    } = dropName ?? LocalizationHelper.GetString("NotSelected");

    public int DropCount
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            NotifyOfPropertyChange(nameof(Title));
        }
    } = dropCount;

    public bool UseMedicine { get; set => SetAndNotify(ref field, value); } = useMedicine;

    public int MedicineCount { get; set => SetAndNotify(ref field, value); } = medicineCount;

    public bool UseStone { get; set => SetAndNotify(ref field, value); } = useStone;

    public int StoneCount { get; set => SetAndNotify(ref field, value); } = stoneCount;

    public int TaskId { get; set; } = taskId;

    // UI 绑定的方法
    [UsedImplicitly]
    public void DropsListDropDownClosed()
    {
        if (FightSettingsUserControlModel.Instance.DropsList.FirstOrDefault(i => i.Display == DropName) is { } item)
        {
            DropId = item.Value;
        }
        else
        {
            DropId = string.Empty;
            DropName = LocalizationHelper.GetString("NotSelected");
            NotifyOfPropertyChange(nameof(DropName));
        }
    }
}
