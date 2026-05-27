// <copyright file="RootViewModel.cs" company="MaaAssistantArknights">
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

#nullable enable

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using HandyControl.Data;
using HandyControl.Tools;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Services;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Microsoft.WindowsAPICodePack.Taskbar;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// The root view model.
/// </summary>
public class RootViewModel : Conductor<Screen>.Collection.OneActive
{
    private static readonly ILogger _logger = Log.ForContext<RootViewModel>();

    /// <inheritdoc/>
    protected override void OnViewLoaded()
    {
        InitViewModels();
        _ = InitProxy();
        ShowVersionMismatchWarningOnStartup();
        if (SettingsViewModel.VersionUpdateSettings.VersionType == VersionUpdateSettingsUserControlModel.UpdateVersionType.Nightly &&
            !SettingsViewModel.VersionUpdateSettings.HasAcknowledgedNightlyWarning)
        {
            MessageBoxHelper.Show(LocalizationHelper.GetString("NightlyWarning"));
        }

        Task.Run(async () => {
            await Instances.AnnouncementDialogViewModel.CheckAndDownloadAnnouncement();
            if (Instances.AnnouncementDialogViewModel.DoNotRemindThisAnnouncementAgain)
            {
                return;
            }

            if (Instances.AnnouncementDialogViewModel.DoNotShowAnnouncement)
            {
                return;
            }

            if (Instances.AnnouncementDialogViewModel.AnnouncementInfo != string.Empty)
            {
                _ = Execute.OnUIThreadAsync(() => Instances.WindowManager.ShowWindow(Instances.AnnouncementDialogViewModel));
            }
        });

        _ = Instances.VersionUpdateDialogViewModel.ShowUpdateOrDownload();

        // 主窗口已显示，此时弹窗不会导致 WPF 因无窗口而退出
        Task.Run(ConfigBrokenCheck);
        Task.Run(ToastNotificationCheck);
    }

    private static void ConfigBrokenCheck()
    {
        var recoveryMessage = ConfigFactory.ConsumePendingRecoveryMessage();
        if (recoveryMessage is not null)
        {
            MessageBoxHelper.Show(recoveryMessage, LocalizationHelper.GetString("ConfigurationBrokenCaption"), MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    private static void ToastNotificationCheck()
    {
        var (isAvailable, detail) = ToastNotification.ToastNotificationCheck();
        if (!isAvailable)
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail), UiLogColor.Error);
        }
    }

    private static void ShowVersionMismatchWarningOnStartup()
    {
        var uiVersion = VersionUpdateSettingsUserControlModel.UiVersion;
        var coreVersion = VersionUpdateSettingsUserControlModel.CoreVersion;
        if (!Instances.VersionUpdateDialogViewModel.IsDebugVersion() && uiVersion != coreVersion)
        {
            MessageBoxHelper.Show(
                LocalizationHelper.GetStringFormat("VersionMismatch", uiVersion, coreVersion),
                LocalizationHelper.GetString("Error"),
                iconKey: ResourceToken.FatalGeometry,
                iconBrushKey: ResourceToken.DangerBrush);
        }
    }

    private static async Task InitProxy()
    {
        try
        {
            await Task.Run(Instances.AsstProxy.Init);
        }
        catch
        {
            // ignored
        }
    }

    private void InitViewModels()
    {
        Items.Add(Instances.TaskQueueViewModel);
        Items.Add(Instances.CopilotViewModel);
        Items.Add(Instances.ToolboxViewModel);
        Items.Add(Instances.SettingsViewModel);

        Instances.SettingsViewModel.UpdateWindowTitle(); // 在标题栏上显示模拟器和IP端口 必须在 Items.Add(settings)之后执行。
        ActiveItem = Instances.TaskQueueViewModel;
    }

    private string _windowTitle = "MAA";

    /// <summary>
    /// Gets or sets the window title.
    /// </summary>
    public string WindowTitle
    {
        get => _windowTitle;
        set => SetAndNotify(ref _windowTitle, value);
    }

    private string _windowVersionUpdateInfo = FakeUpdateHelper.HasPendingFakeUpdate
        ? $"{LocalizationHelper.GetString("NewVersionFoundTitle")}: {FakeUpdateHelper.TargetVersion}"
        : string.Empty;

