// <copyright file="MaterialPurchaseSourceItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

public class MaterialPurchaseSourceItemViewModel(MaterialPurchaseSourceOption option) : PropertyChangedBase
{
    public MaterialPurchaseSource Source { get; } = option.Source;

    public string Name => LocalizationHelper.GetString($"TaskQueue.OperProgress.MaterialPurchase.{Source}");

    public bool IsEnabled { get; set => SetAndNotify(ref field, value); } = option.IsEnabled;

    public void RefreshLocalization() => NotifyOfPropertyChange(nameof(Name));

    public static ObservableCollection<MaterialPurchaseSourceItemViewModel> Create(
        List<MaterialPurchaseSourceOption> options, Action<List<MaterialPurchaseSourceOption>> save)
    {
        var sources = MaterialPurchaseSourceOption.Normalize(options).Concat(MaterialPurchaseSourceOption.Defaults()
            .Where(option => options.All(saved => saved.Source != option.Source)));
        ObservableCollection<MaterialPurchaseSourceItemViewModel> items = [.. sources.Select(option => new MaterialPurchaseSourceItemViewModel(option))];
        void Save() => save(items.Select(item => new MaterialPurchaseSourceOption(item.Source, item.IsEnabled)).ToList());
        foreach (var item in items)
        {
            item.PropertyChanged += (_, args) => {
                if (args.PropertyName == nameof(IsEnabled))
                {
                    Save();
                }
            };
        }
        items.CollectionChanged += (_, _) => Save();
        return items;
    }
}
