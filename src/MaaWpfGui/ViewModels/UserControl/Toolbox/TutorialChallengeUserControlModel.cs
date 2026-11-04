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
/// 工具箱 ｢教程关卡｣ 清单页。关卡完成态即成就解锁态；进入关卡弹出非模态单例操作场窗口。
/// </summary>
public class TutorialChallengeUserControlModel : PropertyChangedBase
{
    private static TutorialChallengeLevelView? _levelWindow;

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
    /// Gets 牛牛币当前总数（即已通关的关卡数）。
    /// </summary>
    public int CoinCount => TutorialChallengeProgress.CoinCount;

    /// <summary>
    /// 打开关卡操作场窗口；窗口非模态且全局单例，已打开时切换到新关卡。
    /// </summary>
    /// <param name="item">清单条目。</param>
    public void StartLevel(TutorialChallengeLevelItem item)
    {
        if (_levelWindow is null)
        {
            _levelWindow = new TutorialChallengeLevelView();
            _levelWindow.Closed += (_, _) => _levelWindow = null;
        }

        _levelWindow.DataContext = new TutorialChallengeLevelViewModel(item.Level);
        _levelWindow.Show();
        _levelWindow.Activate();
    }

    /// <summary>
    /// 依据成就解锁态刷新全部关卡完成态与进度显示，关卡达成时由关卡视图模型回调。
    /// </summary>
    public void RefreshCompletion()
    {
        foreach (var item in LevelItems)
        {
            item.RefreshCompletion();
        }

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
