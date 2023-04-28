// <copyright file="ThemeHelper.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Windows;
using System.Windows.Media;
using HandyControl.Themes;
using HandyControl.Tools;
using MaaWpfGui.Constants;
using Microsoft.Win32;

namespace MaaWpfGui.Helper
{
    public static class ThemeHelper
    {
        #region Swith Theme

        private static void SystemEvents_UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e)
        {
            ThemeManager.Current.ApplicationTheme = ThemeManager.GetSystemTheme(isSystemTheme: false);
            ThemeManager.Current.AccentColor = ThemeManager.Current.GetAccentColorFromSystem();
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToLightMode()
        {
            SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
            ThemeManager.Current.UsingWindowsAppTheme = false;
            ThemeManager.Current.ApplicationTheme = ApplicationTheme.Light;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToDarkMode()
        {
            SystemEvents.UserPreferenceChanged -= SystemEvents_UserPreferenceChanged;
            ThemeManager.Current.UsingWindowsAppTheme = false;
            ThemeManager.Current.ApplicationTheme = ApplicationTheme.Dark;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
        }

        public static void SwitchToSyncWithOSMode()
        {
            ThemeManager.Current.UsingWindowsAppTheme = true;
            Application.Current.Resources["TitleBrush"] = ThemeManager.Current.AccentColor;
            SystemEvents.UserPreferenceChanged += SystemEvents_UserPreferenceChanged;
        }

        #endregion Swith Theme

        #region Check UiLogColor

        // In official version, using ResourceHelper.GetSkin
        private static readonly Color LightBackground = ((SolidColorBrush)ThemeResources.Current.GetThemeDictionary("Light")["RegionBrush"]).Color;

        private static readonly Color DarkBackground = ((SolidColorBrush)ThemeResources.Current.GetThemeDictionary("Dark")["RegionBrush"]).Color;

        public static bool SimilarToBackground(Color color)
        {
            return ColorDistance(color, LightBackground) == 0 || ColorDistance(color, DarkBackground) == 0;
        }

        /// <summary>
        /// Gets the distance between colors c1 and c2.
        /// </summary>
        /// <param name="c1">The color c1.</param>
        /// <param name="c2">The color c2.</param>
        /// <returns>The distance from 0 to 35.</returns>
        public static int ColorDistance(Color c1, Color c2)
        {
            // https://www.compuphase.com/cmetric.htm
            long rmean = (c1.R + c2.R) / 2;
            long r = c1.R - c2.R;
            long g = c1.G - c2.G;
            long b = c1.B - c2.B;
            return (int)(((((512 + rmean) * r * r) >> 8) + (4 * g * g) + (((767 - rmean) * b * b) >> 8)) >> 14);
        }

        #endregion Check UiLogColor

        #region Convert

        public const string DefaultKey = UiLogColor.Message;

        public static SolidColorBrush DefaultBrush => ResourceHelper.GetResource<SolidColorBrush>(DefaultKey);

        public static Color DefaultColor => DefaultBrush.Color;

        public static string DefaultHexString => $"#{DefaultColor.A:X2}{DefaultColor.R:X2}{DefaultColor.G:X2}{DefaultColor.B:X2}";

        public static string Color2HexString(Color color, bool keepAlpha = false)
        {
            if (keepAlpha)
            {
                return $"#{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}";
            }

            return $"#FF{color.R:X2}{color.G:X2}{color.B:X2}";
        }

        public static string Brush2HexString(SolidColorBrush brush, bool keepAlpha = false)
        {
            if (brush != null)
            {
                return Color2HexString(brush.Color, keepAlpha);
            }

            return DefaultHexString;
        }

        public static Color String2Color(string str)
        {
            if (!string.IsNullOrWhiteSpace(str))
            {
                try
                {
                    object obj = ColorConverter.ConvertFromString(str);
                    if (obj is Color color)
                    {
                        return color;
                    }
                }
                catch
                {
                    return DefaultColor;
                }
            }

            return DefaultColor;
        }

        public static SolidColorBrush String2Brush(string str)
        {
            return new SolidColorBrush(String2Color(str));
        }

        #endregion Convert
    }
}
