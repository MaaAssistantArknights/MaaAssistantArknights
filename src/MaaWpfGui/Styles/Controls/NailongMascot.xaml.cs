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
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using HandyControl.Controls;
using Stylet;

namespace MaaWpfGui.Styles.Controls;

public partial class NailongMascot : UserControl
{
    private static readonly Uri _laughUri = new("pack://application:,,,/MAA;component/Res/Skins/Assets/NaiwaLaugh.gif");

    public NailongMascot()
    {
        InitializeComponent();
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
        IsVisibleChanged += (_, _) => UpdateAnimation();
        MouseEnter += (_, _) => UpdateAnimation();
        MouseLeave += (_, _) => UpdateAnimation();
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        SystemParameters.StaticPropertyChanged += OnSystemParametersChanged;
        UpdateAnimation();
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        SystemParameters.StaticPropertyChanged -= OnSystemParametersChanged;
        StopAnimation();
    }

    private void OnSystemParametersChanged(object sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(SystemParameters.ClientAreaAnimation))
        {
            Execute.OnUIThread(UpdateAnimation);
        }
    }

    private void UpdateAnimation()
    {
        if (!IsLoaded || !IsVisible || !IsMouseOver || !SystemParameters.ClientAreaAnimation)
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
