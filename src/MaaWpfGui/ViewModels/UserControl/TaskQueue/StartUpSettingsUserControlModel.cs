// <copyright file="StartUpSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class StartUpSettingsUserControlModel : TaskSettingsViewModel
{
    static StartUpSettingsUserControlModel()
    {
        Instance = new();
    }

    public static StartUpSettingsUserControlModel Instance { get; }

    public string AccountName
    {
        get => GetTaskConfig<StartUpTask>().AccountName;
        set
        {
            value = value.Trim();
            SetTaskConfig<StartUpTask>(t => t.AccountName == value, t => t.AccountName = value);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void AccountSwitchManualRun()
    {
        _ = Instances.TaskQueueViewModel.QuickSwitchAccount();
    }

    public override void ProcSubTaskMsg(AsstMsg msg, JObject details)
    {
        if (msg == AsstMsg.SubTaskExtraInfo && details["what"]?.ToString() == "AccountSwitch")
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AccountSwitch") + $" -->> {details["details"]!["account_name"]}", UiLogColor.Info); // subTaskDetails!["current_account"]
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is StartUpTask)
        {
            Refresh();
        }
    }

    [Obsolete]
    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var clientType = SettingsViewModel.GameSettings.ClientType;
        var startGame = SettingsViewModel.GameSettings.StartGame;
        var accountName = clientType switch
        {
            "Official" or "Bilibili" => AccountName,
            _ => string.Empty,
        };

        var task = new AsstStartUpTask()
        {
            ClientType = clientType,
            StartGame = startGame,
            AccountName = accountName,
        };

        return task.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not StartUpTask startUp)
        {
            return null;
        }

        var clientType = SettingsViewModel.GameSettings.ClientType;
        var accountName = clientType switch
        {
            "Official" or "Bilibili" => startUp.AccountName,
            _ => string.Empty,
        };

        var task = new AsstStartUpTask()
        {
            ClientType = clientType,
            StartGame = !string.IsNullOrEmpty(clientType),
            AccountName = accountName,
        };

        return taskId switch
        {
            int id => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
            _ => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.StartUp, task),
        };
    }
}
