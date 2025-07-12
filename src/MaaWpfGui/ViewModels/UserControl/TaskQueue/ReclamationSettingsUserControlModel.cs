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
using System;
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Theme = MaaWpfGui.Configuration.Single.MaaTask.ReclamationTheme;
using Mode= MaaWpfGui.Configuration.Single.MaaTask.ReclamationMode;
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

    private Theme _reclamationTheme = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationTheme, Theme.Tales);

    /// <summary>
    /// Gets or sets the Reclamation theme.
    /// </summary>
    public Theme ReclamationTheme
    {
        get => _reclamationTheme;
        set
        {
            SetAndNotify(ref _reclamationTheme, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationTheme, value.ToString());
        }
    }

    /// <summary>
    /// Gets the list of reclamation modes.
    /// </summary>
    public List<GenericCombinedData<Mode>> ReclamationModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityNoSave"), Value = Mode.NoArchive },
            new() { Display = LocalizationHelper.GetString("ReclamationModeProsperityInSave"), Value = Mode.Archive },
        ];

    private Mode _reclamationMode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMode, Mode.Archive);

    /// <summary>
    /// Gets or sets 策略，无存档刷生息点数 / 有存档刷生息点数
    /// </summary>
    public Mode ReclamationMode
    {
        get => _reclamationMode;
        set
        {
            SetAndNotify(ref _reclamationMode, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationMode, value.ToString());
        }
    }

    private string _reclamationToolToCraft = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationToolToCraft, string.Empty).Replace('；', ';');

    public string ReclamationToolToCraft
    {
        get
        {
            if (string.IsNullOrEmpty(_reclamationToolToCraft))
            {
                return LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);
            }

            return _reclamationToolToCraft;
        }

        set
        {
            value = value.Replace('；', ';');
            SetAndNotify(ref _reclamationToolToCraft, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationToolToCraft, value);
        }
    }

    private int _reclamationIncrementMode = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationIncrementMode, "0"));

    public int ReclamationIncrementMode
    {
        get => _reclamationIncrementMode;
        set
        {
            SetAndNotify(ref _reclamationIncrementMode, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationIncrementMode, value.ToString());
        }
    }

    /// <summary>
    /// Gets the list of reclamation increment modes.
    /// </summary>
    public List<CombinedData> ReclamationIncrementModeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeClick"), Value = "0" },
            new() { Display = LocalizationHelper.GetString("ReclamationIncrementModeHold"), Value = "1" },
        ];

    private string _reclamationMaxCraftCountPerRound = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, "16");

    public int ReclamationMaxCraftCountPerRound
    {
        get => int.Parse(_reclamationMaxCraftCountPerRound);
        set
        {
            SetAndNotify(ref _reclamationMaxCraftCountPerRound, value.ToString());
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, value.ToString());
        }
    }

    private bool _reclamationClearStore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationClearStore, bool.FalseString));

    public bool ReclamationClearStore
    {
        get => _reclamationClearStore;
        set
        {
            SetAndNotify(ref _reclamationClearStore, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ReclamationClearStore, value.ToString());
        }
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
        return new AsstReclamationTask
        {
            Theme = ReclamationTheme,
            Mode = ReclamationMode,
            IncrementMode = ReclamationIncrementMode,
            MaxCraftCountPerRound = ReclamationMaxCraftCountPerRound,
            ToolToCraft = ReclamationToolToCraft.Split(';').Select(s => s.Trim()).ToList(),
            ClearStore = ReclamationClearStore,
        }.Serialize();
    }
}
