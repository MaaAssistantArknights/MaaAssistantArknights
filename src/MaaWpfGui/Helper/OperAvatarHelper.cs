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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using MaaWpfGui.Constants.Enums;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 干员头像雪碧图（<c>resource/template/avatar/avatar_sprite.png</c>）的裁切加载。
/// 格坐标来自 <c>battle_data.json</c> 中各干员条目的 <c>avatar_sprite</c> 字段，由 tools/ResourceUpdater 生成。
/// </summary>
public static class OperAvatarHelper
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "OperAvatarHelper");

    /// <summary>
    /// 雪碧图列数，与 tools/ResourceUpdater/main.cpp 的 AvatarSpriteColumns 保持一致。
    /// </summary>
    private const int SpriteColumns = 10;

    private static readonly object _spriteLock = new();
    private static readonly ConcurrentDictionary<string, BitmapSource?> _avatarCache = new();

    private static BitmapSource? _spriteSheet;
    private static int _cellSize;
    private static bool _spriteLoadAttempted;

    /// <summary>
    /// 按干员 ID 获取头像。
    /// </summary>
    /// <param name="operId">干员 ID，如 char_002_amiya</param>
    /// <returns>头像图；无雪碧图资源或无坐标时返回 <c>null</c></returns>
    public static BitmapSource? GetOperAvatar(string operId)
    {
        if (string.IsNullOrEmpty(operId))
        {
            return null;
        }

        if (_avatarCache.TryGetValue(operId, out var cached))
        {
            return cached;
        }

        var avatar = LoadOperAvatar(operId);
        _avatarCache.TryAdd(operId, avatar);
        return avatar;
    }

    /// <summary>
    /// 按干员名（含别名解析）获取头像。
    /// </summary>
    /// <param name="operName">干员名</param>
    /// <returns>头像图；解析不到干员或无坐标时返回 <c>null</c></returns>
    public static BitmapSource? GetOperAvatarByName(string operName)
    {
        var info = DataHelper.GetCharacterByNameOrAlias(operName);
        return info == null ? null : GetOperAvatar(info.Id);
    }

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
    /// 获取干员职业图标（复用识别用职业旗标模板 <c>template/OperBox/OperBoxFlagRole{N}.png</c>）。
    /// </summary>
    /// <param name="role">干员职业</param>
    /// <returns>职业图标；无对应图标时返回 <c>null</c></returns>
    public static BitmapSource? GetRoleIcon(OperatorRole role)
    {
        return _roleIconCache.GetOrAdd(role, LoadRoleIcon);
    }

    /// <summary>
    /// 生成「头像 + 干员名」一体的展示元素：头像与名字作为一个整体参与排版，不会被换行拆开。
    /// 无头像（资源缺失或无坐标）时退化为仅名字的元素。
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

        try
        {
            var icon = new BitmapImage();
            icon.BeginInit();
            icon.CacheOption = BitmapCacheOption.OnLoad;
            icon.UriSource = new Uri(imagePath, UriKind.RelativeOrAbsolute);
            icon.EndInit();
            icon.Freeze();
            return icon;
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to load role icon from {ImagePath}", imagePath);
            return null;
        }
    }

    private static BitmapSource? LoadOperAvatar(string operId)
    {
        var sprite = EnsureSpriteSheet();
        if (sprite == null)
        {
            return null;
        }

        var pos = GetAvatarSpritePosition(operId) ?? GetAvatarSpritePosition(DataHelper.GetCanonicalOperId(operId));
        if (pos is not { } cell)
        {
            return null;
        }

        try
        {
            var cropped = new CroppedBitmap(
                sprite,
                new Int32Rect(cell.Col * _cellSize, cell.Row * _cellSize, _cellSize, _cellSize));
            cropped.Freeze();
            return cropped;
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to crop avatar for {OperId}", operId);
            return null;
        }
    }

    private static (int Col, int Row)? GetAvatarSpritePosition(string operId)
    {
        var sprite = DataHelper.GetCharacterById(operId)?.AvatarSprite;
        return sprite is { Length: 2 } ? ((int Col, int Row)?)(sprite[0], sprite[1]) : null;
    }

    private static BitmapSource? EnsureSpriteSheet()
    {
        if (_spriteSheet != null)
        {
            return _spriteSheet;
        }

        lock (_spriteLock)
        {
            if (_spriteLoadAttempted)
            {
                return _spriteSheet;
            }

            _spriteLoadAttempted = true;

            var imagePath = Path.Combine(PathsHelper.ResourceDir, "template", "avatar", "avatar_sprite.png");
            if (!File.Exists(imagePath))
            {
                _logger.Warning("Avatar sprite sheet not found: {ImagePath}", imagePath);
                return null;
            }

            try
            {
                var sprite = new BitmapImage();
                sprite.BeginInit();
                sprite.CacheOption = BitmapCacheOption.OnLoad;
                sprite.UriSource = new Uri(imagePath, UriKind.RelativeOrAbsolute);
                sprite.EndInit();
                sprite.Freeze();

                _cellSize = sprite.PixelWidth / SpriteColumns;
                if (_cellSize <= 0 || sprite.PixelHeight % _cellSize != 0)
                {
                    _logger.Error(
                        "Invalid avatar sprite sheet size: {Width}x{Height}",
                        sprite.PixelWidth,
                        sprite.PixelHeight);
                    return null;
                }

                _spriteSheet = sprite;
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to load avatar sprite sheet from {ImagePath}", imagePath);
            }

            return _spriteSheet;
        }
    }
}
