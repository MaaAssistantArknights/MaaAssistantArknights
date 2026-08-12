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
using System.Drawing;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Forms;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MaaWpfGui.Helper;

/// <summary>
/// 巡展像素画：原图 → 24×24 × 官方 40 色。
/// 色序与游戏右侧色板一致；最近色使用 OKLab 感知距离。
/// </summary>
public static class PixelPaintHelper
{
    /// <summary>网格边长，游戏像素编辑器为 24×24。</summary>
    public const int GridSize = 24;

    /// <summary>官方色板颜色数。</summary>
    public const int ColorCount = 40;

    /// <summary>纯白在 40 色板中的下标（0-based）。</summary>
    public const int WhiteColorIndex = 3;

    /// <summary>预览图边长（像素），按 GridSize 等比放大。</summary>
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

    /// <summary>调色板的 OKLab 预转换缓存，避免最近色比对时反复解码。</summary>
    private static readonly (double L, double A, double B)[] _paletteOklab =
        System.Array.ConvertAll(Palette, c => Srgb8ToOklab(c.R, c.G, c.B));

    /// <summary>原图到 24×24 网格的构图方式。</summary>
    public enum FitMode
    {
        /// <summary>裁剪填充（cover）：取源图最大 1:1 区域铺满网格，裁掉超出部分。</summary>
        Crop,

        /// <summary>完整包含（letterbox）：整张源图等比装入网格，两侧留白。</summary>
        Contain,

        /// <summary>拉伸：忽略宽高比，强行铺满网格。</summary>
        Stretch,
    }

    /// <summary>量化抖动方式。</summary>
    public enum DitherMode
    {
        /// <summary>不抖动：纯最近邻量化，无误差扩散。</summary>
        None,

        /// <summary>Floyd-Steinberg（蛇形扫描，部分误差扩散）：适合照片等渐变内容。</summary>
        FloydSteinberg,

        /// <summary>Atkinson：仅扩散 6/8 误差，噪点较 FS 更柔和。</summary>
        Atkinson,

        /// <summary>插画优先：medoid 代表色 + 边缘感知 MRF，保留干净色块、压交界噪点。</summary>
        Illustration,
    }

    public sealed class ConvertOptions
    {
        /// <summary>原图到 24×24 的构图方式，默认裁剪填充。</summary>
        public FitMode Fit { get; init; } = FitMode.Crop;

        /// <summary>量化抖动方式，默认插画优先。</summary>
        public DitherMode Dither { get; init; } = DitherMode.Illustration;

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

        /// <summary>等比放大的预览位图，供 UI 展示。</summary>
        public required WriteableBitmap Preview { get; init; }

        /// <summary>按色分组后的点列，供 Core 使用（已按 skipWhite 过滤）。</summary>
        public required List<ColorGroup> Groups { get; init; }

        /// <summary>需绘制的格子总数（即 Groups 中所有点数之和）。</summary>
        public int PaintedCellCount { get; init; }
    }

    public sealed class ColorGroup
    {
        /// <summary>色板下标 0~39。</summary>
        public int Color { get; init; }

        /// <summary>格子坐标，左上为 (0,0)，x 右 y 下。</summary>
        public List<int[]> Points { get; init; } = [];
    }

    /// <summary>
    /// 解码源图并转换为 24×24 像素画结果（含预览与分组）。
    /// 内部先 <see cref="Prepare(BitmapSource, bool)"/> 去边，再转交给预处理重载。
    /// </summary>
    /// <param name="source">任意 WPF 可解码的位图源。</param>
    /// <param name="options">转换参数（构图、抖动、滤镜等）。</param>
    /// <param name="skipWhite">是否跳过纯白格（不画），默认 true。</param>
    /// <returns>色号矩阵 + 预览图 + 按色分组。</returns>
    public static ConvertResult Convert(BitmapSource source, ConvertOptions options, bool skipWhite = true)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(options);

