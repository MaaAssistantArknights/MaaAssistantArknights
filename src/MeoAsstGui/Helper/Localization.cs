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
using System.Collections.Generic;

namespace MeoAsstGui
{
    public class Localization
    {
        public static readonly Dictionary<string, string> SupportedLanguages = new Dictionary<string, string> {
            { "zh-cn", "简体中文" },
            { "zh-tw", "繁體中文" },
            { "en-us", "English" },
            { "ja-jp", "日本語" },
            { "ko-kr", "한국어 (help us!)" },
            { "pallas", "呀，博士。你今天走起路来，怎么看着摇摇晃晃的？🍻" }
        };

        public static string DefaultLanguage
        {
            get
            {
                var local = CultureInfo.CurrentCulture.Name.ToLower();
                if (SupportedLanguages.ContainsKey(local))
                {
                    return local;
                }
                foreach (var lang in SupportedLanguages)
                {
                    var key = lang.Key.Contains("-") ? lang.Key.Split('-')[0] : lang.Key;
                    if (local.StartsWith(key) || key.StartsWith(local))
                    {
                        return lang.Key;
                    }
                }
                return "en-us";
            }
        }

        private static readonly string culture = ViewStatusStorage.Get("GUI.Localization", DefaultLanguage);

        public static void Load()
        {
            //var cultureInfo = new CultureInfo(culture);
            //Thread.CurrentThread.CurrentUICulture = cultureInfo;
            //Thread.CurrentThread.CurrentCulture = cultureInfo;

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
