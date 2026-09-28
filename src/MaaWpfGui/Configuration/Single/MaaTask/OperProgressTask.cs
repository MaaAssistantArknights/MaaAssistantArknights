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
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json.Serialization;
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

    public record class Plan(OperatorRole Role, string Name, int Elite, int SkillLevel, SkillMastery SkillMastery);

    [InlineArray(3)]
    public struct SkillMastery
    {
        private int _v;

        public static SkillMastery Of(int a, int b, int c)
        {
            SkillMastery v = default;          // 先清零，保证未初始化元素不会是垃圾
            v[0] = a;
            v[1] = b;
            v[2] = c;
            return v;
        }

        public int[] ToArray()
        {
            int[] arr = new int[3];
            MemoryMarshal.CreateReadOnlySpan(ref _v, 3).CopyTo(arr);
            return arr;
        }
    }

    /// <summary>
    /// 技能培养目标：基础技能等级（<see cref="BaseLevel"/>）与专精等级（<see cref="Mastery"/>）二选一。
    /// </summary>
    [JsonDerivedType(typeof(BaseLevel), typeDiscriminator: nameof(BaseLevel))]
    [JsonDerivedType(typeof(Mastery), typeDiscriminator: nameof(Mastery))]
    public abstract record SkillLevel
    {
        /// <summary>
        /// 基础技能等级目标。
        /// </summary>
        public sealed record BaseLevel(int Level) : SkillLevel
        {
            public static implicit operator int(BaseLevel value) => value.Level;

            public static implicit operator BaseLevel(int value) => new(value);
        }

        /// <summary>
        /// 技能 1/2/3 的专精等级目标，未设定为 0。
        /// </summary>
        public sealed record Mastery(int Skill1, int Skill2, int Skill3) : SkillLevel
        {
            public int[] ToArray() => [Skill1, Skill2, Skill3];

            public static explicit operator int[](Mastery value)
            {
                ArgumentNullException.ThrowIfNull(value);
                return value.ToArray();
            }

            public static explicit operator Mastery(int[] skills)
            {
                ArgumentNullException.ThrowIfNull(skills);
                if (skills.Length != 3)
                {
                    throw new ArgumentException("Exactly three skill levels are required.", nameof(skills));
                }

                return new Mastery(skills[0], skills[1], skills[2]);
            }

            public bool Any(Func<int, bool> predicate) => predicate(Skill1) || predicate(Skill2) || predicate(Skill3);

            public bool All(Func<int, bool> predicate) => predicate(Skill1) && predicate(Skill2) && predicate(Skill3);
        }
    }
}
