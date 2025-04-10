// <copyright file="AwardSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Windows;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class AwardSettingsUserControlModel : TaskViewModel
{
    static AwardSettingsUserControlModel()
    {
        Instance = new();
    }

    public static AwardSettingsUserControlModel Instance { get; }

    public bool ReceiveAward
    {
        get => GetConfigTask<AwardTask>().Award;
        set
        {
            GetConfigTask<AwardTask>().Award = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ReceiveMail
    {
        get => GetConfigTask<AwardTask>().Mail;
        set
        {
            GetConfigTask<AwardTask>().Mail = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ReceiveFreeGacha
    {
        get => GetConfigTask<AwardTask>().FreeGacha;
        set
        {
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

            GetConfigTask<AwardTask>().FreeGacha = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ReceiveOrundum
    {
        get => GetConfigTask<AwardTask>().Orundum;
        set
        {
            GetConfigTask<AwardTask>().Orundum = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ReceiveMining
    {
        get => GetConfigTask<AwardTask>().Mining;
        set
        {
            GetConfigTask<AwardTask>().Mining = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ReceiveSpecialAccess
    {
        get => GetConfigTask<AwardTask>().SpecialAccess;
        set
        {
            GetConfigTask<AwardTask>().SpecialAccess = value;
            NotifyOfPropertyChange();
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is AwardTask)
        {
            Refresh();
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstAwardTask()
        {
            Award = ReceiveAward,
            Mail = ReceiveMail,
            FreeGacha = ReceiveFreeGacha,
            Orundum = ReceiveOrundum,
            Mining = ReceiveMining,
            SpecialAccess = ReceiveSpecialAccess,
        };
        return task.Serialize();
    }
}
