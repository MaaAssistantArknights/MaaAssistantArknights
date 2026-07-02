// <copyright file="MonetPaletteHelper.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Windows.Media;

namespace MaaWpfGui.Helper;

/// <summary>
/// 根据提取到的主色，通过 HSL 线性计算生成 Material You 风格的主题色板。
/// <para>
/// 设计理念：从背景图提取一个主色后，以 5 组「色调（色相+饱和度）」为基准，
/// 在亮度 0~1 之间实时取色。文字与背景之间亮度差 ≥ TextContrastDistance (0.80)，
/// 主色与背景之间亮度差 ≥ PrimaryContrastDistance (0.40)。
/// 深色模式与浅色模式只是亮度方向翻转：浅色模式背景亮(L高)、文字暗(L低)；
/// 深色模式背景暗(L低)、文字亮(L高)。
/// </para>
/// <para>因为是实时计算的，所以不需要预生成 5×12 的离散调色盘，直接计算即可。</para>
/// </summary>
public static class MonetPaletteHelper
{
    /// <summary>
    /// 需要生成的主题资源 key 列表。
    /// </summary>
    public static readonly string[] PaletteKeys =
    [
        "PrimaryBrush",
        "DarkPrimaryBrush",
        "LightPrimaryBrush",
        "TitleBrush",
        "RegionBrush",
        "SecondaryRegionBrush",
        "RegionBrushOpacity10",
        "RegionBrushOpacity25",
        "RegionBrushOpacity50",
        "RegionBrushOpacity75",
        "MouseOverRegionBrush",
        "MouseOverRegionBrushOpacity10",
        "MouseOverRegionBrushOpacity25",
        "MouseOverRegionBrushOpacity50",
        "MouseOverRegionBrushOpacity75",

        // 边框色：固定明度角色，跟随背景体系
        "BorderBrush",

        // 亮度依赖的文字色：随明暗模式翻转，需跟随 region 明度适配
        "PrimaryTextBrush",
        "ThirdlyTextBrush",
        "TraceLogBrush",
        "MessageLogBrush",
    ];

    // 原 Dark.xaml / Light.xaml 的 Alpha 值，保持不变
    private const byte Alpha10 = 0x19; // 10%
    private const byte Alpha25 = 0x40; // 25%
    private const byte Alpha50 = 0x7F; // 50%
    private const byte Alpha75 = 0xBF; // 75%

    // ── 5 组色调（饱和度）基准 ──

    /// <summary>
    /// 主色系固定饱和度，统一降饱和到 Material You 风格。
    /// </summary>
    private const double PrimarySaturation = 0.36;

    /// <summary>
    /// 背景体系（Region / MouseOver / Border）饱和度。
    /// </summary>
    private const double BackgroundSaturation = 0.16;

    /// <summary>
    /// 主要文字色饱和度占原始色饱和度的比例。
    /// 文字色保持低饱和但不至于完全丢失主色调。
    /// </summary>
    private const double TextSatRatio = 0.20;

    /// <summary>
    /// 文字色饱和度上限，避免影响可读性。
    /// </summary>
    private const double TextSatMax = 0.18;

    /// <summary>
    /// 文字色饱和度下限，保证至少能感知到色调。
    /// </summary>
    private const double TextSatMin = 0.04;

    // ── 对比距离 ──

    /// <summary>
    /// 文字与有效背景之间要求的最小亮度差。
    /// 原版深色主题：Text L=0.90 vs Region L=0.11，差 ≈ 0.80。
    /// </summary>
    private const double TextContrastDistance = 0.80;

    /// <summary>
    /// 主色与有效背景之间要求的最小亮度差。
    /// 主色是强调色，不需要像文字那么高的对比度。
    /// </summary>
    private const double PrimaryContrastDistance = 0.40;

    // ── 亮度锚点 ──

    /// <summary>
    /// 浅色模式背景基础亮度。
    /// </summary>
    private const double LightBgL = 0.95;

    /// <summary>
    /// 深色模式背景基础亮度。
    /// </summary>
    private const double DarkBgL = 0.10;

