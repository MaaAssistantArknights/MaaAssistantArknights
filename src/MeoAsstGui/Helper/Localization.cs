// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System;
using System.Windows;
using System.Threading;
using System.Globalization;

namespace MeoAsstGui
{
    public class Localization
    {
        private static string culture = ViewStatusStorage.Get("GUI.Localization", "zh-cn");

        public static void Load()
        {
            var cultureInfo = new CultureInfo(culture);
            Thread.CurrentThread.CurrentUICulture = cultureInfo;
            Thread.CurrentThread.CurrentCulture = cultureInfo;

            var dictionary = new ResourceDictionary
            {
                Source = new Uri($@"Resources\Localizations\{culture}.xaml", UriKind.Relative)
            };
            Application.Current.Resources.MergedDictionaries[0] = dictionary;
        }

        public static string GetString(string key)
        {
            var dictionary = new ResourceDictionary
            {
                Source = new Uri($@"Resources\Localizations\{culture}.xaml", UriKind.Relative)
            };
            if (dictionary.Contains(key))
            {
                return dictionary[key] as string;
            }
            else
            {
                return $"{{{{ {key} }}}}";
            }
        }
    }
}
