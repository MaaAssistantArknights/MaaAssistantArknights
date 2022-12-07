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
using StyletIoC;

namespace MaaWpfGui.MaaHotKeys
{
    public class MaaHotKeyActionHandler : IMaaHotKeyActionHandler
    {
        private readonly IContainer _container;
        private readonly IMainWindowManager _mainWindowManager;

        public MaaHotKeyActionHandler(IContainer container)
        {
            _container = container;
            _mainWindowManager = container.Get<IMainWindowManager>();
        }

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
            var mainModel = _container.Get<TaskQueueViewModel>();

            if (mainModel.Stopping)
            {
                return;
            }

            if (mainModel.Idle)
            {
                mainModel.LinkStart();

                if (_mainWindowManager.GetWindowState() != WindowState.Minimized)
                {
                    return;
                }

                using (var toast = new ToastNotification(Localization.GetString("BackgroundLinkStarted")))
                {
                    toast.Show();
                }
            }
            else
            {
                mainModel.Stop();

                if (Application.Current.MainWindow == null ||
                    Application.Current.MainWindow.WindowState != WindowState.Minimized)
                {
                    return;
                }

                using (var toast = new ToastNotification(Localization.GetString("BackgroundLinkStopped")))
                {
                    toast.Show();
                }
            }
        }
    }
}
