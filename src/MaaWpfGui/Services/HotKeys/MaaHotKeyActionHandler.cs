// <copyright file="MaaHotKeyActionHandler.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Managers;
using MaaWpfGui.ViewModels.UI;
using StyletIoC;

namespace MaaWpfGui.Services.HotKeys
{
    public class MaaHotKeyActionHandler : IMaaHotKeyActionHandler
    {
        private readonly IMainWindowManager _mainWindowManager;

        private readonly TaskQueueViewModel _taskQueueViewModel;

        /// <summary>
        /// Initializes a new instance of the <see cref="MaaHotKeyActionHandler"/> class.
        /// </summary>
        /// <param name="container"></param>
        public MaaHotKeyActionHandler(IContainer container)
        {
            _taskQueueViewModel = container.Get<TaskQueueViewModel>();
            _mainWindowManager = container.Get<IMainWindowManager>();
        }

        /// <inheritdoc/>
        public void HandleKeyPressed(MaaHotKeyAction action)
        {
            switch (action)
            {
                case MaaHotKeyAction.ShowGui:
                    HandleShowGui();
                    break;
                case MaaHotKeyAction.LinkStart:
                    HandleLinkStart();
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(action), action, null);
            }
        }

        protected virtual void HandleShowGui() => _mainWindowManager.SwitchWindowState();

        protected virtual void HandleLinkStart()
        {
            if (_taskQueueViewModel.Stopping)
            {
                return;
            }

            if (_taskQueueViewModel.Idle)
            {
                _taskQueueViewModel.LinkStart();

                if (_mainWindowManager.GetWindowState() != WindowState.Minimized)
                {
                    return;
                }

                using var toast = new ToastNotification(LocalizationHelper.GetString("BackgroundLinkStarted"));
                toast.Show();
            }
            else
            {
                _ = _taskQueueViewModel.Stop();

                if (Application.Current.MainWindow == null ||
                    Application.Current.MainWindow.WindowState != WindowState.Minimized)
                {
                    return;
                }

                using var toast = new ToastNotification(LocalizationHelper.GetString("BackgroundLinkStopped"));
                toast.Show();
            }
        }
    }
}
