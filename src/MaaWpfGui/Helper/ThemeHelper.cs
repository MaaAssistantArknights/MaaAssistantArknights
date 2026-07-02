// <copyright file="ThemeHelper.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using HandyControl.Themes;
using HandyControl.Tools;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;

namespace MaaWpfGui.Helper;

public static class ThemeHelper
{
    #region Swith Theme

    private static void SystemEvents_UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e)
    {
        ThemeManager.Current.ApplicationTheme = ThemeManager.GetSystemTheme(isSystemTheme: false);

        if (!WineRuntimeInformation.IsRunningUnderWine)
        {
            ThemeManager.Current.AccentColor = ThemeManager.Current.GetAccentColorFromSystem();
        }

        Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;

        // 系统主题切换后，莫奈调色板需要按新的明暗模式重新生成
        ReapplyMonet();
    }

    public static void SwitchToLightMode()
    {
        SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
        ThemeManager.Current.UsingWindowsAppTheme = false;
        ThemeManager.Current.ApplicationTheme = ApplicationTheme.Light;
        Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        ReapplyMonet();
    }

    public static void SwitchToDarkMode()
    {
        SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
        ThemeManager.Current.UsingWindowsAppTheme = false;
        ThemeManager.Current.ApplicationTheme = ApplicationTheme.Dark;
        Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        ReapplyMonet();
    }

    public static void SwitchToSyncWithOsMode()
    {
        if (WineRuntimeInformation.IsRunningUnderWine)
        {
            ThemeManager.Current.ApplicationTheme = ThemeManager.GetSystemTheme(isSystemTheme: false);
        }
        else
        {
            ThemeManager.Current.UsingWindowsAppTheme = true;
            SystemEvents.UserPreferenceChanged += SystemEvents_UserPreferenceChanged;
        }

        Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        ReapplyMonet();
    }

    #endregion Swith Theme

    #region Monet（莫奈取色）

    /// <summary>
    /// 缓存的莫奈调色板的生成参数，用于明暗切换后重新生成。
    /// 为 null 表示莫奈取色未启用。
    /// </summary>
    private static Color? _monetBaseColor;

    /// <summary>
    /// 缓存的背景图不透明度，用于 ReapplyMonet 时重新生成文字色。
    /// </summary>
    private static int _monetBackgroundOpacity = 50;

    /// <summary>
    /// 保存被莫奈覆盖前的原始 brush，用于关闭莫奈时恢复。
    /// </summary>
    private static readonly Dictionary<string, SolidColorBrush> _monetOriginalBrushes = [];

    /// <summary>
    /// 莫奈取色是否已启用（即是否已调用过 ApplyMonetPalette 且未 Revert）。
    /// </summary>
    public static bool IsMonetActive => _monetBaseColor != null;

    /// <summary>
    /// 获取当前是否处于深色模式。
    /// </summary>
    /// <returns>深色模式返回 true。</returns>
    public static bool IsDarkMode()
    {
        return ThemeManager.Current.ApplicationTheme == ApplicationTheme.Dark;
    }

    /// <summary>
    /// 应用莫奈调色板，覆盖主题资源中的主色系和背景色系。
    /// 首次调用时会保存原始值以供回退。
    /// </summary>
    /// <param name="baseColor">提取或用户选定的主色。</param>
    /// <param name="backgroundOpacity">背景图不透明度 (0~100)，影响文字色的明度选取。</param>
    public static void ApplyMonetPalette(Color baseColor, int backgroundOpacity = 50)
    {
        var isDark = IsDarkMode();
        _monetBaseColor = baseColor;
        _monetBackgroundOpacity = backgroundOpacity;

        // 调色板计算可在任意线程完成，无需占用 UI 线程
        var palette = MonetPaletteHelper.Generate(baseColor, isDark, backgroundOpacity);

        // 只有实际写入 WPF 资源才需要在 UI 线程执行
        Application.Current?.Dispatcher.Invoke(() =>
        {
            ApplyPaletteToResources(palette, baseColor);
        });
    }

    /// <summary>
    /// 异步应用莫奈调色板：计算在后台线程，资源写入在 UI 线程。
    /// 适用于滑块拖动等高频调用场景，避免阻塞 UI。
    /// </summary>
    /// <param name="baseColor">提取或用户选定的主色。</param>
    /// <param name="backgroundOpacity">背景图不透明度 (0~100)，影响文字色的明度选取。</param>
    /// <param name="cancellationToken">取消令牌，用于防抖时取消过期的调色板计算。</param>
    /// <returns>表示异步操作的任务。</returns>
    public static async Task ApplyMonetPaletteAsync(Color baseColor, int backgroundOpacity = 50, CancellationToken cancellationToken = default)
    {
        var isDark = IsDarkMode();
        _monetBaseColor = baseColor;
        _monetBackgroundOpacity = backgroundOpacity;

        // 在后台线程计算调色板
        var palette = await Task.Run(() => MonetPaletteHelper.Generate(baseColor, isDark, backgroundOpacity), cancellationToken)
            .ConfigureAwait(true);

        // 在 UI 线程写入资源
        Application.Current?.Dispatcher.Invoke(() =>
        {
            ApplyPaletteToResources(palette, baseColor);
        });
    }

    /// <summary>
    /// 将已计算好的调色板写入 WPF 资源（必须在 UI 线程调用）。
    /// </summary>
    private static void ApplyPaletteToResources(Dictionary<string, Color> palette, Color baseColor)
    {
        // 首次应用时保存原始值
        SaveOriginalsIfNeeded(MonetPaletteHelper.PaletteKeys);

        // 先设置 AccentColor：HandyControl 的 ThemeManager 会根据它重新生成
        // PrimaryBrush / DarkPrimaryBrush / TitleBrush 等内部资源，
        // 因此必须在 AccentColor 之后再覆盖为莫奈调色板的值，否则会被覆盖回去
        ThemeManager.Current.AccentColor = new SolidColorBrush(baseColor);

        foreach (var (key, color) in palette)
        {
            Application.Current.Resources[key] = new SolidColorBrush(color);
        }
    }

    /// <summary>
    /// 回退莫奈调色板，恢复所有被覆盖的 brush 到原始值。
    /// </summary>
    public static void RevertMonetPalette()
    {
        if (_monetBaseColor == null)
        {
            return;
        }

        _monetBaseColor = null;

        // 只有资源写入需要在 UI 线程
        Application.Current?.Dispatcher.Invoke(() =>
        {
            // 先恢复 AccentColor 到系统值（同样会重写 HandyControl 内部 brush）
            if (!WineRuntimeInformation.IsRunningUnderWine)
            {
                ThemeManager.Current.AccentColor = ThemeManager.Current.GetAccentColorFromSystem();
            }

            // 在 AccentColor 之后再恢复原始 brush，避免被 HandyControl 覆盖
            foreach (var (key, brush) in _monetOriginalBrushes)
            {
                Application.Current.Resources[key] = brush;
            }

            _monetOriginalBrushes.Clear();

            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        });
    }

    /// <summary>
    /// 当莫奈取色启用时，使用缓存的 baseColor 重新生成调色板。
    /// 用于明暗模式切换后同步更新。
    /// </summary>
    public static void ReapplyMonet()
    {
        if (_monetBaseColor is Color color)
        {
            // 明暗模式切换后，HandyControl 会通过 ThemeResources 切换底层主题字典，
            // 但莫奈之前直接写入 Application.Current.Resources 的值会遮蔽新主题的值。
            // 需要先移除这些直接覆盖项，让底层新主题值透出，再重新保存和生成。
            Application.Current?.Dispatcher.Invoke(() =>
            {
                foreach (var key in MonetPaletteHelper.PaletteKeys)
                {
                    Application.Current.Resources.Remove(key);
                }
            });

            _monetOriginalBrushes.Clear();
            ApplyMonetPalette(color, _monetBackgroundOpacity);
        }
    }

    /// <summary>
    /// 首次覆盖前保存原始 brush 值。
    /// </summary>
    private static void SaveOriginalsIfNeeded(IEnumerable<string> keys)
    {
        if (_monetOriginalBrushes.Count > 0)
        {
            return;
        }

        foreach (var key in keys)
        {
            if (Application.Current?.Resources[key] is SolidColorBrush brush)
            {
                // 冻结副本，避免引用被修改
                _monetOriginalBrushes[key] = brush.Clone();
            }
        }
    }

    #endregion Monet（莫奈取色）

    #region Check UiLogColor

    // In official version, using ResourceHelper.GetSkin
    private static readonly Color _lightBackground = ((SolidColorBrush)ThemeResources.Current.GetThemeDictionary("Light")["RegionBrush"]).Color;

    private static readonly Color _darkBackground = ((SolidColorBrush)ThemeResources.Current.GetThemeDictionary("Dark")["RegionBrush"]).Color;

    public static bool SimilarToBackground(Color color)
    {
        return ColorDistance(color, _lightBackground) == 0 || ColorDistance(color, _darkBackground) == 0;
    }

    /// <summary>
    /// Gets the distance between colors c1 and c2.
    /// </summary>
    /// <param name="c1">The color c1.</param>
    /// <param name="c2">The color c2.</param>
    /// <returns>The distance from 0 to 35.</returns>
    public static int ColorDistance(Color c1, Color c2)
    {
        // https://www.compuphase.com/cmetric.htm
        long rmean = (c1.R + c2.R) / 2;
        long r = c1.R - c2.R;
        long g = c1.G - c2.G;
        long b = c1.B - c2.B;
        return (int)(((((512 + rmean) * r * r) >> 8) + (4 * g * g) + (((767 - rmean) * b * b) >> 8)) >> 14);
    }

    #endregion Check UiLogColor

    #region Convert

    public const string DefaultKey = UiLogColor.Message;

    public static SolidColorBrush DefaultBrush => ResourceHelper.GetResource<SolidColorBrush>(DefaultKey);

    public static Color DefaultColor => DefaultBrush.Color;

    public static string DefaultHexString => $"#{DefaultColor.A:X2}{DefaultColor.R:X2}{DefaultColor.G:X2}{DefaultColor.B:X2}";

    public static string Color2HexString(Color color, bool keepAlpha = false)
    {
        return keepAlpha
            ? $"#{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}"
            : $"#FF{color.R:X2}{color.G:X2}{color.B:X2}";
    }

    [UsedImplicitly]
    public static string Brush2HexString(SolidColorBrush brush, bool keepAlpha = false)
    {
        return brush != null
            ? Color2HexString(brush.Color, keepAlpha)
            : DefaultHexString;
    }

    public static Color String2Color(string str)
    {
        if (string.IsNullOrWhiteSpace(str))
        {
            return DefaultColor;
        }

        try
        {
            object obj = ColorConverter.ConvertFromString(str);
            if (obj is Color color)
            {
                return color;
            }
        }
        catch
        {
            return DefaultColor;
        }

        return DefaultColor;
    }

    public static SolidColorBrush String2Brush(string str)
    {
        return new SolidColorBrush(String2Color(str));
    }

    #endregion Convert
}
