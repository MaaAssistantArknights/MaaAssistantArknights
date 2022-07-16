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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Forms;

namespace MeoAsstGui
{
    public partial class TrayIcon
    {
        private NotifyIcon notifyIcon = new NotifyIcon();
        private TaskQueueViewModel taskQueueViewModel;
        private WindowState ws; //记录窗体状态
        private bool _isMinimizeToTaskbar = false;

        public TrayIcon()
        {
            InitIcon();
        }

        private void InitIcon()
        {
            this.notifyIcon.Text = "MaaAssistantArknights";
            this.notifyIcon.Icon = System.Drawing.Icon.ExtractAssociatedIcon(System.Windows.Forms.Application.ExecutablePath);
            notifyIcon.MouseClick += NotifyIcon_MouseClick;
            notifyIcon.MouseDoubleClick += OnNotifyIconDoubleClick;
            App.Current.MainWindow.StateChanged += MainWindow_StateChanged;

            MenuItem showMenu = new System.Windows.Forms.MenuItem("打开 MAA");
            showMenu.Click += App_show;
            // 不知道怎么调用 TaskQueue.LinkStart, 等一个有缘的大佬来写（
            MenuItem startMenu = new System.Windows.Forms.MenuItem("开始长草");
            startMenu.Click += StartTask;
            MenuItem stopMenu = new System.Windows.Forms.MenuItem("全部停止");
            stopMenu.Click += StopTask;
            MenuItem exitMenu = new System.Windows.Forms.MenuItem("退出");
            exitMenu.Click += App_exit;
            System.Windows.Forms.MenuItem[] menuItems = new MenuItem[] { showMenu, startMenu, stopMenu, exitMenu };
            this.notifyIcon.ContextMenu = new System.Windows.Forms.ContextMenu(menuItems);
        }
        /// <summary>
        /// 只应该在TaskQueueViewModel的构造函数中调用这个函数，不要传入一个随便new出来的TaskQueueViewModel
        /// </summary>
        /// <param name="taskQueueViewModel">一个 不是 随便new出来的TaskQueueViewModel</param>
        public void SetTaskQueueViewModel(TaskQueueViewModel taskQueueViewModel)
        {
            this.taskQueueViewModel = taskQueueViewModel;
        }

        private void MainWindow_StateChanged(object sender, EventArgs e)
        {
            ws = App.Current.MainWindow.WindowState;
            if (ws == WindowState.Minimized)
            {
                SetShowInTaskbar(false);
            }
            else
            {
                SetShowInTaskbar(true);
            }
        }

        private void NotifyIcon_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left)
            {
                return;
            }
            if (ws == WindowState.Minimized)
            {
                ws = App.Current.MainWindow.WindowState = WindowState.Normal;
                App.Current.MainWindow.Activate();
                SetShowInTaskbar(true);
            }
            else
            {
                ws = App.Current.MainWindow.WindowState = WindowState.Minimized;
                SetShowInTaskbar(false);
            }
        }

        private void StartTask(object sender, EventArgs e)
        {
            //taskQueueViewModel意外为null了是不是也可以考虑Log一下
            taskQueueViewModel?.LinkStart();
        }

        private void StopTask(object sender, EventArgs e)
        {
            taskQueueViewModel?.Stop();
        }

        private void App_exit(object sender, EventArgs e)
        {
            System.Windows.Application.Current.Shutdown();
        }

        private void App_show(object sender, EventArgs e)
        {
            SetShowInTaskbar(true);
            ws = App.Current.MainWindow.WindowState = WindowState.Normal;
            App.Current.MainWindow.Activate();
        }

        private void OnNotifyIconDoubleClick(object sender, EventArgs e)
        {
            if (ws == WindowState.Minimized)
            {
                SetShowInTaskbar(true);
                ws = App.Current.MainWindow.WindowState = WindowState.Normal;
                App.Current.MainWindow.Activate();
            }
        }

        private void SetShowInTaskbar(bool state)
        {
            if (_isMinimizeToTaskbar)
            {
                App.Current.MainWindow.ShowInTaskbar = state;
            }
        }

        public void SetVisible(bool visible)
        {
            notifyIcon.Visible = visible;
        }

        public void SetMinimizeToTaskbar(bool enable)
        {
            _isMinimizeToTaskbar = enable;
        }

        public void Close()
        {
            notifyIcon.Icon = null;
            notifyIcon.Visible = false;
            notifyIcon.Dispose();
        }
    }
}
