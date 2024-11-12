// <copyright file="AutoScroll.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;

namespace MaaWpfGui.Styles.Properties
{
    /// <summary>
    /// The auto scroll property.
    /// </summary>
    public static class AutoScroll
    {
        private static bool _autoScroll;

        /// <summary>
        /// Gets auto scroll property.
        /// </summary>
        /// <param name="obj">The <see cref="DependencyObject"/> instance.</param>
        /// <returns>The property value.</returns>
        // ReSharper disable once UnusedMember.Global
        public static bool GetAutoScroll(DependencyObject obj)
        {
            return (bool)obj.GetValue(AutoScrollProperty);
        }

        /// <summary>
        /// Sets auto scroll property.
        /// </summary>
        /// <param name="obj">The <see cref="DependencyObject"/> instance.</param>
        /// <param name="value">The new property value.</param>
        public static void SetAutoScroll(DependencyObject obj, bool value)
        {
            obj.SetValue(AutoScrollProperty, value);
        }

        /// <summary>
        /// The auto scroll property.
        /// </summary>
        public static readonly DependencyProperty AutoScrollProperty =
            DependencyProperty.RegisterAttached("AutoScroll", typeof(bool), typeof(AutoScroll), new PropertyMetadata(false, AutoScrollPropertyChanged));

        private static void AutoScrollPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is ScrollViewer scrollViewer)
            {
                bool alwaysScrollToEnd = (e.NewValue != null) && (bool)e.NewValue;
                if (alwaysScrollToEnd)
                {
                    scrollViewer.ScrollToEnd();
                    scrollViewer.ScrollChanged += ScrollChanged;
                }
                else
                {
                    scrollViewer.ScrollChanged -= ScrollChanged;
                }
            }
            else if (d is ListBox listBox)
            {
                INotifyCollectionChanged view = listBox.Items;
                view.CollectionChanged += (sender, arg) =>
                {
                    switch (arg.Action)
                    {
                        case NotifyCollectionChangedAction.Add:
                            listBox.ScrollIntoView(listBox.Items[arg.NewStartingIndex]);
                            break;
                    }
                };
            }
            else
            {
                throw new InvalidOperationException("The attached AlwaysScrollToEnd property can only be applied to ScrollViewer instances.");
            }
        }

        private static void ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (sender is not ScrollViewer scroll)
            {
                throw new InvalidOperationException("The attached AlwaysScrollToEnd property can only be applied to ScrollViewer instances.");
            }

            if (e.ExtentHeightChange == 0)
            {
                _autoScroll = Math.Abs(scroll.VerticalOffset - scroll.ScrollableHeight) < 1e-6;
            }

            if (_autoScroll && e.ExtentHeightChange != 0)
            {
                scroll.ScrollToVerticalOffset(scroll.ExtentHeight);
            }
        }
    }
}
