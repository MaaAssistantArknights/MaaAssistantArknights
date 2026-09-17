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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Stylet;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>干员培养计划中的单个干员卡片，一个条目对应一名干员。</summary>
public class OperProgressPlanItemViewModel : PropertyChangedBase
{
    /// <summary>触发回写任务配置的属性名集合，其余属性（序号、展开状态、本地化文本）不影响计划内容。</summary>
    private static readonly HashSet<string> PersistedPropertyNames = new() {
        nameof(DoElite),
        nameof(Elite),
        nameof(DoSkillLevel),
        nameof(MainSkillLevel),
        nameof(SpecializationSkillLevel),
    };

    /// <summary>判断属性变更是否影响计划内容，进而需要回写任务配置。</summary>
    /// <param name="propertyName">变更的属性名。</param>
    /// <returns>需要回写时为 true。</returns>
    public static bool IsPersistedProperty(string? propertyName) => propertyName is not null && PersistedPropertyNames.Contains(propertyName);

    private readonly ObservableCollection<OperProgressMasterySkillRow> _masteryRows = [];

    /// <summary>初始化干员卡片。</summary>
    /// <param name="index">列表序号。</param>
    /// <param name="role">干员职业。</param>
    /// <param name="name">干员名。</param>
    /// <param name="doElite">是否设定精英化目标。</param>
    /// <param name="elite">精英化目标。</param>
    /// <param name="mainSkillLevel">技能等级目标。</param>
    /// <param name="specializationSkillLevel">专精目标，未设定的技能为 0。</param>
    public OperProgressPlanItemViewModel(int index, OperatorRole role, string name, bool doElite, int elite, int mainSkillLevel, SkillLevel.Specialization specializationSkillLevel)
    {
        Index = index;
        Role = role;
        Name = name;
        Elite = elite;
        MainSkillLevel = mainSkillLevel;
        SpecializationSkillLevel = specializationSkillLevel;
        DoElite = doElite;
        DisplayName = ResolveDisplayName(name);
        ResetMasteryRows();
    }

    public int Index { get; set => SetAndNotify(ref field, value); }

    public OperatorRole Role { get; set => SetAndNotify(ref field, value); }

