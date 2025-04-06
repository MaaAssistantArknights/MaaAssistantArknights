// <copyright file="GuiSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Windows.Input;
using System.Windows.Media.Imaging;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Microsoft.Win32;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class BackgroundSettingsUserControlModel : PropertyChangedBase
{
    static BackgroundSettingsUserControlModel()
    {
        Instance = new();
    }

    public static BackgroundSettingsUserControlModel Instance { get; }

    private static string _backgroundImagePath = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundImagePath, Path.Combine(Environment.CurrentDirectory, "background\\background.png"));

    public string BackgroundImagePath
    {
        get => _backgroundImagePath;
        set
        {
            SetAndNotify(ref _backgroundImagePath, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.BackgroundImagePath, value);
            BackgroundImage = RefreshBackgroundImage(value);
        }
    }

    public void SelectImagePath()
    {
        var dialog = new OpenFileDialog
        {
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
        set
        {
            SetAndNotify(ref _backgroundImage, value);
        }
    }

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

    private int _backgroundOpacity = int.Parse(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundOpacity, "50"));

    public int BackgroundOpacity
    {
        get => _backgroundOpacity;
        set
        {
            SetAndNotify(ref _backgroundOpacity, value);
        }
    }

    private static int _backgroundBlurEffectRadius = int.Parse(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundBlurEffectRadius, "5"));

    public int BackgroundBlurEffectRadius
    {
        get => _backgroundBlurEffectRadius;
        set
        {
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
}
