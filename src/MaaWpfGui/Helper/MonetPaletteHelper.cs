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
/// <para>所有颜色均由 (色相, 饱和度, 明度) 直接计算，无离散档位量化，保证连续平滑。</para>
/// <para>文字与背景之间保持足够的明度距离以保证可读性。</para>
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
        "RegionBrushOpacity10",
        "RegionBrushOpacity25",
        "RegionBrushOpacity50",
        "RegionBrushOpacity75",
        "MouseOverRegionBrush",
        "MouseOverRegionBrushOpacity10",
        "MouseOverRegionBrushOpacity25",
        "MouseOverRegionBrushOpacity50",
        "MouseOverRegionBrushOpacity75",

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

    // ── 各角色的饱和度常量 ──

    /// <summary>
    /// 主色（Primary）饱和度下限。原始主色饱和度不足此值时使用此值。
    /// </summary>
    private const double PrimaryMinSaturation = 0.55;

    /// <summary>
    /// 背景体系（Neutral Tint）饱和度。
    /// </summary>
    private const double BackgroundSaturation = 0.16;

    /// <summary>
    /// 文字色饱和度，极低饱和度接近中性色。
    /// </summary>
    private const double TextSaturation = 0.06;

    // ── 对比度常量 ──

    /// <summary>
    /// 文字与有效背景之间要求的最小明度差。
    /// 原版深色主题 PrimaryText(L=0.90) 与 Region(L=0.11) 差约 0.79。
    /// </summary>
    private const double TextContrastDistance = 0.70;

    /// <summary>
    /// 主色（PrimaryBrush）与有效背景之间要求的最小明度差。
    /// 原版 Primary(L=0.57) 与 Region(L=0.11) 差约 0.46。
    /// </summary>
    private const double PrimaryContrastDistance = 0.45;

    // ── 固定明度常量（不随透明度变化的角色） ──
    private const double RegionDarkL = 0.10;
    private const double RegionLightL = 0.90;
    private const double MouseOverDarkL = 0.20;
    private const double MouseOverLightL = 0.70;

    /// <summary>
    /// RegionBrushOpacity25 中底层（混合底）的占比（75%）。
    /// Opacity25 覆盖层为 25% RegionBrush + 75% 混合底，
    /// 此常量即混合底在 effectiveRegionL 公式中的权重。
    /// </summary>
    private const double Alpha25Factor = 0.75;

    /// <summary>
    /// 背景图对有效背景明度的影响因子。
    /// 背景图虽然透过半透明层影响视觉，但文字的可读性主要取决于 RegionBrush 系列底色，
    /// 因此只取背景图影响的 50%，避免文字色被背景图过度拉偏。
    /// </summary>
    private const double BackgroundInfluence = 0.5;

    /// <summary>
    /// 根据基础主色生成调色板。
    /// </summary>
    /// <param name="baseColor">提取或用户选定的主色。</param>
    /// <param name="isDark">当前是否为深色模式。</param>
    /// <param name="backgroundOpacity">背景图不透明度 (0~100)，影响文字色的明度选取。</param>
    /// <returns>资源 key → 颜色的映射。</returns>
    public static Dictionary<string, Color> Generate(Color baseColor, bool isDark, int backgroundOpacity = 50)
    {
        var (hue, sat, _) = RgbToHsl(baseColor);

        // 主色饱和度：保留原色鲜艳度，但设下限避免过低饱和
        var primarySat = Math.Max(sat, PrimaryMinSaturation);

        // 有效背景明度：文字实际位于 RegionBrushOpacity25 覆盖层之上。
        // 背景图透过半透明层影响视觉，但对可读性的影响有限，只取部分影响。
        var regionL = isDark ? RegionDarkL : RegionLightL;
        var baseL = RgbToHsl(baseColor).L;
        var alpha = backgroundOpacity / 100.0;
        var blendedL = (regionL * (1 - (alpha * BackgroundInfluence))) + (baseL * (alpha * BackgroundInfluence));

        // Opacity25 覆盖层：25% region + 75% 混合底
        var effectiveRegionL = (blendedL * Alpha25Factor) + (regionL * (1 - Alpha25Factor));

        // ── 固定明度角色：背景体系 ──
        var region = HslToRgb(hue, BackgroundSaturation, isDark ? RegionDarkL : RegionLightL);
        var mouseOver = HslToRgb(hue, BackgroundSaturation, isDark ? MouseOverDarkL : MouseOverLightL);

        // ── 自适应明度角色：主色系（随有效背景明度连续变化） ──
        var primaryTargetL = isDark
            ? Math.Min(effectiveRegionL + PrimaryContrastDistance, 0.90)
            : Math.Max(effectiveRegionL - PrimaryContrastDistance, 0.10);
        var primary = HslToRgb(hue, primarySat, primaryTargetL);

        var darkPrimary = isDark
            ? HslToRgb(hue, primarySat, Math.Max(primaryTargetL - 0.10, 0.05))
            : primary;
        var lightPrimary = HslToRgb(hue, primarySat, isDark ? 0.10 : 0.70);

        // ── 自适应明度角色：文字系 ──
        var textTargetL = isDark
            ? Math.Min(effectiveRegionL + TextContrastDistance, 0.90)
            : Math.Max(effectiveRegionL - TextContrastDistance, 0.05);
        var traceTargetL = isDark
            ? Math.Min(effectiveRegionL + (TextContrastDistance * 0.7), 0.90)
            : Math.Max(effectiveRegionL - (TextContrastDistance * 0.7), 0.05);

        var primaryText = HslToRgb(hue, TextSaturation, textTargetL);
        var traceLog = HslToRgb(hue, TextSaturation, traceTargetL);

        var palette = new Dictionary<string, Color>
        {
            ["PrimaryBrush"] = primary,
            ["DarkPrimaryBrush"] = darkPrimary,
            ["LightPrimaryBrush"] = lightPrimary,
            ["TitleBrush"] = primary,
            ["RegionBrush"] = region,
            ["MouseOverRegionBrush"] = mouseOver,
            ["PrimaryTextBrush"] = primaryText,
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
