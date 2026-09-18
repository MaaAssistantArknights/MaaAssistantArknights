// <copyright file="OperProgressTask.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using MaaWpfGui.Constants.Enums;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// Ordered operator development plan.
/// </summary>
public class OperProgressTask : BaseTask
{
    public OperProgressTask() => TaskType = TaskType.OperProgress;

    public List<Plan> Plans { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether entries reported as completed or already satisfied
    /// are removed from the plan when the whole development task chain finishes.
    /// </summary>
    public bool DeleteOnCompleted { get; set; }

    public record class Plan(OperatorRole role, string name, int elite, int? level, SkillLevel skillLevel);

    public abstract record SkillLevel
    {
        public sealed record BaseLevel(int Level) : SkillLevel;

        public sealed record Specialization(int Skill1, int Skill2, int Skill3) : SkillLevel
        {
            public bool Any(Func<int, bool> predicate) => predicate(Skill1) || predicate(Skill2) || predicate(Skill3);

            public bool All(Func<int, bool> predicate) => predicate(Skill1) && predicate(Skill2) && predicate(Skill3);
        }
    }
}
