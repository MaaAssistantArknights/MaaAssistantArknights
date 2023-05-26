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

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Views.UI
{
    /// <summary>
    ///     ErrorView.xaml 的交互逻辑
    /// </summary>
    public partial class ErrorView : INotifyPropertyChanged
    {
        protected bool ShouldExit { get; set; }

        public string ExceptionMessage { get; set; }

        public string PossibleSolution { get; set; }

        public string ExceptionDetails { get; set; }

        public ErrorView()
        {
            InitializeComponent();
        }

        public ErrorView(Exception exc, bool shouldExit)
        {
            InitializeComponent();
            var exc0 = exc;
            var errorb = new StringBuilder();
            while (true)
            {
                errorb.Append(exc.Message);
                exc = exc.InnerException;
                if (exc != null)
                {
                    errorb.AppendLine();
                }
                else
                {
                    break;
                }
            }

            var error = errorb.ToString();
            var details = exc0.ToString();
            ExceptionMessage = error;
            ExceptionDetails = details;
            PossibleSolution = GetSolution(error, details);
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ExceptionMessage)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(PossibleSolution)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ExceptionDetails)));
            ShouldExit = shouldExit;

            var isZhCn = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage) == "zh-cn";
            ErrorQqGroupLink.Visibility = isZhCn ? Visibility.Visible : Visibility.Collapsed;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private string GetSolution(string error, string details)
        {
            if (details.Contains("AsstGetVersion()") ||
                details.Contains("DllNotFoundException") ||
                details.Contains("lambda_method") ||
                details.Contains("HandyControl"))
            {
                return LocalizationHelper.GetString("ErrorSolutionCrash");
            }

            if (details.Contains("System.IO.File.InternalMove"))
            {
                return LocalizationHelper.GetString("ErrorSolutionFailedToMove");
            }

            return LocalizationHelper.GetString("UnknownErrorOccurs");
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

        private void CopyToClipboard()
        {
            var range = new TextRange(RichTextBox.Document.ContentStart, RichTextBox.Document.ContentEnd);
            var data = new DataObject();
            data.SetText(range.Text);
            if (range.CanSave(DataFormats.Rtf))
            {
                var ms = new MemoryStream();
                range.Save(ms, DataFormats.Rtf);
                var arr = ms.ToArray();

                // Save to RTF doesn't write non-ascii characters (implementation-defined behavior?)
                data.SetData(DataFormats.Rtf, Encoding.UTF8.GetString(arr));
            }

            Clipboard.SetDataObject(data, true);
        }

        private async void CopyErrorMessage_Click(object sender, RoutedEventArgs e)
        {
            CopyToClipboard();
            CopiedTip.IsOpen = true;
            await Task.Delay(3000);
            CopiedTip.IsOpen = false;
        }
    }
}
