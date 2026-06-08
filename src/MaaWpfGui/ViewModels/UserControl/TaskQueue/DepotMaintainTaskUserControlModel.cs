// <copyright file="DepotMaintainTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class DepotMaintainTaskUserControlModel : TaskSettingsViewModel, DepotMaintainTaskUserControlModel.ISerialize
{
    static DepotMaintainTaskUserControlModel()
    {
        Instance = new();
    }

    public DepotMaintainTaskUserControlModel()
    {
        PlanList.CollectionChanged += (_, __) => SavePlan();
    }

    public static DepotMaintainTaskUserControlModel Instance { get; }

    public bool UpdateDepot
    {
        get => GetTaskConfig<DepotMaintainTask>().UpdateDepot;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UpdateDepot == value, t => t.UpdateDepot = value);
    }

    public ObservableCollection<Plan> PlanList { get; set; } = [new Plan()];

    public void AddPlan()
    {
        PlanList.Add(new Plan());
    }

    public bool IsStageManually
    {
        get => GetTaskConfig<DepotMaintainTask>().IsStageManually;
        set => SetTaskConfig<DepotMaintainTask>(t => t.IsStageManually == value, t => t.IsStageManually = value);
    }

    public string PlanInfo => string.Join("\n", PlanList.Select((t, i) => $"{i + 1}: {t.Stage} - {t.DropName} x{t.DropCount}"));

    private void SavePlan()
    {
        var list = PlanList.Select(i => new DepotMaintainTask.Plan(i.Stage, i.DropId, i.DropCount, i.UseMedicine, i.MedicineCount, i.UseStone, i.StoneCount));
        SetTaskConfig<DepotMaintainTask>(t => t.PlanList.SequenceEqual(list), t => t.PlanList = [.. list]);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void CollapseAll()
    {
        foreach (var plan in PlanList)
        {
            plan.IsExpanded = false;
        }
    }

    public class Plan : PropertyChangedBase
    {
        public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

        public string Title => $"{Stage} - {DropName} x{DropCount}";

        public string Stage
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
                Instance.SavePlan();
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料 ID。
        /// </summary>
        public string DropId
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
                Instance.SavePlan();
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料名称。
        /// </summary>
        public string DropName { get; set => SetAndNotify(ref field, value); } = string.Empty;

        public int DropCount
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
                Instance.SavePlan();
            }
        }

        public bool UseMedicine
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.SavePlan();
            }
        }

        public int MedicineCount
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.SavePlan();
            }
        }

        public bool UseStone
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.SavePlan();
            }
        }

        public int StoneCount
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.SavePlan();
            }
        }

        // UI 绑定的方法
        [UsedImplicitly]
        public void DropsListDropDownClosed()
        {
            if (FightSettingsUserControlModel.Instance.DropsList.FirstOrDefault(i => i.Display == DropName) is { } item)
            {
                DropId = item.Value;
            }
            else
            {
                DropId = string.Empty;
                DropName = LocalizationHelper.GetString("NotSelected");
                NotifyOfPropertyChange(nameof(DropName));
            }
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is DepotMaintainTask)
        {
            Refresh();
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not DepotMaintainTask depot || taskId > 0)
            {
                return (null, []);
            }

            var depotList = Instances.ToolboxViewModel?.DepotResult.Where(item => item.Count >= 0).ToDictionary(item => item.Id, item => item.Count) ?? [];
            var taskIds = new List<int>();
            for (int i = 0; i < depot.PlanList.Count; i++)
            {
                var plan = depot.PlanList[i];
                if (string.IsNullOrEmpty(plan.DropId) || plan.DropCount <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: invalid drop item.", UiLogColor.Error);
                    continue;
                }
                var count = depotList.TryGetValue(plan.DropId, out var value) ? value : 0;
                count = plan.DropCount - count;
                if (count <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: Inventory enough.", UiLogColor.Info);
                    continue;
                }
                var stage = FightSettingsUserControlModel.GetFightStage([plan.Stage]);
                if (string.IsNullOrEmpty(stage))
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: stage '{plan.Stage}' is not open.", UiLogColor.Error);
                    continue;
                }
                var fight = new AsstFightTask() {
                    Stage = stage,
                    Drops = new() { { plan.DropId, plan.DropCount } },
                    Medicine = plan.UseMedicine ? plan.MedicineCount : 0,
                    Stone = plan.UseStone ? plan.StoneCount : 0,
                };
                var (ret, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, fight);
                if (!ret)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: add task failed.", UiLogColor.Error);
                }
                else
                {
                    taskIds.Add(id);
                }
            }

            if (taskIds.Count > 0)
            {
                return (true, taskIds);
            }
            return (null, []);
        }
    }
}
