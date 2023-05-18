// <copyright file="LocalizationHelper.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Markup;
using MaaWpfGui.Constants;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// The localization class.
    /// </summary>
    public static class LocalizationHelper
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

        private static readonly string _culture = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, DefaultLanguage);

        /// <summary>
        /// Loads localizations.
        /// </summary>
        public static void Load()
        {
            if (_culture == "pallas")
            {
                return;
            }

            string[] cultureList = _culture switch
            {
                "zh-cn" => new[] { _culture },
                "zh-tw" => new[] { "zh-cn", _culture, },
                "en-us" => new[] { "zh-cn", _culture, },
                _ => new[] { "zh-cn", "en-us", _culture, },
            };

            foreach (var cur in cultureList)
            {
                var dictionary = new ResourceDictionary
                {
                    Source = new Uri($@"Res\Localizations\{cur}.xaml", UriKind.Relative),
                };
                Application.Current.Resources.MergedDictionaries.Add(dictionary);

                if (cur == _culture)
                {
                    break;
                }
            }

            try
            {
                Thread.CurrentThread.CurrentCulture = CultureInfo.GetCultureInfo(_culture);
                FrameworkElement.LanguageProperty.OverrideMetadata(typeof(FrameworkElement), new FrameworkPropertyMetadata(XmlLanguage.GetLanguage(Thread.CurrentThread.CurrentCulture.Name)));
            }
            catch
            {
                /* ignore */
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
                    Source = new Uri($@"Res\Localizations\{culture}.xaml", UriKind.Relative),
                };
                if (dictionary.Contains(key))
                {
                    return Regex.Unescape(dictionary[key].ToString());
                }
            }

            if (_culture == "pallas")
            {
                return GetPallasString();
            }

            var dictList = Application.Current.Resources.MergedDictionaries;
            for (int i = dictList.Count - 1; i >= 0; --i)
            {
                var dict = dictList[i];
                if (dict.Contains(key))
                {
                    return Regex.Unescape(dict[key].ToString());
                }
            }

            return $"{{{{ {key} }}}}";
        }

        private static readonly string[] _PallasChars = { "💃", "🕺", "🍷", "🍸", "🍺", "🍻", "🍷", "🍸", "🍺", "🍻", };
        private static readonly Random _PallasRand = new Random();

        private static string GetPallasString()
        {
            int len = _PallasRand.Next(3, 6);
            string cheers = string.Empty;
            for (int i = 0; i < len; i++)
            {
                cheers += _PallasChars[_PallasRand.Next(0, _PallasChars.Length)];
            }

            return cheers;
        }
    }
}
