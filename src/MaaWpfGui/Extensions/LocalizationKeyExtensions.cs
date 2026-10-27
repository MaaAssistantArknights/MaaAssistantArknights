// <copyright file="LocalizationKeyExtensions.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Extensions;

/// <summary>
/// 枚举成员到本地化资源 key 的显式映射，取代以枚举成员名直接当资源 key 的动态查找。
/// 无映射的成员返回成员名原值，保持查找失败时的回退显示与旧行为一致。
/// </summary>
public static class LocalizationKeyExtensions
{
    /// <summary>
    /// Gets the localization resource key of the infrast room type.
    /// </summary>
    /// <param name="roomType">The infrast room type.</param>
    /// <returns>The localization resource key.</returns>
    public static string LocalizationKey(this InfrastRoomType roomType) => roomType switch
    {
        InfrastRoomType.Mfg => "Infrast.Room.Mfg",
        InfrastRoomType.Trade => "Infrast.Room.Trade",
        InfrastRoomType.Control => "Infrast.Room.Control",
        InfrastRoomType.Power => "Infrast.Room.Power",
        InfrastRoomType.Reception => "Infrast.Room.Reception",
        InfrastRoomType.Office => "Infrast.Room.Office",
        InfrastRoomType.Recycle => "Infrast.Room.Recycle",
        InfrastRoomType.Dorm => "Infrast.Room.Dorm",
        InfrastRoomType.Processing => "Infrast.Room.Processing",
        InfrastRoomType.Training => "Infrast.Room.Training",
        InfrastRoomType.AssistantChange => "Infrast.Room.AssistantChange",
        _ => roomType.ToString(),
    };

    /// <summary>
    /// Gets the localization resource key of the task type.
    /// CloseDown 复用完成后动作的退出文案，RecruitCalc 复用工具箱公招识别页签名。
    /// </summary>
    /// <param name="taskType">The WPF task type.</param>
    /// <returns>The localization resource key.</returns>
    public static string LocalizationKey(this TaskType taskType) => taskType switch
    {
        TaskType.StartUp => "TaskQueue.Task.StartUp",
        TaskType.Fight => "TaskQueue.Task.Fight",
        TaskType.OperProgress => "TaskQueue.Task.OperProgress",
        TaskType.Recruit => "TaskQueue.Task.Recruit",
        TaskType.Infrast => "TaskQueue.Task.Infrast",
        TaskType.Mall => "TaskQueue.Task.Mall",
        TaskType.Award => "TaskQueue.Task.Award",
        TaskType.Roguelike => "TaskQueue.Task.Roguelike",
        TaskType.Copilot => "Copilot.Title",
        TaskType.CloseDown => "TaskQueue.PostAction.ExitArknights",
        TaskType.Depot => "TaskQueue.Task.Depot",
        TaskType.OperBox => "TaskQueue.Task.OperBox",
        TaskType.Gacha => "TaskQueue.Task.Gacha",
        TaskType.Reclamation => "Toolbox.MiniGame.Reclamation.Title",
        TaskType.UserDataUpdate => "TaskQueue.Task.UserDataUpdate",
        TaskType.DepotMaintain => "TaskQueue.Task.DepotMaintain",
        TaskType.SwitchTheme => "TaskQueue.Task.SwitchTheme",
        TaskType.MiniGame => "Toolbox.MiniGame.Title",
        TaskType.Custom => "TaskQueue.Task.Custom",
        TaskType.RecruitCalc => "Recruit.Recognition.Title",
        _ => taskType.ToString(),
    };

    /// <summary>
    /// Gets the localization resource key of the operator role.
    /// Unknown、Trap 无对应资源 key；Token 与 Drone 为同值别名，落入 Drone 分支。
    /// </summary>
    /// <param name="role">The operator role.</param>
    /// <returns>The localization resource key.</returns>
    public static string LocalizationKey(this OperatorRole role) => role switch
    {
        OperatorRole.Warrior => "Game.Role.Warrior",
        OperatorRole.Pioneer => "Game.Role.Pioneer",
        OperatorRole.Medic => "Game.Role.Medic",
        OperatorRole.Tank => "Game.Role.Tank",
        OperatorRole.Sniper => "Game.Role.Sniper",
        OperatorRole.Caster => "Game.Role.Caster",
        OperatorRole.Support => "Game.Role.Support",
        OperatorRole.Special => "Game.Role.Special",
        OperatorRole.Drone => "Game.Role.Drone",
        _ => role.ToString(),
    };

    /// <summary>
    /// Gets the localization resource key of the user data update trigger interval.
    /// </summary>
    /// <param name="interval">The trigger interval.</param>
    /// <returns>The localization resource key.</returns>
    public static string LocalizationKey(this UserDataUpdateTriggerInterval interval) => interval switch
    {
        UserDataUpdateTriggerInterval.EveryTime => "TaskQueue.UserDataUpdate.EveryTime",
        UserDataUpdateTriggerInterval.Daily => "TaskQueue.UserDataUpdate.Daily",
        UserDataUpdateTriggerInterval.Weekly => "TaskQueue.UserDataUpdate.Weekly",
        _ => interval.ToString(),
    };

