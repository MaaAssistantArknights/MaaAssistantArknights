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
using System.Security.Policy;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

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
            if (!string.IsNullOrEmpty(link.NavigateUri.AbsoluteUri))
            {
                Process.Start(new ProcessStartInfo(link.NavigateUri.AbsoluteUri));
            }
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // 在应用程序启动时，遍历所有控件并设置它们的颜色
            SetAllControlColors(Current.MainWindow);
        }

        public static void SetAllControlColors(DependencyObject obj)
        {
            bool setColors = Convert.ToBoolean(ViewStatusStorage.Get("GUI.SetColors", bool.FalseString));
            if (!setColors)
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
                (obj as Control).Background = Brushes.Black;
            }

            // 如果控件具有 Foreground 属性，则将其前景色设置为白色
            if (type.GetProperty("Foreground") != null && (obj as Control)?.Foreground != null)
            {
                (obj as Control).Foreground = Brushes.White;
            }

            if (obj is TextBlock)
            {
                (obj as TextBlock).Foreground = Brushes.White;
            }
            else if ((obj as Control) is Button)
            {
                (obj as Control).BorderBrush = Brushes.White;
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
