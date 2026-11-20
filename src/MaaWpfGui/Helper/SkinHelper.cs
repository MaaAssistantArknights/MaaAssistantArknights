// <copyright file="SkinHelper.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Windows;
using HandyControl.Themes;
using MaaWpfGui.Constants.Enums;
using Stylet;

namespace MaaWpfGui.Helper;

/// <summary>
/// 管理外观皮肤。优先级为基础主题、默认皮肤资源、莫奈取色、当前皮肤；不修改用户配置。
/// </summary>
public static class SkinHelper
{
    private static readonly ResourceDictionary _resources = new();
    private static ResourceDictionary _skinResources;
    private static bool _initialized;

    /// <summary>
    /// 当前皮肤。默认启用奶龙皮肤，明暗模式仍遵循现有设置。
    /// </summary>
    public static SkinType CurrentSkin { get; private set; } = SkinType.Nailong;

    // 与皮肤分层保存，让切回默认皮肤时立即恢复当前莫奈取色，而非旧颜色快照。
    internal static ResourceDictionary ThemeOverrides { get; } = new();

    /// <summary>
    /// 应用指定皮肤。可从任意线程调用，在 UI 线程同步完成；暂不提供设置界面或持久化选项。
    /// </summary>
    /// <param name="skin">要应用的皮肤；Default 恢复原有外观与莫奈取色。</param>
    public static void Apply(SkinType skin)
    {
        if (!Enum.IsDefined(skin))
        {
            throw new ArgumentOutOfRangeException(nameof(skin), skin, "Unknown GUI skin.");
        }

        Execute.OnUIThreadSync(() => {
            EnsureInitialized();
            var source = skin == SkinType.Default
                ? null
                : new Uri($"/MAA;component/Res/Skins/Nailong.{(ThemeManager.Current.ActualApplicationTheme == ApplicationTheme.Dark ? "Dark" : "Light")}.xaml", UriKind.Relative);

            if (_skinResources?.Source != source)
            {
                // 先完整加载再替换，避免加载失败留下半套皮肤。
                var next = source == null ? null : new ResourceDictionary { Source = source };
                if (_skinResources != null)
                {
                    _resources.MergedDictionaries.Remove(_skinResources);
                }

                if (next != null)
                {
                    _resources.MergedDictionaries.Add(next);
                }

                _skinResources = next;
            }

            CurrentSkin = skin;
        });
    }

    private static void EnsureInitialized()
    {
        if (_initialized)
        {
            return;
        }

        _resources.MergedDictionaries.Add(new ResourceDictionary {
            Source = new Uri("/MAA;component/Res/Skins/Default.xaml", UriKind.Relative),
        });
        _resources.MergedDictionaries.Add(ThemeOverrides);
        Application.Current.Resources.MergedDictionaries.Add(_resources);
        _initialized = true;

        // HandyControl 直接写 app 级主色，必须移入中间层，否则它会盖过皮肤。
        CaptureAccentResources();
        DependencyPropertyDescriptor.FromProperty(ThemeManager.AccentColorProperty, typeof(ThemeManager))
            .AddValueChanged(ThemeManager.Current, (_, _) => Execute.OnUIThreadSync(CaptureAccentResources));
        ThemeManager.Current.ActualApplicationThemeChanged += (_, _) => Apply(CurrentSkin);
    }

    private static void CaptureAccentResources()
    {
        var resources = Application.Current.Resources;
        foreach (var key in new[] { "PrimaryBrush", "DarkPrimaryBrush", "TitleBrush" })
        {
            // Keys 只包含直接项；Contains 会搜索合并字典，不能用于判断是否有顶层覆盖。
            if (resources.Keys.Cast<object>().Contains(key))
            {
                ThemeOverrides[key] = resources[key];
                resources.Remove(key);
            }
        }
    }
}