        return Convert(Prepare(source, options.TrimEmptyBorder), options, skipWhite);
    }

    /// <summary>
    /// 用预处理好的图像转换，适合需要多次调参实时预览的场景。
    /// 流程：线性光面积采样 → CSS 风格滤镜 → 量化（按 <see cref="DitherMode"/> 分发）→ 分组/预览。
    /// </summary>
    /// <param name="prepared">已去边的预处理图像，可复用避免重复解码。</param>
    /// <param name="options">转换参数（构图、抖动、滤镜等）。</param>
    /// <param name="skipWhite">是否跳过纯白格（不画），默认 true。</param>
    /// <returns>色号矩阵 + 预览图 + 按色分组。</returns>
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

        var matrix = options.Dither == DitherMode.Illustration
            ? QuantizeIllustration(sample)
            : Quantize(sample, options.Dither);
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

    /// <summary>把色号矩阵按色板下标分桶，输出供 Core 逐色绘制的点列。</summary>
    /// <param name="matrix">24×24 色号矩阵，值域 0~39。</param>
    /// <param name="skipWhite">是否跳过纯白格（<see cref="WhiteColorIndex"/>）。</param>
    /// <returns>非空分组列表，每组含色板下标与该色所有格子坐标。</returns>
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

    /// <summary>把色号矩阵渲染成等比放大的预览位图（每格一个色块）。</summary>
    /// <param name="matrix">24×24 色号矩阵，值域 0~39。</param>
    /// <returns>已冻结的 <see cref="PreviewPixelSize"/>×<see cref="PreviewPixelSize"/> BGRA 位图。</returns>
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

    /// <summary>
    /// 将最多 4 个字渲染为黑字白底位图（24×24）。中文按自适应网格（1 字占满、2~4 字分 2 列）以 SimSun 直接绘制（点阵）；
    /// 英文 / 数字单行横排，放大字号、无抗锯齿渲染后裁掉墨迹空白，1:1 放入（不降采样，保持锐利）。
    /// </summary>
    /// <param name="text">输入文字；取前 4 个字素（按 <see cref="StringInfo"/> 拆分，避免拆开代理对），超出忽略。</param>
    /// <returns>已冻结的 24×24 Bgra32 位图，可直接作为像素画原图。</returns>
    public static BitmapSource RenderTextToBitmap(string text)
    {
        // 按字素取前 4 个，正确处理代理对 / 组合字符
        var chars = new List<string>(4);
        var enumerator = StringInfo.GetTextElementEnumerator(text);
        while (enumerator.MoveNext() && chars.Count < 4)
        {
            chars.Add((string)enumerator.Current);
        }

        var wide = chars.Exists(HasNonAscii);
        const TextFormatFlags flags = TextFormatFlags.NoPadding | TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.SingleLine;

        const int size = 24;          // 目标网格分辨率，1:1 映射

        using var bmp = new Bitmap(size, size, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using var g = Graphics.FromImage(bmp);
        g.Clear(System.Drawing.Color.White);

        if (wide)
        {
            // 中文：自适应网格，1 字占满整张、2~4 字分 2 列；每格 SimSun 该尺寸直接绘制（点阵）
            var n = chars.Count;
            var cols = Math.Min(n, 2);
            var rows = (n + cols - 1) / cols;
            var cellW = size / cols;
            var cellH = size / rows;
            using var cf = new Font("SimSun", Math.Min(cellW, cellH), System.Drawing.FontStyle.Regular, GraphicsUnit.Pixel);
            for (var i = 0; i < n; i++)
            {
                var bounds = new Rectangle((i % cols) * cellW, (i / cols) * cellH, cellW, cellH);
                TextRenderer.DrawText(g, chars[i], cf, bounds, System.Drawing.Color.Black, flags);
            }
        }
        else
        {
            // 英文 / 数字：整串单行横排，放大字号、无抗锯齿渲染后裁墨迹空白，1:1 放入（不降采样）
            DrawInkFit(g, string.Concat(chars), new Rectangle(0, 0, size, size));
        }

        // GDI Bitmap → WPF BitmapSource：逐像素拷贝
        var lockRect = new Rectangle(0, 0, size, size);
        var data = bmp.LockBits(lockRect, System.Drawing.Imaging.ImageLockMode.ReadOnly, bmp.PixelFormat);
        var pixels = new byte[data.Stride * size];
        Marshal.Copy(data.Scan0, pixels, 0, pixels.Length);
        bmp.UnlockBits(data);

        var wb = new WriteableBitmap(size, size, 96, 96, PixelFormats.Bgra32, null);
        wb.WritePixels(new Int32Rect(0, 0, size, size), pixels, data.Stride, 0);
        wb.Freeze();
        return wb;
    }

    /// <summary>
    /// 在目标区域内以 ｢能放下｣ 的最大字号绘制文本：放大字号渲染、裁掉墨迹空白，取墨迹外接框恰能放入区域的字号，
    /// 再 1:1 居中绘制（不做降采样，保持锐利）。
    /// </summary>
    /// <param name="g">目标 GDI 绘图面。</param>
    /// <param name="text">待绘制文本。</param>
    /// <param name="rect">目标区域。</param>
    private static void DrawInkFit(Graphics g, string text, Rectangle rect)
    {
        const TextFormatFlags flags = TextFormatFlags.NoPadding | TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.SingleLine;

        // 二分找最大字号，使墨迹外接框（裁空白后）放入 rect
        var lo = 8f;
        var hi = 128f;
        for (var i = 0; i < 12; i++)
        {
            var mid = (lo + hi) / 2f;
            using var probe = RenderInk(g, text, mid, flags);
            if (probe != null && probe.Width <= rect.Width && probe.Height <= rect.Height)
            {
                lo = mid;
            }
            else
            {
                hi = mid;
            }
        }

        using var ink = RenderInk(g, text, lo, flags);
        if (ink == null)
        {
            return;
        }

        // 1:1 居中放入 rect（DrawImage 在整数坐标、原始尺寸下不重采样）
        var dx = rect.X + ((rect.Width - ink.Width) / 2);
        var dy = rect.Y + ((rect.Height - ink.Height) / 2);
        g.DrawImage(ink, dx, dy);
    }

    /// <summary>
    /// 以给定字号渲染文本，扫描 alpha 通道取墨迹外接框，裁掉四周空白后返回墨迹位图。
    /// </summary>
    /// <param name="g">GDI 绘图面（用于度量）。</param>
    /// <param name="text">待绘制文本。</param>
    /// <param name="emSize">字号（像素）。</param>
    /// <param name="flags">文本格式标志。</param>
    /// <returns>裁空白后的墨迹位图；无墨迹返回 null。</returns>
    private static Bitmap? RenderInk(Graphics g, string text, float emSize, TextFormatFlags flags)
    {
        using var f = new Font("SimSun", emSize, System.Drawing.FontStyle.Regular, GraphicsUnit.Pixel);
        var m = TextRenderer.MeasureText(g, text, f);
        var pad = (int)Math.Ceiling(emSize * 0.2);
        var w = m.Width + (pad * 2);
        var h = m.Height + (pad * 2);

        using var tmp = new Bitmap(w, h, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using (var tg = Graphics.FromImage(tmp))
        {
            // 透明底：让墨迹扫描只命中笔画；无抗锯齿：保持锐利
            tg.Clear(System.Drawing.Color.Transparent);
            tg.TextRenderingHint = System.Drawing.Text.TextRenderingHint.SingleBitPerPixelGridFit;
            TextRenderer.DrawText(tg, text, f, new Rectangle(0, 0, w, h), System.Drawing.Color.Black, flags);
        }

        var ld = tmp.LockBits(new Rectangle(0, 0, w, h), System.Drawing.Imaging.ImageLockMode.ReadOnly, tmp.PixelFormat);
        var px = new byte[ld.Stride * h];
        Marshal.Copy(ld.Scan0, px, 0, px.Length);
        tmp.UnlockBits(ld);

        var minX = w;
        var minY = h;
        var maxX = -1;
        var maxY = -1;
        for (var y = 0; y < h; y++)
        {
            for (var x = 0; x < w; x++)
            {
                if (px[(((y * w) + x) * 4) + 3] > 16)
                {
                    if (x < minX)
                    {
                        minX = x;
                    }

                    if (x > maxX)
                    {
                        maxX = x;
                    }

                    if (y < minY)
                    {
                        minY = y;
                    }

                    if (y > maxY)
                    {
                        maxY = y;
                    }
                }
            }
        }

        if (maxX < 0)
        {
            return null;
        }

        var iw = maxX - minX + 1;
        var ih = maxY - minY + 1;
        var ink = new Bitmap(iw, ih, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using (var ig = Graphics.FromImage(ink))
        {
            ig.DrawImage(tmp, new Rectangle(0, 0, iw, ih), new Rectangle(minX, minY, iw, ih), GraphicsUnit.Pixel);
        }

        return ink;
    }

    /// <summary>
    /// 判断字素中是否含非 ASCII 字符（如中文），用于决定四角布局还是单行横排。
    /// </summary>
    /// <param name="textElement">单个字素。</param>
    /// <returns>含非 ASCII 字符返回 true。</returns>
    private static bool HasNonAscii(string textElement)
    {
        foreach (var c in textElement)
        {
            if (c >= 128)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// OKLab 感知色差（欧氏距离平方）。向后兼容入口，内部逐次转换。
    /// 热路径请用 <see cref="NearestPaletteIndex((double L, double A, double B))"/> 的 OKLab 缓存比对。
    /// </summary>
    /// <param name="c1">颜色一。</param>
    /// <param name="c2">颜色二。</param>
    /// <returns>两色在 OKLab 空间的欧氏距离平方，越小越接近。</returns>
    public static double ColorDistance((byte R, byte G, byte B) c1, (byte R, byte G, byte B) c2)
    {
        var l1 = Srgb8ToOklab(c1.R, c1.G, c1.B);
        var l2 = Srgb8ToOklab(c2.R, c2.G, c2.B);
        var dL = l1.L - l2.L;
        var dA = l1.A - l2.A;
        var dB = l1.B - l2.B;
        return (dL * dL) + (dA * dA) + (dB * dB);
    }

    /// <summary>给定 sRGB 颜色，在 40 色板中找 OKLab 最近项。</summary>
    /// <param name="r">红 0~255。</param>
    /// <param name="g">绿 0~255。</param>
    /// <param name="b">蓝 0~255。</param>
    /// <returns>最近色板下标 0~39。</returns>
    public static int NearestPaletteIndex(byte r, byte g, byte b)
    {
        return NearestPaletteIndex(Srgb8ToOklab(r, g, b));
    }

    /// <summary>取色板下标对应的 #RRGGBB 颜色字符串，供日志等 UI 着色；越界返回 null。</summary>
    /// <param name="index">色板下标 0~39。</param>
    /// <returns>#RRGGBB 字符串；越界时 null。</returns>
    public static string? GetPaletteColorHex(int index)
    {
        if (index < 0 || index >= Palette.Length)
        {
            return null;
        }

        var (r, g, b) = Palette[index];
        return $"#{r:X2}{g:X2}{b:X2}";
    }

    /// <summary>给定 OKLab 坐标，在预转换缓存中找最近色板项。</summary>
    /// <param name="c">OKLab 坐标。</param>
    /// <returns>最近色板下标 0~39。</returns>
    internal static int NearestPaletteIndex((double L, double A, double B) c)
    {
        var best = 0;
        var bestD = double.PositiveInfinity;
        for (var i = 0; i < _paletteOklab.Length; i++)
        {
            var p = _paletteOklab[i];
            var dL = c.L - p.L;
            var dA = c.A - p.A;
            var dB = c.B - p.B;
            var d = (dL * dL) + (dA * dA) + (dB * dB);
            if (d < bestD)
            {
                bestD = d;
                best = i;
            }
        }

        return best;
    }

    /// <summary>
    /// 一格的采样结果：均值（线性光平均后编码回 sRGB）+ 格内子样（sRGB，白底已合成）。
    /// 子样供插画优先模式的 medoid 选取使用。
    /// </summary>
    internal readonly struct CellSample
    {
        /// <summary>格代表色红分量（线性光平均后编码回 sRGB，0~255）。</summary>
        public double R { get; init; }

        /// <summary>格代表色绿分量（线性光平均后编码回 sRGB，0~255）。</summary>
        public double G { get; init; }

        /// <summary>格代表色蓝分量（线性光平均后编码回 sRGB，0~255）。</summary>
        public double B { get; init; }

        /// <summary>格内子样扁平数组 [r,g,b, r,g,b, ...]，sRGB 0~255。</summary>
        public double[] Subs { get; init; }
    }

    /// <summary>sRGB 0~255 → 线性光 0~1（标准 IEC 61966-2-1 解伽马）。</summary>
    /// <param name="v8">sRGB 分量 0~255。</param>
    /// <returns>线性光值 0~1。</returns>
    private static double Srgb8ToLinear(double v8)
    {
        var x = v8 / 255.0;
        return x >= 0.04045 ? Math.Pow((x + 0.055) / 1.055, 2.4) : x / 12.92;
    }

    /// <summary>线性光 0~1 → sRGB 0~255（反向伽马编码）。</summary>
    /// <param name="lin">线性光值 0~1。</param>
    /// <returns>sRGB 分量 0~255。</returns>
    private static double LinearToSrgb8(double lin)
    {
        var v = lin <= 0.0031308 ? lin * 12.92 : (1.055 * Math.Pow(lin, 1.0 / 2.4)) - 0.055;
        return v * 255.0;
    }

    /// <summary>sRGB 8 位 → OKLab（Björn 2020 标准矩阵）。</summary>
    /// <param name="r8">红 0~255。</param>
    /// <param name="g8">绿 0~255。</param>
    /// <param name="b8">蓝 0~255。</param>
    /// <returns>(L, A, B) 感知坐标。</returns>
    internal static (double L, double A, double B) Srgb8ToOklab(byte r8, byte g8, byte b8)
    {
        var r = Srgb8ToLinear(r8);
        var g = Srgb8ToLinear(g8);
        var b = Srgb8ToLinear(b8);

        // 线性 sRGB → LMS'
        var l = (0.4122214708 * r) + (0.5363325363 * g) + (0.0514459929 * b);
        var m = (0.2119034982 * r) + (0.6806995451 * g) + (0.1073969566 * b);
        var s = (0.0883024619 * r) + (0.2817188376 * g) + (0.6299787005 * b);

        // 非线性化（立方根）
        var l_ = Math.Cbrt(l);
        var m_ = Math.Cbrt(m);
        var s_ = Math.Cbrt(s);

        return (
            (0.2104542553 * l_) + (0.7936177850 * m_) - (0.0040720468 * s_),
            (1.9779984951 * l_) - (2.4285922050 * m_) + (0.4505937099 * s_),
            (0.0259040371 * l_) + (0.7827717662 * m_) - (0.8086757660 * s_));
    }

    /// <summary>预处理后的源图（去边后），供多次转换复用，避免每次全图解码。</summary>
    public sealed class PreparedImage
    {
        internal PreparedImage(BgraImage data) => Data = data;

        /// <summary>去边后的 BGRA 像素数据。</summary>
        internal BgraImage Data { get; }
    }

    internal sealed class BgraImage
    {
        /// <summary>图像宽度（像素）。</summary>
        public required int Width { get; init; }

        /// <summary>图像高度（像素）。</summary>
        public required int Height { get; init; }

        /// <summary>BGRA 顺序的像素数据，stride = Width*4。</summary>
        public required byte[] Pixels { get; init; } // BGRA
    }

    /// <summary>解码并（可选）去边，结果可复用于多次转换。</summary>
    /// <param name="source">任意 WPF 可解码的位图源。</param>
    /// <param name="trimEmptyBorder">是否裁掉透明/近白边，默认 true。</param>
    /// <returns>预处理后的图像；若整图为空白则返回未裁剪原图。</returns>
    public static PreparedImage Prepare(BitmapSource source, bool trimEmptyBorder = true)
    {
        ArgumentNullException.ThrowIfNull(source);
        var bgra = LoadBgra(source);

        // 恰好 24×24 视为用户外部已处理好的像素画，不去边
        if (trimEmptyBorder && !(bgra.Width == GridSize && bgra.Height == GridSize))
        {
            bgra = TrimBorder(bgra) ?? bgra;
        }

        return new PreparedImage(bgra);
    }

    /// <summary>把位图源统一转为 BGRA32 像素数组。</summary>
    /// <param name="source">任意像素格式的位图源。</param>
    /// <returns>BGRA32 像素图。</returns>
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

    /// <summary>
    /// 裁掉图像四周的透明（alpha &lt; 16）与近白（RGB ≥ 250）像素，返回内容包围盒。
    /// </summary>
    /// <param name="src">原始 BGRA 像素图。</param>
    /// <returns>裁剪后的像素图；若整图无内容返回 null，无变化时返回原对象。</returns>
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
    /// 将内容图按 Fit + 可选归一化取景，采样到 24×24 格。
    /// 每格在线性光空间对格内多点做面积平均（BOX 滤波），再编码回 sRGB；
    /// 同时保留格内子样的 sRGB 原值，供插画优先模式的 medoid 选取。
    /// </summary>
    /// <param name="src">去边后的 BGRA 像素图。</param>
    /// <param name="options">转换参数（构图、取景等）。</param>
    /// <returns>24×24 采样网格，每格含均值与子样。</returns>
    private static CellSample[,] SampleToGrid(BgraImage src, ConvertOptions options)
    {
        // 源图恰好 24×24：逐像素直接取色，不做插值/面积采样，仅最近色匹配
        if (src.Width == GridSize && src.Height == GridSize)
        {
            var exact = new CellSample[GridSize, GridSize];
            for (var y = 0; y < GridSize; y++)
            {
                for (var x = 0; x < GridSize; x++)
                {
                    var (r, g, b) = GetRgb(src, x, y);
                    exact[y, x] = new CellSample { R = r, G = g, B = b, Subs = [r, g, b] };
                }
            }

            return exact;
        }

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

        // 格内子样数：按源图覆盖面积自适应，最多 4×4。格子像素数越大采样越密。
        var cellW = mapW / GridSize;
        var cellH = mapH / GridSize;
        var sxN = Math.Clamp((int)Math.Ceiling(cellW), 1, 4);
        var syN = Math.Clamp((int)Math.Ceiling(cellH), 1, 4);

        var grid = new CellSample[GridSize, GridSize];
        for (var gy = 0; gy < GridSize; gy++)
        {
            for (var gx = 0; gx < GridSize; gx++)
            {
                // 该格在源图中的矩形
                var gx0 = mapX0 + ((gx / (double)GridSize) * mapW);
                var gy0 = mapY0 + ((gy / (double)GridSize) * mapH);
                var gw = mapW / GridSize;
                var gh = mapH / GridSize;

                // 在格内取 sxN×syN 个均匀子样的格心
                var n = sxN * syN;
                var subs = new double[n * 3];
                double sumR = 0, sumG = 0, sumB = 0;
                double sumLR = 0, sumLG = 0, sumLB = 0; // 线性光累加，用于面积平均
                var k = 0;
                for (var iy = 0; iy < syN; iy++)
                {
                    for (var ix = 0; ix < sxN; ix++)
                    {
                        var sx = gx0 + (((ix + 0.5) / sxN) * gw);
                        var sy = gy0 + (((iy + 0.5) / syN) * gh);
                        var (cr, cg, cb) = SampleBilinear(src, sx, sy);

                        subs[k++] = cr;
                        subs[k++] = cg;
                        subs[k++] = cb;
                        sumR += cr;
                        sumG += cg;
                        sumB += cb;

                        // 线性光空间累加（避免 sRGB 直接平均导致交界偏暗）
                        sumLR += Srgb8ToLinear(cr);
                        sumLG += Srgb8ToLinear(cg);
                        sumLB += Srgb8ToLinear(cb);
                    }
                }

                // 线性光平均后编码回 sRGB，作为该格代表色（供 None/FS/Atkinson 使用）
                var avgR = LinearToSrgb8(sumLR / n);
                var avgG = LinearToSrgb8(sumLG / n);
                var avgB = LinearToSrgb8(sumLB / n);

                grid[gy, gx] = new CellSample {
                    R = avgR,
                    G = avgG,
                    B = avgB,
                    Subs = subs,
                };

                _ = (sumR, sumG, sumB); // 抑制未使用警告（sRGB 简单平均仅作参考，未采用）
            }
        }

        return grid;
    }

    /// <summary>把取景矩形钳制到合法范围 [0,1]，并保证宽高下限不为零。</summary>
    /// <param name="view">原始归一化矩形。</param>
    /// <returns>钳制后的合法矩形。</returns>
    private static Rect NormalizeViewRect(Rect view)
    {
        var x = Math.Clamp(view.X, 0, 1);
        var y = Math.Clamp(view.Y, 0, 1);
        var w = Math.Clamp(view.Width, 1e-4, 1 - x);
        var h = Math.Clamp(view.Height, 1e-4, 1 - y);
        return new Rect(x, y, w, h);
    }

    /// <summary>
    /// 在源图上做双线性插值采样。取景区外返回白色（即未绘制格）。
    /// </summary>
    /// <param name="src">BGRA 像素图。</param>
    /// <param name="sx">采样 x（浮点像素坐标）。</param>
    /// <param name="sy">采样 y（浮点像素坐标）。</param>
    /// <returns>插值后的 sRGB 颜色。</returns>
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

    /// <summary>读取某像素的 sRGB 颜色，透明像素与白底合成（透明→白）。</summary>
    /// <param name="src">BGRA 像素图。</param>
    /// <param name="x">像素 x 坐标。</param>
    /// <param name="y">像素 y 坐标。</param>
    /// <returns>已与白底合成的 sRGB 颜色。</returns>
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
    /// 对每格的均值与全部子样同步施用，使调参对插画优先模式同样生效。
    /// </summary>
    /// <param name="grid">24×24 采样网格，原地修改。</param>
    /// <param name="contrast">对比度系数，1 为原图。</param>
    /// <param name="brightness">亮度系数，1 为原图。</param>
    /// <param name="saturation">饱和度系数，1 为原图。</param>
    private static void ApplyCssLikeFilters(CellSample[,] grid, double contrast, double brightness, double saturation)
    {
        if (Math.Abs(contrast - 1) < 1e-6 && Math.Abs(brightness - 1) < 1e-6 && Math.Abs(saturation - 1) < 1e-6)
        {
            return;
        }

        static (double R, double G, double B) Filter(double r, double g, double b, double contrast, double brightness, double saturation)
        {
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

            return (ClampByte(r), ClampByte(g), ClampByte(b));
        }

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var cell = grid[y, x];

                // 均值
                var (ar, ag, ab) = Filter(cell.R, cell.G, cell.B, contrast, brightness, saturation);

                // 子样（插画优先模式会用）
                var subs = cell.Subs;
                for (var i = 0; i < subs.Length; i += 3)
                {
                    var (fr, fg, fb) = Filter(subs[i], subs[i + 1], subs[i + 2], contrast, brightness, saturation);
                    subs[i] = fr;
                    subs[i + 1] = fg;
                    subs[i + 2] = fb;
                }

                grid[y, x] = new CellSample {
                    R = ar,
                    G = ag,
                    B = ab,
                    Subs = subs,
                };
            }
        }
    }

    /// <summary>把数值钳制到字节范围 [0, 255]。</summary>
    /// <param name="v">任意浮点值。</param>
    /// <returns>钳制后的值。</returns>
    private static double ClampByte(double v) => Math.Clamp(v, 0, 255);

    /// <summary>
    /// 最近邻量化（含可选误差扩散抖动）。FS 采用蛇形扫描 + 部分误差扩散以减弱噪点；
    /// Atkinson 对称扩散 6/8 误差；None 不扩散。
    /// </summary>
    /// <param name="sample">24×24 采样网格（取均值参与量化）。</param>
    /// <param name="dither">抖动方式（None / FloydSteinberg / Atkinson）。</param>
    /// <returns>24×24 色号矩阵，值域 0~39。</returns>
    private static int[,] Quantize(CellSample[,] sample, DitherMode dither)
    {
        var work = new (double R, double G, double B)[GridSize, GridSize];
        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var c = sample[y, x];
                work[y, x] = (c.R, c.G, c.B);
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
            // 蛇形扫描：奇数行反向遍历，消除误差长期偏向一侧的条纹（仅对 FS 生效）
            var serpentine = dither == DitherMode.FloydSteinberg && (y % 2 == 1);
            var fwd = serpentine ? -1 : 1;

            for (var i = 0; i < GridSize; i++)
            {
                var x = serpentine ? (GridSize - 1 - i) : i;
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
                    // 蛇形 FS：行内前向 + 下一行按行向镜像，权重不变。
                    // 24×24 下整图仅 576 格，全强度误差扩散（系数 1.0）噪点过密、观感脏；
                    // 这里仅扩散部分误差（系数 < 1），衰减高频噪点但仍能压住渐变色带。
                    const double fsStrength = 0.6;
                    AddError(x + fwd, y, er, eg, eb, (7.0 / 16.0) * fsStrength);
                    AddError(x - fwd, y + 1, er, eg, eb, (3.0 / 16.0) * fsStrength);
                    AddError(x, y + 1, er, eg, eb, (5.0 / 16.0) * fsStrength);
                    AddError(x + fwd, y + 1, er, eg, eb, (1.0 / 16.0) * fsStrength);
                }
                else
                {
                    // Atkinson：对称扩散，无需蛇形
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

    /// <summary>
    /// 插画优先量化：medoid 代表色 + 边缘感知 MRF 平滑（对齐参考实现）。
    /// 每格在子样里选 ｢最接近格内 OKLab 均值｣ 的真实颜色为代表色，
    /// 再用 ICM 迭代 + 指数衰减软约束让相邻且原图颜色接近的格子倾向同色。
    /// </summary>
    /// <param name="sample">24×24 采样网格（取格内子样参与 medoid 选取）。</param>
    /// <returns>24×24 色号矩阵，值域 0~39。</returns>
    private static int[,] QuantizeIllustration(CellSample[,] sample)
    {
        // 1. 每格选代表色（medoid）：最接近格内 OKLab 均值的真实子样
        var repr = new (double L, double A, double B)[GridSize, GridSize];

        // 子样数固定 ≤4×4=16，循环外一次性分配复用，避免循环内 stackalloc（CA2014）
        const int MaxSubs = 16;
        Span<(double L, double A, double B)> lab = stackalloc (double, double, double)[MaxSubs];

        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var subs = sample[y, x].Subs;
                var n = subs.Length / 3;

                if (n == 0)
                {
                    repr[y, x] = Srgb8ToOklab(255, 255, 255);
                    continue;
                }

                // 预转 OKLab，并累加格内均值
                double sumL = 0, sumA = 0, sumB = 0;
                for (var i = 0; i < n; i++)
                {
                    var v = Srgb8ToOklab(
                        (byte)Math.Clamp((int)Math.Round(subs[i * 3]), 0, 255),
                        (byte)Math.Clamp((int)Math.Round(subs[(i * 3) + 1]), 0, 255),
                        (byte)Math.Clamp((int)Math.Round(subs[(i * 3) + 2]), 0, 255));
                    lab[i] = v;
                    sumL += v.L;
                    sumA += v.A;
                    sumB += v.B;
                }

                var mL = sumL / n;
                var mA = sumA / n;
                var mB = sumB / n;

                // 选离均值最近的真实子样
                var best = 0;
                var bestD = double.PositiveInfinity;
                for (var i = 0; i < n; i++)
                {
                    var dL = lab[i].L - mL;
                    var dA = lab[i].A - mA;
                    var dB = lab[i].B - mB;
                    var d = (dL * dL) + (dA * dA) + (dB * dB);
                    if (d < bestD)
                    {
                        bestD = d;
                        best = i;
                    }
                }

                repr[y, x] = lab[best];
            }
        }

        // 2. 预计算数据项：每格每色板的 OKLab 平方距离 [y,x,c]
        //    OKLab 平方距离量级 ~0.01，与下面的平滑项同尺度
        var dataCost = new double[GridSize, GridSize, ColorCount];
        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var m = repr[y, x];
                for (var c = 0; c < ColorCount; c++)
                {
                    var p = _paletteOklab[c];
                    var dL = m.L - p.L;
                    var dA = m.A - p.A;
                    var dB = m.B - p.B;
                    dataCost[y, x, c] = (dL * dL) + (dA * dA) + (dB * dB);
                }
            }
        }

        // 3. 初始标号：数据项最小的色板
        var labels = new int[GridSize, GridSize];
        for (var y = 0; y < GridSize; y++)
        {
            for (var x = 0; x < GridSize; x++)
            {
                var bestC = 0;
                var bestE = double.PositiveInfinity;
                for (var c = 0; c < ColorCount; c++)
                {
                    if (dataCost[y, x, c] < bestE)
                    {
                        bestE = dataCost[y, x, c];
                        bestC = c;
                    }
                }

                labels[y, x] = bestC;
            }
        }

        // 4. ICM 迭代：每格选使 ｢数据项 + 平滑项｣ 最小的标号
        //    平滑项 = 指数衰减软约束：相邻格原图颜色越接近，惩罚越高（越倾向同色）
        //      w = strength * exp(-oklab_dist² / 0.0025)
        //    邻居标号不同时 energy += w；颜色相差大时 w→0，自然允许分裂（无需显式边缘）
        //    strength 对齐参考实现（0.0012），可按实测微调
        const double strength = 0.0012;
        const double sigma2 = 0.0025;
        for (var iter = 0; iter < 3; iter++)
        {
            var changed = false;
            for (var y = 0; y < GridSize; y++)
            {
                for (var x = 0; x < GridSize; x++)
                {
                    var m = repr[y, x];
                    var bestLabel = labels[y, x];
                    var bestEnergy = double.PositiveInfinity;

                    // 预计算各邻居的平滑权重（基于原图 OKLab 距离）
                    double wL = 0, wR = 0, wU = 0, wD = 0; // 左右上下
                    if (x > 0)
                    {
                        wL = strength * Math.Exp(-OklabSqDist(m, repr[y, x - 1]) / sigma2);
                    }

                    if (x < GridSize - 1)
                    {
                        wR = strength * Math.Exp(-OklabSqDist(m, repr[y, x + 1]) / sigma2);
                    }

                    if (y > 0)
                    {
                        wU = strength * Math.Exp(-OklabSqDist(m, repr[y - 1, x]) / sigma2);
                    }

                    if (y < GridSize - 1)
                    {
                        wD = strength * Math.Exp(-OklabSqDist(m, repr[y + 1, x]) / sigma2);
                    }

                    for (var c = 0; c < ColorCount; c++)
                    {
                        var energy = dataCost[y, x, c];

                        // 平滑项：邻居标号不同则加权惩罚
                        if (x > 0 && labels[y, x - 1] != c)
                        {
                            energy += wL;
                        }

                        if (x < GridSize - 1 && labels[y, x + 1] != c)
                        {
                            energy += wR;
                        }

                        if (y > 0 && labels[y - 1, x] != c)
                        {
                            energy += wU;
                        }

                        if (y < GridSize - 1 && labels[y + 1, x] != c)
                        {
                            energy += wD;
                        }

                        if (energy < bestEnergy)
                        {
                            bestEnergy = energy;
                            bestLabel = c;
                        }
                    }

                    if (bestLabel != labels[y, x])
                    {
                        labels[y, x] = bestLabel;
                        changed = true;
                    }
                }
            }

            if (!changed)
            {
                break;
            }
        }

        return labels;
    }

    /// <summary>计算两 OKLab 颜色的欧氏距离平方。</summary>
    /// <param name="p">颜色一。</param>
    /// <param name="q">颜色二。</param>
    /// <returns>OKLab 距离平方，越小越接近。</returns>
    private static double OklabSqDist((double L, double A, double B) p, (double L, double A, double B) q)
    {
        var dL = p.L - q.L;
        var dA = p.A - q.A;
        var dB = p.B - q.B;
        return (dL * dL) + (dA * dA) + (dB * dB);
    }
}
