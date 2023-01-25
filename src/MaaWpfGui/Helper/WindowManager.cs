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
using System.Windows;
using Stylet;

namespace MaaWpfGui
{
    public class WindowManager : Stylet.WindowManager
    {
        private readonly double Left = double.Parse(ViewStatusStorage.Get("GUI.Position.Left", "NaN"));
        private readonly double Top = double.Parse(ViewStatusStorage.Get("GUI.Position.Top", "NaN"));

        public WindowManager(IViewManager viewManager, Func<IMessageBoxViewModel> messageBoxViewModelFactory, IWindowManagerConfig config)
            : base(viewManager, messageBoxViewModelFactory, config)
        {
            // Change Left and Top after adjusting multiple displays
            Left = Left < SystemParameters.VirtualScreenWidth ? Left : double.NaN;
            Top = Top < SystemParameters.VirtualScreenHeight ? Top : double.NaN;
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

                if (window.ToString() == "MaaWpfGui.RootView")
                {
                    double width = double.Parse(ViewStatusStorage.Get("GUI.Size.Width", "NaN"));
                    double height = double.Parse(ViewStatusStorage.Get("GUI.Size.Height", "NaN"));

                    window.Left = Left;
                    window.Top = Top;
                    window.Width = width;
                    window.Height = height;
                }
                else
                {
                    // Center other windows in MaaWpfGui.RootView
                    window.Left = Left + ((Application.Current.MainWindow.Width - window.Width) / 2);
                    window.Top = Top + ((Application.Current.MainWindow.Height - window.Height) / 2);
                }
            }

            return window;
        }
    }
}