    /// <summary>
    /// Gets or sets the version update info.
    /// </summary>
    public string WindowVersionUpdateInfo
    {
        get => _windowVersionUpdateInfo;
        set => SetAndNotify(ref _windowVersionUpdateInfo, value);
    }

    private string _windowResourceUpdateInfo = string.Empty;

    /// <summary>
    /// Gets or sets the resource update info.
    /// </summary>
    public string WindowResourceUpdateInfo
    {
        get => _windowResourceUpdateInfo;
        set => SetAndNotify(ref _windowResourceUpdateInfo, value);
    }

    private (int Current, int Max)? _taskProgress;

    /// <summary>
    /// Gets or sets the TaskProgress.
    /// 0.0 to 1.0.
    /// 置 0 以隐藏进度条.
    /// </summary>
    public (int Current, int Max)? TaskProgress
    {
        get => _taskProgress;
        set {
            SetAndNotify(ref _taskProgress, value);

            Execute.OnUIThreadAsync(() => {
                if (Application.Current.MainWindow == null || !Application.Current.MainWindow.IsVisible)
                {
                    return;
                }

                try
                {
                    if (value is null)
                    {
                        TaskbarManager.Instance.SetProgressValue(0, 0, Application.Current.MainWindow);
                    }
                    else
                    {
                        TaskbarManager.Instance.SetProgressValue(value.Value.Current, value.Value.Max, Application.Current.MainWindow);
                    }
                }
                catch (Exception e)
                {
                    // 不知道会不会有异常，先捕获一下
                    Logger.Warning("TaskbarManager Exception: " + e.Message);
                }
            });
        }
    }

