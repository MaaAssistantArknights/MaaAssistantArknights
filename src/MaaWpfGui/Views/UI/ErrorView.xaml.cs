// <copyright file="ErrorView.xaml.cs" company="MaaAssistantArknights">
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

        private bool _congratulationsOnError = true;

        public string ErrorString { get; set; } = LocalizationHelper.GetString("Error");

        public string ErrorCongratulationsString { get; set; } = LocalizationHelper.GetString("ErrorCongratulations");

        /// <summary>
        /// Gets or sets a value indicating whether to enable congratulation mode for ErrorView.
        /// </summary>
        public bool CongratulationsOnError
        {
            get => _congratulationsOnError;
            set
            {
                _congratulationsOnError = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(CongratulationsOnError)));
            }
        }

        public ErrorView()
        {
            InitializeComponent();
        }

        public ErrorView(Exception exc, bool shouldExit)
        {
            InitializeComponent();
            var exc0 = exc;
            var errorStr = new StringBuilder();
            while (true)
            {
                errorStr.Append(exc.Message);
                if (exc.InnerException != null)
                {
                    errorStr.AppendLine();
                    exc = exc.InnerException;
                }
                else
                {
                    break;
                }
            }

            var error = errorStr.ToString();
            var details = exc0.ToString();
            ExceptionMessage = error;
            ExceptionDetails = details;
            PossibleSolution = GetSolution(error, details);
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ExceptionMessage)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(PossibleSolution)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ExceptionDetails)));
            ShouldExit = shouldExit;

            var isZhCn = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage) == "zh-cn";
            ErrorQqGroupLink.Visibility = isZhCn ? Visibility.Visible : Visibility.Collapsed;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private static string GetSolution(string error, string details)
        {
            _ = error; // To avoid warning
            if (details.Contains("MaaCore.dll not found!") ||
                details.Contains("resource folder not found!"))
            {
                return LocalizationHelper.GetString("ErrorSolutionMoveMaaExeOutOfFolder");
            }

            if (details.Contains("AsstGetVersion()") ||
                details.Contains("DllNotFoundException") ||
                details.Contains("lambda_method") ||
                details.Contains("HandyControl") ||
                (details.Contains("System.Net.Http") && details.Contains("Version")))
            {
                return LocalizationHelper.GetString("ErrorSolutionCrash");
            }

            if (details.Contains("CheckAndUpdateNow()") && details.Contains("MoveFile"))
            {
                return LocalizationHelper.GetString("ErrorSolutionUpdatePackageExtractionFailed");
            }

            if (details.Contains("Hyperlink_Click") && details.Contains("StartWithShellExecuteEx"))
            {
                return LocalizationHelper.GetString("ErrorSolutionSelectDefaultBrowser");
            }

            // ReSharper disable once ConvertIfStatementToReturnStatement
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
            Process.Start(new ProcessStartInfo(((Hyperlink)sender).NavigateUri.AbsoluteUri) { UseShellExecute = true });
        }

        private void CopyToClipboard()
        {
            var data = new DataObject();
            var textToCopy =
                $"{LocalizationHelper.GetString("ErrorProlog")}\n" +
                $"\t{ExceptionMessage}\n" +
                $"{LocalizationHelper.GetString("ErrorSolution")}\n" +
                $"\t{PossibleSolution}\n" +
                $"{LocalizationHelper.GetString("ErrorDetails")}\n" +
                $"{ExceptionDetails}";
            data.SetText(textToCopy);

            try
            {
                Clipboard.SetDataObject(data, true);
            }
            catch
            {
                // 有时候报错了也能复制上去，这个时候复制不了也没办法了
            }
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
