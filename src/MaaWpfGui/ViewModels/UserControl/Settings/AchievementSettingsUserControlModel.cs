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
using System.Windows.Forms;
using System.Windows.Media;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Views.UI;
using Stylet;
using Application = System.Windows.Application;

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
        using var dialog = new FolderBrowserDialog();
        dialog.SelectedPath = Environment.CurrentDirectory;
        dialog.ShowNewFolderButton = true;

        if (dialog.ShowDialog() != DialogResult.OK)
        {
            return;
        }

        var selectedPath = dialog.SelectedPath;
        var fileName = $"Achievement_{DateTime.UtcNow:yyyyMMdd_HHmmss}";
        bool success = JsonDataHelper.Set(fileName, AchievementTrackerHelper.Instance.Achievements, selectedPath);

        if (success)
        {
            var growlInfo = new GrowlInfo
            {
                IsCustom = true,
                Message = $"{LocalizationHelper.GetString("AchievementBackupSuccess")} {Path.Combine(selectedPath, fileName)}.json",
                StaysOpen = true,
            };
            Growl.Success(growlInfo);
        }
        else
        {
            Growl.Error(LocalizationHelper.GetString("AchievementRestoreFailed"));
        }
    }

    public void RestoreAchievements()
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "JSON|*.json",
            InitialDirectory = Path.Combine(Environment.CurrentDirectory, "data"),
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

            AchievementTrackerHelper.Instance.Save();
            Growl.Success(LocalizationHelper.GetString("AchievementRestoreSuccess"));
        }
        else
        {
            Growl.Error(LocalizationHelper.GetString("AchievementRestoreFailed"));
        }
    }

    public void OnShowAchievementsClick()
    {
        var win = new AchievementListWindow
        {
            Owner = Application.Current.MainWindow,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
        };
        win.ShowDialog();
    }

    private static readonly SolidColorBrush _hiddenMedalBrush = Application.Current.Resources["HiddenMedalBrush"] is SolidColorBrush brush ? brush : new(Colors.Gold);

    private static readonly SolidColorBrush _normalMedalBrush = Application.Current.Resources["NormalMedalBrush"] is SolidColorBrush brush ? brush : new(Colors.Silver);

    private int _clickCount = 0;
    private static readonly Random _random = new();
    private bool _isTriggered = false;

    public void OnDebugClick()
    {
        NotifyOfPropertyChange(nameof(Tip));
        if (_isTriggered)
        {
            ResetDebugState();
            return;
        }

        if (Instances.VersionUpdateViewModel.IsDebugVersion())
        {
            EnableDebugMode();
            return;
        }

        if (++_clickCount < 10)
        {
            return;
        }

        _clickCount = 0;

        bool shouldTriggerDebug = _random.NextDouble() < 0.1;
        if (shouldTriggerDebug)
        {
            EnableDebugMode();
        }
    }

    public string Tip => LocalizationHelper.GetPallasString(1, 10);

    private void ResetDebugState()
    {
        IsDebugVersion = false;
        MedalBrush = _normalMedalBrush;
        _isTriggered = false;
    }

    private void EnableDebugMode()
    {
        IsDebugVersion = true;
        MedalBrush = _hiddenMedalBrush;
        _isTriggered = true;
    }

    private SolidColorBrush _medalBrush = _normalMedalBrush;

    public SolidColorBrush MedalBrush
    {
        get => _medalBrush;
        set => SetAndNotify(ref _medalBrush, value);
    }

    private bool _isDebugVersion = false;

    public bool IsDebugVersion
    {
        get => _isDebugVersion;
        set => SetAndNotify(ref _isDebugVersion, value);
    }

    public void UnlockAll()
    {
        _ = AchievementTrackerHelper.Instance.UnlockAll();
    }

    public void LockAll()
    {
        AchievementTrackerHelper.Instance.LockAll();
    }
}
