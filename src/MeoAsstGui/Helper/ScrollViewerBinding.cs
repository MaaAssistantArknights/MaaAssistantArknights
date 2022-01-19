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

using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;

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
        private static readonly DependencyProperty VerticalOffsetBindingProperty =
            DependencyProperty.RegisterAttached("VerticalOffsetBinding", typeof(bool?), typeof(ScrollViewerBinding));

        public static double GetVerticalOffset(DependencyObject depObj)
        {
            if (!(depObj is ScrollViewer))
                return 0;

            return (double)depObj.GetValue(VerticalOffsetProperty);
        }

        public static void SetVerticalOffset(DependencyObject depObj, double value)
        {
            if (!(depObj is ScrollViewer))
                return;

            depObj.SetValue(VerticalOffsetProperty, value);
        }

        private static void OnVerticalOffsetPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ScrollViewer scrollViewer))
                return;

            BindVerticalOffset(scrollViewer);
            scrollViewer.ScrollToVerticalOffset((double)e.NewValue);
        }

        private static void BindVerticalOffset(ScrollViewer scrollViewer)
        {
            if (scrollViewer.GetValue(VerticalOffsetBindingProperty) != null)
                return;

            scrollViewer.SetValue(VerticalOffsetBindingProperty, true);
            scrollViewer.ScrollChanged += (s, se) =>
            {
                if (se.VerticalChange == 0)
                    return;
                SetVerticalOffset(scrollViewer, se.VerticalOffset);
            };
        }

        #endregion VerticalOffset attached property

        #region ViewportHeight attached property

        public static readonly DependencyProperty ViewportHeightProperty =
            DependencyProperty.RegisterAttached("ViewportHeight", typeof(double),
            typeof(ScrollViewerBinding), new FrameworkPropertyMetadata(double.NaN,
                FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                OnViewportHeightPropertyChanged));

        private static readonly DependencyProperty ViewportHeightBindingProperty =
            DependencyProperty.RegisterAttached("ViewportHeightBinding", typeof(bool?), typeof(ScrollViewerBinding));

        public static double GetViewportHeight(DependencyObject depObj)
        {
            if (!(depObj is ScrollViewer scrollViewer))
                return double.NaN;

            return scrollViewer.ViewportHeight;
        }

        public static void SetViewportHeight(DependencyObject depObj, double value)
        {
            if (!(depObj is ScrollViewer))
                return;

            depObj.SetValue(ViewportHeightProperty, value);
        }

        private static void OnViewportHeightPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ScrollViewer scrollViewer))
                return;

            BindViewportHeight(scrollViewer);
        }

        private static void BindViewportHeight(ScrollViewer scrollViewer)
        {
            if (scrollViewer.GetValue(ViewportHeightBindingProperty) != null)
                return;

            scrollViewer.SetValue(ViewportHeightBindingProperty, true);

            scrollViewer.Loaded += (s, se) =>
            {
                SetViewportHeight(scrollViewer, scrollViewer.ViewportHeight);
            };

            scrollViewer.ScrollChanged += (s, se) =>
            {
                SetViewportHeight(scrollViewer, se.ViewportHeight);
            };
        }

        #endregion ViewportHeight attached property

        #region ExtentHeight attached property

        public static readonly DependencyProperty ExtentHeightProperty =
            DependencyProperty.RegisterAttached("ExtentHeight", typeof(double),
            typeof(ScrollViewerBinding), new FrameworkPropertyMetadata(double.NaN,
                FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                OnExtentHeightPropertyChanged));

        private static readonly DependencyProperty ExtentHeightBindingProperty =
            DependencyProperty.RegisterAttached("ExtentHeightBinding", typeof(bool?), typeof(ScrollViewerBinding));

        public static double GetExtentHeight(DependencyObject depObj)
        {
            if (!(depObj is ScrollViewer scrollViewer))
                return double.NaN;

            return scrollViewer.ExtentHeight;
        }

        public static void SetExtentHeight(DependencyObject depObj, double value)
        {
            if (!(depObj is ScrollViewer))
                return;

            depObj.SetValue(ExtentHeightProperty, value);
        }

        private static void OnExtentHeightPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ScrollViewer scrollViewer))
                return;

            BindExtentHeight(scrollViewer);
        }

        private static void BindExtentHeight(ScrollViewer scrollViewer)
        {
            if (scrollViewer.GetValue(ExtentHeightBindingProperty) != null)
                return;

            scrollViewer.SetValue(ExtentHeightBindingProperty, true);

            scrollViewer.Loaded += (s, se) =>
            {
                SetExtentHeight(scrollViewer, scrollViewer.ExtentHeight);
            };
        }

        #endregion ExtentHeight attached property

        #region RectangleVerticalOffsetList attached property

        /// <summary>
        /// RectangleIndex attached property
        /// </summary>
        public static readonly DependencyProperty RectangleVerticalOffsetListProperty =
            DependencyProperty.RegisterAttached("RectangleVerticalOffsetList",
                typeof(List<double>),
                typeof(ScrollViewerBinding),
                new FrameworkPropertyMetadata(new List<double>(),
                    FrameworkPropertyMetadataOptions.AffectsRender |
                    FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                    OnRectangleVerticalOffsetListPropertyChanged));

        /// <summary>
        /// Just a flag that the binding has been applied.
        /// </summary>
        private static readonly DependencyProperty RectangleVerticalOffsetListBindingProperty =
            DependencyProperty.RegisterAttached("RectangleVerticalOffsetListBinding", typeof(bool?), typeof(ScrollViewerBinding));

        public static List<double> GetRectangleVerticalOffsetList(DependencyObject depObj)
        {
            return (List<double>)depObj.GetValue(RectangleVerticalOffsetListProperty);
        }

        public static void SetRectangleVerticalOffsetList(DependencyObject depObj, List<double> value)
        {
            if (!(depObj is ScrollViewer))
                return;

            depObj.SetValue(RectangleVerticalOffsetListProperty, value);
        }

        private static void OnRectangleVerticalOffsetListPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ScrollViewer scrollViewer))
                return;

            BindRectangleVerticalOffsetList(scrollViewer);
        }

        private static void BindRectangleVerticalOffsetList(ScrollViewer scrollViewer)
        {

            if (scrollViewer.GetValue(RectangleVerticalOffsetListBindingProperty) != null)
                return;

            scrollViewer.SetValue(RectangleVerticalOffsetListBindingProperty, true);

            // 当滚动条载入时，遍历 StackPanel 中的所有 Rectangle 子元素对应位置
            scrollViewer.Loaded += (s, se) =>
            {
                if (!scrollViewer.HasContent || !(scrollViewer.Content is StackPanel))
                    return;

                var rectangleOffsetList = new List<double>();
                var stackPanel = (StackPanel)scrollViewer.Content;
                var point = new Point(10, scrollViewer.VerticalOffset);

                foreach (var child in stackPanel.Children)
                {
                    // 以 Rectangle 为定位元素
                    if (!(child is Rectangle))
                        continue;

                    // 转换计算并保存 Rectangle 在滚动面板中的垂直位置
                    var targetPosition = ((FrameworkElement)child).TransformToVisual(scrollViewer).Transform(point);
                    rectangleOffsetList.Add(targetPosition.Y);
                }

                if (rectangleOffsetList.Count > 0)
                    SetRectangleVerticalOffsetList(scrollViewer, rectangleOffsetList);
            };
        }

        #endregion RectangleVerticalOffsetList attached property

    }
}
