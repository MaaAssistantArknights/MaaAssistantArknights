// <copyright file="WindowManager.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Windows;
using Stylet;

namespace MaaWpfGui
{
    public class WindowManager : Stylet.WindowManager
    {
        public WindowManager(IViewManager viewManager, Func<IMessageBoxViewModel> messageBoxViewModelFactory, IWindowManagerConfig config)
    : base(viewManager, messageBoxViewModelFactory, config)
        {
        }

        private readonly string ScreenName = ViewStatusStorage.Get("GUI.Monitor.Number", string.Empty);
        private readonly int ScreenWidth = int.Parse(ViewStatusStorage.Get("GUI.Monitor.Width", "-1"));
        private readonly int ScreenHeight = int.Parse(ViewStatusStorage.Get("GUI.Monitor.Height", "-1"));

        private readonly double Left = double.Parse(ViewStatusStorage.Get("GUI.Position.Left", "NaN"));
        private readonly double Top = double.Parse(ViewStatusStorage.Get("GUI.Position.Top", "NaN"));
        private readonly double Width = double.Parse(ViewStatusStorage.Get("GUI.Size.Width", "NaN"));
        private readonly double Height = double.Parse(ViewStatusStorage.Get("GUI.Size.Height", "NaN"));

        private readonly string RootView = "MaaWpfGui.RootView";

        public void MoveWindowToDisplay(string displayName, Window window)
        {
            var screen = System.Windows.Forms.Screen.AllScreens.FirstOrDefault(x => x.DeviceName == displayName);
            if (screen != null)
            {
                var screenRect = screen.Bounds;
                if (screenRect.Height == ScreenHeight && screenRect.Width == ScreenWidth)
                {
                    window.Left = (int)(screenRect.Left + Left);
                    window.Top = (int)(screenRect.Top + Top);
                    window.Width = Width;
                    window.Height = Height;
                    return;
                }
            }

            window.WindowStartupLocation = WindowStartupLocation.CenterScreen;
        }

        /// <inheritdoc/>
        protected override Window CreateWindow(object viewModel, bool isDialog, IViewAware ownerViewModel)
        {
            Window window = base.CreateWindow(viewModel, isDialog, ownerViewModel);
            if (bool.Parse(ViewStatusStorage.Get("GUI.PositionAndSize.Load", bool.TrueString)))
            {
                if (isDialog || ownerViewModel != null || double.IsNaN(Left) || double.IsNaN(Top))
                {
                    return window;
                }

                // In Stylet, CreateWindow().WindowStartupLocation is CenterScreen or CenterOwner (if w.WSLoc == Manual && w.Left == NaN && w.Top == NaN && ...)
                window.WindowStartupLocation = WindowStartupLocation.Manual;

                if (window.ToString() == RootView)
                {
                    MoveWindowToDisplay(ScreenName, window);
                }
                else
                {
                    // Center other windows in MaaWpfGui.RootView
                    var mainWindow = Application.Current.MainWindow;
                    window.Left = mainWindow.Left + ((mainWindow.Width - window.Width) / 2);
                    window.Top = mainWindow.Top + ((mainWindow.Height - window.Height) / 2);
                }
            }

            return window;
        }
    }
}
