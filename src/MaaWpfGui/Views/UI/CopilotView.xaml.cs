// <copyright file="CopilotView.xaml.cs" company="MaaAssistantArknights">
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
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UI;
using Point = System.Windows.Point;
using Rect = System.Windows.Rect;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// Interaction logic for CopilotView.xaml
/// </summary>
public partial class CopilotView
{
    private static readonly Duration CopilotTabAnimationDuration = new(TimeSpan.FromMilliseconds(180));

    public CopilotView()
    {
        InitializeComponent();
    }

    private void CopilotTabList_Loaded(object sender, RoutedEventArgs e) => QueueCopilotTabIndicatorUpdate(false);

    private void CopilotTabList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (e.Source == sender)
        {
            QueueCopilotTabIndicatorUpdate(true);
        }
    }

    private void CopilotTabList_SizeChanged(object sender, SizeChangedEventArgs e) => QueueCopilotTabIndicatorUpdate(false);

    private void CopilotTabItem_Loaded(object sender, RoutedEventArgs e) => QueueCopilotTabIndicatorUpdate(false);

    private void CopilotTabItem_SizeChanged(object sender, SizeChangedEventArgs e) => QueueCopilotTabIndicatorUpdate(false);

    private void QueueCopilotTabIndicatorUpdate(bool animate) => Dispatcher.BeginInvoke(() => UpdateCopilotTabIndicator(animate), DispatcherPriority.Loaded);

    private void UpdateCopilotTabIndicator(bool animate)
    {
        if (!TryGetSelectedCopilotTabBounds(out var bounds))
        {
            if (!IsLoaded || CopilotTabList.SelectedItem is null)
            {
                HideCopilotTabIndicator();
            }

            return;
        }

        CopilotTabIndicator.Opacity = 1;
        TranslateTransform translateTransform = EnsureCopilotTabIndicatorTransform();
        bool shouldAnimate = animate && CopilotTabIndicator.Width > 0 && CopilotTabIndicator.Height > 0;
        if (!shouldAnimate)
        {
            StopCopilotTabIndicatorAnimations();
            SetCopilotTabIndicatorBounds(translateTransform, bounds);
            return;
        }

        AnimateCopilotTabIndicator(translateTransform, TranslateTransform.XProperty, bounds.X);
        AnimateCopilotTabIndicator(translateTransform, TranslateTransform.YProperty, bounds.Y);
        AnimateCopilotTabIndicator(CopilotTabIndicator, WidthProperty, bounds.Width);
        AnimateCopilotTabIndicator(CopilotTabIndicator, HeightProperty, bounds.Height);
    }

    private void HideCopilotTabIndicator()
    {
        StopCopilotTabIndicatorAnimations();
        CopilotTabIndicator.Opacity = 0;
    }

    private bool TryGetSelectedCopilotTabBounds(out Rect bounds)
    {
        bounds = default;
        if (!IsLoaded || CopilotTabList.SelectedItem is null)
        {
            return false;
        }

        if (CopilotTabList.ItemContainerGenerator.ContainerFromItem(CopilotTabList.SelectedItem) is not ListBoxItem item ||
            !item.IsLoaded ||
            item.ActualWidth <= 0 ||
            item.ActualHeight <= 0)
        {
            return false;
        }

        try
        {
            Point topLeft = item.TransformToAncestor(CopilotTabHost).Transform(new Point(0, 0));
            bounds = new Rect(topLeft.X, topLeft.Y, item.ActualWidth, item.ActualHeight);
            return true;
        }
        catch
        {
            return false;
        }
    }

    private TranslateTransform EnsureCopilotTabIndicatorTransform()
    {
        if (CopilotTabIndicator.RenderTransform is TranslateTransform translateTransform)
        {
            return translateTransform;
        }

        translateTransform = new TranslateTransform();
        CopilotTabIndicator.RenderTransform = translateTransform;
        return translateTransform;
    }

    private void SetCopilotTabIndicatorBounds(TranslateTransform translateTransform, Rect bounds)
    {
        translateTransform.X = bounds.X;
        translateTransform.Y = bounds.Y;
        CopilotTabIndicator.Width = bounds.Width;
        CopilotTabIndicator.Height = bounds.Height;
    }

    private void StopCopilotTabIndicatorAnimations()
    {
        if (CopilotTabIndicator.RenderTransform is TranslateTransform translateTransform)
        {
            StopCopilotTabIndicatorAnimations(translateTransform);
        }

        CopilotTabIndicator.BeginAnimation(WidthProperty, null);
        CopilotTabIndicator.BeginAnimation(HeightProperty, null);
    }

    private static void StopCopilotTabIndicatorAnimations(TranslateTransform translateTransform)
    {
        translateTransform.BeginAnimation(TranslateTransform.XProperty, null);
        translateTransform.BeginAnimation(TranslateTransform.YProperty, null);
    }

    private static void AnimateCopilotTabIndicator(TranslateTransform target, DependencyProperty property, double toValue)
    {
        target.BeginAnimation(property, CreateCopilotTabIndicatorAnimation(toValue));
    }

    private static void AnimateCopilotTabIndicator(Border target, DependencyProperty property, double toValue)
    {
        target.BeginAnimation(property, CreateCopilotTabIndicatorAnimation(toValue));
    }

    private static DoubleAnimation CreateCopilotTabIndicatorAnimation(double toValue)
    {
        return new DoubleAnimation {
            To = toValue,
            Duration = CopilotTabAnimationDuration,
            EasingFunction = new CubicEase { EasingMode = EasingMode.EaseOut },
        };
    }

    private void FileTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is CopilotViewModel viewModel && e.NewValue is CopilotFileItem fileItem && !fileItem.IsFolder)
        {
            viewModel.OnFileSelected(fileItem);
        }
    }

    private bool _lostFocus = false;

    private async void Popup_LostFocus(object sender, RoutedEventArgs e)
    {
        _lostFocus = true;
        await Task.Delay(500);
        _lostFocus = false;
    }

    private void Border_MouseUp(object sender, RoutedEventArgs e)
    {
        if (_lostFocus)
        {
            return;
        }

        if (DataContext is CopilotViewModel viewModel)
        {
            viewModel.ToggleFilePopup();
        }
    }
}
