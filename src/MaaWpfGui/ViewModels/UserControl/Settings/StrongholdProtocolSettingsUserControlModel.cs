// <copyright file="StrongholdProtocolSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class StrongholdProtocolSettingsUserControlModel : PropertyChangedBase
{
    private const int MinUpgradeRounds = 2;
    private const int MaxUpgradeRounds = 9;
    private readonly MiniGame _miniGame;

    private StrongholdProtocolSettingsUserControlModel()
    {
        _miniGame = ConfigFactory.CurrentConfig.Toolbox.MiniGame;
        _strategy = Settings.Strategy;
        _coreMajorBond = Settings.CoreMajorBond;
        _coreMinorBond = Settings.CoreMinorBond;
        var plan = Settings.UpgradePlan ?? [];
        UpgradeRounds = Enumerable.Range(1, Math.Clamp(plan.Count, MinUpgradeRounds, MaxUpgradeRounds))
            .Select(round => new UpgradeRoundItem(round, round <= plan.Count ? plan[round - 1] : 0, SaveUpgradePlan))
            .ToArray();

        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    public static StrongholdProtocolSettingsUserControlModel Instance { get; } = new();

    internal StrongholdProtocolSettings Settings => _miniGame.StrongholdProtocol;

    public LocalizedObservableList<StrongholdProtocolStrategy> StrategyList { get; } = new(
        (StrongholdProtocolStrategy.Standard, "Toolbox.MiniGame.StrongholdProtocol.Strategy.Standard"),
        (StrongholdProtocolStrategy.Covenant, "Toolbox.MiniGame.StrongholdProtocol.Strategy.Covenant"));

    public LocalizedObservableList<int> BadgeTargetList { get; } = new(
        (3, "Toolbox.MiniGame.StrongholdProtocol.Target.Gold"),
        (6, "Toolbox.MiniGame.StrongholdProtocol.Target.Color"));

    public int BadgeTarget
    {
        get => _miniGame.StrongholdProtocolTarget;
        set {
            if (_miniGame.StrongholdProtocolTarget != value)
            {
                _miniGame.StrongholdProtocolTarget = value;
                NotifyOfPropertyChange();
            }
        }
    }

    public LocalizedObservableList<string> MajorBondList { get; } = new(
        (string.Empty, "Common.NotSelected"),
        ("乌萨斯", "乌萨斯"),
        ("阿戈尔", "阿戈尔"),
        ("谢拉格", "谢拉格"),
        ("萨尔贡", "萨尔贡"),
        ("莱塔尼亚", "莱塔尼亚"),
        ("维多利亚", "维多利亚"),
        ("卡西米尔", "卡西米尔"),
        ("伊比利亚", "伊比利亚"),
        ("叙拉古", "叙拉古"),
        ("炎", "炎"));

    public LocalizedObservableList<string> MinorBondList { get; } = new(
        (string.Empty, "Common.NotSelected"),
        ("不屈", "不屈"),
        ("坚守", "坚守"),
        ("精准", "精准"),
        ("突袭", "突袭"),
        ("灵巧", "灵巧"),
        ("奥术", "奥术"),
        ("迅捷", "迅捷"),
        ("聚合", "聚合"),
        ("远见", "远见"),
        ("助力", "助力"),
        ("奇迹", "奇迹"),
        ("独行", "独行"),
        ("调和", "调和"),
        ("协防干员", "协防干员"),
        ("投资人", "投资人"));

    private StrongholdProtocolStrategy _strategy;

    public StrongholdProtocolStrategy Strategy
    {
        get => _strategy;
        set {
            if (SetAndNotify(ref _strategy, value))
            {
                Settings.Strategy = value;
            }
        }
    }

    [PropertyDependsOn(nameof(Strategy))]
    public bool IsCovenantStrategy => Strategy == StrongholdProtocolStrategy.Covenant;

    private string _coreMajorBond;

    public string CoreMajorBond
    {
        get => _coreMajorBond;
        set {
            if (value != null && SetAndNotify(ref _coreMajorBond, value))
            {
                Settings.CoreMajorBond = value;
            }
        }
    }

    private string _coreMinorBond;

    public string CoreMinorBond
    {
        get => _coreMinorBond;
        set {
            if (value != null && SetAndNotify(ref _coreMinorBond, value))
            {
                Settings.CoreMinorBond = value;
            }
        }
    }

    public IReadOnlyList<UpgradeRoundItem> UpgradeRounds { get; private set => SetAndNotify(ref field, value); }

    [PropertyDependsOn(nameof(UpgradeRounds))]
    public bool CanAddUpgradeRound => UpgradeRounds.Count < MaxUpgradeRounds;

    [PropertyDependsOn(nameof(UpgradeRounds))]
    public bool CanRemoveUpgradeRound => UpgradeRounds.Count > MinUpgradeRounds;

    public void AddUpgradeRound()
    {
        if (CanAddUpgradeRound)
        {
            UpgradeRounds = [.. UpgradeRounds, new UpgradeRoundItem(UpgradeRounds.Count + 1, 0, SaveUpgradePlan)];
            SaveUpgradePlan();
        }
    }

    public void RemoveUpgradeRound()
    {
        if (CanRemoveUpgradeRound)
        {
            UpgradeRounds = UpgradeRounds.Take(UpgradeRounds.Count - 1).ToArray();
            SaveUpgradePlan();
        }
    }

    private void SaveUpgradePlan() => Settings.UpgradePlan = UpgradeRounds.Select(round => round.Target).ToList();

    private void RefreshLocalization()
    {
        StrategyList.RefreshLocalization();
        BadgeTargetList.RefreshLocalization();
        MajorBondList.RefreshLocalization();
        MinorBondList.RefreshLocalization();
        foreach (var round in UpgradeRounds)
        {
            round.RefreshDisplay();
        }
    }

    public class UpgradeRoundItem(int round, int target, Action save) : PropertyChangedBase
    {
        public string Display => string.Format(LocalizationHelper.GetString("TaskQueue.Task.SeasonalTask.UpgradePlan.Round"), round);

        public int Target
        {
            get;
            set {
                if (SetAndNotify(ref field, value))
                {
                    save();
                }
            }
        } = target;

        public void RefreshDisplay() => NotifyOfPropertyChange(nameof(Display));
    }
}
