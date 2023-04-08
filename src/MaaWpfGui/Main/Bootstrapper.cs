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
using MaaWpfGui.Helper;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.Views.UI;
using Microsoft.Toolkit.Uwp.Notifications;
using Serilog;
using Serilog.Core;
using Stylet;
using StyletIoC;

namespace MaaWpfGui.Main
{
    /// <summary>
    /// The bootstrapper.
    /// </summary>
    public class Bootstrapper : Bootstrapper<RootViewModel>
    {
        private static SettingsViewModel _settingsViewModel;
        private static TrayIcon _trayIconInSettingsViewModel;

        private static readonly FieldInfo _settingsViewModelIContainerFiled =
            typeof(SettingsViewModel).GetField("_container", BindingFlags.NonPublic | BindingFlags.Instance);

        private static ILogger _logger = Logger.None;

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
            _settingsViewModel = settingsViewModel;
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
            if (Directory.Exists("debug") is false)
            {
                Directory.CreateDirectory("debug");
            }

            string logFilename = "debug/gui.log";
            string logBakFilename = "debug/gui.bak.log";
            if (File.Exists(logFilename) && new FileInfo(logFilename).Length > 4 * 1024 * 1024)
            {
                if (File.Exists(logBakFilename))
                {
                    Directory.Delete(logBakFilename);
                }

                Directory.Move(logFilename, logBakFilename);
            }

            // Bootstrap serilog
            var loggerConfiguration = new LoggerConfiguration()
                .WriteTo.Debug(
                    outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] <{ThreadId}><{ThreadName}> {Message:lj}{NewLine}{Exception}")
                .WriteTo.File(
                    logFilename,
                    outputTemplate: "[{Timestamp:yyyy-MM-dd HH:mm:ss.fff}] <{ThreadId}><{ThreadName}> {Message:lj}{NewLine}{Exception}")
                .Enrich.FromLogContext()
                .Enrich.WithThreadId()
                .Enrich.WithThreadName();

            var maaEnv = Environment.GetEnvironmentVariable("MAA_ENVIRONMENT") == "Debug"
                ? "Debug"
                : "Production";
            loggerConfiguration = maaEnv == "Debug"
                ? loggerConfiguration.MinimumLevel.Verbose()
                : loggerConfiguration.MinimumLevel.Information();

            Log.Logger = loggerConfiguration.CreateLogger();
            _logger = Log.Logger.ForContext<Bootstrapper>();
            _logger.Information("===================================");
            _logger.Information("MaaAssistantArknights GUI started");
            _logger.Information("Maa ENV: {MaaEnv}", maaEnv);
            _logger.Information("===================================");

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
            ConfigurationHelper.Load();
            LocalizationHelper.Load();
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
            builder.Bind<StageManager>().ToSelf();

            builder.Bind<HotKeyManager>().ToSelf().InSingletonScope();
            builder.Bind<IMaaHotKeyManager>().To<MaaHotKeyManager>().InSingletonScope();
            builder.Bind<IMaaHotKeyActionHandler>().To<MaaHotKeyActionHandler>().InSingletonScope();

            builder.Bind<IMainWindowManager>().To<MainWindowManager>().InSingletonScope();

            builder.Bind<IHttpService>().To<HttpService>().InSingletonScope();
            builder.Bind<IMaaApiService>().To<MaaApiService>().InSingletonScope();
        }

        /// <inheritdoc/>
        protected override void DisplayRootView(object rootViewModel)
        {
            var windowManager = (Helper.WindowManager)GetInstance(typeof(Helper.WindowManager));
            windowManager.ShowWindow(rootViewModel);
        }

        /// <inheritdoc/>
        /// <remarks>退出时执行啥自己加。</remarks>
        protected override void OnExit(ExitEventArgs e)
        {
            // MessageBox.Show("O(∩_∩)O 拜拜");
            _settingsViewModel.Sober();

            // 关闭程序时清理操作中心中的通知
            var os = RuntimeInformation.OSDescription;
            if (string.Compare(os, "Microsoft Windows 10.0.10240", StringComparison.Ordinal) >= 0)
            {
                ToastNotificationManagerCompat.History.Clear();
            }

            // 注销任务栏图标
            _trayIconInSettingsViewModel.Close();
            ConfigurationHelper.Release();

            _logger.Information("MaaAssistantArknights GUI exited");
            _logger.Information(string.Empty);
        }

        /// <inheritdoc/>
        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            if (_logger != Logger.None)
            {
                _logger.Fatal(e.Exception, "Unhandled exception");
            }

            var errorView = new ErrorView(e.Exception.Message, e.Exception.StackTrace, true);
            errorView.ShowDialog();
        }
    }
}
