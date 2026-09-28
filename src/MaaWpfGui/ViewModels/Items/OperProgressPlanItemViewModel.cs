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
    public static readonly string[] MainProperty = [nameof(IsEliteSelected), nameof(Elite), nameof(IsMainSkillLevelSelected), nameof(MainSkillLevel), nameof(IsSpecializationSkill1Selected), nameof(IsSpecializationSkill2Selected), nameof(IsSpecializationSkill3Selected), nameof(SpecializationSkill1), nameof(SpecializationSkill2), nameof(SpecializationSkill3)];

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
    public OperProgressPlanItemViewModel(int index, OperatorRole role, string name, int elite, int mainSkillLevel, SkillMastery specializationSkillLevel)
    {
        Index = index;
        Role = role;
        Name = name;
        IsEliteSelected = elite > 0;
        Elite = elite;
        IsMainSkillLevelSelected = mainSkillLevel > 0;
        MainSkillLevel = mainSkillLevel;
        IsSpecializationSkill1Selected = specializationSkillLevel[0] > 0;
        IsSpecializationSkill2Selected = specializationSkillLevel[1] > 0;
        IsSpecializationSkill3Selected = specializationSkillLevel[2] > 0;
        SpecializationSkill1 = specializationSkillLevel[0];
        SpecializationSkill2 = specializationSkillLevel[1];
        SpecializationSkill3 = specializationSkillLevel[2];

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
            IsSpecializationSkill3Selected = SkillCount >= 3 && IsSpecializationSkill3Selected;
            IsSpecializationSkill2Selected = SkillCount >= 2 && IsSpecializationSkill2Selected;
            IsSpecializationSkill1Selected = SkillCount >= 1 && IsSpecializationSkill1Selected;
        }
        else
        {
            Log.Warning("Operator {Name} not found in data, cannot resolve role and skill count", name);
            SkillCount = 3;
            DisplayName = name;
        }
    }

    public int Index { get; set => SetAndNotify(ref field, value); }

    public OperatorRole Role { get; set => SetAndNotify(ref field, value); }

    public string Name { get; set => SetAndNotify(ref field, value); }

    public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets 本地化干员名，语言切换后由 <see cref="RefreshLocalizedText"/>
    /// 刷新。</summary>
    public string DisplayName { get; private set => SetAndNotify(ref field, value); }

    public bool IsEliteSelected { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets or sets 精英化目标，0 表示不设定。</summary>
    public int Elite
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            IsEliteSelected = value > 0;
            NotifyOfPropertyChange(nameof(EliteIconPath));
        }
    }

    public string EliteIconPath => $"/Res/Img/Operator/Elite_{Elite}.png";

    public bool IsMainSkillLevelSelected { get; set => SetAndNotify(ref field, value); }

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
            IsMainSkillLevelSelected = value > 0;
        }
    }

    /// <summary>
    /// 干员技能数：3 星 1 个技能，4/5 星 2 个，6 星与阿米娅 3 个，其余无技能；专精行按该值启用。
    /// </summary>
    public int SkillCount { get; set => SetAndNotify(ref field, value); }

    public bool IsSpecializationSkill1Selected
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            if (value && SpecializationSkill1 == 0)
            {
                SpecializationSkill1 = 3; // 未选择专几且激活专精时，自动设为专3
            }
        }
    }

    public bool IsSpecializationSkill2Selected
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            if (value && SpecializationSkill2 == 0)
            {
                SpecializationSkill2 = 3; // 未选择专几且激活专精时，自动设为专3
            }
        }
    }

    public bool IsSpecializationSkill3Selected
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }
            if (value && SpecializationSkill3 == 0)
            {
                SpecializationSkill3 = 3; // 未选择专几且激活专精时，自动设为专3
            }
        }
    }

    /// <summary>Gets or sets 技能 1 的专精等级，0 表示不专精。设定专精会把不足 7 级的技能等级目标补到 7 级。</summary>
    public int SpecializationSkill1 { get; set => SetSpecializationTarget(1, ref field, value); }

    /// <summary>Gets or sets 技能 2 的专精等级，0 表示不专精。</summary>
    public int SpecializationSkill2 { get; set => SetSpecializationTarget(2, ref field, value); }

    /// <summary>Gets or sets 技能 3 的专精等级，0 表示不专精。</summary>
    public int SpecializationSkill3 { get; set => SetSpecializationTarget(3, ref field, value); }

    /// <summary>Gets 技能序号 1 的专精行标签，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string SkillLabel1 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 1);

    /// <summary>Gets 技能序号 2 的专精行标签。</summary>
    public string SkillLabel2 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 2);

    /// <summary>Gets 技能序号 3 的专精行标签。</summary>
    public string SkillLabel3 { get; } = LocalizationHelper.GetStringFormat("OperProgressSkillNumber", 3);

    public SkillMastery SpecializationSkillLevel => SkillMastery.Of(IsSpecializationSkill1Selected ? SpecializationSkill1 : 0, IsSpecializationSkill2Selected ? SpecializationSkill2 : 0, IsSpecializationSkill3Selected ? SpecializationSkill3 : 0);

    /// <summary>语言切换后刷新本地化文本（干员名、专精行标签与目标描述）。</summary>
    public void RefreshLocalizedText()
    {
        DisplayName = ResolveDisplayName(Name);
        NotifyOfPropertyChange(nameof(SkillLabel1));
        NotifyOfPropertyChange(nameof(SkillLabel2));
        NotifyOfPropertyChange(nameof(SkillLabel3));
    }

    /// <summary>写入单个技能的专精等级，并在需要时补齐专精前置的基础技能等级。</summary>
    private void SetSpecializationTarget(int index, ref int field, int value, [CallerMemberName] string propertyName = "")
    {
        if (!SetAndNotify(ref field, value, propertyName))
        {
            return;
        }

        // 专精某技能且需要提升基础技能等级，则自动补到 7 级
        if (value > 0 && MainSkillLevel != 0)
        {
            MainSkillLevel = 7;
        }
        if (index == 1)
        {
            IsSpecializationSkill1Selected = value > 0;
        }
        else if (index == 2)
        {
            IsSpecializationSkill2Selected = value > 0;
        }
        else if (index == 3)
        {
            IsSpecializationSkill3Selected = value > 0;
        }
    }

    private static string ResolveDisplayName(string name) => DataHelper.GetLocalizedCharacterName(name) ?? name;
}
