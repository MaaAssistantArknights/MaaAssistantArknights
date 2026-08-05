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
using HandyControl.Controls;
using HandyControl.Data;
using HandyControl.Tools;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Microsoft.WindowsAPICodePack.Taskbar;
using Serilog;
using Stylet;
using Point = System.Windows.Point;

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

        // 宿醉彩蛋弹窗依赖 RootView 已渲染完毕的 Dialog 容器，
        // 必须在主窗口显示之后再弹出，否则弹不出
        // 必须在其他内容初始化之前执行，否则其他内容语言可能已经被初始化为非宿醉语言
        Instances.SettingsViewModel.HangoverEnd();

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
        if (!SettingsViewModel.GuiSettings.UseNotify)
        {
            return;
        }

        var (isAvailable, detail) = ToastNotification.ToastNotificationCheck();
        if (!isAvailable)
        {
            Growl.Error(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail));
            _logger.Error(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail));
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

    /// <summary>
    /// Gets or sets a value indicating whether to scroll the window title.
    /// </summary>
    public bool WindowTitleScrollable { get; set => SetAndNotify(ref field, value); } = ConfigFactory.Root.Gui.WindowTitleScrollable;

    /// <summary>
    /// Gets or sets a value indicating whether to show close button.
    /// </summary>
    public bool ShowCloseButton { get; set => SetAndNotify(ref field, value); } = !ConfigFactory.Root.Gui.HideCloseButton;

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

        PendingUpdateApplier.PackageInspectionResult packageInspection =
            PendingUpdateApplier.InspectLocalUpdatePackage(packagePath, currentVersion, architecture);

#if DEBUG
        // Debug 专用：Ctrl+Shift 拖入时只做检测判断，不实际注册，用于快速验证正则匹配
        if (Keyboard.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift))
        {
            DebugInspectDroppedPackage(packagePath, packageInspection, currentVersion, normalizedArchitecture);
            return;
        }
#endif

        // 不是版本更新包文件名模式时，尝试作为资源更新包导入（需读取 zip entry，开销较高）
        if (!packageInspection.MatchedPattern)
        {
            if (ResourceUpdater.IsResourcePackage(packagePath, out var resourceDateTime))
            {
                _logger.Information("Dropped package detected as resource package: {PackagePath}", packagePath);
                _ = ResourceUpdater.ImportLocalResourcePackageAndReloadAsync(packagePath, resourceDateTime);
                return;
            }

            ShowUnsupportedPackageWarning(packagePath, currentVersion, normalizedArchitecture);
            return;
        }

        // 版本更新包模式匹配，但架构或版本方向被拒
        if (!packageInspection.IsSupported)
        {
            ShowUnsupportedPackageWarning(packagePath, currentVersion, normalizedArchitecture);
            return;
        }

        // 完整包覆盖安装需用户二次确认（OTA 增量包不需要）
        if (packageInspection.Status == PendingUpdateApplier.PackageInspectionStatus.FullSupported
            && !Dialogs.VersionUpdateDialogViewModel.ConfirmFullPackageUpdate(packagePath))
        {
            _logger.Information("Dropped full package import canceled by user before registration: {PackagePath}", packagePath);
            return;
        }

        var importResult = PendingUpdateApplier.TryRegisterLocalPackage(
            packagePath,
            currentVersion,
            architecture,
            packageInspection);
        _logger.Information(
            "Dropped zip import result: status={Status}, sourceVersion={SourceVersion}, targetVersion={TargetVersion}",
            importResult.Status,
            importResult.SourceVersion,
            importResult.TargetVersion);

        if (importResult.Status
            is PendingUpdateApplier.LocalPackageImportStatus.OtaPackageRegistered
            or PendingUpdateApplier.LocalPackageImportStatus.FullPackageRegistered)
        {
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
        }

        ShowUnsupportedPackageWarning(packagePath, currentVersion, normalizedArchitecture);
    }

    private static void ShowUnsupportedPackageWarning(string packagePath, string currentVersion, string normalizedArchitecture)
    {
        _logger.Warning("Showing unsupported package warning for dropped package: {PackagePath}", packagePath);
        MessageBoxHelper.Show(
            LocalizationHelper.GetStringFormat("LocalUpdatePackageUnsupported", Path.GetFileName(packagePath), currentVersion, normalizedArchitecture),
            LocalizationHelper.GetString("Warning"),
            MessageBoxButton.OK,
            MessageBoxImage.Warning,
            ok: LocalizationHelper.GetString("Ok"));
    }

#if DEBUG
    /// <summary>
    /// Debug 专用：展示拖入包的检测结果（状态、版本、架构），不执行注册或解压。
    /// 触发方式：按住 Ctrl+Shift 拖入 zip。
    /// </summary>
    private static void DebugInspectDroppedPackage(
        string packagePath,
        PendingUpdateApplier.PackageInspectionResult inspection,
        string currentVersion,
        string normalizedArchitecture)
    {
        bool isResource = ResourceUpdater.IsResourcePackage(packagePath, out var resourceDateTime);

        string detail = string.Format(
            """
            [DebugInspectDroppedPackage] 仅检测，不注册

            文件: {0}
            当前版本: {1}
            当前架构: {2}

            检测状态: {3}
            MatchedPattern: {4}
            IsSupported: {5}
            SourceVersion: {6}
            TargetVersion: {7}

            是资源包: {8}
            资源包版本: {9}
            """,
            Path.GetFileName(packagePath),
            currentVersion,
            normalizedArchitecture,
            inspection.Status,
            inspection.MatchedPattern,
            inspection.IsSupported,
            inspection.SourceVersion ?? "(null)",
            inspection.TargetVersion ?? "(null)",
            isResource,
            isResource ? resourceDateTime.ToLocalTimeString() : "—");

        _logger.Information("DebugInspectDroppedPackage:\n{Detail}", detail);
        MessageBoxHelper.Show(
            detail,
            "DebugInspectDroppedPackage",
            MessageBoxButton.OK,
            MessageBoxImage.Information);
    }
#endif

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
