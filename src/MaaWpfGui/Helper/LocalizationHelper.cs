// <copyright file="LocalizationHelper.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
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
        public static readonly Dictionary<string, string> SupportedLanguages = new()
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

                foreach (var lang in from lang in SupportedLanguages
                                     let key = lang.Key.Contains('-') ? lang.Key.Split('-')[0] : lang.Key
                                     where local.StartsWith(key) || key.StartsWith(local)
                                     select lang)
                {
                    return lang.Key;
                }

                return "en-us";
            }
        }

        private static readonly string _culture = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, DefaultLanguage);

        private static readonly string _customCulture = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.CustomCulture, string.Empty);

        public static CultureInfo CustomCultureInfo
        {
            get
            {
                if (string.IsNullOrEmpty(_customCulture))
                {
                    return CultureInfo.CurrentCulture;
                }

                return CultureInfo.GetCultures(CultureTypes.AllCultures)
                    .Any(c => c.Name.Equals(_customCulture, StringComparison.OrdinalIgnoreCase))
                    ? new CultureInfo(_customCulture)
                    : CultureInfo.CurrentCulture;
            }
        }

        /// <summary>
        /// Loads localizations.
        /// </summary>
        public static void Load()
        {
            if (_culture == "pallas")
            {
                var dictionary = new ResourceDictionary
                {
                    Source = new Uri(@"Res\Localizations\zh-cn.xaml", UriKind.Relative),
                };
                foreach (var key in dictionary.Keys)
                {
                    dictionary[key] = GetPallasString();
                }

                Application.Current.Resources.MergedDictionaries.Add(dictionary);
                return;
            }

            string[] cultureList = _culture switch
            {
                "zh-cn" => [_culture],
                "zh-tw" => ["zh-cn", _culture],
                "en-us" => ["zh-cn", _culture],
                _ => ["zh-cn", "en-us", _culture],
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
                Thread.CurrentThread.CurrentCulture = !string.IsNullOrEmpty(_customCulture)
                    ? CustomCultureInfo
                    : CultureInfo.GetCultureInfo(_culture);
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
        public static string GetString(string key, string? culture = null)
        {
            if (_culture == "pallas")
            {
                return GetPallasString();
            }

            if (!string.IsNullOrEmpty(culture))
            {
                var dictionary = new ResourceDictionary
                {
                    Source = new Uri($@"Res\Localizations\{culture}.xaml", UriKind.Relative),
                };
                if (dictionary.Contains(key))
                {
                    return Regex.Unescape(dictionary[key]?.ToString() ?? $"{{{{ {key} }}}}");
                }
            }

            var dictList = Application.Current.Resources.MergedDictionaries;
            for (int i = dictList.Count - 1; i >= 0; --i)
            {
                var dict = dictList[i];
                if (dict.Contains(key))
                {
                    return Regex.Unescape(dict[key]?.ToString() ?? $"{{{{ {key} }}}}");
                }
            }

            return $"{{{{ {key} }}}}";
        }

        private static readonly string[] _pallasChars = ["💃", "🕺", "🍷", "🍸", "🍺", "🍻", "🍷", "🍸", "🍺", "🍻"];
        private static readonly Random _pallasRand = new();

        private static string GetPallasString()
        {
            int len = _pallasRand.Next(3, 6);
            StringBuilder cheersBuilder = new StringBuilder(len);
            for (int i = 0; i < len; i++)
            {
                cheersBuilder.Append(_pallasChars[_pallasRand.Next(0, _pallasChars.Length)]);
            }

            return cheersBuilder.ToString();
        }
    }
}
