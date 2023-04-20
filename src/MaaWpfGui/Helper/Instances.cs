// <copyright file="Instances.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using GlobalHotKey;
using MaaWpfGui.Main;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// 实例管理者，只能在RootView显示之后取用实例，方便在各种地方取所需要的非null实例
    /// </summary>
    public static class Instances
    {
        public static IContainer Container { get; private set; }

        public static IWindowManager WindowManager { get; private set; }

        public static TaskQueueViewModel TaskQueueViewModel { get; private set; }

        public static RecognizerViewModel RecognizerViewModel { get; private set; }

        public static SettingsViewModel SettingsViewModel { get; private set; }

        public static CopilotViewModel CopilotViewModel { get; private set; }

        public static VersionUpdateViewModel VersionUpdateViewModel { get; private set; }

        public static AsstProxy AsstProxy { get; private set; }

        public static TrayIcon TrayIcon { get; private set; }

        public static StageManager StageManager { get; private set; }

        public static HotKeyManager HotKeyManager { get; private set; }

        public static IMaaHotKeyManager MaaHotKeyManager { get; private set; }

        public static IMaaHotKeyActionHandler MaaHotKeyActionHandler { get; private set; }

        public static IMainWindowManager MainWindowManager { get; private set; }

        public static IHttpService HttpService { get; private set; }

        public static IMaaApiService MaaApiService { get; private set; }

        public static void InitializeInstances(IContainer container)
        {
            Container = container;
            WindowManager = container.Get<WindowManager>();

            TaskQueueViewModel = container.Get<TaskQueueViewModel>();
            RecognizerViewModel = container.Get<RecognizerViewModel>();
            SettingsViewModel = container.Get<SettingsViewModel>();
            CopilotViewModel = container.Get<CopilotViewModel>();
            VersionUpdateViewModel = container.Get<VersionUpdateViewModel>();

            AsstProxy = container.Get<AsstProxy>();
            TrayIcon = container.Get<TrayIcon>();
            StageManager = container.Get<StageManager>();

            HotKeyManager = container.Get<HotKeyManager>();
            MaaHotKeyManager = container.Get<MaaHotKeyManager>();
            MaaHotKeyActionHandler = container.Get<MaaHotKeyActionHandler>();

            MainWindowManager = container.Get<MainWindowManager>();

            HttpService = container.Get<HttpService>();
            MaaApiService = container.Get<MaaApiService>();
        }
    }
}
