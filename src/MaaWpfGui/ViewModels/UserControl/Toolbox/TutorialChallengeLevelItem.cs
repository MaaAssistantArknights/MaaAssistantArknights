// <copyright file="TutorialChallengeLevelItem.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using MaaWpfGui.Models.TutorialChallenge;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Toolbox;

/// <summary>
/// 关卡清单条目。
/// </summary>
public class TutorialChallengeLevelItem(TutorialChallengeLevel level) : PropertyChangedBase
{
    private bool _isCompleted;

    /// <summary>
    /// Gets 关卡定义。
    /// </summary>
    public TutorialChallengeLevel Level => level;

    /// <summary>
    /// Gets 关卡名。
    /// </summary>
    public string Title => LocalizationHelper.GetString(level.TitleKey);

    /// <summary>
    /// Gets 关卡目标描述。
    /// </summary>
    public string Goal => LocalizationHelper.GetString(level.GoalKey);

    /// <summary>
    /// Gets a value indicating whether 本关已通关（进度存档已记录）。
    /// </summary>
    public bool IsCompleted
    {
        get => _isCompleted;
        private set => SetAndNotify(ref _isCompleted, value);
    }

    /// <summary>
    /// 从进度存档同步完成态。
    /// </summary>
    public void RefreshCompletion() => IsCompleted = TutorialChallengeProgress.IsCleared(level.Id);

    /// <summary>
    /// 语言切换后刷新条目文案。
    /// </summary>
    public void RefreshLocalization()
    {
        NotifyOfPropertyChange(nameof(Title));
        NotifyOfPropertyChange(nameof(Goal));
    }
}
