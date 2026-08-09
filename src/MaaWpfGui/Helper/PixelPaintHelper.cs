// <copyright file="PixelPaintHelper.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MaaWpfGui.Helper;

/// <summary>
/// 巡展像素画：原图 → 24×24 × 官方 40 色。
/// 色序与游戏右侧色板一致；最近色使用 CompuPhase 加权距离。
/// </summary>
public static class PixelPaintHelper
{
    public const int GridSize = 24;

    public const int ColorCount = 40;

    /// <summary>纯白在 40 色板中的下标（0-based）。</summary>
    public const int WhiteColorIndex = 3;

    public const int PreviewPixelSize = 216;

    /// <summary>
    /// 官方 40 色 RGB（与游戏色板顺序一致）。
    /// </summary>
    public static readonly (byte R, byte G, byte B)[] Palette =
    [
        (34, 34, 34), (180, 180, 180), (234, 231, 223), (255, 255, 255),
        (211, 47, 54), (156, 10, 0), (214, 12, 74), (230, 150, 141),
        (254, 152, 117), (247, 208, 192), (252, 239, 234), (251, 246, 232),
        (220, 210, 200), (226, 206, 171), (213, 99, 34), (212, 140, 66),
        (242, 153, 0), (249, 201, 51), (252, 228, 153), (179, 180, 122),
        (194, 218, 114), (108, 110, 0), (177, 145, 85), (169, 143, 116),
        (170, 146, 40), (63, 43, 18), (116, 73, 31), (83, 70, 88),
        (42, 36, 70), (57, 69, 153), (90, 69, 157), (186, 163, 215),
        (182, 188, 223), (169, 172, 190), (99, 171, 185), (180, 210, 220),
        (145, 216, 230), (71, 174, 160), (182, 211, 200), (39, 56, 100),
    ];

    public enum FitMode
    {
        Crop,
        Contain,
        Stretch,
    }

    public enum DitherMode
    {
        None,
        FloydSteinberg,
        Atkinson,
    }

    public sealed class ConvertOptions
    {
        public FitMode Fit { get; init; } = FitMode.Crop;

        public DitherMode Dither { get; init; } = DitherMode.FloydSteinberg;

        /// <summary>对比度百分比，100 为原图。</summary>
        public double ContrastPercent { get; init; } = 100;

        /// <summary>亮度百分比，100 为原图。</summary>
        public double BrightnessPercent { get; init; } = 100;

        /// <summary>饱和度百分比，100 为原图。</summary>
        public double SaturationPercent { get; init; } = 100;

        /// <summary>
        /// 取景区：相对「去边后内容图」的归一化矩形（0~1）。
        /// null 表示用完整去边结果再按 Fit 适配。
        /// </summary>
        public Rect? ContentViewNormalized { get; init; }

        /// <summary>导入时是否裁掉透明/近白边。</summary>
        public bool TrimEmptyBorder { get; init; } = true;
    }

    public sealed class ConvertResult
    {
        /// <summary>24×24 色号，行优先，值域 0~39。</summary>
        public required int[,] Matrix { get; init; }

        public required WriteableBitmap Preview { get; init; }

        /// <summary>按色分组后的点列，供 Core 使用（已按 skipWhite 过滤）。</summary>
        public required List<ColorGroup> Groups { get; init; }

        public int PaintedCellCount { get; init; }
    }

    public sealed class ColorGroup
    {
        public int Color { get; init; }

        /// <summary>格子坐标，左上为 (0,0)，x 右 y 下。</summary>
        public List<int[]> Points { get; init; } = [];
    }

    public static ConvertResult Convert(BitmapSource source, ConvertOptions options, bool skipWhite = true)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(options);

