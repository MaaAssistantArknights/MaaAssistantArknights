// <copyright file="AutoRaiseMasterySkillRow.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>培养目标弹窗中的单个技能专精行</summary>
public sealed class AutoRaiseMasterySkillRow : PropertyChangedBase
{
    public AutoRaiseMasterySkillRow(int skillIndex)
    {
        SkillIndex = skillIndex;
        Label = LocalizationHelper.GetStringFormat("AutoRaiseSkillNumber", skillIndex);
    }

    public int SkillIndex { get; }

    public string Label { get; }

    private bool _isSelected;

    public bool IsSelected { get => _isSelected; set => SetAndNotify(ref _isSelected, value); }

    private int _target = 3;

    public int Target
    {
        get => _target;
        set {
            if (SetAndNotify(ref _target, value))
            {
                IsSelected = true;
            }
        }
    }
}
