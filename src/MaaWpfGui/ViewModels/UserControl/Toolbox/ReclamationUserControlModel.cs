// <copyright file="ReclamationUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;
using DataHelper = MaaWpfGui.Helper.DataHelper;
using Mode = MaaWpfGui.Configuration.Single.Settings.ReclamationMode;
using Theme = MaaWpfGui.Configuration.Single.Settings.ReclamationTheme;

namespace MaaWpfGui.ViewModels.UserControl.Toolbox;

/// <summary>
/// 生息演算（牛杂条目）设置与启动。
/// </summary>
public class ReclamationUserControlModel : PropertyChangedBase
{
    static ReclamationUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
        PropertyDependsOnUtility.InitializePropertyDependencies(Instance);
    }

    public static ReclamationUserControlModel Instance { get; }

    private ReclamationUserControlModel()
    {
        // 构造时填充模式列表，避免打开页面时列表为空
        RefreshModeList();
    }

    private static Reclamation ReclamationConfig => ConfigFactory.CurrentConfig.Toolbox.MiniGame.Reclamation;

    /// <summary>
    /// Gets the list of reclamation themes.
    /// </summary>
    public LocalizedObservableList<Theme> ReclamationThemeList { get; } = new(
        (Theme.Fire, "Toolbox.MiniGame.Reclamation.Theme.Fire")
            .WithSecondaryKey("Toolbox.MiniGame.Reclamation.ClosedStage"),
        (Theme.Tales, "Toolbox.MiniGame.Reclamation.Theme.Tales"),
        (Theme.RelaunchAnchor, "Toolbox.MiniGame.Reclamation.Theme.RelaunchAnchor")
            .WithSecondaryKey("Common.NotAdapted", () => SettingsViewModel.GameSettings.ClientType is not (ClientType.Official or ClientType.Bilibili or ClientType.KR)));

    /// <summary>
    /// Gets or sets 生息演算主题["Tales"].
    /// </summary>
    public Theme ReclamationTheme
    {
        get; set {
            SetAndNotify(ref field, value);
            ReclamationConfig.Theme = value;
            RefreshModeList();
            if (value == Theme.RelaunchAnchor)
            {
                ReclamationClearStore = false;
            }
        }
    } = ReclamationConfig.Theme;

    public void RefreshModeList()
    {
        var mode = ReclamationMode;
        if (ReclamationTheme == Theme.Tales)
        {
            ModeList = [
                new() { Display = LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Mode.ProsperityNoSave"), Value = Mode.ProsperityNoSave },
                new() { Display = LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Mode.ProsperityInSave"), Value = Mode.ProsperityInSave },
            ];
        }
        else if (ReclamationTheme == Theme.RelaunchAnchor)
        {
            ModeList = [
                new() { Display = LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Mode.RA1"), Value = Mode.RA1 },
                new() { Display = LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Mode.RA4"), Value = Mode.RA4 },
                new() { Display = LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Mode.RA15"), Value = Mode.RA15 },
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
        get; set {
            SetAndNotify(ref field, value);
            ReclamationConfig.Mode = value;
        }
    } = ReclamationConfig.Mode;

    /// <summary>
    /// Gets or sets 要组装的支援道具
    /// </summary>
    public string ReclamationToolToCraft
    {
        get; set {
            value = value.Replace('；', ';').Trim();
            SetAndNotify(ref field, value);
            ReclamationConfig.ToolToCraft = value;
        }
    } = ReclamationConfig.ToolToCraft;

    /// <summary>
    /// Gets the list of reclamation increment modes.
    /// </summary>
    public LocalizedObservableList<int> ReclamationIncrementModeList { get; } = new(
        (0, "Toolbox.MiniGame.Reclamation.IncrementMode.Click"),
        (1, "Toolbox.MiniGame.Reclamation.IncrementMode.Hold"));

    /// <summary>
    /// Gets or sets 点击类型：0 连点；1 长按
    /// </summary>
    public int ReclamationIncrementMode
    {
        get; set {
            SetAndNotify(ref field, value);
            ReclamationConfig.IncrementMode = value;
        }
    } = ReclamationConfig.IncrementMode;

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int ReclamationMaxCraftCountPerRound
    {
        get; set {
            SetAndNotify(ref field, value);
            ReclamationConfig.MaxCraftCountPerRound = value;
        }
    } = ReclamationConfig.MaxCraftCountPerRound;

    /// <summary>
    /// Gets or sets a value indicating whether 刷完点数后是否清空商店
    /// </summary>
    public bool ReclamationClearStore
    {
        get; set {
            SetAndNotify(ref field, value);
            ReclamationConfig.ClearStore = value;
        }
    } = ReclamationConfig.ClearStore;

    /// <summary>
    /// Gets the theme-specific tip text.
    /// </summary>
    [PropertyDependsOn(nameof(ReclamationTheme), nameof(ReclamationMode))]
    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    public string ReclamationTip
    {
        get {
            var theme = ReclamationTheme;
            var mode = ReclamationMode;

            switch (theme)
            {
                case Theme.Fire:
                    return LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.Fire.Tip");
                case Theme.RelaunchAnchor:
                    {
                        var stageNum = mode switch {
                            Mode.RA15 => 15,
                            Mode.RA4 => 4,
                            _ => 1,
                        };
                        var stageTipKey = $"Toolbox.MiniGame.Reclamation.RelaunchAnchor.RA{stageNum}";
                        if (LocalizationHelper.TryGetString(stageTipKey, out var stageTip))
                        {
                            return stageTip;
                        }

                        return LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.RelaunchAnchor.RA1");
                    }

                case Theme.Tales:
                    return LocalizationHelper.GetString(
                        ReclamationMode == Mode.ProsperityInSave ? "Toolbox.MiniGame.Reclamation.TalesInSave.Tip" : "Toolbox.MiniGame.Reclamation.TalesNoSave.Tip");
                default:
                    return string.Empty;
            }
        }
    }

    /// <summary>
    /// 以当前设置提交并启动生息演算任务。
    /// </summary>
    /// <returns>任务是否成功启动。</returns>
    public bool AppendAndStartReclamationTask()
    {
        var toolToCraft = !string.IsNullOrEmpty(ReclamationToolToCraft)
            ? ReclamationToolToCraft
            : LocalizationHelper.GetString("Toolbox.MiniGame.Reclamation.ToolToCraft.Placeholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);
        var task = new AsstReclamationTask {
            Theme = ReclamationTheme,
            Mode = (int)ReclamationMode,
            IncrementMode = ReclamationIncrementMode,
            MaxCraftCountPerRound = ReclamationMaxCraftCountPerRound,
            ToolToCraft = [.. toolToCraft.Split(';').Select(s => s.Trim())],
            ClearStore = ReclamationClearStore,
        };

        return Instances.AsstProxy.AsstReclamation(task) && Instances.AsstProxy.AsstStart();
    }

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        ReclamationThemeList.RefreshLocalization();
        RefreshModeList();
        ReclamationIncrementModeList.RefreshLocalization();
    }
}