        return Convert(Prepare(source, options.TrimEmptyBorder), options, skipWhite);
    }

    /// <summary>用预处理好的图像转换，适合需要多次调参实时预览的场景。</summary>
    public static ConvertResult Convert(PreparedImage prepared, ConvertOptions options, bool skipWhite = true)
    {
        ArgumentNullException.ThrowIfNull(prepared);
        ArgumentNullException.ThrowIfNull(options);

        var bgra = prepared.Data;
        var sample = SampleToGrid(bgra, options);
        ApplyCssLikeFilters(
            sample,
            options.ContrastPercent / 100.0,
            options.BrightnessPercent / 100.0,
            options.SaturationPercent / 100.0);

        var matrix = Quantize(sample, options.Dither);
        var groups = BuildGroups(matrix, skipWhite);
        var preview = RenderPreview(matrix);
        var painted = 0;
        foreach (var g in groups)
        {
            painted += g.Points.Count;
        }

        return new ConvertResult {
            Matrix = matrix,
            Preview = preview,
            Groups = groups,
            PaintedCellCount = painted,
        };
    }

    public static List<ColorGroup> BuildGroups(int[,] matrix, bool skipWhite)
    {
        var buckets = new List<int[]>[ColorCount];
        for (var c = 0; c < ColorCount; c++)
        {
            buckets[c] = [];
        }

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var idx = matrix[y, x];
                if (skipWhite && idx == WhiteColorIndex)
                {
                    continue;
                }

                buckets[idx].Add([x, y]);
            }
        }

        var groups = new List<ColorGroup>();
        for (var c = 0; c < ColorCount; c++)
        {
            if (buckets[c].Count == 0)
            {
                continue;
            }

            groups.Add(new ColorGroup { Color = c, Points = buckets[c] });
        }

        return groups;
    }

    public static WriteableBitmap RenderPreview(int[,] matrix)
    {
        var cell = PreviewPixelSize / GridSize;
        var bmp = new WriteableBitmap(PreviewPixelSize, PreviewPixelSize, 96, 96, PixelFormats.Bgra32, null);
        var pixels = new byte[PreviewPixelSize * PreviewPixelSize * 4];

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var (r, g, b) = Palette[matrix[y, x]];
                for (var dy = 0; dy < cell; dy++)
                {
                    for (var dx = 0; dx < cell; dx++)
                    {
                        var px = (x * cell) + dx;
                        var py = (y * cell) + dy;
                        var i = ((py * PreviewPixelSize) + px) * 4;
                        pixels[i] = b;
                        pixels[i + 1] = g;
                        pixels[i + 2] = r;
                        pixels[i + 3] = 255;
                    }
                }
            }
        }

        bmp.WritePixels(new Int32Rect(0, 0, PreviewPixelSize, PreviewPixelSize), pixels, PreviewPixelSize * 4, 0);
        bmp.Freeze();
        return bmp;
    }

    public static double ColorDistance((byte R, byte G, byte B) c1, (byte R, byte G, byte B) c2)
    {
        // CompuPhase RGB perceptual-ish distance
        var rmean = (c1.R + c2.R) / 2.0;
        var dr = c1.R - c2.R;
        var dg = c1.G - c2.G;
        var db = c1.B - c2.B;
        return ((2.0 + (rmean / 256.0)) * dr * dr)
               + (4.0 * dg * dg)
               + ((2.0 + ((255.0 - rmean) / 256.0)) * db * db);
    }

    public static int NearestPaletteIndex(byte r, byte g, byte b)
    {
        var best = 0;
        var bestD = double.PositiveInfinity;
        var c = (r, g, b);
        for (var i = 0; i < Palette.Length; i++)
        {
            var d = ColorDistance(c, Palette[i]);
            if (d < bestD)
            {
                bestD = d;
                best = i;
            }
        }

        return best;
    }

    /// <summary>预处理后的源图（去边后），供多次转换复用，避免每次全图解码。</summary>
    public sealed class PreparedImage
    {
        internal PreparedImage(BgraImage data) => Data = data;

        internal BgraImage Data { get; }
    }

    internal sealed class BgraImage
    {
        public required int Width { get; init; }

        public required int Height { get; init; }

        public required byte[] Pixels { get; init; } // BGRA
    }

    /// <summary>解码并（可选）去边，结果可复用于多次转换。</summary>
    public static PreparedImage Prepare(BitmapSource source, bool trimEmptyBorder = true)
    {
        ArgumentNullException.ThrowIfNull(source);
        var bgra = LoadBgra(source);
        if (trimEmptyBorder)
        {
            bgra = TrimBorder(bgra) ?? bgra;
        }

        return new PreparedImage(bgra);
    }

    private static BgraImage LoadBgra(BitmapSource source)
    {
        var converted = source.Format == PixelFormats.Bgra32
            ? source
            : new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);

        var w = converted.PixelWidth;
        var h = converted.PixelHeight;
        var stride = w * 4;
        var pixels = new byte[stride * h];
        converted.CopyPixels(pixels, stride, 0);
        return new BgraImage { Width = w, Height = h, Pixels = pixels };
    }

    private static BgraImage? TrimBorder(BgraImage src)
    {
        var w = src.Width;
        var h = src.Height;
        var px = src.Pixels;

        static bool IsContent(byte b, byte g, byte r, byte a)
        {
            if (a < 16)
            {
                return false;
            }

            // 近白视为空白边
            return r < 250 || g < 250 || b < 250;
        }

        var minX = w;
        var minY = h;
        var maxX = -1;
        var maxY = -1;

        for (var y = 0; y < h; y++)
        {
            for (var x = 0; x < w; x++)
            {
                var i = ((y * w) + x) * 4;
                if (!IsContent(px[i], px[i + 1], px[i + 2], px[i + 3]))
                {
                    continue;
                }

                if (x < minX)
                {
                    minX = x;
                }

                if (y < minY)
                {
                    minY = y;
                }

                if (x > maxX)
                {
                    maxX = x;
                }

                if (y > maxY)
                {
                    maxY = y;
                }
            }
        }

        if (maxX < minX || maxY < minY)
        {
            return null;
        }

        var nw = maxX - minX + 1;
        var nh = maxY - minY + 1;
        if (nw == w && nh == h)
        {
            return src;
        }

        var np = new byte[nw * nh * 4];
        for (var y = 0; y < nh; y++)
        {
            Buffer.BlockCopy(px, (((minY + y) * w) + minX) * 4, np, y * nw * 4, nw * 4);
        }

        return new BgraImage { Width = nw, Height = nh, Pixels = np };
    }

    /// <summary>
    /// 将内容图按 Fit + 可选归一化取景，采样到 24×24 浮点 RGB。
    /// </summary>
    private static (double R, double G, double B)[,] SampleToGrid(BgraImage src, ConvertOptions options)
    {
        var view = options.ContentViewNormalized ?? new Rect(0, 0, 1, 1);
        view = NormalizeViewRect(view);

        double srcX0 = view.X * src.Width;
        double srcY0 = view.Y * src.Height;
        double srcW = Math.Max(1e-6, view.Width * src.Width);
        double srcH = Math.Max(1e-6, view.Height * src.Height);

        // 在取景矩形内再按 Fit 映射到 24×24
        double mapX0, mapY0, mapW, mapH;
        switch (options.Fit)
        {
            case FitMode.Stretch:
                mapX0 = srcX0;
                mapY0 = srcY0;
                mapW = srcW;
                mapH = srcH;
                break;
            case FitMode.Contain:
            {
                // 外接矩形包含整张源图（Max），等比装入目标并留白
                var scale = Math.Max(srcW / GridSize, srcH / GridSize);
                mapW = GridSize * scale;
                mapH = GridSize * scale;
                mapX0 = srcX0 + ((srcW - mapW) / 2.0);
                mapY0 = srcY0 + ((srcH - mapH) / 2.0);
                break;
            }

            default: // Crop = cover
            {
                // 源图内最大的 1:1 采样矩形（Min），裁掉多余边并铺满
                var scale = Math.Min(srcW / GridSize, srcH / GridSize);
                mapW = GridSize * scale;
                mapH = GridSize * scale;
                mapX0 = srcX0 + ((srcW - mapW) / 2.0);
                mapY0 = srcY0 + ((srcH - mapH) / 2.0);
                break;
            }
        }

        var grid = new (double R, double G, double B)[GridSize, GridSize];
        for (var gy = 0; gy < GridSize; gy++)
        {
            for (var gx = 0; gx < GridSize; gx++)
            {
                // 格心采样
                var sx = mapX0 + (((gx + 0.5) / GridSize) * mapW);
                var sy = mapY0 + (((gy + 0.5) / GridSize) * mapH);
                grid[gy, gx] = SampleBilinear(src, sx, sy);
            }
        }

        return grid;
    }

    private static Rect NormalizeViewRect(Rect view)
    {
        var x = Math.Clamp(view.X, 0, 1);
        var y = Math.Clamp(view.Y, 0, 1);
        var w = Math.Clamp(view.Width, 1e-4, 1 - x);
        var h = Math.Clamp(view.Height, 1e-4, 1 - y);
        return new Rect(x, y, w, h);
    }

    private static (double R, double G, double B) SampleBilinear(BgraImage src, double sx, double sy)
    {
        // 取景区外填白
        if (sx < 0 || sy < 0 || sx >= src.Width || sy >= src.Height)
        {
            return (255, 255, 255);
        }

        var x0 = (int)Math.Floor(sx);
        var y0 = (int)Math.Floor(sy);
        var x1 = Math.Min(x0 + 1, src.Width - 1);
        var y1 = Math.Min(y0 + 1, src.Height - 1);
        var tx = sx - x0;
        var ty = sy - y0;

        var c00 = GetRgb(src, x0, y0);
        var c10 = GetRgb(src, x1, y0);
        var c01 = GetRgb(src, x0, y1);
        var c11 = GetRgb(src, x1, y1);

        static double Lerp(double a, double b, double t) => a + ((b - a) * t);

        var r = Lerp(Lerp(c00.R, c10.R, tx), Lerp(c01.R, c11.R, tx), ty);
        var g = Lerp(Lerp(c00.G, c10.G, tx), Lerp(c01.G, c11.G, tx), ty);
        var b = Lerp(Lerp(c00.B, c10.B, tx), Lerp(c01.B, c11.B, tx), ty);
        return (r, g, b);
    }

    private static (double R, double G, double B) GetRgb(BgraImage src, int x, int y)
    {
        var i = ((y * src.Width) + x) * 4;
        var a = src.Pixels[i + 3] / 255.0;
        double b = src.Pixels[i];
        double g = src.Pixels[i + 1];
        double r = src.Pixels[i + 2];

        // 透明与白底合成
        r = (r * a) + (255 * (1 - a));
        g = (g * a) + (255 * (1 - a));
        b = (b * a) + (255 * (1 - a));
        return (r, g, b);
    }

    /// <summary>
    /// 在 sRGB 0~255 上近似 CSS filter（固定顺序：亮度 → 对比度 → 饱和度）。
    /// </summary>
    private static void ApplyCssLikeFilters((double R, double G, double B)[,] grid, double contrast, double brightness, double saturation)
    {
        if (Math.Abs(contrast - 1) < 1e-6 && Math.Abs(brightness - 1) < 1e-6 && Math.Abs(saturation - 1) < 1e-6)
        {
            return;
        }

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var (r, g, b) = grid[y, x];

                // brightness
                r *= brightness;
                g *= brightness;
                b *= brightness;

                // contrast around 0.5 mid gray in 0~1 space then back
                r = ((((r / 255.0) - 0.5) * contrast) + 0.5) * 255.0;
                g = ((((g / 255.0) - 0.5) * contrast) + 0.5) * 255.0;
                b = ((((b / 255.0) - 0.5) * contrast) + 0.5) * 255.0;

                // saturate via luma
                var luma = (0.2126 * r) + (0.7152 * g) + (0.0722 * b);
                r = luma + ((r - luma) * saturation);
                g = luma + ((g - luma) * saturation);
                b = luma + ((b - luma) * saturation);

                grid[y, x] = (ClampByte(r), ClampByte(g), ClampByte(b));
            }
        }
    }

    private static double ClampByte(double v) => Math.Clamp(v, 0, 255);

    private static int[,] Quantize((double R, double G, double B)[,] sample, DitherMode dither)
    {
        var work = new (double R, double G, double B)[GridSize, GridSize];
        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                work[y, x] = sample[y, x];
            }
        }

        var result = new int[GridSize, GridSize];

        void AddError(int x, int y, double er, double eg, double eb, double factor)
        {
            if (x < 0 || y < 0 || x >= GridSize || y >= GridSize)
            {
                return;
            }

            var p = work[y, x];
            work[y, x] = (
                ClampByte(p.R + (er * factor)),
                ClampByte(p.G + (eg * factor)),
                ClampByte(p.B + (eb * factor)));
        }

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var old = work[y, x];
                var or = (byte)Math.Clamp((int)Math.Round(old.R), 0, 255);
                var og = (byte)Math.Clamp((int)Math.Round(old.G), 0, 255);
                var ob = (byte)Math.Clamp((int)Math.Round(old.B), 0, 255);
                var idx = NearestPaletteIndex(or, og, ob);
                result[y, x] = idx;

                if (dither == DitherMode.None)
                {
                    continue;
                }

                var (nr, ng, nb) = Palette[idx];
                var er = old.R - nr;
                var eg = old.G - ng;
                var eb = old.B - nb;

                if (dither == DitherMode.FloydSteinberg)
                {
                    AddError(x + 1, y, er, eg, eb, 7.0 / 16.0);
                    AddError(x - 1, y + 1, er, eg, eb, 3.0 / 16.0);
                    AddError(x, y + 1, er, eg, eb, 5.0 / 16.0);
                    AddError(x + 1, y + 1, er, eg, eb, 1.0 / 16.0);
                }
                else
                {
                    // Atkinson
                    var f = 1.0 / 8.0;
                    AddError(x + 1, y, er, eg, eb, f);
                    AddError(x + 2, y, er, eg, eb, f);
                    AddError(x - 1, y + 1, er, eg, eb, f);
                    AddError(x, y + 1, er, eg, eb, f);
                    AddError(x + 1, y + 1, er, eg, eb, f);
                    AddError(x, y + 2, er, eg, eb, f);
                }
            }
        }

        return result;
    }
}