    /// <summary>
    /// 背景图对文字层的有效穿透率。
    /// <para>
    /// UI 层次：RegionBrush(固态) → Image(α) → RegionBrushOpacity25 → RegionBrushOpacity25 → 文字。
    /// 每层 RegionBrushOpacity25 的 Alpha = 0x40/255 ≈ 0.251，透光率 = 0.749。
    /// 两层叠加后背景图穿透率 = 0.749² ≈ 0.561。
    /// </para>
    /// <para>
    /// 有效背景亮度 = regionL×(1 - 穿透率×α) + baseL×(穿透率×α)。
    /// α=0 时为纯 Region 色，α=1 时为 Region×0.439 + BaseColor×0.561。
    /// </para>
    /// </summary>
    private const double BgImageVisibility = 0.561;

    /// <summary>
    /// 根据基础主色生成调色板。
    /// </summary>
    /// <param name="baseColor">提取或用户选定的主色。</param>
    /// <param name="isDark">当前是否为深色模式。</param>
    /// <param name="backgroundOpacity">背景图不透明度 (0~100)，影响背景有效亮度。</param>
    /// <returns>资源 key → 颜色的映射。</returns>
    public static Dictionary<string, Color> Generate(Color baseColor, bool isDark, int backgroundOpacity = 50)
    {
        var (hue, sat, baseL) = RgbToHsl(baseColor);

        // 5 组色调：主色饱和度固定为 0.36（统一降饱和到 Material You 风格）
        var primarySat = PrimarySaturation;

        // ── 计算有效背景亮度 ──
        // 背景图透过两层 25% 蒙版影响文字层，穿透率 = BgImageVisibility (0.561)。
        // 有效背景 = Region×(1 - 穿透率×α) + BaseColor×(穿透率×α)
        var bgAlpha = Math.Clamp(backgroundOpacity / 100.0, 0.0, 1.0);
        var regionBaseL = isDark ? DarkBgL : LightBgL;
        var bgInfluence = BgImageVisibility * bgAlpha;
        var effectiveBgL = (regionBaseL * (1 - bgInfluence)) + (baseL * bgInfluence);

        // ── 从背景亮度推导各角色亮度 ──
        // 文字与背景保持 TextContrastDistance (0.80) 距离
        var textL = isDark
            ? Math.Min(effectiveBgL + TextContrastDistance, 0.95)
            : Math.Max(effectiveBgL - TextContrastDistance, 0.05);
        var secondaryTextL = isDark
            ? Math.Min(effectiveBgL + (TextContrastDistance * 0.7), 0.85)
            : Math.Max(effectiveBgL - (TextContrastDistance * 0.7), 0.15);

        // 主色亮度：与有效背景保持 PrimaryContrastDistance 距离
        // 深色模式取亮侧，浅色模式取暗侧，与文字使用同一套对比逻辑
        var primaryL = isDark
            ? Math.Min(effectiveBgL + PrimaryContrastDistance, 0.90)
            : Math.Max(effectiveBgL - PrimaryContrastDistance, 0.10);

        // 背景体系亮度：Region 固定，SecondaryRegion/MouseOver 稍亮（深色）/稍暗（浅色）
        var regionL = regionBaseL;
        var secondaryRegionL = isDark ? 0.15 : 0.90;
        var mouseOverL = isDark ? 0.20 : 0.80;
        var borderL = isDark ? 0.25 : 0.80;

        // 文字色饱和度：按原始色饱和度等比缩放，保留微弱色调
        var textSat = Math.Clamp(sat * TextSatRatio, TextSatMin, TextSatMax);

        // ── 生成颜色 ──
        var region = HslToRgb(hue, BackgroundSaturation, regionL);
        var secondaryRegion = HslToRgb(hue, BackgroundSaturation, secondaryRegionL);
        var mouseOver = HslToRgb(hue, BackgroundSaturation, mouseOverL);
        var border = HslToRgb(hue, BackgroundSaturation, borderL);

        var primary = HslToRgb(hue, primarySat, primaryL);
        var darkPrimary = HslToRgb(hue, primarySat, Math.Max(primaryL - 0.15, 0.05));
        var lightPrimary = HslToRgb(hue, primarySat, isDark ? 0.10 : 0.85);

        var primaryText = HslToRgb(hue, textSat, textL);
        var traceLog = HslToRgb(hue, Math.Clamp(sat * TextSatRatio * 1.25, TextSatMin * 1.5, TextSatMax * 1.5), secondaryTextL);

        var palette = new Dictionary<string, Color>
        {
            ["PrimaryBrush"] = primary,
            ["DarkPrimaryBrush"] = darkPrimary,
            ["LightPrimaryBrush"] = lightPrimary,
            ["TitleBrush"] = primary,
            ["RegionBrush"] = region,
            ["MouseOverRegionBrush"] = mouseOver,
            ["BorderBrush"] = border,
            ["PrimaryTextBrush"] = primaryText,
            ["SecondaryRegionBrush"] = secondaryRegion,
            ["ThirdlyTextBrush"] = Color.FromArgb(0x7F, primaryText.R, primaryText.G, primaryText.B),
            ["TraceLogBrush"] = traceLog,
            ["MessageLogBrush"] = primaryText,
        };

        // Opacity 系列
        palette["RegionBrushOpacity10"] = Color.FromArgb(Alpha10, region.R, region.G, region.B);
        palette["RegionBrushOpacity25"] = Color.FromArgb(Alpha25, region.R, region.G, region.B);
        palette["RegionBrushOpacity50"] = Color.FromArgb(Alpha50, region.R, region.G, region.B);
        palette["RegionBrushOpacity75"] = Color.FromArgb(Alpha75, region.R, region.G, region.B);

        palette["MouseOverRegionBrushOpacity10"] = Color.FromArgb(Alpha10, mouseOver.R, mouseOver.G, mouseOver.B);
        palette["MouseOverRegionBrushOpacity25"] = Color.FromArgb(Alpha25, mouseOver.R, mouseOver.G, mouseOver.B);
        palette["MouseOverRegionBrushOpacity50"] = Color.FromArgb(Alpha50, mouseOver.R, mouseOver.G, mouseOver.B);
        palette["MouseOverRegionBrushOpacity75"] = Color.FromArgb(Alpha75, mouseOver.R, mouseOver.G, mouseOver.B);

        return palette;
    }

