// <copyright file="FightTaskUserControlModel.Callback.cs" company="MaaAssistantArknights">
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
using System;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using static MaaWpfGui.Helper.Instances.Data;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public partial class FightTaskUserControlModel
{
    private static void ProcSubTaskMsg(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (type != AsstMsg.SubTaskExtraInfo || msg is null)
        {
            return;
        }

        switch (msg.What)
        {
            case "UseMedicine":
                var report = msg.Details?.ToObject<MedicineUsingInfo>();
                if (report is null)
                {
                    break;
                }

                string medicineLog;
                if (!report.IsExpiring)
                {
                    MedicineUsedTimes += report.Count;
                    medicineLog = LocalizationHelper.GetString("MedicineUsed") + $" {MedicineUsedTimes}(+{report.Count})";
                }
                else
                {
                    ExpiringMedicineUsedTimes += report.Count;
                    var item = Instances.TaskQueueViewModel.TaskItemViewModels.FirstOrDefault(i => i.TaskIds.Contains(msg.TaskId));
                    var expireOut = "--";
                    if (item is not null && item.Index >= 0 && item.Index < ConfigFactory.CurrentConfig.TaskQueue.Count)
                    {
                        if (ConfigFactory.CurrentConfig.TaskQueue[item.Index] is FightTask fightTask)
                        {
                            var yjTime = DateTimeOffset.Now.ToYjDateTime().ToLocalTime();
                            var daysUntilEndOfWeek = ((7 - (int)yjTime.DayOfWeek + 7) % 7) + 1; // 距离本周结束的天数, 用鹰历计算
                            var expireDays = Math.Max(
                                fightTask.UseExpiringMedicine
                                    ? fightTask.MedicineExpireDays : 0,
                                Instance.ActivityExpireIn2Days && fightTask.UseExpireMedicineForActivity
                                    ? daysUntilEndOfWeek : 0);
                            expireOut = $"{expireDays}";
                        }
                    }
                    medicineLog = LocalizationHelper.GetStringFormat("ExpiringMedicineUsed", expireOut) + $" {ExpiringMedicineUsedTimes}(+{report.Count})";
                    AchievementTrackerHelper.Instance.SetProgress(AchievementIds.SanityExpire, ExpiringMedicineUsedTimes);
                }

                AchievementTrackerHelper.Instance.AddProgressToGroup(AchievementIds.SanitySaverGroup, report.Count);
                if (report.Medicines?.Count > 0)
                {
                    var list = report.Medicines?.Select(i => LocalizationHelper.GetStringFormat("UseMedicine.MedicineInfo", i.Use, i.Inventory)).ToList();
                    medicineLog += "\n" + string.Join("\n", list ?? []);
                }
                Instances.TaskQueueViewModel.AddLog(medicineLog, UiLogColor.Info);
                break;

            case "FightTimes":
                {
                    FightReport = null;
                    if ((msg.Details?.Children())?.Any() is true)
                    {
                        FightReport = msg.Details.ToObject<FightTimes>()!;
                        if (FightReport.TimesFinished > 0)
                        {
                            AchievementTrackerHelper.Instance.SetProgress(AchievementIds.OverLimitAgent, FightReport.TimesFinished);
                        }

                        var task = Instance.GetConfigByTaskId<FightTask>(msg.TaskId);
                        if (task is not null && task.EnableTimesLimit != false && FightReport.IsFinished && FightReport.TimesFinished < task.TimesLimit)
                        {
                            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("FightTimesUnused", FightReport.TimesFinished, FightReport.Series, FightReport.TimesFinished + FightReport.Series, task.TimesLimit), UiLogColor.Warning);
                        }
                    }

                    break;
                }
        }
    }
}
