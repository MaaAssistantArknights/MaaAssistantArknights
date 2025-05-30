// <copyright file="AchievementSettingsUserControlModel.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.IO;
using System.Windows;
using HandyControl.Controls;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Views.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class AchievementSettingsUserControlModel : PropertyChangedBase
{
    static AchievementSettingsUserControlModel()
    {
        Instance = new();
    }

    public static AchievementSettingsUserControlModel Instance { get; }

    public void BackupAchievements()
    {
        var path = $"Achievement_{DateTime.UtcNow:yyyyMMdd_HHmmss}";
        JsonDataHelper.Set(path, AchievementTrackerHelper.Instance.Achievements);
        Growl.Success($"已备份到 data/{path}.json");
    }

    public void RestoreAchievements()
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "JSON|*.json",
            InitialDirectory = Path.Combine(System.Environment.CurrentDirectory, "data"),
        };

        if (dlg.ShowDialog() != true)
        {
            return;
        }

        var data = JsonDataHelper.Get(Path.GetFileNameWithoutExtension(dlg.FileName), new Dictionary<string, Achievement>());
        if (data is { Count: > 0 })
        {
            foreach (var pair in data)
            {
                if (AchievementTrackerHelper.Instance.Get(pair.Key) is not { } existing)
                {
                    continue;
                }

                existing.Progress = pair.Value.Progress;
                existing.IsUnlocked = pair.Value.IsUnlocked;
                existing.UnlockedTime = pair.Value.UnlockedTime;
            }

            Growl.Success("已恢复成就进度");
        }
        else
        {
            Growl.Error("恢复失败，文件格式无效");
        }
    }

    public void OnShowAchievementsClick()
    {
        var win = new AchievementListWindow
        {
            DataContext = new
            {
                Achievements = AchievementTrackerHelper.Instance.Achievements.Values,
            },
            Owner = Application.Current.MainWindow,
        };
        win.ShowDialog();
    }
}
