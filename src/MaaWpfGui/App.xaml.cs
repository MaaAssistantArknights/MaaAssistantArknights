// <copyright file="App.xaml.cs" company="MaaAssistantArknights">
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
using System.Threading;
using System.Windows;
using System.Windows.Documents;

namespace MaaWpfGui
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        public void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            Hyperlink link = sender as Hyperlink;
            if (!string.IsNullOrEmpty(link?.NavigateUri?.AbsoluteUri))
            {
                Process.Start(new ProcessStartInfo(link.NavigateUri.AbsoluteUri));
            }
        }

        private Mutex _mutex;

        protected override void OnStartup(StartupEventArgs e)
        {
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
                Current.Shutdown();
                return;
            }

            base.OnStartup(e);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // 释放互斥量
            _mutex.ReleaseMutex();
            _mutex.Dispose();

            base.OnExit(e);
        }
    }
}
