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
using System.Collections.Generic;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class StartUpSettingsUserControlModel : TaskSettingsViewModel, StartUpSettingsUserControlModel.ISerialize
{
    static StartUpSettingsUserControlModel()
    {
        Instance = new();
    }

    public static StartUpSettingsUserControlModel Instance { get; }

    public string AccountName
    {
        get => GetTaskConfig<StartUpTask>().AccountName;
        set {
            value = value.Trim();
            SetTaskConfig<StartUpTask>(t => t.AccountName == value, t => t.AccountName = value);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public async void AccountSwitchManualRun()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is not StartUpTask startUp)
        {
            Instances.TaskQueueViewModel.AddLog("Current task is not StartUpTask", UiLogColor.Error);
            return;
        }
        var task = new StartUpTask() { AccountName = startUp.AccountName };
        await Instances.TaskQueueViewModel.LinkStartWithTasks([task]);
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

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not StartUpTask startUp)
            {
                return (null, []);
            }

            var clientType = SettingsViewModel.GameSettings.ClientType;
            var accountName = clientType switch {
                ClientType.Official or ClientType.Bilibili => startUp.AccountName,
                _ => string.Empty,
            };

            var task = new AsstStartUpTask() {
                ClientType = clientType,
                StartGame = SettingsViewModel.GameSettings.StartGame,
                AccountName = accountName,
            };

            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.StartUp, task)),
                _ => (null, []),
            };
        }
    }
}
