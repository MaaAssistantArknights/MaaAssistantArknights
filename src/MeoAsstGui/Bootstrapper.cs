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

using System.Windows;
using System.Windows.Threading;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class Bootstrapper : Bootstrapper<RootViewModel>
    {
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
        }

        // 退出时执行啥自己加
        protected override void OnExit(ExitEventArgs e)
        {
            // MessageBox.Show("O(∩_∩)O 拜拜");
            ViewStatusStorage.Save();
        }

        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
        }
    }
}
