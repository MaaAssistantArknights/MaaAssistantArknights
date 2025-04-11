// <copyright file="MouseWheelHelper.cs" company="MaaAssistantArknights">
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

using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MaaWpfGui.Helper
{
    public static class MouseWheelHelper
    {
        public static void RouteMouseWheelToParent(object sender, MouseWheelEventArgs e)
        {
            if (e.Handled)
            {
                return;
            }

            e.Handled = true;
            var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta)
            {
                RoutedEvent = UIElement.MouseWheelEvent,
            };
            var parent = ((Control)sender).Parent as UIElement;
            parent?.RaiseEvent(eventArg);
        }

        public static void DisableComboBoxScrollWhenClosed(object sender, MouseWheelEventArgs e)
        {
            if (sender is ComboBox { IsDropDownOpen: false })
            {
                e.Handled = true; // 阻止滚动
            }
        }
    }
}
