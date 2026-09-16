// <copyright file="OperProgressPlanItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;
using Stylet;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask;

namespace MaaWpfGui.ViewModels.Items;

public class OperProgressPlanItemViewModel(int index, OperatorRole role, string name, bool doElite, int elite, int mainSkillLevel, SkillLevel.Specialization specializationSkillLevel) : PropertyChangedBase
{
    public int Index { get; set => SetAndNotify(ref field, value); } = index;

    public OperatorRole Role { get; set => SetAndNotify(ref field, value); } = role;

    public string Name { get; set => SetAndNotify(ref field, value); } = name;

    public bool DoElite { get; set => SetAndNotify(ref field, value); } = doElite;

    public int Elite { get; set => SetAndNotify(ref field, value); } = elite;

    // public int Level { get; set => SetAndNotify(ref field, value); } = 1;
    public int MainSkillLevel { get; set => SetAndNotify(ref field, value); } = mainSkillLevel;

    public SkillLevel.Specialization SpecializationSkillLevel { get; set => SetAndNotify(ref field, value); } = specializationSkillLevel;
}
