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
        // 打开页面即按配置当前主题填充模式列表，并把不属于该主题的存量模式归一到列表首项
        RefreshModeList();
    }

    private static Reclamation ReclamationConfig => ConfigFactory.CurrentConfig.Toolbox.MiniGame.Reclamation;

    /// <summary>
    /// Gets the list of reclamation themes.
    /// </summary>
    public LocalizedObservableList<Theme> ReclamationThemeList { get; } = new(
        (Theme.Fire, "ReclamationThemeFire", "ClosedStage"),
        (Theme.Tales, "ReclamationThemeTales", null),
        (Theme.RelaunchAnchor, "ReclamationThemeRelaunchAnchor", null));

    /// <summary>
    /// Gets or sets 生息演算主题["Tales"].
    /// </summary>
    public Theme ReclamationTheme
    {
        get => ReclamationConfig.Theme;
        set {
            if (ReclamationConfig.Theme == value)
            {
                return;
            }

            ReclamationConfig.Theme = value;
            NotifyOfPropertyChange();
            RefreshModeList();
            if (value == Theme.RelaunchAnchor)
            {
                ReclamationClearStore = false;
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
        NotifyOfPropertyChange(nameof(ReclamationMode));
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
        get => ReclamationConfig.Mode;
        set {
            if (ReclamationConfig.Mode != value)
            {
                ReclamationConfig.Mode = value;
                NotifyOfPropertyChange();
            }
        }
    }

    /// <summary>
    /// Gets or sets 要组装的支援道具
    /// </summary>
    public string ReclamationToolToCraft
    {
        get => ReclamationConfig.ToolToCraft;
        set {
            value = value.Replace('；', ';').Trim();
            if (ReclamationConfig.ToolToCraft != value)
            {
                ReclamationConfig.ToolToCraft = value;
                NotifyOfPropertyChange();
            }
        }
    }

    /// <summary>
    /// Gets the list of reclamation increment modes.
    /// </summary>
    public LocalizedObservableList<int> ReclamationIncrementModeList { get; } = new(
        (0, "ReclamationIncrementModeClick"),
        (1, "ReclamationIncrementModeHold"));

    /// <summary>
    /// Gets or sets 点击类型：0 连点；1 长按
    /// </summary>
    public int ReclamationIncrementMode
    {
        get => ReclamationConfig.IncrementMode;
        set {
            if (ReclamationConfig.IncrementMode != value)
            {
                ReclamationConfig.IncrementMode = value;
                NotifyOfPropertyChange();
            }
        }
    }

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int ReclamationMaxCraftCountPerRound
    {
        get => ReclamationConfig.MaxCraftCountPerRound;
        set {
            if (ReclamationConfig.MaxCraftCountPerRound != value)
            {
                ReclamationConfig.MaxCraftCountPerRound = value;
                NotifyOfPropertyChange();
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 刷完点数后是否清空商店
    /// </summary>
    public bool ReclamationClearStore
    {
        get => ReclamationConfig.ClearStore;
        set {
            if (ReclamationConfig.ClearStore != value)
            {
                ReclamationConfig.ClearStore = value;
                NotifyOfPropertyChange();
            }
        }
    }

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
                    return LocalizationHelper.GetString(
                        ReclamationMode == Mode.ProsperityInSave ? "ReclamationTipTalesInSave" : "ReclamationTipTalesNoSave");
                default:
                    return string.Empty;
            }
        }
    }

    /// <summary>
    /// 以当前配置提交生息演算任务并启动；远程控制 LinkStart-Reclamation 亦走此入口。
    /// </summary>
    /// <returns>任务是否成功启动。</returns>
    public bool AppendAndStartReclamationTask()
    {
        var config = ReclamationConfig;
        var toolToCraft = !string.IsNullOrEmpty(config.ToolToCraft)
            ? config.ToolToCraft
            : LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder", DataHelper.ClientLanguageMapper[SettingsViewModel.GameSettings.ClientType]);
        var task = new AsstReclamationTask {
            Theme = config.Theme,
            Mode = (int)config.Mode,
            IncrementMode = config.IncrementMode,
            MaxCraftCountPerRound = config.MaxCraftCountPerRound,
            ToolToCraft = [.. toolToCraft.Split(';').Select(s => s.Trim())],
            ClearStore = config.ClearStore,
        };

        return Instances.AsstProxy.AsstReclamation(task);
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
