// <copyright file="TutorialChallengeUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.TutorialChallenge;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.Views.Dialogs;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Toolbox;

/// <summary>
/// 工具箱 ｢教程关卡｣ 清单页。完成态读取进度存档；进入关卡弹出非模态单例操作场窗口。
/// </summary>
public class TutorialChallengeUserControlModel : PropertyChangedBase
{
    private static TutorialChallengeLevelView? _levelWindow;
    private static TutorialChallengeShopView? _shopWindow;

    static TutorialChallengeUserControlModel()
    {
        Instance = new();
    }

    public static TutorialChallengeUserControlModel Instance { get; }

    private TutorialChallengeUserControlModel()
    {
        TutorialChallengeProgress.Load();
        LevelItems = [.. TutorialChallengeLevels.All.Select(level => new TutorialChallengeLevelItem(level))];
        RefreshCompletion();
        TutorialChallengeProgress.Changed += (_, _) => RefreshCompletion();
        LocalizationHelper.LanguageChanged += OnLanguageChanged;
    }

    /// <summary>
    /// Gets 关卡清单条目，顺序即建议学习路径，关卡间无解锁依赖。
    /// </summary>
    public ObservableCollection<TutorialChallengeLevelItem> LevelItems { get; }

    /// <summary>
    /// Gets 已通关关数。
    /// </summary>
    public int CompletedCount => LevelItems.Count(item => item.IsCompleted);

    /// <summary>
    /// Gets 关卡总数。
    /// </summary>
    public int TotalCount => LevelItems.Count;

    /// <summary>
    /// Gets 进度文本（已通关/总数）。
    /// </summary>
    public string ProgressText => $"{CompletedCount}/{TotalCount}";

    /// <summary>
    /// Gets 牛牛币当前总数（已通关关卡的难度值之和）。
    /// </summary>
    public int CoinCount => TutorialChallengeProgress.CoinCount;

    /// <summary>
    /// 通关状态筛选档位。
    /// </summary>
    public enum StatusFilterKind
    {
        /// <summary>全部关卡。</summary>
        All,

        /// <summary>仅未通关。</summary>
        Uncompleted,

        /// <summary>仅已通关。</summary>
        Completed,
    }

    /// <summary>
    /// 难度筛选档位。
    /// </summary>
    public enum DifficultyFilterKind
    {
        /// <summary>全部难度。</summary>
        All,

        /// <summary>仅低难度。</summary>
        Low,

        /// <summary>仅中难度。</summary>
        Medium,

        /// <summary>仅高难度。</summary>
        High,
    }

    private StatusFilterKind _statusFilter = StatusFilterKind.All;
    private DifficultyFilterKind _difficultyFilter = DifficultyFilterKind.All;

