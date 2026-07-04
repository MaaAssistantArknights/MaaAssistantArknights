// <copyright file="ColorExtractorHelper.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MaaWpfGui.Helper;

/// <summary>
/// 从图片中提取主色（dominant color）的工具类。
/// 采用降采样 + 颜色量化桶统计的方式，性能优于完整的 k-means。
/// </summary>
public static class ColorExtractorHelper
{
    /// <summary>
    /// 量化桶步长，将 RGB 空间按每 <see cref="BucketSize"/> 为一格分桶。
    /// </summary>
    private const int BucketSize = 16;

    /// <summary>
    /// 降采样后的最大边长（像素），用于加速取色。
    /// </summary>
    private const int SampleSize = 32;

    /// <summary>
    /// 从 BitmapImage 中提取出现频率最高的颜色。
    /// </summary>
    /// <param name="image">源图片。必须已调用过EndInit并完成解码。</param>
    /// <returns>提取到的主色；若图片无效则返回白色。</returns>
    public static Color ExtractDominantColor(BitmapSource image)
    {
        if (image == null)
        {
            return Colors.White;
        }

        // 降采样到 ~32×32 以加速
        var scaledWidth = Math.Min(SampleSize, image.PixelWidth);
        var scaledHeight = Math.Min(SampleSize, image.PixelHeight);

        var converted = new FormatConvertedBitmap(image, PixelFormats.Bgra32, null, 0);
        var resized = new TransformedBitmap(converted, new ScaleTransform(
            (double)scaledWidth / image.PixelWidth,
            (double)scaledHeight / image.PixelHeight));

        var pixels = new byte[scaledWidth * scaledHeight * 4];
        resized.CopyPixels(pixels, scaledWidth * 4, 0);

        // 量化到桶，统计频率
        var buckets = new Dictionary<long, (long Count, long R, long G, long B)>();

        for (var i = 0; i < pixels.Length; i += 4)
        {
            var a = pixels[i + 3];
            if (a < 128)
            {
                // 跳过近透明像素
                continue;
            }

            var r = pixels[i + 2]; // BGRA
            var g = pixels[i + 1];
            var b = pixels[i];

            var bucketR = r / BucketSize;
            var bucketG = g / BucketSize;
            var bucketB = b / BucketSize;
            var key = (((bucketR * 256L) + bucketG) * 256L) + bucketB;

            if (buckets.TryGetValue(key, out var entry))
            {
                buckets[key] = (entry.Count + 1, entry.R + r, entry.G + g, entry.B + b);
            }
            else
            {
                buckets[key] = (1, r, g, b);
            }
        }

        if (buckets.Count == 0)
        {
            return Colors.White;
        }

        // 对每个桶计算「频率 × 鲜艳度」的综合得分，优先选高占比且鲜艳的颜色
        Color bestColor = Colors.White;
        var bestScore = -1.0;

        foreach (var (_, value) in buckets)
        {
            long count = value.Count;
            var avgR = (byte)(value.R / count);
            var avgG = (byte)(value.G / count);
            var avgB = (byte)(value.B / count);
            var avgColor = Color.FromRgb(avgR, avgG, avgB);

            var (_, sat, _) = RgbToHsl(avgColor);

            // 频率占比 × 饱和度²，饱和度平方确保鲜艳颜色优先
            var frequencyRatio = (double)count / (scaledWidth * scaledHeight);
            var score = frequencyRatio * sat * sat;

            if (score > bestScore)
            {
                bestScore = score;
                bestColor = avgColor;
            }
        }

        return bestColor;
    }

    /// <summary>
    /// 异步版本的取色方法，将计算放到后台线程。
    /// </summary>
    /// <param name="image">源图片。</param>
    /// <returns>提取到的主色。</returns>
    public static Task<Color> ExtractDominantColorAsync(BitmapSource image)
    {
        if (image == null)
        {
            return Task.FromResult(Colors.White);
        }

        // BitmapImage 必须 Freeze 后才能跨线程访问像素
        if (!image.IsFrozen)
        {
            image.Freeze();
        }

        return Task.Run(() => ExtractDominantColor(image));
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
}
