// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System.Windows;
using System.Windows.Controls;

namespace MeoAsstGui
{
    public static class ScrollViewerBinding
    {
        #region VerticalOffset attached property

        /// <summary>
        /// VerticalOffset attached property
        /// </summary>
        public static readonly DependencyProperty VerticalOffsetProperty =
            DependencyProperty.RegisterAttached("VerticalOffset", typeof(double),
            typeof(ScrollViewerBinding), new FrameworkPropertyMetadata(double.NaN,
                FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                OnVerticalOffsetPropertyChanged));

        /// <summary>
        /// Just a flag that the binding has been applied.
        /// </summary>
        private static readonly DependencyProperty VerticalScrollBindingProperty =
            DependencyProperty.RegisterAttached("VerticalScrollBinding", typeof(bool?), typeof(ScrollViewerBinding));

        public static double GetVerticalOffset(DependencyObject depObj)
        {
            ScrollViewer scrollViewer = depObj as ScrollViewer;
            if (scrollViewer == null)
                return 0;
            double maxHeight = scrollViewer.ScrollableHeight;
            return (double)depObj.GetValue(VerticalOffsetProperty) / maxHeight;
        }

        public static void SetVerticalOffset(DependencyObject depObj, double value)
        {
            if (value <= 1)
            {
                // by set
                ScrollViewer scrollViewer = depObj as ScrollViewer;
                if (scrollViewer == null)
                    return;
                double maxHeight = scrollViewer.ScrollableHeight;

                depObj.SetValue(VerticalOffsetProperty, value * maxHeight);
            }
            else
            {
                // by scroll
                depObj.SetValue(VerticalOffsetProperty, value);
            }
        }

        private static void OnVerticalOffsetPropertyChanged(DependencyObject d,
            DependencyPropertyChangedEventArgs e)
        {
            ScrollViewer scrollViewer = d as ScrollViewer;
            if (scrollViewer == null)
                return;

            BindVerticalOffset(scrollViewer);
            scrollViewer.ScrollToVerticalOffset((double)e.NewValue);
        }

        public static void BindVerticalOffset(ScrollViewer scrollViewer)
        {
            if (scrollViewer.GetValue(VerticalScrollBindingProperty) != null)
                return;

            scrollViewer.SetValue(VerticalScrollBindingProperty, true);
            scrollViewer.ScrollChanged += (s, se) =>
            {
                if (se.VerticalChange == 0)
                    return;
                SetVerticalOffset(scrollViewer, se.VerticalOffset);
            };
        }

        #endregion VerticalOffset attached property

        #region ScrollableHeight attached property

        //public static readonly DependencyProperty ScrollableHeightProperty =
        //    DependencyProperty.RegisterAttached("ScrollableHeight", typeof(double),
        //    typeof(ScrollViewerBinding), new FrameworkPropertyMetadata(double.NaN, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnScrollableHeightPropertyChanged));

        //public static double GetScrollableHeight(DependencyObject depObj)
        //{
        //    ScrollViewer scrollViewer = depObj as ScrollViewer;
        //    if (scrollViewer == null)
        //        return 0;
        //    return scrollViewer.ScrollableHeight;
        //}

        //public static void SetScrollableHeight(DependencyObject depObj, double value)
        //{
        //    // pass
        //}

        //private static readonly DependencyProperty ScrollableHeightBindingProperty =
        //    DependencyProperty.RegisterAttached("ScrollableHeightBinding", typeof(bool?), typeof(ScrollViewerBinding));

        //private static void OnScrollableHeightPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        //{
        //}

        #endregion ScrollableHeight attached property
    }
}