    public string Name { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets a value indicating whether 卡片展开状态。</summary>
    public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets 本地化干员名，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string DisplayName { get; private set => SetAndNotify(ref field, value); }

    /// <summary>Gets or sets a value indicating whether 设定精英化目标。勾选时补齐合法目标值。</summary>
    public bool DoElite
    {
        get;
        set {
            if (SetAndNotify(ref field, value) && value && Elite is < 1 or > 2)
            {
                Elite = 2;
            }

            NotifyOfPropertyChange(nameof(TargetDescription));
        }
    }

    /// <summary>Gets or sets 精英化目标。</summary>
    public int Elite
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(TargetDescription));
            }
        }
    }

    /// <summary>为 true 时不响应专精行变更，避免互斥切换时互相回写。</summary>
    private bool _isSyncingTargets;

    /// <summary>Gets or sets a value indicating whether 设定技能等级目标。与专精互斥：勾选技能等级会清空专精。</summary>
    public bool DoSkillLevel
    {
        get => !SpecializationSkillLevel.Any(x => x > 0) && MainSkillLevel > 0;
        set {
            if (value)
            {
                if (MainSkillLevel <= 0)
                {
                    MainSkillLevel = 7;
                }

                ClearSpecialization();
            }
            else
            {
                MainSkillLevel = 0;
            }

            NotifyOfPropertyChange();
        }
    }

    /// <summary>Gets or sets 技能等级目标，0 表示不设定。</summary>
    public int MainSkillLevel
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(DoSkillLevel));
                NotifyOfPropertyChange(nameof(TargetDescription));
            }
        }
    }

    /// <summary>Gets or sets 专精目标，由 <see cref="MasteryRows"/> 的勾选状态同步。</summary>
    public SkillLevel.Specialization SpecializationSkillLevel
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(DoSkillLevel));
                NotifyOfPropertyChange(nameof(TargetDescription));
            }
        }
    }

    /// <summary>Gets 技能专精行，按干员稀有度与已有专精目标生成。</summary>
    public ObservableCollection<OperProgressMasterySkillRow> MasteryRows => _masteryRows;

    public IReadOnlyList<int> EliteOptions { get; } = [1, 2];

    public IReadOnlyList<int> SkillLevelOptions { get; } = [2, 3, 4, 5, 6, 7];

    public IReadOnlyList<int> MasteryTargetOptions { get; } = [1, 2, 3];

    /// <summary>Gets 卡片当前培养目标的本地化描述，多个目标以「 / 」连接。</summary>
    public string TargetDescription
    {
        get {
            var parts = new List<string>(4);
            foreach (var row in _masteryRows.Where(row => row.IsSelected))
            {
                parts.Add(LocalizationHelper.GetStringFormat("OperProgressMasteryTarget", row.SkillIndex, row.Target));
            }

            if (DoSkillLevel)
            {
                parts.Add(LocalizationHelper.GetStringFormat("OperProgressSkillLevelTarget", MainSkillLevel));
            }

            if (DoElite)
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
        ResetMasteryRows();
        NotifyOfPropertyChange(nameof(TargetDescription));
    }

    /// <summary>按当前专精目标重建专精行，行标签随语言切换刷新。</summary>
    private void ResetMasteryRows()
    {
        foreach (var row in _masteryRows)
        {
            row.PropertyChanged -= MasteryRow_PropertyChanged;
        }

        _masteryRows.Clear();
        int maxSkill = Math.Clamp(Math.Max(GetMaxMasterySkill(Name), MaxSelectedSkill()), 1, 3);
        for (int skillIndex = 1; skillIndex <= maxSkill; ++skillIndex)
        {
            var row = new OperProgressMasterySkillRow(skillIndex);
            int target = GetSpecializationTarget(skillIndex);
            if (target > 0)
            {
                row.Target = target;
                row.IsSelected = true;
            }

            row.PropertyChanged += MasteryRow_PropertyChanged;
            _masteryRows.Add(row);
        }
    }

    private void MasteryRow_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_isSyncingTargets || e.PropertyName is not (nameof(OperProgressMasterySkillRow.IsSelected) or nameof(OperProgressMasterySkillRow.Target)))
        {
            return;
        }

        // 勾选专精时清空技能等级目标，与 DoSkillLevel 的互斥方向对称：后操作者生效
        if (GetRowTarget(sender as OperProgressMasterySkillRow) > 0)
        {
            MainSkillLevel = 0;
        }

        SpecializationSkillLevel = new SkillLevel.Specialization(GetRowTarget(1), GetRowTarget(2), GetRowTarget(3));
    }

    /// <summary>清空全部专精勾选（含已折叠的专精行），用于技能等级与专精的互斥切换。</summary>
    private void ClearSpecialization()
    {
        if (!SpecializationSkillLevel.Any(x => x > 0))
        {
            return;
        }

        _isSyncingTargets = true;
        try
        {
            foreach (var row in _masteryRows)
            {
                row.IsSelected = false;
            }

            SpecializationSkillLevel = new(0, 0, 0);
        }
        finally
        {
            _isSyncingTargets = false;
        }
    }

    /// <summary>读触发变更的单行当前勾选值，未勾选或行已移除时返回 0。</summary>
    private static int GetRowTarget(OperProgressMasterySkillRow? row) => row is { IsSelected: true } ? row.Target : 0;

    /// <summary>按技能序号读专精行的勾选值，未勾选或该技能不存在时返回 0。</summary>
    private int GetRowTarget(int skillIndex) =>
        _masteryRows.FirstOrDefault(row => row.SkillIndex == skillIndex && row.IsSelected)?.Target ?? 0;

    private int GetSpecializationTarget(int skillIndex) => skillIndex switch {
        1 => SpecializationSkillLevel.Skill1,
        2 => SpecializationSkillLevel.Skill2,
        3 => SpecializationSkillLevel.Skill3,
        _ => 0,
    };

    private int MaxSelectedSkill()
    {
        for (int skillIndex = 3; skillIndex >= 1; --skillIndex)
        {
            if (GetSpecializationTarget(skillIndex) > 0)
            {
                return skillIndex;
            }
        }

        return 0;
    }

    /// <summary>专精可选技能数按稀有度过滤，规则与 CopilotViewModel 一致：3 技能需 6 星（或阿米娅），2 技能需 4 星。</summary>
    private static int GetMaxMasterySkill(string name)
    {
        var character = DataHelper.GetCharacterByNameOrAlias(name);
        int rarity = character?.Rarity ?? -1;
        return rarity >= 6 || character?.Id == "char_002_amiya" ? 3 : rarity >= 4 ? 2 : 1;
    }

    private static string ResolveDisplayName(string name) => DataHelper.GetLocalizedCharacterName(name) ?? name;
}
