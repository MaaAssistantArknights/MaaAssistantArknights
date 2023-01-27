// <copyright file="Bootstrapper.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Threading;
using GlobalHotKey;
using MaaWpfGui.MaaHotKeys;
using MaaWpfGui.Views;
using Microsoft.Toolkit.Uwp.Notifications;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
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
            Directory.SetCurrentDirectory(AppDomain.CurrentDomain.BaseDirectory);
            Directory.CreateDirectory("debug");
            {
                // FIXME: 目录迁移，过几个版本删除这段
                File.Delete("gui.log");
                File.Delete("gui.bak.log");
            }

            // 清理日志文件
            var log_path = "debug\\gui.log";
            var backup_log_path = "debug\\gui.bak.log";
            if (File.Exists(log_path))
            {
                var fileInfo = new FileInfo(log_path);
                if (fileInfo.Length > 128 * 1024 /*128K*/)
                {
                    if (File.Exists(backup_log_path))
                    {
                        File.Delete(backup_log_path);
                    }

                    File.Move(log_path, backup_log_path);
                }
            }

            try
            {
                Directory.Delete(".old", true);
            }
            catch (Exception)
            {
                // ignored
            }

            foreach (var file in new DirectoryInfo(".").GetFiles("*.old"))
            {
                try
                {
                    file.Delete();
                }
                catch (Exception)
                {
                    // ignored
                }
            }

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
            builder.Bind<DepotViewModel>().ToSelf().InSingletonScope();
            builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
            builder.Bind<TrayIcon>().ToSelf().InSingletonScope();
            builder.Bind<HotKeyManager>().ToSelf().InSingletonScope();
            builder.Bind<IMaaHotKeyManager>().To<MaaHotKeyManager>().InSingletonScope();
            builder.Bind<IMaaHotKeyActionHandler>().To<MaaHotKeyActionHandler>().InSingletonScope();
            builder.Bind<IMainWindowManager>().To<MainWindowManager>().InSingletonScope();
        }

        /// <inheritdoc/>
        protected override void DisplayRootView(object rootViewModel)
        {
            var windowManager = (WindowManager)GetInstance(typeof(WindowManager));
            windowManager.ShowWindow(rootViewModel);
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
            ViewStatusStorage.Release();
        }

        /// <inheritdoc/>
        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
            Logger.Error(e.Exception.ToString(), MethodBase.GetCurrentMethod().Name);
            var errorView = new ErrorView(e.Exception.Message, e.Exception.StackTrace, true);
            errorView.ShowDialog();
        }
    }
}
