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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using Serilog;
using Stylet;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>干员培养计划中的单个干员卡片，一个条目对应一名干员。</summary>
public class OperProgressPlanItemViewModel : PropertyChangedBase
{
    /// <summary>触发回写任务配置的属性名集合，其余属性（序号、展开状态、本地化文本）不影响计划内容。</summary>
    // 弃用（即将被移除）：集合本身后续不再保留，改由各属性直接在 setter 中请求回写。
    private static readonly HashSet<string> PersistedPropertyNames = [
        nameof(Elite),
        nameof(MainSkillLevel),
        nameof(SpecializationSkill1),
        nameof(SpecializationSkill2),
        nameof(SpecializationSkill3),
    ];

    /// <summary>判断属性变更是否影响计划内容，进而需要回写任务配置。</summary>
    /// <param name="propertyName">变更的属性名。</param>
    /// <returns>需要回写时为 true。</returns>
    public static bool IsPersistedProperty(string? propertyName) => propertyName is not null && PersistedPropertyNames.Contains(propertyName);

    /// <summary>
    /// Initializes a new instance of the <see cref="OperProgressPlanItemViewModel"/> class.
    /// 初始化干员卡片。
    /// </summary>
    /// <param name="index">列表序号。</param>
    /// <param name="role">干员职业。</param>
    /// <param name="name">干员名。</param>
    /// <param name="elite">精英化目标，0 表示不设定。</param>
    /// <param name="mainSkillLevel">技能等级目标，0 表示不设定。</param>
    /// <param name="specializationSkillLevel">专精目标，未设定的技能为 0。</param>
    public OperProgressPlanItemViewModel(int index, OperatorRole role, string name, int elite, int mainSkillLevel, SkillLevel.Specialization specializationSkillLevel)
    {
        Index = index;
        Role = role;
        Name = name;
        Elite = elite;
        MainSkillLevel = mainSkillLevel;
        SpecializationSkill1 = specializationSkillLevel.Skill1;
        SpecializationSkill2 = specializationSkillLevel.Skill2;
        SpecializationSkill3 = specializationSkillLevel.Skill3;

        var oper = DataHelper.Characters.Values.FirstOrDefault(c => (role == OperatorRole.Unknown || c.Role == role) && c.Name == name);
        if (oper is not null)
        {
            DisplayName = DataHelper.GetLocalizedCharacterName(oper) ?? name;
            if (Role == OperatorRole.Unknown)
            {
                Role = oper.Role;
            }
            if (oper.Id == "char_002_amiya")
            {
                SkillCount = 3;
            }
            else
            {
                SkillCount = oper.Rarity switch {
                    6 => 3,
                    5 or 4 => 2,
                    3 => 1,
                    _ => 0,
                };
            }
        }
        else
        {
            Log.Warning("干员 {Name} 不存在于数据中，无法解析职业与技能数", name);
            SkillCount = 3;
            DisplayName = name;
        }
    }

    public int Index { get; set => SetAndNotify(ref field, value); }

    public OperatorRole Role { get; set => SetAndNotify(ref field, value); }

    public string Name { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets a value indicating whether 卡片展开状态。</summary>
    public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets 本地化干员名，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string DisplayName { get; private set => SetAndNotify(ref field, value); }

    /// <summary>Gets or sets 精英化目标，0 表示不设定。</summary>
    public int Elite
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            if (Elite == 1)
            {
                MainSkillLevel = Math.Min(MainSkillLevel, 4);
            }

            NotifyOfPropertyChange(nameof(TargetDescription));
        }
    }

    /// <summary>Gets or sets 技能等级目标，0 表示不设定。</summary>
    public int MainSkillLevel
    {
        get;
        set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            // 技能的专精前置为 7 级：设定不足 7 级的技能等级目标时清空专精，二者不共存。
            if (value > 0 && value < 7)
            {
                SpecializationSkill1 = 0;
                SpecializationSkill2 = 0;
                SpecializationSkill3 = 0;
            }
            if (value > 4 && Elite == 1)
            {
                Elite = 2;
            }

            NotifyOfPropertyChange(nameof(TargetDescription));
        }
    }

    /// <summary>
    /// 干员技能数：3 星 1 个技能，4/5 星 2 个，6 星与阿米娅 3 个，其余无技能；专精行按该值启用。
    /// </summary>
    public int SkillCount { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets or sets 技能 1 的专精等级，0 表示不专精。设定专精会把不足 7 级的技能等级目标补到 7 级。</summary>
    public int SpecializationSkill1 { get; set => SetSpecializationTarget(ref field, value); }

    /// <summary>Gets or sets 技能 2 的专精等级，0 表示不专精。</summary>
    public int SpecializationSkill2 { get; set => SetSpecializationTarget(ref field, value); }

    /// <summary>Gets or sets 技能 3 的专精等级，0 表示不专精。</summary>
    public int SpecializationSkill3 { get; set => SetSpecializationTarget(ref field, value); }

    /// <summary>Gets 技能序号 1 的专精行标签，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string SkillLabel1 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 1);

    /// <summary>Gets 技能序号 2 的专精行标签。</summary>
    public string SkillLabel2 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 2);

    /// <summary>Gets 技能序号 3 的专精行标签。</summary>
    public string SkillLabel3 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 3);

    public SkillLevel.Specialization SpecializationSkillLevel => new(SpecializationSkill1, SpecializationSkill2, SpecializationSkill3);

    /// <summary>Gets 卡片当前培养目标的本地化描述，多个目标以「 / 」连接。</summary>
    public string TargetDescription
    {
        get {
            var parts = new List<string>(4);
            for (int skill = 1; skill <= 3; ++skill)
            {
                int target = GetSpecializationTarget(skill);
                if (target > 0)
                {
                    parts.Add(LocalizationHelper.GetStringFormat("OperProgressMasteryTarget", skill, target));
                }
            }

            if (MainSkillLevel > 0)
            {
                parts.Add(LocalizationHelper.GetStringFormat("OperProgressSkillLevelTarget", MainSkillLevel));
            }

            if (Elite > 0)
            {
                parts.Add(LocalizationHelper.GetStringFormat("OperProgressEliteTarget", Elite));
            }

            return string.Join(" / ", parts);
        }
    }

    /// <summary>语言切换后刷新本地化文本（干员名、专精行标签与目标描述）。</summary>
    public void RefreshLocalizedText()
    {
        DisplayName = ResolveDisplayName(Name);
        NotifyOfPropertyChange(nameof(SkillLabel1));
        NotifyOfPropertyChange(nameof(SkillLabel2));
        NotifyOfPropertyChange(nameof(SkillLabel3));
        NotifyOfPropertyChange(nameof(TargetDescription));
    }

    /// <summary>写入单个技能的专精等级，并在需要时补齐专精前置的基础技能等级。</summary>
    private void SetSpecializationTarget(ref int field, int value, [CallerMemberName] string propertyName = "")
    {
        if (!SetAndNotify(ref field, value, propertyName))
        {
            return;
        }

        if (value > 0 && MainSkillLevel != 0) // 专精某技能且需要提成基础技能等级，则自动补到7级
        {
            MainSkillLevel = 7;
        }

        NotifyOfPropertyChange(nameof(TargetDescription));
    }

    private int GetSpecializationTarget(int skillIndex) => skillIndex switch {
        1 => SpecializationSkill1,
        2 => SpecializationSkill2,
        3 => SpecializationSkill3,
        _ => 0,
    };

    private static string ResolveDisplayName(string name) => DataHelper.GetLocalizedCharacterName(name) ?? name;
}
