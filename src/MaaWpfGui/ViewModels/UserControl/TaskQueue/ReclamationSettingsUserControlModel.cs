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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;
using Mode = MaaWpfGui.Configuration.Single.MaaTask.ReclamationMode;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.ReclamationTheme;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class ReclamationSettingsUserControlModel : TaskViewModel
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
        ];

    /// <summary>
    /// Gets or sets the Reclamation theme.
    /// </summary>
    public Theme ReclamationTheme
    {
        get => GetTaskConfig<ReclamationTask>()?.Theme ?? default;
        set => SetTaskConfig<ReclamationTask>(t => t.Theme == value, t => t.Theme = value);
    }

    /// <summary>
    /// Gets the list of reclamation modes.
    /// </summary>
    public List<GenericCombinedData<Mode>> ReclamationModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityNoSave"), Value = Mode.NoArchive },
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityInSave"), Value = Mode.Archive },
        ];

    /// <summary>
    /// Gets or sets 策略，无存档刷生息点数 / 有存档刷生息点数
    /// </summary>
    public Mode ReclamationMode
    {
        get => GetTaskConfig<ReclamationTask>()?.Mode ?? default;
        set => SetTaskConfig<ReclamationTask>(t => t.Mode == value, t => t.Mode = value);
    }

    public string ReclamationToolToCraft
    {
        get => GetTaskConfig<ReclamationTask>()?.ToolToCraft ?? string.Empty;
        set
        {
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

    public int ReclamationIncrementMode
    {
        get => GetTaskConfig<ReclamationTask>()?.IncrementMode ?? default;
        set => SetTaskConfig<ReclamationTask>(t => t.IncrementMode == value, t => t.IncrementMode = value);
    }

    public int ReclamationMaxCraftCountPerRound
    {
        get => GetTaskConfig<ReclamationTask>()?.MaxCraftCountPerRound ?? default;
        set => SetTaskConfig<ReclamationTask>(t => t.MaxCraftCountPerRound == value, t => t.MaxCraftCountPerRound = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is ReclamationTask)
        {
            Refresh();
        }
    }

    public bool ReclamationClearStore
    {
        get => GetTaskConfig<ReclamationTask>()?.ClearStore ?? default;
        set => SetTaskConfig<ReclamationTask>(t => t.ClearStore == value, t => t.ClearStore = value);
    }

    /// <summary>
    /// 自动生息演算。
    /// </summary>
    /// <param name="theme">生息演算主题["Tales"]</param>
    /// <param name="mode">
    /// 策略。可用值包括：
    /// <list type="bullet">
    ///     <item>
    ///         <term><c>0</c></term>
    ///         <description>无存档时通过进出关卡刷生息点数</description>
    ///     </item>
    ///     <item>
    ///         <term><c>1</c></term>
    ///         <description>有存档时通过合成支援道具刷生息点数</description>
    ///     </item>
    /// </list>
    /// </param>
    /// <param name="increment_mode">点击类型：0 连点；1 长按</param>
    /// <param name="num_craft_batches">单次最大制造轮数</param>
    /// <param name="tools_to_craft">要组装的支援道具。</param>
    /// <param name="clear_store">刷完点数后是否清空商店。</param>
    /// <returns>返回(Asst任务类型, 参数)</returns>
    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var toolToCraft = !string.IsNullOrEmpty(ReclamationToolToCraft) ? ReclamationToolToCraft : LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);

        return new AsstReclamationTask()
        {
            Theme = ReclamationTheme,
            Mode = ReclamationMode,
            IncrementMode = ReclamationIncrementMode,
            MaxCraftCountPerRound = ReclamationMaxCraftCountPerRound,
            ToolToCraft = toolToCraft.Split(';').Select(s => s.Trim()).ToList(),
            ClearStore = ReclamationClearStore,
        }.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not ReclamationTask task)
        {
            return null;
        }

        var toolToCraft = !string.IsNullOrEmpty(task.ToolToCraft) ? task.ToolToCraft : LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);
        var asstTask = new AsstReclamationTask()
        {
            Theme = task.Theme,
            Mode = task.Mode,
            IncrementMode = task.IncrementMode,
            MaxCraftCountPerRound = task.MaxCraftCountPerRound,
            ToolToCraft = [.. toolToCraft.Split(';').Select(s => s.Trim())],
            ClearStore = task.ClearStore,
        };

        if (taskId is int id)
        {
            return Instances.AsstProxy.AsstSetTaskParamsEncoded(id, asstTask);
        }
        else
        {
            return Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Reclamation, asstTask);
        }
    }
}
