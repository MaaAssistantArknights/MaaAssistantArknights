// <copyright file="Bootstrapper.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Threading;
using Microsoft.Toolkit.Uwp.Notifications;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    /// <summary>
    /// The bootstrapper.
    /// </summary>
    public class Bootstrapper : Bootstrapper<RootViewModel>
    {
        private static TrayIcon _trayIconInSettingsViewModel;

        private static readonly FieldInfo _settingsViewModelIContainerFiled =
            typeof(SettingsViewModel).GetField("_container", BindingFlags.NonPublic | BindingFlags.Instance);

        /// <summary>
        /// Sets tray icon in <see cref="SettingsViewModel"/>.
        /// </summary>
        /// <param name="settingsViewModel">The <see cref="SettingsViewModel"/> instance.</param>
        /// <remarks>
        /// 应当只能是 <see cref="SettingsViewModel"/> 在构造时调用这个函数。用反射拿 <see cref="SettingsViewModel._container"/> 只是为了不额外修改 <see cref="SettingsViewModel"/> 的定义，
        /// 并顺便检查传入的 <see cref="SettingsViewModel._container"/> 不为空（即不是随便 <see langword="new"/> 出来的一个 <see cref="SettingsViewModel"/>）。
        /// </remarks>
        internal static void SetTrayIconInSettingsViewModel(SettingsViewModel settingsViewModel)
        {
            var container = (IContainer)_settingsViewModelIContainerFiled.GetValue(settingsViewModel);
            if (container != null)
            {
                _trayIconInSettingsViewModel = container.Get<TrayIcon>();
            }

            // TODO:出现不符合要求的settingsViewModel应当Log一下，等一个有缘人
        }

        /// <inheritdoc/>
        /// <remarks>初始化些啥自己加。</remarks>
        protected override void OnStart()
        {
            System.IO.Directory.SetCurrentDirectory(AppDomain.CurrentDomain.BaseDirectory);
            base.OnStart();
            ViewStatusStorage.Load();
            Localization.Load();
        }

        /// <inheritdoc/>
        protected override void ConfigureIoC(IStyletIoCBuilder builder)
        {
            builder.Bind<TaskQueueViewModel>().ToSelf().InSingletonScope();
            builder.Bind<RecruitViewModel>().ToSelf().InSingletonScope();
            builder.Bind<SettingsViewModel>().ToSelf().InSingletonScope();
            builder.Bind<CopilotViewModel>().ToSelf().InSingletonScope();
            builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
            builder.Bind<TrayIcon>().ToSelf().InSingletonScope();
        }

        /// <inheritdoc/>
        /// <remarks>退出时执行啥自己加。</remarks>
        protected override void OnExit(ExitEventArgs e)
        {
            // MessageBox.Show("O(∩_∩)O 拜拜");

            // 关闭程序时清理操作中心中的通知
            var os = RuntimeInformation.OSDescription;
            if (string.Compare(os, "Microsoft Windows 10.0.10240", StringComparison.Ordinal) >= 0)
            {
                ToastNotificationManagerCompat.History.Clear();
            }

            // 注销任务栏图标
            _trayIconInSettingsViewModel.Close();
            ViewStatusStorage.Save();
        }

        /// <inheritdoc/>
        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
        }
    }
}
