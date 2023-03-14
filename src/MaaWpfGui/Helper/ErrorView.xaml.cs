// <copyright file="ErrorView.xaml.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Helper;
using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Documents;

namespace MaaWpfGui.Views
{
    /// <summary>
    ///     ErrorView.xaml 的交互逻辑
    /// </summary>
    public partial class ErrorView : Window
    {
        protected bool ShouldExit { get; set; }

        public ErrorView()
        {
            InitializeComponent();
        }

        public ErrorView(string error, string details, bool shouldExit)
        {
            InitializeComponent();
            Title = error;
            Error.Text = error;
            ErrorDetails.Text = details;
            ErrorSolution.Text = GetSolution(error, details);

            ShouldExit = shouldExit;

            var isZhCn = ViewStatusStorage.Get(ConfigKeys.Localization, Localization.DefaultLanguage) == "zh-cn";
            ErrorQqGroupLink.Visibility = isZhCn ? Visibility.Visible : Visibility.Hidden;
        }

        private string GetSolution(string error, string details)
        {
            if (details.Contains("AsstGetVersion()") || details.Contains("DllNotFoundException"))
            {
                return Localization.GetString("ErrorSolutionCrash");
            }
            else if (details.Contains("SettingsViewModel.<ReplaceADB>"))
            {
                return Localization.GetString("ErrorSolutionReplaceADB");
            }

            return Localization.GetString("UnknownErrorOccurs");
        }

        protected override void OnClosed(EventArgs e)
        {
            if (ShouldExit)
            {
                Environment.Exit(0);
            }

            base.OnClosed(e);
        }

        private void Hyperlink_OnClick(object sender, RoutedEventArgs e)
        {
            Process.Start(((Hyperlink)sender).NavigateUri.AbsoluteUri);
        }
    }
}
