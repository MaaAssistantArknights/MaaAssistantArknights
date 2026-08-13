// <copyright file="CrashDiagnosticDialog.xaml.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1636
#nullable enable

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.Diagnostics;
using MaaWpfGui.Services.Diagnostics;

namespace MaaWpfGui.Views.Dialogs;

public partial class CrashDiagnosticDialog : INotifyPropertyChanged
{
    private readonly WpfDiagnosticManager _manager;
    private readonly DiagnosticFailure _failure;
    private bool _isBusy;
    private string _buildReportText;
    private string _reportResult = string.Empty;

    internal CrashDiagnosticDialog(WpfDiagnosticManager manager, DiagnosticFailure failure)
    {
        _manager = manager;
        _failure = failure;
        PromptTitle = LocalizationHelper.GetString("DiagnosticPreviousFailureTitle");
        PromptMessage = LocalizationHelper.GetString("DiagnosticPreviousFailurePrompt");
        TechnicalDetailsHeading = LocalizationHelper.GetString("DiagnosticTechnicalDetails");
        ReportOptionsHeading = LocalizationHelper.GetString("DiagnosticReportContents");
        PrivacyText = LocalizationHelper.GetString("DiagnosticDialogPrivacy");
        IncludeConfigurationText = LocalizationHelper.GetString("DiagnosticIncludeConfiguration");
        IncludeScreenshotsText = LocalizationHelper.GetString("DiagnosticIncludeScreenshots");
        IncludeDumpsText = LocalizationHelper.GetString("DiagnosticIncludeDumps");
        _buildReportText = LocalizationHelper.GetString("DiagnosticBuildReport");
        DismissText = LocalizationHelper.GetString("DiagnosticDismiss");
        FailureSummary = $"{failure.TimestampUtc.ToLocalTime():yyyy-MM-dd HH:mm:ss}  ·  {failure.Code}  ·  {failure.CaseId}";
        TechnicalDetails = BuildTechnicalDetailsSafely(failure);
        InitializeComponent();
        Title = PromptTitle;
        Owner = Application.Current.MainWindow;
        Closing += OnClosing;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string PromptTitle { get; }

    public string PromptMessage { get; }

    public string FailureSummary { get; }

    public string TechnicalDetailsHeading { get; }

    public string TechnicalDetails { get; }

    public string ReportOptionsHeading { get; }

    public string PrivacyText { get; }

    public string IncludeConfigurationText { get; }

    public string IncludeScreenshotsText { get; }

    public string IncludeDumpsText { get; }

    public string BuildReportText
    {
        get => _buildReportText;
        private set {
            _buildReportText = value;
            OnPropertyChanged(nameof(BuildReportText));
        }
    }

    public string DismissText { get; }

    public bool IncludeConfiguration { get; set; } = true;

    public bool IncludeScreenshots { get; set; } = true;

    public bool IncludeDumps { get; set; } = true;

    public bool CanInteract => !_isBusy;

    public string ReportResult
    {
        get => _reportResult;
        private set {
            _reportResult = value;
            OnPropertyChanged(nameof(ReportResult));
        }
    }

    private void Dismiss_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    private async void BuildReport_Click(object sender, RoutedEventArgs e)
    {
        if (_isBusy)
        {
            return;
        }

        SetBusy(true);
        DiagnosticReportResult result;
        try
        {
            result = await _manager.BuildReportAsync(_failure, new() {
                IncludeConfigurationSummary = IncludeConfiguration,
                IncludeScreenshots = IncludeScreenshots,
                IncludeDumps = IncludeDumps,
            });
        }
        catch
        {
            ReportResult = LocalizationHelper.GetString("DiagnosticReportFailed");
            SetBusy(false);
            return;
        }

        SetBusy(false);
        if (result.IsMinimal)
        {
            MessageBoxHelper.Show(LocalizationHelper.GetString("DiagnosticMinimalReportCreated"));
        }

        try
        {
            Process.Start(new ProcessStartInfo("explorer.exe", $"/select,\"{result.ReportPath}\"") { UseShellExecute = true });
        }
        catch
        {
            // Revealing the completed report is optional and must not turn a successful build into an error.
        }

        Close();
    }

    private static string BuildTechnicalDetailsSafely(DiagnosticFailure failure)
    {
        try
        {
            return DiagnosticTechnicalDetailsFormatter.Format(failure);
        }
        catch
        {
            // A malformed legacy diagnostic record must never prevent this dialog from opening.
            return failure.TechnicalDetails ?? string.Empty;
        }
    }

    private void SetBusy(bool value)
    {
        _isBusy = value;
        BuildReportText = LocalizationHelper.GetString(value ? "DiagnosticBuildingReport" : "DiagnosticBuildReport");
        OnPropertyChanged(nameof(CanInteract));
    }

    private void OnClosing(object? sender, CancelEventArgs e)
    {
        if (_isBusy)
        {
            e.Cancel = true;
        }
    }

    private void OnPropertyChanged(string propertyName) => PropertyChanged?.Invoke(this, new(propertyName));
}
