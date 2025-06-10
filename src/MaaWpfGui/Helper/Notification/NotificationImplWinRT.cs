// <copyright file="NotificationImplWinRT.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using Microsoft.Toolkit.Uwp.Notifications;

namespace MaaWpfGui.Helper.Notification;

internal class NotificationImplWinRT : INotificationPoster, IDisposable
{
    public event EventHandler<string> ActionActivated;

    public NotificationImplWinRT()
    {
        ToastNotificationManagerCompat.OnActivated += OnWinRTActivated;
    }

    public void Dispose()
    {
        ToastNotificationManagerCompat.OnActivated -= OnWinRTActivated;

        // ToastNotificationManagerCompat.History.Clear();
    }

    private void OnWinRTActivated(ToastNotificationActivatedEventArgsCompat args)
    {
        ActionActivated?.Invoke(this, args.Argument);
    }

    public void ShowNotification(NotificationContent content)
    {
        try
        {
            Application.Current.Dispatcher.InvokeAsync(() =>
            {
                var builder = new ToastContentBuilder().AddText(content.Body).AddText(content.Summary);

                foreach (var action in content.Actions)
                {
                    builder.AddButton(new ToastButton()
                        .SetContent(action.Label)
                        .AddArgument(action.Tag));
                }

                builder.GetToastContent().ActivationType = ToastActivationType.Protocol;
                builder.Show();
            });
        }
        catch
        {
            // ignored
        }
    }
}
