// <copyright file="AwardSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Windows;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class AwardSettingsUserControlModel : TaskSettingsViewModel
{
    static AwardSettingsUserControlModel()
    {
        Instance = new();
    }

    public static AwardSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets a value indicating whether receive award is enabled.
    /// </summary>
    public bool ReceiveAward
    {
        get => GetTaskConfig<AwardTask>().Award;
        set => SetTaskConfig<AwardTask>(t => t.Award == value, t => t.Award = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether receive mail is enabled.
    /// </summary>
    public bool ReceiveMail
    {
        get => GetTaskConfig<AwardTask>().Mail;
        set => SetTaskConfig<AwardTask>(t => t.Mail == value, t => t.Mail = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether receive mail is enabled.
    /// </summary>
    public bool ReceiveFreeGacha
    {
        get => GetTaskConfig<AwardTask>().FreeGacha;
        set {
            if (value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("GachaWarning"),
                    LocalizationHelper.GetString("Warning"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning,
                    no: LocalizationHelper.GetString("Confirm"),
                    yes: LocalizationHelper.GetString("Cancel"),
                    iconBrushKey: "DangerBrush");
                if (result != MessageBoxResult.No)
                {
                    return;
                }
            }

            SetTaskConfig<AwardTask>(t => t.FreeGacha == value, t => t.FreeGacha = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether receive orundum is enabled.
    /// </summary>
    public bool ReceiveOrundum
    {
        get => GetTaskConfig<AwardTask>().Orundum;
        set => SetTaskConfig<AwardTask>(t => t.Orundum == value, t => t.Orundum = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether receive mining is enabled.
    /// </summary>
    public bool ReceiveMining
    {
        get => GetTaskConfig<AwardTask>().Mining;
        set => SetTaskConfig<AwardTask>(t => t.Mining == value, t => t.Mining = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to collect special access rewards.
    /// </summary>
    public bool ReceiveSpecialAccess
    {
        get => GetTaskConfig<AwardTask>().SpecialAccess;
        set => SetTaskConfig<AwardTask>(t => t.SpecialAccess == value, t => t.SpecialAccess = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is AwardTask)
        {
            Refresh();
        }
    }

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null)
    {
        if (baseTask is not AwardTask award)
        {
            return null;
        }

        var task = new AsstAwardTask() {
            Award = award.Award,
            Mail = award.Mail,
            FreeGacha = award.FreeGacha,
            Orundum = award.Orundum,
            Mining = award.Mining,
            SpecialAccess = award.SpecialAccess,
        };
        return taskId switch {
            int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
            null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Award, task),
            _ => null,
        };
    }
}
