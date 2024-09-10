// <copyright file="Bootstrapper.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
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
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Threading;
using GlobalHotKey;
using MaaWpfGui.Configuration;
using MaaWpfGui.Helper;
using MaaWpfGui.Properties;
using MaaWpfGui.Services;
using MaaWpfGui.Services.HotKeys;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.Services.RemoteControl;
using MaaWpfGui.Services.Web;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.Views.UI;
using MaaWpfGui.WineCompat;
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
                Bootstrapper.Shutdown();
                return;
            }
            */

            Directory.SetCurrentDirectory(AppContext.BaseDirectory);
            if (!Directory.Exists("debug"))
            {
                Directory.CreateDirectory("debug");
            }

            // TODO: Remove after 5.10.0
            string[] filesToDelete = ["MAA_win7.exe", "启动旧版.cmd"];
            string curDir = Directory.GetCurrentDirectory();
            foreach (var file in filesToDelete)
            {
                string path = Path.Combine(curDir, file);
                if (File.Exists(path))
                {
                    File.Delete(path);
                }
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

            var uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";
            uiVersion = uiVersion == "0.0.1" ? "DEBUG VERSION" : uiVersion;
            var builtDate = Assembly.GetExecutingAssembly().GetCustomAttribute<BuildDateTimeAttribute>()?.BuildDateTime ?? DateTime.MinValue;
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
            _logger.Information($"Version {uiVersion}");
            _logger.Information($"Built at {builtDate:O}");
            _logger.Information($"Maa ENV: {maaEnv}");
            _logger.Information($"User Dir {Directory.GetCurrentDirectory()}");
            if (IsUserAdministrator())
            {
                _logger.Information("Run as Administrator");
            }

            if (WineRuntimeInformation.IsRunningUnderWine)
            {
                _logger.Information($"Running under Wine {WineRuntimeInformation.WineVersion} on {WineRuntimeInformation.HostSystemName}");
                RenderOptions.ProcessRenderMode = RenderMode.SoftwareOnly;
                _logger.Information($"MaaWineBridge status: {MaaWineBridge.Availability}");
                _logger.Information($"MaaDesktopIntegration available: {MaaDesktopIntegration.Availabile}");
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

            // 检查 MaaCore.dll 是否存在
            if (!File.Exists("MaaCore.dll"))
            {
                throw new FileNotFoundException("MaaCore.dll not found!");
            }

            // 检查 resource 文件夹是否存在
            if (!Directory.Exists("resource"))
            {
                throw new DirectoryNotFoundException("resource folder not found!");
            }
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
            Release();

            _logger.Information("MaaAssistantArknights GUI exited");
            _logger.Information(string.Empty);
            Log.CloseAndFlush();
            base.OnExit(e);

            if (!_isRestartingWithoutArgs)
            {
                return;
            }

            if (Environment.ProcessPath is null)
            {
                return;
            }

            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                FileName = Environment.ProcessPath,
            };

            Process.Start(startInfo);
        }

        public static void Release()
        {
            ETagCache.Save();
            Instances.SettingsViewModel.Sober();
            Instances.MaaHotKeyManager.Release();

            // 关闭程序时清理操作中心中的通知
            ToastNotification.Cleanup();

            ConfigurationHelper.Release();
            ConfigFactory.Release();
        }

        private static bool _isRestartingWithoutArgs;

        /// <summary>
        /// 重启，不带参数
        /// </summary>
        public static void ShutdownAndRestartWithoutArgs()
        {
            _isRestartingWithoutArgs = true;
            _logger.Information("Shutdown and restart without Args");
            Execute.OnUIThread(Application.Current.Shutdown);
        }

        public static void Shutdown([CallerMemberName]string caller = "")
        {
            _logger.Information($"Shutdown called by {caller}");
            Execute.OnUIThread(Application.Current.Shutdown);
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
            LogUnhandledException(e.Exception);
            ShowErrorDialog(e.Exception);
            e.Handled = true;
        }

        private static void LogUnhandledException(Exception exception)
        {
            if (_logger != Logger.None)
            {
                _logger.Fatal(exception, "Unhandled exception occurred");
            }
        }

        private static void ShowErrorDialog(Exception exception)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                // DragDrop.DoDragSourceMove 会导致崩溃，但不需要退出程序
                // 这是一坨屎，但是没办法，只能这样了
                var isDragDropException = exception is COMException && exception.ToString()!.Contains("DragDrop.DoDragSourceMove");

                var shouldExit = !isDragDropException;

                var errorView = new ErrorView(exception, shouldExit);
                errorView.ShowDialog();
            });
        }
    }
}
