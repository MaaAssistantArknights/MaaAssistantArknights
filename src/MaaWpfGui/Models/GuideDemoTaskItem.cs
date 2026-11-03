// <copyright file="GuideDemoTaskItem.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Models;

/// <summary>
/// 设置指引 ｢任务设置｣ 步互动演示用的任务项，纯内存演示数据，不与配置的任务队列持久化交互。
/// 关卡、周计划、主题等演示设置字段随任务类型而定，仅对标注的类型有效，其余类型为默认值。
/// </summary>
public class GuideDemoTaskItem : PropertyChangedBase, IDisposable
{
    private string _customName = string.Empty;

    private bool _isChecked;

    private bool _enableSetting;

    private bool _gateBadgeVisible;

    private string _demoStage = string.Empty;

    private bool _demoUseWeeklySchedule;

    private bool _demoStartGame = true;

    /// <summary>
    /// Initializes a new instance of the <see cref="GuideDemoTaskItem"/> class.
    /// </summary>
    /// <param name="taskType">演示任务的任务类型。</param>
    public GuideDemoTaskItem(TaskType taskType)
    {
        TaskType = taskType;
        PropertyDependsOnUtility.InitializePropertyDependencies(this);

        // 周计划默认全不勾（真实默认全勾，照搬则引导的勾选操作无步骤可做）
        DemoWeeklySchedule = taskType == TaskType.Fight
            ? [.. Enum.GetValues<DayOfWeek>().Select(day => new WeeklyScheduleItem(day) { Value = false })]
            : [];

        DemoThemes = taskType == TaskType.SwitchTheme ? [] : null!;
    }

    /// <summary>
    /// Gets the demo task type，右侧演示设置面板与引导 gate 判定按此分支。
    /// </summary>
    public TaskType TaskType { get; }

    /// <summary>
    /// Gets a value indicating whether the task is a StartUp task（右侧设置面板切换用）。
    /// </summary>
    public bool IsStartUpTask => TaskType == TaskType.StartUp;

    /// <summary>
    /// Gets a value indicating whether the task is a Fight task（右侧设置面板切换用）。
    /// </summary>
    public bool IsFightTask => TaskType == TaskType.Fight;

    /// <summary>
    /// Gets a value indicating whether the task is a SwitchTheme task（右侧设置面板切换用）。
    /// </summary>
    public bool IsSwitchThemeTask => TaskType == TaskType.SwitchTheme;

    /// <summary>
    /// Gets the localization resource key of the task type；重命名不改变本值（名称走自定义文本），
    /// 复制产生的副本带本值，但副本必然携带自定义名，实际显示不经此处。
    /// </summary>
    public string LocalizationKey => TaskType.LocalizationKey();

    /// <summary>
    /// Gets the display name：重命名/复制产生的自定义名，否则按 <see cref="LocalizationKey"/> 实时取本地化名；
    /// 语言切换经 <see cref="PropertyDependsOnAttribute"/> 依赖语言设置属性自动重新求值。
    /// </summary>
    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    public string Name
    {
        get => _customName.Length > 0
            ? _customName
            : LocalizationHelper.GetString(LocalizationKey);
        set => SetAndNotify(ref _customName, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the demo task is checked.
    /// </summary>
    public bool IsChecked
    {
        get => _isChecked;
        set => SetAndNotify(ref _isChecked, value);
    }

    /// <summary>
    /// Gets a value indicating whether the task carries a custom name（重命名/复制产生），引导 gate 的重命名判定用。
    /// </summary>
    public bool HasCustomName => _customName.Length > 0;

    /// <summary>
    /// Gets or sets a value indicating whether the demo task's gear button is toggled on.
    /// 互斥单选，右侧设置面板跟随当前选中的任务切换。
    /// </summary>
    public bool EnableSetting
    {
        get => _enableSetting;
        set => SetAndNotify(ref _enableSetting, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the gate badge shows on this task item：
    /// 引导红点跟随 ｢下一个待操作步骤｣ 定位，由 <c>SettingsViewModel</c> 在 gate 重算时写入。
    /// </summary>
    public bool IsGateBadgeVisible
    {
        get => _gateBadgeVisible;
        set => SetAndNotify(ref _gateBadgeVisible, value);
    }

    /// <summary>
    /// Gets or sets the demo stage of a Fight task：空串为 ｢当前/上次｣，剿灭关卡值与 FightTaskUserControlModel.AnnihilationName 一致。仅 Fight 任务有效。
    /// </summary>
    public string DemoStage
    {
        get => _demoStage;
        set => SetAndNotify(ref _demoStage, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the demo weekly schedule of a Fight task is enabled. 仅 Fight 任务有效。
    /// </summary>
    public bool DemoUseWeeklySchedule
    {
        get => _demoUseWeeklySchedule;
        set => SetAndNotify(ref _demoUseWeeklySchedule, value);
    }

    /// <summary>
    /// Gets the demo weekly schedule of a Fight task，星期顺序与真实周计划一致（自周日起）。仅 Fight 任务有效，其余类型为空集合。
    /// </summary>
    public ObservableCollection<WeeklyScheduleItem> DemoWeeklySchedule { get; }

    /// <summary>
    /// Gets or sets a value indicating whether the demo StartUp task launches the game client，默认勾选（对齐真实默认）。仅 StartUp 任务有效。
    /// </summary>
    public bool DemoStartGame
    {
        get => _demoStartGame;
        set => SetAndNotify(ref _demoStartGame, value);
    }

    /// <summary>
    /// Gets the demo theme list of a SwitchTheme task. 仅 SwitchTheme 任务有效，其余类型为 null。
    /// </summary>
    public ObservableCollection<GuideDemoThemeItem> DemoThemes { get; }

    /// <summary>
    /// 清理依赖注册，避免被静态依赖表强引用持有（内存泄漏与语言切换时的僵尸通知）。
    /// </summary>
    void IDisposable.Dispose()
    {
        PropertyDependsOnUtility.UnInitializePropertyDependencies(this);
        GC.SuppressFinalize(this);
    }
}
