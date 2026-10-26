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
using MaaWpfGui.Utilities.ValueType;
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
    /// <param name="reward">开局奖励，名称和效果随语言切换刷新。</param>
    public BlackFlowStartRewardPriorityItemViewModel(int index, GenericCombinedData<RoguelikeBlackFlowStartReward> reward)
    {
        Index = index;
        Reward = reward;
    }

    public int Index { get; set => SetAndNotify(ref field, value); }

    /// <summary>Gets 开局奖励，取自期望奖励、羽蛇奖励下拉框的同一项。</summary>
    public GenericCombinedData<RoguelikeBlackFlowStartReward> Reward { get; }
}
