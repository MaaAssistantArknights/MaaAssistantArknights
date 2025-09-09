// <copyright file="ResourceReferenceHelper.cs" company="MaaAssistantArknights">
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

using System.Windows;
using System.Windows.Controls;

namespace MaaWpfGui.Extensions.UIBehaviors
{
    public static class ResourceReferenceHelper
    {
        public static readonly DependencyProperty ForegroundResourceKeyProperty =
            DependencyProperty.RegisterAttached(
                "ForegroundResourceKey",
                typeof(string),
                typeof(ResourceReferenceHelper),
                new PropertyMetadata(null, (d, e) =>
                {
                    if (d is FrameworkElement fe && e.NewValue is string key)
                    {
                        fe.SetResourceReference(Control.ForegroundProperty, key);
                    }
                }));

        public static void SetForegroundResourceKey(DependencyObject d, string value) => d.SetValue(ForegroundResourceKeyProperty, value);

        public static string GetForegroundResourceKey(DependencyObject d) => (string)d.GetValue(ForegroundResourceKeyProperty);

        public static readonly DependencyProperty BackgroundResourceKeyProperty =
            DependencyProperty.RegisterAttached(
                "BackgroundResourceKey",
                typeof(string),
                typeof(ResourceReferenceHelper),
                new PropertyMetadata(null, (d, e) =>
                {
                    if (d is FrameworkElement fe && e.NewValue is string key)
                    {
                        fe.SetResourceReference(Control.BackgroundProperty, key);
                    }
                }));

        public static void SetBackgroundResourceKey(DependencyObject d, string value) => d.SetValue(BackgroundResourceKeyProperty, value);

        public static string GetBackgroundResourceKey(DependencyObject d) => (string)d.GetValue(BackgroundResourceKeyProperty);
    }
}
