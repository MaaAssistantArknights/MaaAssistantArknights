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

#pragma warning disable SA1401, CS8632

using GlobalHotKey;
using MaaWpfGui.Main;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// The instance manager.
    /// </summary>
    public static class Instances
    {
        public static class Data
        {
            public static FightSettingsUserControlModel.FightTimes? FightTimes { get; set; }

            public static FightSettingsUserControlModel.SanityInfo? SanityReport { get; set; }

            public static int MedicineUsedTimes { get; set; }

            public static int ExpiringMedicineUsedTimes { get; set; }

            public static int StoneUsedTimes { get; set; }

            public static bool HasPrintedScreencapWarning { get; set; }

            public static void ClearCache()
            {
                SanityReport = null;
                MedicineUsedTimes = 0;
                ExpiringMedicineUsedTimes = 0;
                StoneUsedTimes = 0;
                HasPrintedScreencapWarning = false;
            }
        }

        public static IWindowManager WindowManager { get; private set; }

        public static TaskQueueViewModel TaskQueueViewModel { get; private set; }

        public static RecognizerViewModel RecognizerViewModel { get; private set; }

        public static SettingsViewModel SettingsViewModel { get; private set; }

        public static CopilotViewModel CopilotViewModel { get; private set; }

        public static VersionUpdateViewModel VersionUpdateViewModel { get; private set; }

        public static AnnouncementViewModel AnnouncementViewModel { get; private set; }

        public static AsstProxy AsstProxy { get; private set; }

        public static HotKeyManager HotKeyManager { get; private set; }

        public static IMaaHotKeyManager MaaHotKeyManager { get; private set; }

        public static IMaaHotKeyActionHandler MaaHotKeyActionHandler { get; private set; }

        // 别的地方有用到这个吗？
        // ReSharper disable once UnusedAutoPropertyAccessor.Global
        public static RemoteControlService RemoteControlService { get; private set; }

        public static IMainWindowManager MainWindowManager { get; private set; }

        public static IHttpService HttpService { get; private set; }

        public static IMaaApiService MaaApiService { get; private set; }

        public static void Instantiate(IContainer container)
        {
            WindowManager = container.Get<WindowManager>();

            AsstProxy = container.Get<AsstProxy>();
            TaskQueueViewModel = container.Get<TaskQueueViewModel>();
            RecognizerViewModel = container.Get<RecognizerViewModel>();
            SettingsViewModel = container.Get<SettingsViewModel>();
            CopilotViewModel = container.Get<CopilotViewModel>();
            VersionUpdateViewModel = container.Get<VersionUpdateViewModel>();
            AnnouncementViewModel = container.Get<AnnouncementViewModel>();

            // 这两实例化时存在依赖顺序
            HttpService = container.Get<HttpService>();
            MaaApiService = container.Get<MaaApiService>();

            RemoteControlService = container.Get<RemoteControlService>();

            HotKeyManager = container.Get<HotKeyManager>();
            MaaHotKeyManager = container.Get<MaaHotKeyManager>();
            MaaHotKeyActionHandler = container.Get<MaaHotKeyActionHandler>();
        }

        public static void InstantiateOnRootViewDisplayed(IContainer container)
        {
            MainWindowManager = container.Get<MainWindowManager>();
        }
    }
}