    /// <summary>
    /// RGB → HSL 转换。
    /// </summary>
    private static (double H, double S, double L) RgbToHsl(Color color)
    {
        var r = color.R / 255.0;
        var g = color.G / 255.0;
        var b = color.B / 255.0;

        var max = Math.Max(r, Math.Max(g, b));
        var min = Math.Min(r, Math.Min(g, b));
        var delta = max - min;

        var l = (max + min) / 2.0;

        if (delta == 0)
        {
            return (0, 0, l);
        }

        var s = delta / (1 - Math.Abs((2 * l) - 1));

        double h;
        if (max == r)
        {
            h = ((g - b) / delta) % 6;
        }
        else if (max == g)
        {
            h = ((b - r) / delta) + 2;
        }
        else
        {
            h = ((r - g) / delta) + 4;
        }

        h *= 60;
        if (h < 0)
        {
            h += 360;
        }

        return (h, s, l);
    }

    /// <summary>
    /// HSL → RGB 转换。
    /// </summary>
    private static Color HslToRgb(double h, double s, double l)
    {
        if (s == 0)
        {
            var gray = (byte)(l * 255);
            return Color.FromRgb(gray, gray, gray);
        }

        var c = (1 - Math.Abs((2 * l) - 1)) * s;
        var hp = h / 60.0;
        var x = c * (1 - Math.Abs((hp % 2) - 1));

        double r1, g1, b1;
        switch ((int)hp)
        {
            case 0: r1 = c; g1 = x; b1 = 0; break;
            case 1: r1 = x; g1 = c; b1 = 0; break;
            case 2: r1 = 0; g1 = c; b1 = x; break;
            case 3: r1 = 0; g1 = x; b1 = c; break;
            case 4: r1 = x; g1 = 0; b1 = c; break;
            default: r1 = c; g1 = 0; b1 = x; break;
        }

        var m = l - (c / 2.0);
        return Color.FromRgb(
            (byte)((r1 + m) * 255),
            (byte)((g1 + m) * 255),
            (byte)((b1 + m) * 255));
    }
}
