// <copyright file="ItemSelectionDialogViewModel.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Linq;
using Stylet;

namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// ViewModel for ItemSelectionDialogView
/// </summary>
public class ItemSelectionDialogViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="ItemSelectionDialogViewModel"/> class.
    /// </summary>
    /// <param name="availableItems">可选项</param>
    /// <param name="windowTitle">窗口标题</param>
    /// <param name="promptMessage">提示信息</param>
    public ItemSelectionDialogViewModel(IEnumerable<string> availableItems, string? windowTitle = null, string? promptMessage = null)
    {
        var itemsList = availableItems.ToList();
        Items = itemsList;
        WindowTitle = windowTitle ?? Helper.LocalizationHelper.GetString("SelectItem");
        PromptMessage = promptMessage ?? Helper.LocalizationHelper.GetString("PleaseSelectItem");
        if (itemsList.Count > 0)
        {
            SelectedItem = itemsList.First();
        }
    }

    /// <summary>
    /// Gets the window title.
    /// </summary>
    public string WindowTitle { get; }

    /// <summary>
    /// Gets the prompt message.
    /// </summary>
    public string PromptMessage { get; }

    /// <summary>
    /// Gets the list of items.
    /// </summary>
    public List<string> Items { get; }

    private string? _selectedItem;

    /// <summary>
    /// Gets or sets the selected item.
    /// </summary>
    public string? SelectedItem
    {
        get => _selectedItem;
        set => SetAndNotify(ref _selectedItem, value);
    }
}
