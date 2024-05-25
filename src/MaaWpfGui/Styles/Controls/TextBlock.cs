// <copyright file="TextBlock.cs" company="MaaAssistantArknights">
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
using System.Windows.Media;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Styles.Controls
{
    public class TextBlock : System.Windows.Controls.TextBlock
    {
        static TextBlock()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(TextBlock), new FrameworkPropertyMetadata(typeof(TextBlock)));
        }

        public static readonly DependencyProperty CustomForegroundProperty = DependencyProperty.Register(nameof(CustomForeground), typeof(Brush), typeof(TextBlock), new PropertyMetadata(ThemeHelper.DefaultBrush));

        public Brush CustomForeground
        {
            get { return (Brush)GetValue(CustomForegroundProperty); }
            set { SetValue(CustomForegroundProperty, value); }
        }

        public static readonly DependencyProperty ForegroundKeyProperty = DependencyProperty.Register(nameof(ForegroundKey), typeof(string), typeof(TextBlock), new PropertyMetadata(ThemeHelper.DefaultKey, OnForegroundKeyChanged));

        private static void OnForegroundKeyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var element = (TextBlock)d;
            if (e.NewValue != null)
            {
                element.ForegroundKey = (string)e.NewValue;
            }
        }

        public string ForegroundKey
        {
            get
            {
                return (string)GetValue(ForegroundKeyProperty);
            }

            set
            {
                SetValue(ForegroundKeyProperty, value);
                if (Application.Current.Resources.Contains(value))
                {
                    SetResourceReference(ForegroundProperty, value);
                    return;
                }

                var brush = ThemeHelper.String2Brush(value);
                if (ThemeHelper.SimilarToBackground(brush.Color))
                {
                    SetResourceReference(ForegroundProperty, ThemeHelper.DefaultKey);
                    return;
                }

                SetValue(ForegroundProperty, brush);
            }
        }
    }
}
