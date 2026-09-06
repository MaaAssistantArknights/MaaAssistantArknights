// <copyright file="SwitchThemeTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class SwitchThemeTaskUserControlModel : TaskSettingsViewModel, SwitchThemeTaskUserControlModel.ISerialize
{
    static SwitchThemeTaskUserControlModel()
    {
        Instance = new();
    }

    public static SwitchThemeTaskUserControlModel Instance { get; }

    public ObservableCollection<ThemeItem> Themes { get; } = [];

    /// <summary>
    /// Gets or sets Core 本次随机选中的目标主题名，由 AsstProxy 从 SelectTheme 子任务回调的识别文本写入，供切换结果日志引用。
    /// </summary>
    public string CurrentTargetTheme { get; set; } = string.Empty;

    public SwitchThemeTaskUserControlModel()
    {
        Themes.CollectionChanged += Themes_CollectionChanged;
    }

    public void AddTheme()
    {
        var item = new ThemeItem();
        item.PropertyChanged += ThemeItem_PropertyChanged;
        Themes.Add(item);
    }

    public void RemoveTheme(ThemeItem theme)
    {
        Themes.Remove(theme);
    }

    /// <summary>
    /// 将主题名列表同步回任务配置，空白项不保存。
    /// </summary>
    private void SyncThemesToTaskConfig()
    {
        var list = Themes.Select(t => t.Name.Trim()).Where(name => !string.IsNullOrEmpty(name)).ToList();
        SetTaskConfig<SwitchThemeTask>(t => t.Themes.SequenceEqual(list), t => t.Themes = list);
    }

    private void Themes_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action is NotifyCollectionChangedAction.Remove or NotifyCollectionChangedAction.Replace)
        {
            e.OldItems?.OfType<ThemeItem>().ToList().ForEach(item => item.PropertyChanged -= ThemeItem_PropertyChanged);
        }
        SyncThemesToTaskConfig();
    }

    private void ThemeItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!IsRefreshingUI)
        {
            SyncThemesToTaskConfig();
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not SwitchThemeTask task)
        {
            return;
        }

        IsRefreshingUI = true;
        foreach (var item in Themes)
        {
            item.PropertyChanged -= ThemeItem_PropertyChanged;
        }

        Themes.CollectionChanged -= Themes_CollectionChanged;
        Themes.Clear();
        foreach (var name in task.Themes)
        {
            var item = new ThemeItem(name);
            item.PropertyChanged += ThemeItem_PropertyChanged;
            Themes.Add(item);
        }

        Themes.CollectionChanged += Themes_CollectionChanged;
        IsRefreshingUI = false;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not SwitchThemeTask theme || taskId > 0)
            {
                return (null, []);
            }

            // 未填写主题名时跳过任务
            if (theme.Themes.Count == 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("SwitchThemeSkipped"), UiLogColor.Info);
                return (null, []);
            }

            Instances.TaskQueueViewModel.AddLogSection(theme.NameOrTaskType);

            var (isSuccess, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                TaskType.SwitchTheme,
                new AsstSwitchThemeTask { Themes = theme.Themes });
            if (!isSuccess)
            {
                return (false, []);
            }

            return (true, [id]);
        }
    }

    public class ThemeItem(string name = "") : PropertyChangedBase
    {
        public string Name { get => field; set => SetAndNotify(ref field, value); } = name;
    }
}
