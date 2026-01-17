// <copyright file="Instances.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1401

using System;
using GlobalHotKey;
using MaaWpfGui.Main;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.Helper;

/// <summary>
/// The instance manager.
/// </summary>
public static class Instances
{
    public static class Data
    {
        public static int MedicineUsedTimes { get; set; } = 0;

        public static int ExpiringMedicineUsedTimes { get; set; } = 0;

        public static int StoneUsedTimes { get; set; } = 0;

        public static bool HasPrintedScreencapWarning { get; set; } = false;

        public static int RecruitConfirmTime { get; set; } = 0;

        public static void ClearCache()
        {
            FightSettingsUserControlModel.SanityReport = null;
            MedicineUsedTimes = 0;
            ExpiringMedicineUsedTimes = 0;
            StoneUsedTimes = 0;
            RecruitConfirmTime = 0;
            HasPrintedScreencapWarning = false;
        }
    }

    public static IWindowManager WindowManager { get; private set; }

    public static TaskQueueViewModel TaskQueueViewModel { get; private set; }

    public static ToolboxViewModel ToolboxViewModel { get; private set; }

    public static SettingsViewModel SettingsViewModel { get; private set; }

    public static CopilotViewModel CopilotViewModel { get; private set; }

    public static VersionUpdateDialogViewModel VersionUpdateDialogViewModel { get; private set; }

    public static AnnouncementDialogViewModel AnnouncementDialogViewModel { get; private set; }

    public static AsstProxy AsstProxy { get; private set; }

    public static StageManager StageManager { get; private set; }

    public static HotKeyManager HotKeyManager { get; private set; }

    public static IMaaHotKeyManager MaaHotKeyManager { get; private set; }

    public static IMaaHotKeyActionHandler MaaHotKeyActionHandler { get; private set; }

    public static OverlayViewModel OverlayViewModel { get; private set; }

    // 别的地方有用到这个吗？
    // ReSharper disable once UnusedAutoPropertyAccessor.Global
    public static RemoteControlService RemoteControlService { get; private set; }

    public static IMainWindowManager MainWindowManager { get; private set; }

    public static event EventHandler MainWindowManagerInstantiated;

    public static IHttpService HttpService { get; private set; }

    public static IMaaApiService MaaApiService { get; private set; }

    public static void Instantiate(IContainer container)
    {
        WindowManager = container.Get<WindowManager>();

        // 这两实例化时存在依赖顺序
        HttpService = container.Get<HttpService>();
        MaaApiService = container.Get<MaaApiService>();

        VersionUpdateDialogViewModel = container.Get<VersionUpdateDialogViewModel>();
        AnnouncementDialogViewModel = container.Get<AnnouncementDialogViewModel>();
        AsstProxy = container.Get<AsstProxy>();

        // 这些实例化时存在依赖顺序
        StageManager = container.Get<StageManager>();
        TaskQueueViewModel = container.Get<TaskQueueViewModel>();
        ToolboxViewModel = container.Get<ToolboxViewModel>();

        SettingsViewModel = container.Get<SettingsViewModel>();
        CopilotViewModel = container.Get<CopilotViewModel>();

        RemoteControlService = container.Get<RemoteControlService>();

        OverlayViewModel = container.Get<OverlayViewModel>();

        HotKeyManager = container.Get<HotKeyManager>();
        MaaHotKeyManager = container.Get<MaaHotKeyManager>();
        MaaHotKeyActionHandler = container.Get<MaaHotKeyActionHandler>();
    }

    public static void InstantiateOnRootViewDisplayed(IContainer container)
    {
        MainWindowManager = container.Get<MainWindowManager>();
        MainWindowManagerInstantiated?.Invoke(null, EventArgs.Empty);
    }
}
