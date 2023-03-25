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
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using System.Windows.Media;
using HandyControl.Tools;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;

namespace MaaWpfGui
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        [DllImport("DwmApi.dll")]
        public static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int attrLen);

        public void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            Hyperlink link = sender as Hyperlink;
            if (!string.IsNullOrEmpty(link.NavigateUri.AbsoluteUri))
            {
                Process.Start(new ProcessStartInfo(link.NavigateUri.AbsoluteUri));
            }
        }

        private readonly SolidColorBrush black = new SolidColorBrush(Color.FromRgb(49, 51, 56));
        private readonly SolidColorBrush white = new SolidColorBrush(Color.FromRgb(181, 186, 193));

        public static bool SetColors => Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.SetColors, bool.FalseString));

        public void darkToStart()
        {
            if (!SetColors)
            {
                return;
            }

            int darkModeEnabled = 1;
            DwmSetWindowAttribute(this.MainWindow.GetHandle(), 20, ref darkModeEnabled, sizeof(int));
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            if (!SetColors)
            {
                return;
            }

            // 在应用程序启动时，遍历所有控件并设置它们的颜色
            SetAllControlColors(this.MainWindow);

            // 修改下拉框的颜色
            EventManager.RegisterClassHandler(typeof(ComboBox), UIElement.PreviewMouseLeftButtonDownEvent, new RoutedEventHandler(ComboBox_DropDownOpened));
        }

        private void ComboBox_DropDownOpened(object sender, EventArgs e)
        {
            if (!(sender is ComboBox comboBox) || !(comboBox.Template.FindName("PART_Popup", comboBox) is Popup popup) || popup.Child == null)
            {
                return;
            }

            this.Dispatcher.BeginInvoke(new Action(() =>
            {
                SetAllControlColors(popup.Child);
            }));
        }

        public static void SetAllControlColors(DependencyObject obj)
        {
            if (!SetColors)
            {
                return;
            }

            // 遍历控件并设置颜色
            if (obj is DependencyObject container)
            {
                int count = VisualTreeHelper.GetChildrenCount(container);
                for (int i = 0; i < count; i++)
                {
                    DependencyObject child = VisualTreeHelper.GetChild(container, i);
                    App app = (App)Current;
                    app.SetControlColors(child);
                }
            }
        }

        public void SetControlColors(DependencyObject obj)
        {
            // 检查控件是否为null
            if (obj == null)
            {
                return;
            }

            // 获取控件类型
            Type type = obj.GetType();

            // 如果控件具有 Background 属性，则将其背景颜色设置为黑色
            if (type.GetProperty("Background") != null && (obj as Control)?.Background != null)
            {
                (obj as Control).Background = black;
            }

            // 如果控件具有 Foreground 属性，则将其前景色设置为白色
            if (type.GetProperty("Foreground") != null && (obj as Control)?.Foreground != null)
            {
                (obj as Control).Foreground = white;
            }

            // 如果控件具有 BorderBrush 属性，则将其边框颜色设置为白色
            if (type.GetProperty("BorderBrush") != null && (obj as Control)?.BorderBrush != null)
            {
                (obj as Control).BorderBrush = white;
            }

            if (obj is TextBlock)
            {
                (obj as TextBlock).Foreground = white;
            }
            else if (obj is DockPanel)
            {
                (obj as DockPanel).Background = black;
            }

            // 遍历子控件并递归调用此方法
            int count = VisualTreeHelper.GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                DependencyObject child = VisualTreeHelper.GetChild(obj, i);
                SetControlColors(child);
            }
        }
    }
}
