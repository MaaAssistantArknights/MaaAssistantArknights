// <copyright file="StartUpTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class StartUpTaskUserControlModel : TaskSettingsViewModel, StartUpTaskUserControlModel.ISerialize
{
    static StartUpTaskUserControlModel()
    {
        Instance = new();
        Instances.AsstProxy.AsstSubTaskMsgEvent += Instance.ProcSubTaskMsg;
    }

    public static StartUpTaskUserControlModel Instance { get; }

    /// <summary>
    /// Gets a value indicating whether 当前连接配置非 PC 端（启动客户端开关与账号切换分区仅对非 PC 配置显示）。
    /// 默认转发真实连接设置，通知由跨实例属性依赖机制接管；沙盒子类可覆写为固定演示值。
    /// </summary>
    [PropertyDependsOn(typeof(ConnectSettingsUserControlModel), nameof(ConnectSettingsUserControlModel.ConnectConfig))]
    public virtual bool IsNotPcConnectConfig => ConnectSettingsUserControlModel.Instance.ConnectConfig != ConnectConfig.PC;

    /// <summary>
    /// Gets a value indicating whether 当前客户端为国服系（官服/B 服/台服/韩服，账号相关设置仅对这些客户端显示）。
    /// 默认转发真实游戏设置，通知由跨实例属性依赖机制接管；沙盒子类可覆写为固定演示值。
    /// </summary>
    [PropertyDependsOn(typeof(GameSettingsUserControlModel), nameof(GameSettingsUserControlModel.ClientType))]
    public virtual bool IsOfficialLikeClient => GameSettingsUserControlModel.Instance.ClientType is ClientType.Official or ClientType.Bilibili or ClientType.Txwy or ClientType.KR;

    public string AccountName
    {
        get => GetTaskConfig<StartUpTask>().AccountName;
        set {
            value = value.Trim();
            SetTaskConfig<StartUpTask>(t => t.AccountName == value, t => t.AccountName = value);
        }
    }

    public bool AccountSwitchEnabled
    {
        get => GetTaskConfig<StartUpTask>().AccountSwitchEnabled ?? false;
        set => SetTaskConfig<StartUpTask>(t => t.AccountSwitchEnabled == value, t => t.AccountSwitchEnabled = value);
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
        var task = new StartUpTask() { AccountSwitchEnabled = true, AccountName = startUp.AccountName };
        await Instances.TaskQueueViewModel.LinkStartWithTasks([task]);
    }

    public void ProcSubTaskMsg(AsstMsg msg, AsstSubTaskMsg? details)
    {
        if (msg == AsstMsg.SubTaskExtraInfo && details?.What == "AccountSwitch")
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Connection.AccountSwitch") + $" -->> {details?.Details?["account_name"]}", UiLogColor.Info); // subTaskDetails!["current_account"]
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
            var accountName = !SettingsViewModel.ConnectSettings.IsPCConnectConfig &&
                clientType is ClientType.Official or ClientType.Bilibili or ClientType.Txwy or ClientType.KR &&
                startUp.AccountSwitchEnabled is true
                    ? startUp.AccountName
                    : string.Empty;

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
