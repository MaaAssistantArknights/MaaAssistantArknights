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
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Converter.Specific;
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

    public record Plan(OperatorRole Role, string Name, int Elite, int SkillLevel, SkillMastery SkillMastery);

    /// <summary>
    /// 干员技能 1/2/3 的专精等级，未设定的技能为 0。
    /// </summary>
    /// <remarks>
    /// 该结构体没有公开的可读写成员，<see cref="System.Text.Json"/> 无法原生读写，故由
    /// <see cref="SkillMasteryConverter"/> 显式按数组处理；同理运行时也不会生成默认的
    /// <see cref="Equals(object)"/> 与 <see cref="GetHashCode"/>（调用即抛 <see cref="NotSupportedException"/>），
    /// 必须自行实现，否则 <see cref="Plan"/> 的记录相等比较会失败。
    /// </remarks>
    [InlineArray(3)]
    [JsonConverter(typeof(SkillMasteryConverter))]
    public struct SkillMastery : IEquatable<SkillMastery>
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

        /// <inheritdoc/>
        public readonly bool Equals(SkillMastery other) => ((ReadOnlySpan<int>)this).SequenceEqual((ReadOnlySpan<int>)other);

        /// <inheritdoc/>
        public readonly override bool Equals([NotNullWhen(true)] object? obj) => obj is SkillMastery other && Equals(other);

        /// <inheritdoc/>
        public readonly override int GetHashCode()
        {
            HashCode hashCode = default;
            foreach (var level in (ReadOnlySpan<int>)this)
            {
                hashCode.Add(level);
            }

            return hashCode.ToHashCode();
        }
    }
}
