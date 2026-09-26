// <copyright file="OperPreviewLogItem.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Documents;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.Copilot;

namespace MaaWpfGui.ViewModels.Items;

public class OperPreviewLogItem : LogItemViewModel
{
    public OperPreviewLogItem(CopilotOutput output)
        : base(output.Content, output.Color ?? UiLogColor.Message, dateFormat: "HH':'mm':'ss", showTime: false)
    {
        Inlines = output.Parts.Select(part => {
            if (part.OperName is not { } operName)
            {
                return (Inline)new Run(part.Text);
            }

            var id = DataHelper.GetCharacterByNameOrAlias(operName)?.Id ?? string.Empty;
            var badge = OperAvatarHelper.CreateOperBadge(id, part.Text);
            return new InlineUIContainer(badge) { BaselineAlignment = BaselineAlignment.Center };
        }).ToList();
    }

    public IReadOnlyList<Inline> Inlines { get; }
}
