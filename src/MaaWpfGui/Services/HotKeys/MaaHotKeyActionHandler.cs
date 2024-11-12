// <copyright file="MaaHotKeyActionHandler.cs" company="MaaAssistantArknights">
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
using System.Windows;
using MaaWpfGui.Helper;
using MaaWpfGui.States;

namespace MaaWpfGui.Services.HotKeys
{
    public class MaaHotKeyActionHandler : IMaaHotKeyActionHandler
    {
        private readonly RunningState _runningState;

        /// <summary>
        /// Initializes a new instance of the <see cref="MaaHotKeyActionHandler"/> class.
        /// </summary>
        public MaaHotKeyActionHandler()
        {
            _runningState = RunningState.Instance;
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

        protected virtual void HandleShowGui() => Instances.MainWindowManager.SwitchWindowState();

        protected virtual void HandleLinkStart()
        {
            if (Instances.TaskQueueViewModel.Stopping)
            {
                return;
            }

            if (_runningState.GetIdle())
            {
                Instances.TaskQueueViewModel.LinkStart();

                if (Instances.MainWindowManager.GetWindowState() != WindowState.Minimized)
                {
                    return;
                }

                ToastNotification.ShowDirect(LocalizationHelper.GetString("BackgroundLinkStarted"));
            }
            else
            {
                _ = Instances.TaskQueueViewModel.Stop();

                if (Application.Current.MainWindow == null ||
                    Application.Current.MainWindow.WindowState != WindowState.Minimized)
                {
                    return;
                }

                ToastNotification.ShowDirect(LocalizationHelper.GetString("BackgroundLinkStopped"));
            }
        }
    }
}
