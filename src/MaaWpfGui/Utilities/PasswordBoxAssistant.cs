// <copyright file="PasswordBoxAssistant.cs" company="MaaAssistantArknights">
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

using System.Windows;
using HandyControl.Controls;

namespace MaaWpfGui.Utilities
{
    public static class PasswordBoxAssistant
    {
        public static readonly DependencyProperty PasswordProperty =
                DependencyProperty.RegisterAttached("Password",
                typeof(string), typeof(PasswordBoxAssistant),
                new FrameworkPropertyMetadata(string.Empty, OnPasswordPropertyChanged));

        public static readonly DependencyProperty AttachProperty =
            DependencyProperty.RegisterAttached("Attach",
            typeof(bool), typeof(PasswordBoxAssistant), new PropertyMetadata(false, Attach));

        private static readonly DependencyProperty _isUpdatingProperty =
           DependencyProperty.RegisterAttached("_isUpdating", typeof(bool),
           typeof(PasswordBoxAssistant));

        public static void SetAttach(DependencyObject dp, bool value)
        {
            dp.SetValue(AttachProperty, value);
        }

        public static bool GetAttach(DependencyObject dp)
        {
            return (bool)dp.GetValue(AttachProperty);
        }

        public static string GetPassword(DependencyObject dp)
        {
            return (string)dp.GetValue(PasswordProperty);
        }

        public static void SetPassword(DependencyObject dp, string value)
        {
            dp.SetValue(PasswordProperty, value);
        }

        private static bool Get_isUpdating(DependencyObject dp)
        {
            return (bool)dp.GetValue(_isUpdatingProperty);
        }

        private static void Set_isUpdating(DependencyObject dp, bool value)
        {
            dp.SetValue(_isUpdatingProperty, value);
        }

        private static void OnPasswordPropertyChanged(DependencyObject sender,
            DependencyPropertyChangedEventArgs e)
        {
            PasswordBox passwordBox = sender as PasswordBox;
            passwordBox!.ActualPasswordBox.PasswordChanged -= PasswordChanged;

            if (!(bool)Get_isUpdating(passwordBox))
            {
                passwordBox.Password = (string)e.NewValue;
            }
            passwordBox.ActualPasswordBox.PasswordChanged += PasswordChanged;
        }

        private static void Attach(DependencyObject sender,
            DependencyPropertyChangedEventArgs e)
        {
            if (!(sender is PasswordBox passwordBox))
            {
                return;
            }

            if ((bool)e.OldValue)
            {
                passwordBox.ActualPasswordBox.PasswordChanged -= PasswordChanged;
            }

            if ((bool)e.NewValue)
            {
                passwordBox.ActualPasswordBox.PasswordChanged += PasswordChanged;
            }
        }

        private static void PasswordChanged(object sender, RoutedEventArgs e)
        {
            PasswordBox passwordBox = sender as PasswordBox;
            Set_isUpdating(passwordBox, true);
            SetPassword(passwordBox, passwordBox.Password);
            Set_isUpdating(passwordBox, false);
        }
    }
}
