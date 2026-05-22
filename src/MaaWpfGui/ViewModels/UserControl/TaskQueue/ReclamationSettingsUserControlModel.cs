// <copyright file="ReclamationSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using static MaaWpfGui.Main.AsstProxy;
using Mode = MaaWpfGui.Configuration.Single.MaaTask.ReclamationMode;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.ReclamationTheme;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class ReclamationSettingsUserControlModel : TaskSettingsViewModel, ReclamationSettingsUserControlModel.ISerialize
{
    static ReclamationSettingsUserControlModel()
    {
        Instance = new();
    }

    public static ReclamationSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets the list of reclamation themes.
    /// </summary>
    public List<GenericCombinedData<Theme>> ReclamationThemeList { get; } =
        [
            new() { Display = $"{LocalizationHelper.GetString("ReclamationThemeFire")} ({LocalizationHelper.GetString("ClosedStage")})", Value = Theme.Fire },
            new() { Display = LocalizationHelper.GetString("ReclamationThemeTales"), Value = Theme.Tales },
            new() { Display = LocalizationHelper.GetString("ReclamationThemeRelaunchAnchor"), Value = Theme.RelaunchAnchor },
        ];

    /// <summary>
    /// Gets or sets 生息演算主题["Tales"].
    /// </summary>
    public Theme ReclamationTheme
    {
        get => GetTaskConfig<ReclamationTask>().Theme;
        set {
            if (SetTaskConfig<ReclamationTask>(t => t.Theme == value, t => t.Theme = value))
            {
                RefreshModeList();
                if (value == Theme.RelaunchAnchor)
                {
                    ReclamationClearStore = false;
                }

                // 主题变更后刷新高级设置可见性
                TaskSettingVisibilityInfo.Instance.RefreshAdvancedSettingsVisibility();
            }
        }
    }

    public void RefreshModeList()
    {
        var mode = ReclamationMode;
        if (ReclamationTheme == Theme.Tales)
        {
            ModeList = [
                new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityNoSave"), Value = Mode.ProsperityNoSave },
                new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityInSave"), Value = Mode.ProsperityInSave },
            ];
        }
        else if (ReclamationTheme == Theme.RelaunchAnchor)
        {
            ModeList = [
                new() { Display = LocalizationHelper.GetString("ReclamationModeRA1"), Value = Mode.RA1 },
                new() { Display = LocalizationHelper.GetString("ReclamationModeRA4"), Value = Mode.RA4 },
                new() { Display = LocalizationHelper.GetString("ReclamationModeRA15"), Value = Mode.RA15 },
            ];
        }
        ReclamationMode = ModeList.Any(x => x.Value == mode) ? mode : ModeList.FirstOrDefault()?.Value ?? default;
    }

    /// <summary>
    /// Gets 生息演算模式列表
    /// </summary>
    public List<GenericCombinedData<Mode>> ModeList { get => field; private set => SetAndNotify(ref field, value); } = [];

    /// <summary>
    /// Gets or sets 生息演算模式（含义由主题决定）
    /// </summary>
    [PropertyDependsOn(nameof(ReclamationTheme))]
    public Mode ReclamationMode
    {
        get => GetTaskConfig<ReclamationTask>().Mode;
        set => SetTaskConfig<ReclamationTask>(t => t.Mode == value, t => t.Mode = value);
    }

    /// <summary>
    /// Gets or sets 要组装的支援道具
    /// </summary>
    public string ReclamationToolToCraft
    {
        get => GetTaskConfig<ReclamationTask>().ToolToCraft;
        set {
            value = value.Replace('；', ';').Trim();
            SetTaskConfig<ReclamationTask>(t => t.ToolToCraft == value, t => t.ToolToCraft = value);
        }
    }

    /// <summary>
    /// Gets the list of reclamation increment modes.
    /// </summary>
    public List<GenericCombinedData<int>> ReclamationIncrementModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeClick"), Value = 0 },
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeHold"), Value = 1 },
        ];

    /// <summary>
    /// Gets or sets 点击类型：0 连点；1 长按
    /// </summary>
    public int ReclamationIncrementMode
    {
        get => GetTaskConfig<ReclamationTask>().IncrementMode;
        set => SetTaskConfig<ReclamationTask>(t => t.IncrementMode == value, t => t.IncrementMode = value);
    }

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int ReclamationMaxCraftCountPerRound
    {
        get => GetTaskConfig<ReclamationTask>().MaxCraftCountPerRound;
        set => SetTaskConfig<ReclamationTask>(t => t.MaxCraftCountPerRound == value, t => t.MaxCraftCountPerRound = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 刷完点数后是否清空商店
    /// </summary>
    public bool ReclamationClearStore
    {
        get => GetTaskConfig<ReclamationTask>().ClearStore;
        set => SetTaskConfig<ReclamationTask>(t => t.ClearStore == value, t => t.ClearStore = value);
    }

    /// <summary>
    /// Gets the theme-specific tip text.
    /// </summary>
    [PropertyDependsOn(nameof(ReclamationTheme), nameof(ReclamationMode))]
    public string ReclamationTip
    {
        get {
            var theme = ReclamationTheme;
            var mode = ReclamationMode;

            switch (theme)
            {
                case Theme.Fire:
                    return LocalizationHelper.GetString("ReclamationTipFire");
                case Theme.RelaunchAnchor:
                    {
                        // mode 0 = RA-1, mode 1 = RA-4, mode 2 = RA-15
                        var stageNum = mode switch
                        {
                            Mode.RA15 => 15,
                            Mode.RA4 => 4,
                            _ => 1,
                        };
                        var stageTipKey = $"ReclamationTipRelaunchAnchorRA{stageNum}";
                        if (LocalizationHelper.TryGetString(stageTipKey, out var stageTip))
                        {
                            return stageTip;
                        }

                        return LocalizationHelper.GetString("ReclamationTipRelaunchAnchorRA1");
                    }

                case Theme.Tales:
                    return LocalizationHelper.GetString("ReclamationTipTales");
                default:
                    return string.Empty;
            }
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is ReclamationTask)
        {
            RefreshModeList();
            Refresh();
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not ReclamationTask reclamation)
            {
                return (null, []);
            }

            var toolToCraft = !string.IsNullOrEmpty(reclamation.ToolToCraft) ? reclamation.ToolToCraft : LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);
            var task = new AsstReclamationTask() {
                Theme = reclamation.Theme,
                Mode = (int)reclamation.Mode,
                IncrementMode = reclamation.IncrementMode,
                MaxCraftCountPerRound = reclamation.MaxCraftCountPerRound,
                ToolToCraft = [.. toolToCraft.Split(';').Select(s => s.Trim())],
                ClearStore = reclamation.ClearStore,
            };

            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Reclamation, task)),
                _ => (null, []),
            };
        }
    }
}
