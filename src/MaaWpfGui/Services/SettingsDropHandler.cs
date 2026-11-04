// <copyright file="SettingsDropHandler.cs" company="MaaAssistantArknights">
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

using System.Collections.ObjectModel;
using System.Windows;
using GongSolutions.Wpf.DragDrop;
using MaaWpfGui.ViewModels.Items;

namespace MaaWpfGui.Services;

/// <summary>
/// 将设置导航的拖动排序限制在同一分组内。
/// </summary>
/// <param name="settings">设置导航的完整分区列表。</param>
public class SettingsDropHandler(ObservableCollection<SettingItemViewModel> settings) : DefaultDropHandler
{
    /// <inheritdoc />
    public override void DragOver(IDropInfo dropInfo)
    {
        dropInfo.Effects = CanMove(dropInfo) ? DragDropEffects.Move : DragDropEffects.None;
        dropInfo.DropTargetAdorner = CanMove(dropInfo) ? DropTargetAdorners.Insert : null;
    }

    /// <inheritdoc />
    public override void Drop(IDropInfo dropInfo)
    {
        if (!CanMove(dropInfo))
        {
            return;
        }

        var sourceIndex = settings.IndexOf((SettingItemViewModel)dropInfo.Data);
        var targetIndex = settings.IndexOf((SettingItemViewModel)dropInfo.TargetItem);
        if (dropInfo.InsertPosition.HasFlag(RelativeInsertPosition.AfterTargetItem))
        {
            targetIndex++;
        }

        if (sourceIndex < targetIndex)
        {
            targetIndex--;
        }

        if (sourceIndex != targetIndex)
        {
            settings.Move(sourceIndex, targetIndex);
        }
    }

    private bool CanMove(IDropInfo dropInfo) =>
        dropInfo.Data is SettingItemViewModel source && source.IsVisible && settings.Contains(source)
        && dropInfo.TargetItem is SettingItemViewModel target && target.IsVisible && settings.Contains(target)
        && source.IsAdvanced == target.IsAdvanced;
}
