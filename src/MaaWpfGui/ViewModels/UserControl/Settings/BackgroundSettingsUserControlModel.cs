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
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using HandyControl.Controls;
using HandyControl.Tools;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Stylet;
using MonetModeType = MaaWpfGui.Configuration.Global.GUI.MonetModeType;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class BackgroundSettingsUserControlModel : PropertyChangedBase
{
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

    private static string _backgroundImagePath = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundImagePath, "background/background.png");

    public string BackgroundImagePath
    {
        get => _backgroundImagePath;
        set {
            SetAndNotify(ref _backgroundImagePath, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.BackgroundImagePath, value);
            BackgroundImage = RefreshBackgroundImage(value);

            // 背景图变更后，若莫奈自动取色开启，重新提取主色
            UpdateMonet();

            AchievementTrackerHelper.Instance.Unlock(AchievementIds.CustomizationMaster);
        }
    }

    public void SelectImagePath()
    {
        var dialog = new OpenFileDialog {
            Filter = "Image|*.jpg;*.png",
        };

        if (dialog.ShowDialog() == true)
        {
            BackgroundImagePath = dialog.FileName;
        }
    }

    private static BitmapImage? _backgroundImage = RefreshBackgroundImage(_backgroundImagePath);

    public BitmapImage? BackgroundImage
    {
        get => _backgroundImage;
        set {
            SetAndNotify(ref _backgroundImage, value);
        }
    }

    private static Stretch _backgroundImageStretchMode = (Stretch)Enum.Parse(typeof(Stretch), ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundImageStretchMode, Stretch.Fill.ToString()));

    public Stretch BackgroundImageStretchMode
    {
        get => _backgroundImageStretchMode;
        set {
            SetAndNotify(ref _backgroundImageStretchMode, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.BackgroundImageStretchMode, value.ToString());
        }
    }

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

    private int _backgroundOpacity = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundOpacity, 50);

    public int BackgroundOpacity
    {
        get => _backgroundOpacity;
        set {
            SetAndNotify(ref _backgroundOpacity, value);

            // 透明度变化后，若莫奈取色开启，重新生成调色板（文字色需根据新的有效背景明度选取）
            // 滑块拖动会高频触发，使用防抖避免阻塞 UI
            if (BackgroundMonetEnabled)
            {
                ScheduleMonetUpdate();
            }
        }
    }

    private static int _backgroundBlurEffectRadius = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundBlurEffectRadius, 5);

    public int BackgroundBlurEffectRadius
    {
        get => _backgroundBlurEffectRadius;
        set {
            SetAndNotify(ref _backgroundBlurEffectRadius, value);
        }
    }

    public void PreviewSlider_MouseUp(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton != MouseButton.Left)
        {
            return;
        }

        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.BackgroundOpacity, BackgroundOpacity.ToString(CultureInfo.InvariantCulture));
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.BackgroundBlurEffectRadius, BackgroundBlurEffectRadius.ToString(CultureInfo.InvariantCulture));
    }

    #region 莫奈取色（Monet Color）

    /// <summary>
    /// 莫奈取色是否启用。
    /// </summary>
    public bool BackgroundMonetEnabled
    {
        get => ConfigFactory.Root.GUI.BackgroundMonetEnabled;
        set {
            ConfigFactory.Root.GUI.BackgroundMonetEnabled = value;
            NotifyOfPropertyChange();
            UpdateMonet();
        }
    }

    /// <summary>
    /// 莫奈取色模式（Auto / Custom）。
    /// </summary>
    public MonetModeType BackgroundMonetMode
    {
        get => ConfigFactory.Root.GUI.BackgroundMonetMode;
        set {
            ConfigFactory.Root.GUI.BackgroundMonetMode = value;
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
        get => ConfigFactory.Root.GUI.BackgroundMonetCustomColor;
        set {
            ConfigFactory.Root.GUI.BackgroundMonetCustomColor = value;
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
    /// 打开 HandyControl ColorPicker 弹窗让用户选择自定义颜色。
    /// </summary>
    public void SelectMonetColor()
    {
        var picker = SingleOpenHelper.CreateControl<ColorPicker>();
        var window = new PopupWindow {
            PopupElement = picker,
            Title = LocalizationHelper.GetString("BackgroundMonetSelectColor"),
        };

        window.Loaded += (_, _) => Helper.WindowManager.MoveWindowToRootCenter(window);

        // 初始化 ColorPicker 为当前颜色
        var current = ThemeHelper.String2Color(BackgroundMonetCustomColor);
        picker.SelectedBrush = new SolidColorBrush(current);

        picker.Confirmed += (_, e) => {
            var color = e.Info;
            BackgroundMonetCustomColor = ThemeHelper.Color2HexString(color);
            UpdateMonet();
            window.Close();
        };

        picker.Canceled += (_, _) => window.Close();

        window.Show();
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
    public void UpdateMonet()
    {
        _monetUpdateCts?.Cancel();
        _ = UpdateMonetAsync(CancellationToken.None);
    }

    /// <summary>
    /// 异步执行莫奈取色，支持取消（用于防抖）。
    /// </summary>
    /// <param name="cancellationToken">取消令牌。</param>
    private async Task UpdateMonetAsync(CancellationToken cancellationToken)
    {
        if (!BackgroundMonetEnabled)
        {
            ThemeHelper.RevertMonetPalette();
            NotifyOfPropertyChange(nameof(CurrentMonetColor));
            return;
        }

        if (BackgroundMonetMode == MonetModeType.Custom)
        {
            // 自定义模式：直接用用户选定的颜色，计算放后台线程
            var color = ThemeHelper.String2Color(BackgroundMonetCustomColor);
            await ThemeHelper.ApplyMonetPaletteAsync(color, BackgroundOpacity);
            cancellationToken.ThrowIfCancellationRequested();
            NotifyOfPropertyChange(nameof(CurrentMonetColor));
        }
        else
        {
            // 自动模式：从背景图提取主色（本身就是异步的）
            await UpdateMonetAutoAsync(cancellationToken);
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

        var rawColor = await ColorExtractorHelper.ExtractDominantColorAsync(image);
        cancellationToken.ThrowIfCancellationRequested();

        // 直接使用提取到的原始主色生成调色板
        _lastExtractedColor = rawColor;

        await ThemeHelper.ApplyMonetPaletteAsync(rawColor, BackgroundOpacity);
        cancellationToken.ThrowIfCancellationRequested();

        NotifyOfPropertyChange(nameof(CurrentMonetColor));
    }

    #endregion 莫奈取色（Monet Color）
}
