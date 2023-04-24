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
    /// The instance manager.
    /// </summary>
    public static class Instances
    {
        public static IWindowManager WindowManager { get; private set; }

        public static TaskQueueViewModel TaskQueueViewModel { get; private set; }

        public static RecognizerViewModel RecognizerViewModel { get; private set; }

        public static SettingsViewModel SettingsViewModel { get; private set; }

        public static CopilotViewModel CopilotViewModel { get; private set; }

        public static VersionUpdateViewModel VersionUpdateViewModel { get; private set; }

        public static AsstProxy AsstProxy { get; private set; }

        public static TrayIcon TrayIcon { get; private set; }

        public static HotKeyManager HotKeyManager { get; private set; }

        public static IMaaHotKeyManager MaaHotKeyManager { get; private set; }

        public static IMaaHotKeyActionHandler MaaHotKeyActionHandler { get; private set; }

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

            // 这仨实例化时存在依赖顺序
            HttpService = container.Get<HttpService>();
            MaaApiService = container.Get<MaaApiService>();

            HotKeyManager = container.Get<HotKeyManager>();
            MaaHotKeyManager = container.Get<MaaHotKeyManager>();
            MaaHotKeyActionHandler = container.Get<MaaHotKeyActionHandler>();
        }

        public static void InstantiateOnRootViewDisplayed(IContainer container)
        {
            MainWindowManager = container.Get<MainWindowManager>();
            TrayIcon = container.Get<TrayIcon>();
        }
    }
}
