// <copyright file="NotificationChannelViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System;
using System.ComponentModel;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public sealed class NotificationChannelViewModel : PropertyChangedBase, IDisposable
{
    public NotificationChannelViewModel(NotificationChannelConfig config)
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        Config = config;
        Provider = ExternalNotificationChannel.ForConfig(config.Provider).ReadConfig(config.Provider);
        DeliverySettings = new(config.Delivery);
        Provider.PropertyChanged += ProviderChanged;
        config.Delivery.PropertyChanged += PolicyChanged;
    }

    public NotificationChannelConfig Config { get; }

    public Guid Id => Config.Id;

    public BaseConfig Provider { get; }

    public ExternalNotificationDeliverySettingsModel DeliverySettings { get; }

    public int Position { get; set => SetAndNotify(ref field, value); }

    [PropertyDependsOn(nameof(Position))]
    public string DisplayName => $"{ExternalNotificationChannel.ForEditor(Provider)!.DisplayName} ({Position})";

    public void RefreshLabel(int position)
    {
        Position = position;
        NotifyOfPropertyChange(nameof(DisplayName));
    }

    public void TestChannel() => Instances.NotificationService.TestExternalNotification(Id);

    public void Dispose()
    {
        PropertyDependsOnUtility.UnInitializePropertyDependencies(this);
        Provider.PropertyChanged -= ProviderChanged;
        Config.Delivery.PropertyChanged -= PolicyChanged;
    }

    private void ProviderChanged(object? sender, PropertyChangedEventArgs args) => Config.Provider = Provider.ToConfig();

    private void PolicyChanged(object? sender, PropertyChangedEventArgs args)
    {
        if (args.PropertyName is nameof(ExternalNotification.DeliverySettings.Enable)
            or nameof(ExternalNotification.DeliverySettings.SendWhenComplete)
            or nameof(ExternalNotification.DeliverySettings.SendWhenError)
            or nameof(ExternalNotification.DeliverySettings.SendWhenStalled)
            or nameof(ExternalNotification.DeliverySettings.SendBeforeScheduledStart)
            or nameof(ExternalNotification.DeliverySettings.SendAfterLogCount)
            or nameof(ExternalNotification.DeliverySettings.NewLogCount)
            or nameof(ExternalNotification.DeliverySettings.SendWhenContentMatches)
            or nameof(ExternalNotification.DeliverySettings.Whitelist))
        {
            Instances.NotificationService?.ResetChannel(Id);
        }
        NotifyOfPropertyChange(nameof(DeliverySettings));
    }
}
