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
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Principal;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using GlobalHotKey;
using MaaWpfGui.Helper;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.Services.Web;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.Views.UI;
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
        private static readonly RunningState _runningState = RunningState.Instance;
        private static ILogger _logger = Logger.None;

        // private static Mutex _mutex;

        /// <inheritdoc/>
        /// <remarks>初始化些啥自己加。</remarks>
        protected override void OnStart()
        {
            /*
            // 设置互斥量的名称
            string mutexName = "MAA_" + Directory.GetCurrentDirectory().Replace("\\", "_").Replace(":", string.Empty);
            _mutex = new Mutex(true, mutexName);

            if (!_mutex.WaitOne(TimeSpan.Zero, true))
            {
                // 这里还没加载语言包，就不 GetString 了
                MessageBox.Show("同一路径下只能启动一个实例！\n\n" +
                                "同一路徑下只能啟動一個實例！\n\n" +
                                "Only one instance can be launched under the same path!\n\n" +
                                "同じパスの下で1つのインスタンスしか起動できません！\n\n" +
                                "동일한 경로에는 하나의 인스턴스만 실행할 수 있습니다!");
                Application.Current.Shutdown();
                return;
            }
            */

            Directory.SetCurrentDirectory(AppContext.BaseDirectory);
            if (Directory.Exists("debug") is false)
            {
                Directory.CreateDirectory("debug");
            }

            const string LogFilename = "debug/gui.log";
            const string LogBakFilename = "debug/gui.bak.log";
            if (File.Exists(LogFilename) && new FileInfo(LogFilename).Length > 4 * 1024 * 1024)
            {
                if (File.Exists(LogBakFilename))
                {
                    File.Delete(LogBakFilename);
                }

                File.Move(LogFilename, LogBakFilename);
            }

            // Bootstrap serilog
            var loggerConfiguration = new LoggerConfiguration()
                .WriteTo.Debug(
                    outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] <{ThreadId}><{ThreadName}> {Message:lj}{NewLine}{Exception}")
                .WriteTo.File(
                    LogFilename,
                    outputTemplate: "[{Timestamp:yyyy-MM-dd HH:mm:ss.fff}] <{ThreadId}><{ThreadName}> {Message:lj}{NewLine}{Exception}")
                .Enrich.FromLogContext()
                .Enrich.WithThreadId()
                .Enrich.WithThreadName();

            var uiVersion = Assembly.GetExecutingAssembly().GetName().Version?.ToString(3) ?? "0.0.1";
            uiVersion = uiVersion == "0.0.1" ? "DEBUG VERSION" : uiVersion;
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
            _logger.Information("Version {UiVersion}", uiVersion);
            _logger.Information("Maa ENV: {MaaEnv}", maaEnv);
            _logger.Information("User Dir {CurrentDirectory}", Directory.GetCurrentDirectory());
            if (IsUserAdministrator())
            {
                _logger.Information("Run as Administrator");
            }

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
            ETagCache.Load();
        }

        private static bool IsUserAdministrator()
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            WindowsPrincipal principal = new WindowsPrincipal(identity);
            SecurityIdentifier adminSid = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);

            return principal.IsInRole(adminSid);
        }

        /// <inheritdoc/>
        protected override void ConfigureIoC(IStyletIoCBuilder builder)
        {
            builder.Bind<TaskQueueViewModel>().ToSelf().InSingletonScope();
            builder.Bind<RecognizerViewModel>().ToSelf().InSingletonScope();
            builder.Bind<SettingsViewModel>().ToSelf().InSingletonScope();
            builder.Bind<CopilotViewModel>().ToSelf().InSingletonScope();

            builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
            builder.Bind<StageManager>().ToSelf();

            builder.Bind<HotKeyManager>().ToSelf().InSingletonScope();

            builder.Bind<IMaaHotKeyManager>().To<MaaHotKeyManager>().InSingletonScope();
            builder.Bind<IMaaHotKeyActionHandler>().To<MaaHotKeyActionHandler>().InSingletonScope();

            builder.Bind<RemoteControlService>().To<RemoteControlService>().InSingletonScope();

            builder.Bind<IMainWindowManager>().To<MainWindowManager>().InSingletonScope();

            builder.Bind<IHttpService>().To<HttpService>().InSingletonScope();
            builder.Bind<IMaaApiService>().To<MaaApiService>().InSingletonScope();
        }

        protected override void Configure()
        {
            base.Configure();
            Instances.Instantiate(Container);
        }

        /// <inheritdoc/>
        protected override void DisplayRootView(object rootViewModel)
        {
            Instances.WindowManager.ShowWindow(rootViewModel);
            Instances.InstantiateOnRootViewDisplayed(Container);
        }

        /// <inheritdoc/>
        /// <remarks>退出时执行啥自己加。</remarks>
        protected override void OnExit(ExitEventArgs e)
        {
            /*
             // 释放互斥量
            _mutex?.ReleaseMutex();
            _mutex?.Dispose();
            */

            // MessageBox.Show("O(∩_∩)O 拜拜");
            ETagCache.Save();
            Instances.SettingsViewModel.Sober();
            Instances.MaaHotKeyManager.Release();

            // 关闭程序时清理操作中心中的通知
            // 使用 handyorg 的 ShowBalloonTip，不需要清理
            /*
            var os = RuntimeInformation.OSDescription;
            if (string.Compare(os, "Microsoft Windows 10.0.10240", StringComparison.Ordinal) >= 0)
            {
                new ToastNotificationHistory().Clear();
            }
            */

            ConfigurationHelper.Release();

            _logger.Information("MaaAssistantArknights GUI exited");
            _logger.Information(string.Empty);
            Log.CloseAndFlush();
            base.OnExit(e);

            if (_isRestartingWithoutArgs)
            {
                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = System.Windows.Forms.Application.ExecutablePath,
                };

                Process.Start(startInfo);
            }
        }

        private static bool _isRestartingWithoutArgs;

        /// <summary>
        /// 重启，不带参数
        /// </summary>
        public static void ShutdownAndRestartWithoutArgs()
        {
            _isRestartingWithoutArgs = true;
            _logger.Information("Shutdown and restart without Args");
            Application.Current.Shutdown();
        }

        private static bool _isWaitingToRestart;

        public static async Task RestartAfterIdleAsync()
        {
            if (_isWaitingToRestart)
            {
                return;
            }

            _isWaitingToRestart = true;

            await _runningState.UntilIdleAsync(60000);
            ShutdownAndRestartWithoutArgs();
        }

        /// <inheritdoc/>
        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            if (_logger != Logger.None)
            {
                _logger.Fatal(e.Exception, "Unhandled exception");
            }

            var errorView = new ErrorView(e.Exception, true);
            errorView.ShowDialog();
        }
    }
}
