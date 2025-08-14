// <copyright file="LocalizationHelper.cs" company="MaaAssistantArknights">
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
                    Source = new(@"Res\Localizations\zh-cn.xaml", UriKind.Relative),
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
                    Source = new($@"Res\Localizations\{cur}.xaml", UriKind.Relative),
                };
                _preprocessedCultures.Add(cur);
                PreprocessDictionary(dictionary, cur);
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

        private static readonly HashSet<string> _preprocessedCultures = [];

        private static void PreprocessDictionary(ResourceDictionary dictionary, string culture)
        {
            foreach (var keyObj in dictionary.Keys)
            {
                if (keyObj is not string key)
                {
                    continue;
                }

                if (dictionary[key] is not string raw || !raw.Contains("{key="))
                {
                    continue;
                }

                dictionary[key] = GetFormattedString(key, culture);
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
                    Source = new($@"Res\Localizations\{culture}.xaml", UriKind.Relative),
                };

                if (_preprocessedCultures.Add(culture))
                {
                    PreprocessDictionary(dictionary, culture);
                }

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

        /// <summary>
        /// Gets a formatted localized string.
        /// </summary>
        /// <param name="key">The key of the string.</param>
        /// <param name="culture">The language of the string</param>
        /// <returns>The formatted string.</returns>
        private static string GetFormattedString(string key, string? culture = null)
        {
            return ResolveNestedKeys(key, GetString(key, culture), culture, new());
        }

        private static string ResolveNestedKeys(string currentKey, string input, string? culture, Stack<string> visited)
        {
            if (visited.Contains(currentKey))
            {
                throw new InvalidOperationException($"Circular reference: {string.Join(" -> ", visited.Reverse())} -> {currentKey}");
            }

            visited.Push(currentKey);

            var result = Regex.Replace(input, @"\{key=(\w+)\}", match =>
            {
                var innerKey = match.Groups[1].Value;
                var innerValue = GetString(innerKey, culture);
                return ResolveNestedKeys(innerKey, innerValue, culture, visited);
            });

            visited.Pop();
            return result;
        }

        private static readonly string[] _pallasChars = ["💃", "🕺", "🍷", "🍸", "🍺", "🍻", "🍷", "🍸", "🍺", "🍻"];
        private static readonly Random _pallasRand = new();

        public static string GetPallasString(int low = 3, int high = 6)
        {
            int len = _pallasRand.Next(low, high);
            StringBuilder cheersBuilder = new StringBuilder(len);
            for (int i = 0; i < len; i++)
            {
                cheersBuilder.Append(_pallasChars[_pallasRand.Next(0, _pallasChars.Length)]);
            }

            return cheersBuilder.ToString();
        }

        public static string FormatVersion(string? version, DateTime dateTime)
        {
            dateTime = dateTime.ToLocalTime();
            return CustomCultureInfo.Name.ToLowerInvariant() switch
            {
                "zh-cn" => $"{version}{dateTime:#MMdd}",
                "zh-tw" => $"{version}{dateTime:#MMdd}",
                "en-us" => $"{dateTime:dd/MM} {version}",
                _ => $"{dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern.Replace("yyyy", string.Empty).Trim('/', '.'))} {version}",
            };
        }

        public static string FormatDateTime(DateTime dateTime)
        {
            return CustomCultureInfo.Name.ToLowerInvariant() switch
            {
                "en-us" => dateTime.ToString("yyyy/MM/dd"),
                _ => dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern),
            };
        }
    }
}