    /// <summary>
    /// Gets or sets a value indicating whether 状态筛选为全部（RadioButton 绑定）。
    /// </summary>
    public bool IsStatusAll
    {
        get => _statusFilter == StatusFilterKind.All;
        set
        {
            if (value)
            {
                SetStatusFilter(StatusFilterKind.All);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 状态筛选为未通关。
    /// </summary>
    public bool IsStatusUncompleted
    {
        get => _statusFilter == StatusFilterKind.Uncompleted;
        set
        {
            if (value)
            {
                SetStatusFilter(StatusFilterKind.Uncompleted);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 状态筛选为已通关。
    /// </summary>
    public bool IsStatusCompleted
    {
        get => _statusFilter == StatusFilterKind.Completed;
        set
        {
            if (value)
            {
                SetStatusFilter(StatusFilterKind.Completed);
            }
        }
    }

    private void SetStatusFilter(StatusFilterKind kind)
    {
        if (_statusFilter == kind)
        {
            return;
        }

        _statusFilter = kind;
        NotifyOfPropertyChange(nameof(IsStatusAll));
        NotifyOfPropertyChange(nameof(IsStatusUncompleted));
        NotifyOfPropertyChange(nameof(IsStatusCompleted));
        ApplyFilter();
    }

    /// <summary>
    /// Gets or sets a value indicating whether 难度筛选为全部（RadioButton 绑定）。
    /// </summary>
    public bool IsDifficultyAll
    {
        get => _difficultyFilter == DifficultyFilterKind.All;
        set
        {
            if (value)
            {
                SetDifficultyFilter(DifficultyFilterKind.All);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 难度筛选为低难度。
    /// </summary>
    public bool IsDifficultyLow
    {
        get => _difficultyFilter == DifficultyFilterKind.Low;
        set
        {
            if (value)
            {
                SetDifficultyFilter(DifficultyFilterKind.Low);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 难度筛选为中难度。
    /// </summary>
    public bool IsDifficultyMedium
    {
        get => _difficultyFilter == DifficultyFilterKind.Medium;
        set
        {
            if (value)
            {
                SetDifficultyFilter(DifficultyFilterKind.Medium);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 难度筛选为高难度。
    /// </summary>
    public bool IsDifficultyHigh
    {
        get => _difficultyFilter == DifficultyFilterKind.High;
        set
        {
            if (value)
            {
                SetDifficultyFilter(DifficultyFilterKind.High);
            }
        }
    }

    private void SetDifficultyFilter(DifficultyFilterKind kind)
    {
        if (_difficultyFilter == kind)
        {
            return;
        }

        _difficultyFilter = kind;
        NotifyOfPropertyChange(nameof(IsDifficultyAll));
        NotifyOfPropertyChange(nameof(IsDifficultyLow));
        NotifyOfPropertyChange(nameof(IsDifficultyMedium));
        NotifyOfPropertyChange(nameof(IsDifficultyHigh));
        ApplyFilter();
    }

    /// <summary>
    /// 按当前状态与难度筛选条件刷新各条目可见性，两个条件取交集。
    /// </summary>
    private void ApplyFilter()
    {
        foreach (var item in LevelItems)
        {
            var statusMatch = _statusFilter switch
            {
                StatusFilterKind.Uncompleted => !item.IsCompleted,
                StatusFilterKind.Completed => item.IsCompleted,
                _ => true,
            };
            var difficultyMatch = _difficultyFilter switch
            {
                DifficultyFilterKind.Low => item.Difficulty == TutorialChallengeDifficulty.Low,
                DifficultyFilterKind.Medium => item.Difficulty == TutorialChallengeDifficulty.Medium,
                DifficultyFilterKind.High => item.Difficulty == TutorialChallengeDifficulty.High,
                _ => true,
            };
            item.IsVisibleInFilter = statusMatch && difficultyMatch;
        }
    }

    /// <summary>
    /// 打开关卡操作场窗口；窗口非模态且全局单例，已打开时切换到新关卡。
    /// </summary>
    /// <param name="item">清单条目。</param>
    public void StartLevel(TutorialChallengeLevelItem item)
    {
        if (_levelWindow is null)
        {
            _levelWindow = new TutorialChallengeLevelView();
            _levelWindow.Loaded += (_, _) => Helper.WindowManager.MoveWindowToRootCenter(_levelWindow);
            _levelWindow.Closed += (_, _) =>
            {
                (_levelWindow?.DataContext as TutorialChallengeLevelViewModel)?.Detach();
                _levelWindow = null;
            };
        }

        (_levelWindow.DataContext as TutorialChallengeLevelViewModel)?.Detach();
        _levelWindow.DataContext = new TutorialChallengeLevelViewModel(item.Level);
        Helper.WindowManager.ShowWindow(_levelWindow);
    }

    /// <summary>
    /// 打开牛牛币商城窗口；非模态单例，已打开时激活前置。
    /// </summary>
    public void OpenShop()
    {
        if (_shopWindow is null)
        {
            _shopWindow = new TutorialChallengeShopView { DataContext = new TutorialChallengeShopViewModel() };
            _shopWindow.Loaded += (_, _) => Helper.WindowManager.MoveWindowToRootCenter(_shopWindow);
            _shopWindow.Closed += (_, _) => _shopWindow = null;
        }

        Helper.WindowManager.ShowWindow(_shopWindow);
    }

    /// <summary>
    /// 依据进度存档刷新全部关卡完成态与进度显示。
    /// </summary>
    private void RefreshCompletion()
    {
        foreach (var item in LevelItems)
        {
            item.RefreshCompletion();
        }

        ApplyFilter();
        NotifyOfPropertyChange(nameof(CompletedCount));
        NotifyOfPropertyChange(nameof(ProgressText));
        NotifyOfPropertyChange(nameof(CoinCount));
    }

    private void OnLanguageChanged()
    {
        foreach (var item in LevelItems)
        {
            item.RefreshLocalization();
        }
    }
}