    /// <summary>
    /// Gets the localization resource key of the settings page.
    /// </summary>
    /// <param name="settingKey">The settings page key.</param>
    /// <returns>The localization resource key.</returns>
    public static string LocalizationKey(this SettingKey settingKey) => settingKey switch
    {
        SettingKey.SwitchConfiguration => "Settings.Configuration.Title",
        SettingKey.ScheduleSettings => "Settings.Schedule.Title",
        SettingKey.PerformanceSettings => "Settings.Performance.Title",
        SettingKey.GameSettings => "Settings.Game.Title",
        SettingKey.ConnectionSettings => "Settings.Connection.Title",
        SettingKey.StartupSettings => "Settings.Startup.Title",
        SettingKey.RemoteControlSettings => "Settings.RemoteControl.Title",
        SettingKey.UiSettings => "Settings.Ui.Title",
        SettingKey.NotificationSettings => "Settings.Notification.Title",
        SettingKey.ThirdPartyServiceSettings => "Settings.ThirdParty.Title",
        SettingKey.HotKeySettings => "Settings.HotKey.Title",
        SettingKey.AchievementSettings => "Settings.Achievement.Title",
        SettingKey.UpdateSettings => "Settings.Update.Title",
        SettingKey.IssueReport => "Settings.IssueReport.Title",
        SettingKey.AboutUs => "Settings.About.Title",
        _ => settingKey.ToString(),
    };

    /// <summary>
    /// 将 Core 侧 CopilotAction 上报的动作名映射为本地化 key；未收录的动作名原样返回，
    /// GetString 落空时显示占位符，便于发现 Core 新增动作的遗漏。
    /// </summary>
    /// <param name="action">Core 侧 BattleProcessTask 上报的动作名（Deploy/UseSkill 等）</param>
    /// <returns>动作名对应的本地化 key；未收录时返回动作名原文</returns>
    public static string ActionLocalizationKey(string action)
    {
        return action switch {
            "Deploy" => "Copilot.Action.Deploy",
            "UseSkill" => "Copilot.Action.UseSkill",
            "Retreat" => "Copilot.Action.Retreat",
            "SkillDaemon" => "Copilot.Action.SkillDaemon",
            "SwitchSpeed" => "Copilot.Action.SwitchSpeed",
            "SkillUsage" => "Copilot.Action.SkillUsage",
            "BulletTime" => "Copilot.Action.BulletTime",
            "Output" => "Copilot.Action.Output",
            "MoveCamera" => "Copilot.Action.MoveCamera",
            "DrawCard" => "Copilot.Action.DrawCard",
            "CheckIfStartOver" => "Copilot.Action.CheckIfStartOver",
            "ResetStopwatch" => "Copilot.Action.ResetStopwatch",
            "Click" => "Copilot.Action.Click",
            "Swipe" => "Copilot.Action.Swipe",
            "SetUnitLocation" => "Copilot.Action.SetUnitLocation",
            _ => action,
        };
    }

    /// <summary>
    /// Maps a core task chain name to its localization resource key. 仅用于回调链名的显示查找，
    /// 发给内核的协议值不经此转换。与 <see cref="TaskType"/> 成员一致的链名经
    /// <see cref="LocalizationKey(TaskType)"/> 映射；仅 Copilot 页直启的合成链名在此显式列出。
    /// 未收录的链名返回原值，GetString 落空时显示占位符，便于发现遗漏。
    /// </summary>
    /// <param name="taskChain">The core task chain name.</param>
    /// <returns>The localization resource key.</returns>
    public static string TaskChainLocalizationKey(string taskChain) => taskChain switch
    {
        "SSSCopilot" => "Copilot.SSS",
        "ParadoxCopilot" => "Copilot.ParadoxSimulation",
        "SingleStep" => "Copilot.SingleStep",
        _ when Enum.TryParse<TaskType>(taskChain, out var type) => type.LocalizationKey(),
        _ => taskChain,
    };

    /// <summary>
    /// Maps a core infrast facility name to its localization resource key. 仅用于回调设施名的显示查找，
    /// 发给内核的协议值不经此转换。解析失败时返回原值。
    /// </summary>
    /// <param name="facility">The core infrast facility name, same member set as <see cref="InfrastRoomType"/>.</param>
    /// <returns>The localization resource key.</returns>
    public static string FacilityLocalizationKey(string facility)
    {
        return Enum.TryParse<InfrastRoomType>(facility, out var roomType) ? roomType.LocalizationKey() : facility;
    }
}