    private bool _windowTitleScrollable = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleScrollable, false);

    /// <summary>
    /// Gets or sets a value indicating whether to scroll the window title.
    /// </summary>
    public bool WindowTitleScrollable
    {
        get => _windowTitleScrollable;
        set => SetAndNotify(ref _windowTitleScrollable, value);
    }

    private bool _showCloseButton = !ConfigurationHelper.GetGlobalValue(ConfigurationKeys.HideCloseButton, false);

    /// <summary>
    /// Gets or sets a value indicating whether to show close button.
    /// </summary>
    public bool ShowCloseButton
    {
        get => _showCloseButton;
        set => SetAndNotify(ref _showCloseButton, value);
    }

    private bool _isWindowTopMost;

    public bool IsWindowTopMost
    {
        get => _isWindowTopMost;
        set {
            if (_isWindowTopMost == value)
            {
                return;
            }

            SetAndNotify(ref _isWindowTopMost, value);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void ToggleTopMostCommand()
    {
        IsWindowTopMost = !IsWindowTopMost;
    }

    [UsedImplicitly]
    public void ManualPackagePreviewDragOver(object sender, DragEventArgs e)
    {
        if (!TryGetDroppedZipFile(e, out _))
        {
            return;
        }

        e.Effects = DragDropEffects.Copy;
        e.Handled = true;
    }

    [UsedImplicitly]
    public void ManualPackageDrop(object sender, DragEventArgs e)
    {
        if (!TryGetDroppedZipFile(e, out string packagePath))
        {
            return;
        }

        _logger.Information("Dropped zip file detected in main window: {PackagePath}", packagePath);
        e.Handled = true;
        HandleImportedPackage(packagePath);
    }

    /// <inheritdoc/>
    protected override void OnClose()
    {
        Bootstrapper.Shutdown();
    }

    private static readonly string[] _gitList =
    [
        "/Res/Img/EasterEgg/1.gif",
        "/Res/Img/EasterEgg/2.gif",
        "/Res/Img/EasterEgg/3.gif",
    ];

    private static int _gifIndex = -1;

    private static string _gifPath = string.Empty;

    private static bool TryGetDroppedZipFile(DragEventArgs e, out string packagePath)
    {
        packagePath = string.Empty;
        if (!e.Data.GetDataPresent(DataFormats.FileDrop))
        {
            return false;
        }

        if (e.Data.GetData(DataFormats.FileDrop) is not string[] files || files.Length == 0)
        {
            return false;
        }

        string candidatePath = files[0];
        if (!File.Exists(candidatePath) || !candidatePath.EndsWith(".zip", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        packagePath = candidatePath;
        return true;
    }

    private static void HandleImportedPackage(string packagePath)
    {
        string currentVersion = VersionUpdateSettingsUserControlModel.CoreVersion;
        string architecture = RuntimeInformation.OSArchitecture.ToString().ToLowerInvariant();
        string normalizedArchitecture = architecture.StartsWith("arm", StringComparison.OrdinalIgnoreCase)
            ? "arm64"
            : "x64";

        PendingUpdateApplier.FullPackageInspectionResult fullPackageInspection =
            PendingUpdateApplier.InspectSupportedLocalFullPackage(packagePath, currentVersion, architecture);

        if (fullPackageInspection.IsSupported
            && !Dialogs.VersionUpdateDialogViewModel.ConfirmFullPackageUpdate(packagePath))
        {
            _logger.Information("Dropped full package import canceled by user before registration: {PackagePath}", packagePath);
            return;
        }

        var importResult = PendingUpdateApplier.TryRegisterLocalPackage(
            packagePath,
            currentVersion,
            architecture,
            fullPackageInspection);
        _logger.Information(
            "Dropped zip import result: status={Status}, sourceVersion={SourceVersion}, targetVersion={TargetVersion}",
            importResult.Status,
            importResult.SourceVersion,
            importResult.TargetVersion);

        switch (importResult.Status)
        {
            case PendingUpdateApplier.LocalPackageImportStatus.OtaPackageRegistered:
            case PendingUpdateApplier.LocalPackageImportStatus.FullPackageRegistered:
                string targetVersion = importResult.TargetVersion ?? string.Empty;
                bool preserveExistingUpdateInfo = PendingUpdateApplier.ShouldPreserveExistingUpdateBody(targetVersion);
                Instances.VersionUpdateDialogViewModel.UpdateTag = targetVersion;
                if (!preserveExistingUpdateInfo)
                {
                    Instances.VersionUpdateDialogViewModel.UpdateInfo = string.Empty;
                }

                Instances.VersionUpdateDialogViewModel.UpdatePackageName = packagePath;
                _logger.Information(
                    "Showing restart prompt for imported update package: {PackagePath}, status={Status}",
                    packagePath,
                    importResult.Status);
                _ = Instances.VersionUpdateDialogViewModel.AskToRestartForImportedPackage();
                return;

            default:
                _logger.Warning("Showing unsupported package warning for dropped package: {PackagePath}", packagePath);
                MessageBoxHelper.Show(
                    LocalizationHelper.GetStringFormat("LocalUpdatePackageUnsupported", Path.GetFileName(packagePath), currentVersion, normalizedArchitecture),
                    LocalizationHelper.GetString("Warning"),
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning,
                    ok: LocalizationHelper.GetString("Ok"));
                return;
        }
    }

    public string GifPath
    {
        get => _gifPath;
        set => SetAndNotify(ref _gifPath, value);
    }

    private bool _gifVisibility = true;

    public bool GifVisibility
    {
        get => _gifVisibility;
        set => SetAndNotify(ref _gifVisibility, value);
    }

    public void ChangeGif()
    {
        if (++_gifIndex >= _gitList.Length)
        {
            _gifIndex = 0;
        }

        GifPath = _gitList[_gifIndex];
    }

    private static bool _isDragging = false;
    private static Point _offset;

    // UI 绑定的方法
    [UsedImplicitly]
    public void DraggableElementMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is not HandyControl.Controls.GifImage childElement)
        {
            return;
        }

        _isDragging = true;
        _offset = e.GetPosition(childElement);
        childElement.CaptureMouse();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DraggableElementMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (sender is not HandyControl.Controls.GifImage childElement)
        {
            return;
        }

        _isDragging = false;
        childElement.ReleaseMouseCapture();
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DraggableElementMouseMove(object sender, MouseEventArgs e)
    {
        if (!_isDragging || sender is not HandyControl.Controls.GifImage { Parent: Grid parentElement } childElement)
        {
            return;
        }

        Point currentPosition = e.GetPosition(parentElement);

        // 计算偏移量
        double newX = currentPosition.X - _offset.X;
        double newY = currentPosition.Y - _offset.Y;

        // 确保元素在父元素范围内
        newX = Math.Max(10, Math.Min(newX, parentElement.ActualWidth - childElement.ActualWidth - 10));
        newY = Math.Max(10, Math.Min(newY, parentElement.ActualHeight - childElement.ActualHeight - 10));

        childElement.HorizontalAlignment = HorizontalAlignment.Left;
        childElement.VerticalAlignment = VerticalAlignment.Top;
        childElement.Margin = new(newX, newY, 10, 10);
    }
}
