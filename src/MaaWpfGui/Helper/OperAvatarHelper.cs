// <copyright file="OperAvatarHelper.cs" company="MaaAssistantArknights">
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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using MaaWpfGui.Constants.Enums;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 干员头像（<c>resource/template/avatar/&lt;干员id&gt;.png</c>，由 tools/ResourceUpdater 随资源更新生成）的加载与「头像 + 干员名」组合元素的构建。
/// </summary>
public static class OperAvatarHelper
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "OperAvatarHelper");

    /// <summary>
    /// 未拥有干员头像的色彩保留比例（0=全灰，1=原色）。
    /// </summary>
    private const double DesaturatedColorKeep = 0.35;

    /// <summary>
    /// CreateOperBadge 生成的元素写入 <see cref="FrameworkElement.Tag"/> 的载荷前缀，供 ToolTipHelper 等需要克隆元素的场景按参数重建。
    /// </summary>
    internal const string OperBadgeTagPrefix = "OperBadge";

    /// <summary>
    /// OperBadge 载荷的分隔符（干员名等文本中不会出现）。
    /// </summary>
    internal const char OperBadgeTagSeparator = '\u0001';

    // per-key Lazy single-flight：同一 key 并发只解码一次，加载失败（null）同样被缓存，不在工厂外判空重试
    private static readonly ConcurrentDictionary<string, Lazy<BitmapSource?>> _avatarCache = new();
    private static readonly ConcurrentDictionary<string, Lazy<BitmapSource?>> _desaturatedAvatarCache = new();
    private static readonly ConcurrentDictionary<OperatorRole, BitmapSource?> _roleIconCache = new();

    // 与 MaaCore OperBoxImageAnalyzer 的职业旗标顺序保持一致
    private static readonly Dictionary<OperatorRole, int> _roleTemplateIndex = new()
    {
        [OperatorRole.Caster] = 1,
        [OperatorRole.Medic] = 2,
        [OperatorRole.Pioneer] = 3,
        [OperatorRole.Sniper] = 4,
        [OperatorRole.Special] = 5,
        [OperatorRole.Support] = 6,
        [OperatorRole.Tank] = 7,
        [OperatorRole.Warrior] = 8,
    };

    /// <summary>
    /// 按干员 ID 获取头像。
    /// </summary>
    /// <param name="operId">干员 ID，如 char_002_amiya</param>
    /// <param name="desaturated">是否降低饱和度（如识别结果中未拥有的干员）</param>
    /// <returns>头像图；资源缺失或干员无法识别时返回 <c>null</c></returns>
    public static BitmapSource? GetOperAvatar(string operId, bool desaturated = false)
    {
        if (string.IsNullOrEmpty(operId))
        {
            return null;
        }

        if (!desaturated)
        {
            return _avatarCache.GetOrAdd(operId, CreateAvatarLazy).Value;
        }

        return _desaturatedAvatarCache.GetOrAdd(
            operId,
            id => new Lazy<BitmapSource?>(
                () =>
                {
                    var normal = _avatarCache.GetOrAdd(id, CreateAvatarLazy).Value;
                    return normal != null ? Desaturate(normal, DesaturatedColorKeep) : normal;
                },
                LazyThreadSafetyMode.ExecutionAndPublication)).Value;

        static Lazy<BitmapSource?> CreateAvatarLazy(string id)
            => new(() => LoadAvatar(id), LazyThreadSafetyMode.ExecutionAndPublication);
    }

    /// <summary>
    /// 按干员名（含别名解析）获取头像。
    /// </summary>
    /// <param name="operName">干员名</param>
    /// <returns>头像图；解析不到干员或资源缺失时返回 <c>null</c></returns>
    public static BitmapSource? GetOperAvatarByName(string operName)
    {
        var info = DataHelper.GetCharacterByNameOrAlias(operName);
        return info == null ? null : GetOperAvatar(info.Id);
    }

    /// <summary>
    /// 获取干员职业图标（复用识别用职业旗标模板 <c>template/OperBox/OperBoxFlagRole{N}.png</c>）。
    /// </summary>
    /// <param name="role">干员职业</param>
    /// <returns>职业图标；无对应图标时返回 <c>null</c></returns>
    public static BitmapSource? GetRoleIcon(OperatorRole role)
    {
        return _roleIconCache.GetOrAdd(role, LoadRoleIcon);
    }

    private static readonly Lazy<BitmapSource?> _maaIcon = new(DecodeMaaIconCore, LazyThreadSafetyMode.ExecutionAndPublication);

    private static readonly Lazy<BitmapSource?> _desaturatedMaaIcon = new(
        () =>
        {
            var normal = _maaIcon.Value;
            return normal != null ? Desaturate(normal, DesaturatedColorKeep) : normal;
        },
        LazyThreadSafetyMode.ExecutionAndPublication);

    /// <summary>
    /// 获取 MAA 应用图标（打包资源 <c>newlogo.ico</c>），供干员识别中的特殊干员展示使用。
    /// </summary>
    /// <param name="desaturated">是否降低饱和度（如识别结果中未拥有的特殊干员）</param>
    /// <returns>MAA 图标；解码失败时返回 <c>null</c></returns>
    public static BitmapSource? GetMaaIcon(bool desaturated = false)
    {
        return (desaturated ? _desaturatedMaaIcon : _maaIcon).Value;
    }

    private static BitmapSource? DecodeMaaIconCore()
    {
        try
        {
            var decoder = new IconBitmapDecoder(
                new Uri("pack://application:,,,/newlogo.ico"),
                BitmapCreateOptions.None,
                BitmapCacheOption.OnLoad);
            var frame = decoder.Frames.OrderByDescending(f => f.PixelWidth).FirstOrDefault();
            frame?.Freeze();
            return frame;
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to decode MAA icon from newlogo.ico");
            return null;
        }
    }

    /// <summary>
    /// 生成「头像 + 干员名」一体的展示元素：头像与名字作为一个整体参与排版，不会被换行拆开。
    /// 无头像（资源缺失或干员无法识别）时退化为仅名字的元素。
    /// 生成参数同时写入 <see cref="FrameworkElement.Tag"/>，ToolTipHelper 等场景可据此重建克隆。
    /// </summary>
    /// <param name="operId">干员 ID</param>
    /// <param name="displayName">展示文本（本地化后的干员名及附加文本）</param>
    /// <param name="avatarSize">头像边长</param>
    /// <param name="foregroundResourceKey">名字前景色资源键（如稀有度颜色），null 则继承所在处默认前景色</param>
    /// <returns>可直接用于 UI 或 <see cref="System.Windows.Documents.InlineUIContainer"/> 的元素</returns>
    public static FrameworkElement CreateOperBadge(string operId, string displayName, double avatarSize = 18, string? foregroundResourceKey = null)
    {
        var badge = new StackPanel { Orientation = Orientation.Horizontal };

        var avatar = GetOperAvatar(operId);
        if (avatar != null)
        {
            badge.Children.Add(new Image
            {
                Source = avatar,
                Width = avatarSize,
                Height = avatarSize,
                VerticalAlignment = VerticalAlignment.Center,
            });
        }

        var name = new TextBlock
        {
            Text = displayName,
            VerticalAlignment = VerticalAlignment.Center,
            Margin = avatar == null ? default : new Thickness(3, 0, 0, 0),
        };
        if (!string.IsNullOrEmpty(foregroundResourceKey))
        {
            name.SetResourceReference(TextBlock.ForegroundProperty, foregroundResourceKey);
        }

        badge.Children.Add(name);
        badge.ToolTip = operId;
        badge.Tag = string.Join(
            OperBadgeTagSeparator,
            OperBadgeTagPrefix,
            operId,
            displayName,
            avatarSize,
            foregroundResourceKey ?? string.Empty);
        return badge;
    }

    /// <summary>
    /// 按干员名（含别名解析）生成「头像 + 干员名」一体的展示元素，规则同 <see cref="CreateOperBadge(string,string,double,string?)"/>。
    /// </summary>
    /// <param name="operName">干员名</param>
    /// <param name="avatarSize">头像边长</param>
    /// <param name="foregroundResourceKey">名字前景色资源键，null 则继承所在处默认前景色</param>
    /// <returns>可直接用于 UI 或 <see cref="System.Windows.Documents.InlineUIContainer"/> 的元素</returns>
    public static FrameworkElement CreateOperBadgeByName(string operName, double avatarSize = 18, string? foregroundResourceKey = null)
    {
        var info = DataHelper.GetCharacterByNameOrAlias(operName);
        return CreateOperBadge(info?.Id ?? string.Empty, operName, avatarSize, foregroundResourceKey);
    }

    /// <summary>
    /// 从 Tag 载荷重建 CreateOperBadge 生成的元素。
    /// </summary>
    /// <param name="tag">CreateOperBadge 写入的 Tag 载荷</param>
    /// <returns>重建出的元素；载荷不合法时返回 <c>null</c></returns>
    internal static FrameworkElement? TryRecreateOperBadge(object? tag)
    {
        if (tag is string payload && payload.StartsWith(OperBadgeTagPrefix, StringComparison.Ordinal))
        {
            var parts = payload.Split(OperBadgeTagSeparator);
            if (parts.Length == 5 && double.TryParse(parts[3], out var avatarSize))
            {
                return CreateOperBadge(parts[1], parts[2], avatarSize, parts[4].Length == 0 ? null : parts[4]);
            }
        }

        return null;
    }

    private static BitmapSource? LoadAvatar(string operId)
    {
        var avatarDir = Path.Combine(PathsHelper.ResourceDir, "template", "avatar");
        var canonicalId = DataHelper.GetCanonicalOperId(operId);
        return DecodeImage(Path.Combine(avatarDir, $"{operId}.png"))
            ?? (canonicalId == operId ? null : DecodeImage(Path.Combine(avatarDir, $"{canonicalId}.png")));
    }

    private static BitmapSource? LoadRoleIcon(OperatorRole role)
    {
        if (!_roleTemplateIndex.TryGetValue(role, out var index))
        {
            return null;
        }

        var imagePath = Path.Combine(PathsHelper.ResourceDir, "template", "OperBox", $"OperBoxFlagRole{index}.png");
        if (!File.Exists(imagePath))
        {
            _logger.Warning("Role icon not found: {ImagePath}", imagePath);
            return null;
        }

        return DecodeImage(imagePath);
    }

    /// <summary>
    /// 在调用线程解码图片并冻结，冻结后的位图可跨线程用于 UI 绑定。
    /// </summary>
    /// <param name="imagePath">图片路径</param>
    /// <returns>解码并冻结的位图；失败时返回 <c>null</c></returns>
    private static BitmapSource? DecodeImage(string imagePath)
    {
        if (!File.Exists(imagePath))
        {
            return null;
        }

        return DecodeImageCore(imagePath);
    }

    private static BitmapSource? DecodeImageCore(string imagePath)
    {
        try
        {
            var image = new BitmapImage();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.UriSource = new Uri(imagePath, UriKind.RelativeOrAbsolute);
            image.EndInit();
            image.Freeze();
            return image;
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to decode image from {ImagePath}", imagePath);
            return null;
        }
    }

    /// <summary>
    /// 将头像按比例降低饱和度（保留原色彩观感，不做全灰）。
    /// </summary>
    /// <param name="source">原头像</param>
    /// <param name="colorKeep">色彩保留比例（0=全灰，1=原色）</param>
    /// <returns>降饱和后的头像</returns>
    private static BitmapSource Desaturate(BitmapSource source, double colorKeep)
    {
        var converted = new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);
        var writeable = new WriteableBitmap(converted);
        writeable.Lock();
        try
        {
            unsafe
            {
                var pixels = (uint*)writeable.BackBuffer;
                int pixelCount = writeable.PixelWidth * writeable.PixelHeight;
                for (int i = 0; i < pixelCount; i++)
                {
                    uint pixel = pixels[i];
                    byte a = (byte)(pixel >> 24);
                    double b = pixel & 0xFF;
                    double g = (pixel >> 8) & 0xFF;
                    double r = (pixel >> 16) & 0xFF;
                    double gray = (r * 0.299) + (g * 0.587) + (b * 0.114);
                    byte nb = (byte)((b * colorKeep) + (gray * (1 - colorKeep)));
                    byte ng = (byte)((g * colorKeep) + (gray * (1 - colorKeep)));
                    byte nr = (byte)((r * colorKeep) + (gray * (1 - colorKeep)));
                    pixels[i] = ((uint)a << 24) | ((uint)nr << 16) | ((uint)ng << 8) | nb;
                }
            }

            writeable.AddDirtyRect(new Int32Rect(0, 0, writeable.PixelWidth, writeable.PixelHeight));
        }
        finally
        {
            writeable.Unlock();
        }

        writeable.Freeze();
        return writeable;
    }
}
