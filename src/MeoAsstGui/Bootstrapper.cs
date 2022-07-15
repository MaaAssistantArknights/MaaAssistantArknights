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

using System.Runtime.InteropServices;
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
        private static bool _isTrayIconInSettingsViewModelSet = false;
        public static void SetTrayIconInSettingsViewModel(TrayIcon trayIcon)
        {
            if (!_isTrayIconInSettingsViewModelSet)
            {
                _trayIconInSettingsViewModel = trayIcon;
                _isTrayIconInSettingsViewModelSet = true;
            }
            else
            {
                // 什么，SettingsViewModel会重复初始化。我相信后人的智慧。
                throw new System.InvalidOperationException(
                    "不可重复对源自SettingsViewModel的trayIcon引用赋值，它会在SettingsViewModel初始化时被初始化。");
            }
        }

        // 初始化些啥自己加
        protected override void OnStart()
        {
            ViewStatusStorage.Load();
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

            //关闭程序时清理操作中心中的通知
            var os = RuntimeInformation.OSDescription.ToString();
            if (os.ToString().CompareTo("Microsoft Windows 10.0.10240") >= 0)
            {
                ToastNotificationManagerCompat.History.Clear();
            }
            //注销任务栏图标
            _trayIconInSettingsViewModel.Close();
            ViewStatusStorage.Save();
        }

        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
        }
    }
}
