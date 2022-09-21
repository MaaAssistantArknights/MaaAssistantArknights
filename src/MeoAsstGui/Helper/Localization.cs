// <copyright file="Localization.cs" company="MaaAssistantArknights">
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
// </copyright>

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.ConstrainedExecution;
using System.Windows;
using System.Windows.Input;

namespace MeoAsstGui
{
    /// <summary>
    /// The localization class.
    /// </summary>
    public class Localization
    {
        /// <summary>
        /// The supported languages.
        /// </summary>
        public static readonly Dictionary<string, string> SupportedLanguages = new Dictionary<string, string>
        {
            { "zh-cn", "简体中文" },
            { "zh-tw", "繁體中文" },
            { "en-us", "English" },
            { "ja-jp", "日本語" },
            { "ko-kr", "한국어" },
            { "pallas", "🍻🍻🍻🍻" },
        };

        /// <summary>
        /// Gets the default language.
        /// </summary>
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

        private static readonly string _culture = ViewStatusStorage.Get("GUI.Localization", DefaultLanguage);

        /// <summary>
        /// Loads localizations.
        /// </summary>
        public static void Load()
        {
            var culureList = new string[] { "zh-cn", "en-us", _culture, };
            foreach (var cur in culureList)
            {
                var dictionary = new ResourceDictionary
                {
                    Source = new Uri($@"Resources\Localizations\{cur}.xaml", UriKind.Relative),
                };
                Application.Current.Resources.MergedDictionaries.Add(dictionary);

                if (cur == _culture)
                {
                    break;
                }
            }
        }

        /// <summary>
        /// Gets a localized string.
        /// </summary>
        /// <param name="key">The key of the string.</param>
        /// <param name="culture">The language of the string</param>
        /// <returns>The string.</returns>
        public static string GetString(string key, string culture = null)
        {
            if (!string.IsNullOrEmpty(culture))
            {
                var dictionary = new ResourceDictionary
                {
                    Source = new Uri($@"Resources\Localizations\{culture}.xaml", UriKind.Relative),
                };
                if (dictionary.Contains(key))
                {
                    return dictionary[key].ToString();
                }
            }

            var dictList = Application.Current.Resources.MergedDictionaries;
            for (int i = dictList.Count - 1; i >= 0; --i)
            {
                var dict = dictList[i];
                if (dict.Contains(key))
                {
                    return dict[key].ToString();
                }
            }

            return $"{{{{ {key} }}}}";
        }
    }
}
