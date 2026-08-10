// <copyright file="BackgroundImageItem.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MaaWpfGui.Models;

/// <summary>
/// 背景图片项数据模型，用于 TreeView 显示。
/// </summary>
public class BackgroundImageItem
{
    /// <summary>
    /// 缩略图解码宽度（像素）。仅缩小解码，不保留原图像素。
    /// </summary>
    private const int ThumbnailDecodeWidth = 64;

    private ImageSource? _thumbnail;
    private bool _thumbnailLoaded;

    public string Name { get; set; } = string.Empty;

    public string? FullPath { get; set; }

    public string? RelativePath { get; set; }

    public bool IsFolder { get; set; }

    public ObservableCollection<BackgroundImageItem> Children { get; set; } = [];

    /// <summary>
    /// 图片项的小预览图；文件夹或加载失败时为 null。懒加载，首次绑定时解码。
    /// </summary>
    public ImageSource? Thumbnail
    {
        get
        {
            if (_thumbnailLoaded)
            {
                return _thumbnail;
            }

            _thumbnailLoaded = true;
            if (!IsFolder && !string.IsNullOrEmpty(FullPath))
            {
                _thumbnail = LoadThumbnail(FullPath);
            }

            return _thumbnail;
        }
    }

    private static BitmapImage? LoadThumbnail(string path)
    {
        try
        {
            if (!File.Exists(path))
            {
                return null;
            }

            using var stream = File.OpenRead(path);
            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.StreamSource = stream;
            bitmap.DecodePixelWidth = ThumbnailDecodeWidth;
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
            bitmap.EndInit();
            bitmap.Freeze();
            return bitmap;
        }
        catch (Exception)
        {
            return null;
        }
    }
}
