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
using System.Globalization;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Threading;
using Microsoft.Toolkit.Uwp.Notifications;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class Bootstrapper : Bootstrapper<RootViewModel>
    {
        private static TrayIcon _trayIconInSettingsViewModel;

        private static readonly FieldInfo _settingsViewModelIContainerFiled =
            typeof(SettingsViewModel).GetField("_container", BindingFlags.NonPublic | BindingFlags.Instance);

        /// <summary>
        /// 应当只能是SettingsViewModel在构造时调用这个函数。用反射拿_container只是为了不额外修改SettingsViewModel的定义，
        /// 并顺便检查传入的SettingsViewModel中的_container不为空（即不是随便new出来的一个SettingsViewModel）
        /// </summary>
        /// <param name="settingsViewModel">SettingsViewModel的this</param>
        internal static void SetTrayIconInSettingsViewModel(SettingsViewModel settingsViewModel)
        {
            var container = (IContainer)_settingsViewModelIContainerFiled.GetValue(settingsViewModel);
            if (container != null)
            {
                _trayIconInSettingsViewModel = container.Get<TrayIcon>();
            }

            // TODO:出现不符合要求的settingsViewModel应当Log一下，等一个有缘人
        }

        // 初始化些啥自己加
        protected override void OnStart()
        {
            base.OnStart();
            ViewStatusStorage.Load();
            Localization.Load();
        }

        protected override void ConfigureIoC(IStyletIoCBuilder builder)
        {
            builder.Bind<TaskQueueViewModel>().ToSelf().InSingletonScope();
            builder.Bind<RecruitViewModel>().ToSelf().InSingletonScope();
            builder.Bind<SettingsViewModel>().ToSelf().InSingletonScope();
            builder.Bind<CopilotViewModel>().ToSelf().InSingletonScope();
            builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
            builder.Bind<TrayIcon>().ToSelf().InSingletonScope();
        }

        // 退出时执行啥自己加
        protected override void OnExit(ExitEventArgs e)
        {
            // MessageBox.Show("O(∩_∩)O 拜拜");

            // 关闭程序时清理操作中心中的通知
            var os = RuntimeInformation.OSDescription.ToString();
            if (os.ToString().CompareTo("Microsoft Windows 10.0.10240") >= 0)
            {
                ToastNotificationManagerCompat.History.Clear();
            }

            // 注销任务栏图标
            _trayIconInSettingsViewModel.Close();
            ViewStatusStorage.Save();
        }

        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
        }
    }
}
