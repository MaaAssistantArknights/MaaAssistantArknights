// <copyright file="BackgroundSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using HandyControl.Controls;
using HandyControl.Tools;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Serilog;
using Stylet;
using MonetModeType = MaaWpfGui.Configuration.Global.Gui.MonetModeType;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class BackgroundSettingsUserControlModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<BackgroundSettingsUserControlModel>();

    /// <summary>
    /// 莫奈取色更新的防抖延迟（毫秒）。
    /// </summary>
    private const int MonetDebounceMs = 150;

    static BackgroundSettingsUserControlModel()
    {
        Instance = new();
    }

    private BackgroundSettingsUserControlModel()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    public void RefreshLocalization()
    {
        BackgroundImageStretchModeList.RefreshLocalization();
        BackgroundMonetModeList.RefreshLocalization();
    }

    public static BackgroundSettingsUserControlModel Instance { get; }

    public string BackgroundImagePath
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.Background.ImagePath = value;
            BackgroundImage = RefreshBackgroundImage(value);

            // 背景图变更后，若莫奈自动取色开启，重新提取主色
            UpdateMonet();

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.CustomizationMaster);
        }
    } = ConfigFactory.Root.Gui.Background.ImagePath;

    public void SelectImagePath()
    {
        var dialog = new OpenFileDialog {
            Filter = "Image|*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp",
        };

        if (dialog.ShowDialog() == true)
        {
            BackgroundImagePath = dialog.FileName;
        }
    }

    /// <summary>
    /// 背景图片下拉树数据源，读取 <c>Res/Backgrounds</c>。
    /// <c>Wallpapers</c> 只展开其子目录/图片，不作为折叠节点；<c>Internal</c> 跳过。
    /// </summary>
    public ObservableCollection<BackgroundImageItem> BackgroundImageItems { get; } = [];

    /// <summary>
    /// 背景图片下拉 Popup 是否打开。
    /// </summary>
    public bool IsBackgroundImagePopupOpen { get => field; set => SetAndNotify(ref field, value); }

    private static readonly string BackgroundsRoot = Path.Combine(PathsHelper.BaseDir, "Res", "Backgrounds");

    /// <summary>
    /// 这些目录本身不显示，只把内部子项提升到上一级。
    /// </summary>
    private static readonly HashSet<string> FlattenDirectoryNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "Wallpapers",
    };

    /// <summary>
    /// 内部资源目录，不出现在用户可选列表中。
    /// </summary>
    private static readonly HashSet<string> SkippedDirectoryNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "Internal",
    };

    private static readonly string[] SupportedImageExtensions = [".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp"];

    /// <summary>
    /// 加载 <c>Res/Backgrounds</c> 下的文件夹与图片，供下拉 TreeView 使用。
    /// </summary>
    [UsedImplicitly]
    public void LoadBackgroundImageItems()
    {
        try
        {
            BackgroundImageItems.Clear();

            if (!Directory.Exists(BackgroundsRoot))
            {
                Directory.CreateDirectory(BackgroundsRoot);
                return;
            }

            AddDirectoryContent(BackgroundsRoot, BackgroundsRoot, promoteChildrenOfFlattenDirs: true);
        }
        catch (Exception ex)
        {
            BackgroundImageItems.Clear();
            _logger.Error(ex, "Failed to load background images from {BackgroundsRoot}", BackgroundsRoot);
        }
    }

    /// <summary>
    /// 将目录内容加入下拉列表。
    /// </summary>
    /// <param name="dirPath">当前扫描目录</param>
    /// <param name="relativeRoot">相对路径根（Backgrounds）</param>
    /// <param name="promoteChildrenOfFlattenDirs">是否对 Wallpapers 等目录做一层展开</param>
    private void AddDirectoryContent(string dirPath, string relativeRoot, bool promoteChildrenOfFlattenDirs)
    {
        foreach (var file in Directory.GetFiles(dirPath)
                     .Where(IsSupportedImage)
                     .OrderBy(Path.GetFileName, StringComparer.CurrentCultureIgnoreCase))
        {
            BackgroundImageItems.Add(CreateFileItem(file, relativeRoot));
        }

        foreach (var dir in Directory.GetDirectories(dirPath)
                     .OrderBy(Path.GetFileName, StringComparer.CurrentCultureIgnoreCase))
        {
            var dirName = Path.GetFileName(dir);
            if (SkippedDirectoryNames.Contains(dirName))
            {
                continue;
            }

            // Wallpapers 等容器目录：不生成折叠节点，直接提升其子目录/图片
            if (promoteChildrenOfFlattenDirs && FlattenDirectoryNames.Contains(dirName))
            {
                AddDirectoryContent(dir, relativeRoot, promoteChildrenOfFlattenDirs: false);
                continue;
            }

            var folderItem = LoadBackgroundFolderItem(dir, relativeRoot);
            if (folderItem != null)
            {
                BackgroundImageItems.Add(folderItem);
            }
        }
    }

    private static BackgroundImageItem CreateFileItem(string filePath, string relativeRoot)
    {
        return new BackgroundImageItem {
            Name = Path.GetFileName(filePath),
            FullPath = filePath,
            RelativePath = Path.GetRelativePath(relativeRoot, filePath),
            IsFolder = false,
        };
    }

    /// <summary>
    /// 递归加载背景图片文件夹项。
    /// </summary>
    /// <param name="dirPath">文件夹路径</param>
    /// <param name="relativeRoot">相对路径根</param>
    /// <returns>文件夹项；若为空则返回 null</returns>
    private static BackgroundImageItem? LoadBackgroundFolderItem(string dirPath, string relativeRoot)
    {
        var folderItem = new BackgroundImageItem {
            Name = Path.GetFileName(dirPath),
            IsFolder = true,
        };

        foreach (var file in Directory.GetFiles(dirPath)
                     .Where(IsSupportedImage)
                     .OrderBy(Path.GetFileName, StringComparer.CurrentCultureIgnoreCase))
        {
            folderItem.Children.Add(CreateFileItem(file, relativeRoot));
        }

        foreach (var subDir in Directory.GetDirectories(dirPath)
                     .OrderBy(Path.GetFileName, StringComparer.CurrentCultureIgnoreCase))
        {
            var subFolderItem = LoadBackgroundFolderItem(subDir, relativeRoot);
            if (subFolderItem != null)
            {
                folderItem.Children.Add(subFolderItem);
            }
        }

        return folderItem.Children.Count == 0 ? null : folderItem;
    }

    private static bool IsSupportedImage(string path)
    {
        var ext = Path.GetExtension(path);
        return SupportedImageExtensions.Any(supported => ext.Equals(supported, StringComparison.OrdinalIgnoreCase));
    }

    /// <summary>
    /// 处理 TreeView 中选中的背景图片。
    /// </summary>
    /// <param name="imageItem">选中的图片项</param>
    [UsedImplicitly]
    public void OnBackgroundImageSelected(BackgroundImageItem? imageItem)
    {
        if (imageItem == null || imageItem.IsFolder || string.IsNullOrEmpty(imageItem.FullPath))
        {
            return;
        }

        BackgroundImagePath = imageItem.FullPath;
        IsBackgroundImagePopupOpen = false;
    }

    /// <summary>
    /// 切换背景图片下拉 Popup；打开前刷新目录列表。
    /// </summary>
    [UsedImplicitly]
    public void ToggleBackgroundImagePopup()
    {
        if (!IsBackgroundImagePopupOpen)
        {
            LoadBackgroundImageItems();
        }

        IsBackgroundImagePopupOpen = !IsBackgroundImagePopupOpen;
    }

    private static BitmapImage? _backgroundImage = RefreshBackgroundImage(ConfigFactory.Root.Gui.Background.ImagePath);

    public BitmapImage? BackgroundImage
    {
        get => _backgroundImage;
        set {
            SetAndNotify(ref _backgroundImage, value);
        }
    }

    public Stretch BackgroundImageStretchMode
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Gui.Background.StretchMode = value;
        }
    } = ConfigFactory.Root.Gui.Background.StretchMode;

    public LocalizedObservableList<Stretch> BackgroundImageStretchModeList { get; } = new(
        (Stretch.None, "BackgroundImageStretchModeNone"),
        (Stretch.Fill, "BackgroundImageStretchModeFill"),
        (Stretch.Uniform, "BackgroundImageStretchModeUniform"),
        (Stretch.UniformToFill, "BackgroundImageStretchModeUniformToFill"));

    private static BitmapImage? RefreshBackgroundImage(string imagePath)
    {
        if (string.IsNullOrEmpty(imagePath) || !File.Exists(imagePath))
        {
            return null;
        }

        try
        {
            var imageBytes = File.ReadAllBytes(imagePath);
            using var memoryStream = new MemoryStream(imageBytes);

            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.StreamSource = memoryStream;
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.EndInit();
            bitmap.Freeze();
            return bitmap;
        }
        catch
        {
            return null;
        }
    }

    public int BackgroundOpacity
    {
        get; set {
            SetAndNotify(ref field, value);

            // 透明度变化后，若莫奈取色开启，重新生成调色板（文字色需根据新的有效背景明度选取）
            // 滑块拖动会高频触发，使用防抖避免阻塞 UI
            if (BackgroundMonetEnabled)
            {
                ScheduleMonetUpdate();
            }
        }
    } = ConfigFactory.Root.Gui.Background.Opacity;

    public int BackgroundBlurEffectRadius { get; set => SetAndNotify(ref field, value); } = ConfigFactory.Root.Gui.Background.BlurEffectRadius;

    public void PreviewSlider_MouseUp(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton != MouseButton.Left)
        {
            return;
        }

        ConfigFactory.Root.Gui.Background.Opacity = BackgroundOpacity;
        ConfigFactory.Root.Gui.Background.BlurEffectRadius = BackgroundBlurEffectRadius;
    }

    #region 莫奈取色（Monet Color）

    /// <summary>
    /// 莫奈取色是否启用。
    /// </summary>
    public bool BackgroundMonetEnabled
    {
        get => ConfigFactory.Root.Gui.BackgroundMonetEnabled;
        set {
            ConfigFactory.Root.Gui.BackgroundMonetEnabled = value;
            NotifyOfPropertyChange();
            UpdateMonet();
        }
    }

    /// <summary>
    /// 莫奈取色模式（Auto / Custom）。
    /// </summary>
    public MonetModeType BackgroundMonetMode
    {
        get => ConfigFactory.Root.Gui.BackgroundMonetMode;
        set {
            ConfigFactory.Root.Gui.BackgroundMonetMode = value;
            NotifyOfPropertyChange();
            UpdateMonet();
        }
    }

    [PropertyDependsOn(nameof(BackgroundMonetMode))]
    public bool IsBackgroundMonetAuto => BackgroundMonetMode == MonetModeType.Auto;

    [PropertyDependsOn(nameof(BackgroundMonetMode))]
    public bool IsBackgroundMonetCustom => BackgroundMonetMode == MonetModeType.Custom;

    /// <summary>
    /// 莫奈取色模式下拉列表。
    /// </summary>
    public LocalizedObservableList<MonetModeType> BackgroundMonetModeList { get; } = new(
        (MonetModeType.Auto, "BackgroundMonetAuto"),
        (MonetModeType.Custom, "BackgroundMonetCustom"));

    /// <summary>
    /// 用户自定义颜色（HEX 字符串）。
    /// </summary>
    public string BackgroundMonetCustomColor
    {
        get => ConfigFactory.Root.Gui.BackgroundMonetCustomColor;
        set {
            ConfigFactory.Root.Gui.BackgroundMonetCustomColor = value;
            NotifyOfPropertyChange();
            NotifyOfPropertyChange(nameof(CurrentMonetColor));
        }
    }

    /// <summary>
    /// 当前生效的莫奈主色，用于色块预览。
    /// </summary>
    public Color CurrentMonetColor
    {
        get {
            if (!BackgroundMonetEnabled)
            {
                // 关闭时显示默认主色
                return ThemeHelper.String2Color("#326CF3");
            }

            return IsBackgroundMonetCustom
                ? ThemeHelper.String2Color(BackgroundMonetCustomColor)
                : _lastExtractedColor ?? ThemeHelper.String2Color("#326CF3");
        }
    }

    /// <summary>
    /// 上次自动提取到的颜色（缓存）。
    /// </summary>
    private Color? _lastExtractedColor;

    /// <summary>
    /// 从配置中读取上次持久化的自动取色主色。
    /// 启动时先用它同步应用调色板，避免界面先闪烁原版颜色。
    /// </summary>
    /// <returns>持久化缓存的颜色；无缓存或格式无效时返回 null。</returns>
    private static Color? LoadCachedMonetColor()
    {
        var hex = ConfigFactory.Root.Gui.BackgroundMonetCachedColor;
        if (string.IsNullOrEmpty(hex))
        {
            return null;
        }

        try
        {
            return (Color)ColorConverter.ConvertFromString(hex);
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// 将自动提取的主色持久化到配置，供下次启动时同步恢复。
    /// </summary>
    private static void SaveCachedMonetColor(Color color)
    {
        ConfigFactory.Root.Gui.BackgroundMonetCachedColor = ThemeHelper.Color2HexString(color);
    }

    /// <summary>
    /// 打开 HandyControl ColorPicker 弹窗让用户选择自定义颜色。
    /// </summary>
    public void SelectMonetColor()
    {
        var picker = SingleOpenHelper.CreateControl<ColorPicker>();

        // 初始化 ColorPicker 为当前颜色
        var current = ThemeHelper.String2Color(BackgroundMonetCustomColor);
        picker.SelectedBrush = new SolidColorBrush(current);

        var dialog = Dialog.Show(picker, nameof(Views.UI.RootView));

        picker.Confirmed += (_, e) => {
            var color = e.Info;
            BackgroundMonetCustomColor = ThemeHelper.Color2HexString(color);
            SaveCachedMonetColor(color);
            UpdateMonet();
            Cleanup();
        };

        picker.Canceled += (_, _) => Cleanup();
        return;

        void Cleanup()
        {
            dialog.Close();
            picker.Dispose();
        }
    }

    /// <summary>
    /// 防抖取消令牌源，避免滑块拖动时高频重新计算调色板。
    /// </summary>
    private CancellationTokenSource? _monetUpdateCts;

    /// <summary>
    /// 调度一次莫奈取色更新（防抖 150ms）。
    /// 滑块拖动等高频场景调用此方法，而非直接 <see cref="UpdateMonet"/>。
    /// </summary>
    private void ScheduleMonetUpdate()
    {
        _monetUpdateCts?.Cancel();
        _monetUpdateCts = new CancellationTokenSource();
        _ = UpdateMonetAsync(_monetUpdateCts.Token);
    }

    /// <summary>
    /// 根据当前配置执行莫奈取色逻辑。
    /// 自动 / 自定义模式都会将计算放到后台线程，仅资源写入在 UI 线程。
    /// </summary>
    /// <param name="skipDebounce">是否跳过防抖延迟。初始化时应传 true 以避免界面先闪烁原版颜色。</param>
    public void UpdateMonet(bool skipDebounce = false)
    {
        _monetUpdateCts?.Cancel();
        _ = UpdateMonetAsync(CancellationToken.None, skipDebounce);
    }

    /// <summary>
    /// 异步执行莫奈取色，支持取消（用于防抖）。
    /// 所有异常均在方法内部捕获并记录，避免 fire-and-forget 调用产生未观察异常。
    /// </summary>
    /// <param name="cancellationToken">取消令牌。</param>
    /// <param name="skipDebounce">是否跳过防抖延迟。</param>
    private async Task UpdateMonetAsync(CancellationToken cancellationToken, bool skipDebounce = false)
    {
        // 防抖延迟：等待 150ms，期间若被取消则直接返回
        // 初始化时跳过延迟，避免界面先显示原版颜色再切换为莫奈主题（闪烁）
        if (!skipDebounce)
        {
            try
            {
                await Task.Delay(MonetDebounceMs, cancellationToken).ConfigureAwait(true);
            }
            catch (OperationCanceledException)
            {
                return;
            }
        }

        try
        {
            if (!BackgroundMonetEnabled)
            {
                ThemeHelper.RevertMonetPalette();
                NotifyOfPropertyChange(nameof(CurrentMonetColor));
                return;
            }

            // 启动时先用持久化缓存的主色同步应用调色板，避免界面先闪烁原版颜色
            if (skipDebounce && LoadCachedMonetColor() is { } cached)
            {
                _lastExtractedColor = cached;
                ThemeHelper.ApplyMonetPalette(cached, BackgroundOpacity);
                NotifyOfPropertyChange(nameof(CurrentMonetColor));
            }

            if (BackgroundMonetMode == MonetModeType.Custom)
            {
                // 自定义模式：直接用用户选定的颜色，计算放后台线程
                var color = ThemeHelper.String2Color(BackgroundMonetCustomColor);
                await ThemeHelper.ApplyMonetPaletteAsync(color, BackgroundOpacity, cancellationToken);
                NotifyOfPropertyChange(nameof(CurrentMonetColor));
            }
            else
            {
                // 自动模式：从背景图提取主色（本身就是异步的）
                await UpdateMonetAutoAsync(cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // 正常的防抖取消，静默忽略
        }
        catch (Exception ex)
        {
            // 兜底：fire-and-forget 调用中的异常不能逃逸，记录后忽略
            _logger.Error(ex, "Failed to update Monet palette.");
        }
    }

    /// <summary>
    /// 异步执行自动取色：提取主色 → 应用调色板。
    /// 颜色提取和调色板计算在后台线程，资源写入在 UI 线程。
    /// </summary>
    /// <param name="cancellationToken">取消令牌。</param>
    private async Task UpdateMonetAutoAsync(CancellationToken cancellationToken)
    {
        var image = BackgroundImage;
        if (image == null)
        {
            // 没有背景图时，取色无意义，先回退
            ThemeHelper.RevertMonetPalette();
            NotifyOfPropertyChange(nameof(CurrentMonetColor));
            return;
        }

        var rawColor = await ColorExtractorHelper.ExtractDominantColorAsync(image).ConfigureAwait(true);
        cancellationToken.ThrowIfCancellationRequested();

        // 直接使用提取到的原始主色生成调色板
        _lastExtractedColor = rawColor;

        // 持久化提取结果，下次启动时可同步恢复，避免闪烁
        SaveCachedMonetColor(rawColor);

        await ThemeHelper.ApplyMonetPaletteAsync(rawColor, BackgroundOpacity, cancellationToken);

        NotifyOfPropertyChange(nameof(CurrentMonetColor));
    }

    #endregion 莫奈取色（Monet Color）
}
