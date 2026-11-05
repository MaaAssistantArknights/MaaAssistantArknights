// <copyright file="NailongMascot.xaml.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using GifImage = HandyControl.Controls.GifImage;

namespace MaaWpfGui.Styles.Controls;

public partial class NailongMascot : UserControl
{
    public static readonly DependencyProperty ShowCaptionsProperty = DependencyProperty.Register(
        nameof(ShowCaptions), typeof(bool), typeof(NailongMascot), new PropertyMetadata(true));

    private static readonly Uri _laughUri = new("pack://application:,,,/MAA;component/Res/Skins/Assets/NaiwaLaugh.gif");
    private Window _window;

    public NailongMascot()
    {
        InitializeComponent();
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
        IsVisibleChanged += (_, _) => UpdateAnimation();
    }

    /// <summary>
    /// Gets or sets a value indicating whether the laugh and sanity captions are shown.
    /// </summary>
    public bool ShowCaptions
    {
        get => (bool)GetValue(ShowCaptionsProperty);
        set => SetValue(ShowCaptionsProperty, value);
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        _window = Window.GetWindow(this);
        if (_window != null)
        {
            _window.StateChanged += OnWindowStateChanged;
        }

        UpdateAnimation();
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        if (_window != null)
        {
            _window.StateChanged -= OnWindowStateChanged;
            _window = null;
        }

        StopAnimation();
    }

    private void OnWindowStateChanged(object sender, EventArgs e) => UpdateAnimation();

    private void UpdateAnimation()
    {
        if (!IsLoaded || !IsVisible || _window?.WindowState == WindowState.Minimized)
        {
            StopAnimation();
            return;
        }

        if (AnimationHost.Child == null)
        {
            // GifImage loads Source once on Loaded; setting Uri also loads it before attachment.
            AnimationHost.Child = new GifImage { Source = new BitmapImage(_laughUri), Stretch = Stretch.Uniform };
            StillImage.Visibility = Visibility.Hidden;
        }
    }

    private void StopAnimation()
    {
        if (AnimationHost.Child is GifImage image)
        {
            AnimationHost.Child = null;
            image.Dispose();
            StillImage.Visibility = Visibility.Visible;
        }
    }
}
