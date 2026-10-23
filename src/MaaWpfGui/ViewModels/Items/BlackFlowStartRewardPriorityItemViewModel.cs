// <copyright file="BlackFlowStartRewardPriorityItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>黑流树海凹第一层实托邦的开局奖励选择优先级中的一项。</summary>
public class BlackFlowStartRewardPriorityItemViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="BlackFlowStartRewardPriorityItemViewModel"/> class.
    /// 初始化奖励条目。
    /// </summary>
    /// <param name="index">列表序号。</param>
    /// <param name="reward">开局奖励。</param>
    public BlackFlowStartRewardPriorityItemViewModel(int index, RoguelikeBlackFlowStartReward reward)
    {
        Index = index;
        Reward = reward;
        RefreshLocalizedText();
    }

    public int Index { get; set => SetAndNotify(ref field, value); }

    public RoguelikeBlackFlowStartReward Reward { get; }

    /// <summary>Gets 奖励名，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string Display { get; private set => SetAndNotify(ref field, value); } = string.Empty;

    /// <summary>Gets 奖励效果，作为悬浮提示，语言切换后由 <see cref="RefreshLocalizedText"/> 刷新。</summary>
    public string Tip { get; private set => SetAndNotify(ref field, value); } = string.Empty;

    /// <summary>语言切换后刷新奖励名与效果。</summary>
    public void RefreshLocalizedText()
    {
        Display = LocalizationHelper.GetString($"RoguelikeBlackFlowReward{Reward}");
        Tip = LocalizationHelper.GetString($"RoguelikeBlackFlowReward{Reward}Tip");
    }
}
