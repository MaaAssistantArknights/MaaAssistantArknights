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
using MaaWpfGui.Configuration.Factory;
using Stylet;

namespace MaaWpfGui.Helper;

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
        get {
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

    private static string _culture = ConfigFactory.Root.Gui.Localization;

    private static string _customCulture = ConfigFactory.Root.Gui.CustomCulture;

    /// <summary>
    /// 获取当前语言。
    /// </summary>
    public static string CurrentCulture => _culture;

    public static CultureInfo CustomCultureInfo
    {
        get {
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
    /// 语言变更事件，运行时切换语言后触发，订阅者应刷新缓存的本地化文本。
    /// <para>
    /// 订阅约定：该事件为静态事件，订阅者必须是应用级单例（通过 Stylet IoC 容器管理，
    /// 如 <see cref="ViewModels.UI.ToolboxViewModel"/>、各 <c>*UserControlModel</c> 的 <c>Instance</c> 单例），
    /// 因此订阅后无需取消订阅。非单例类型（如 <see cref="ViewModels.TaskItemViewModel"/>）
    /// 必须在 <see cref="IDisposable.Dispose"/> 时通过
    /// <see cref="Utilities.PropertyDependsOnUtility.UnInitializePropertyDependencies"/> 清理跨实例依赖。
    /// </para>
    /// </summary>
    public static event Action? LanguageChanged;

    /// <summary>
    /// Loads localizations.
    /// </summary>
    public static void Load()
    {
        LoadLocalizationDictionaries(_culture);
        ApplyCultureToThread();
    }

    /// <summary>
    /// 运行时切换语言，热替换 ResourceDictionary 并通知订阅者刷新。
    /// </summary>
    /// <param name="newCulture">新语言代码，如 "en-us"。</param>
    public static void Reload(string newCulture)
    {
        Execute.OnUIThread(() => ReloadCore(newCulture));
    }

    private static void ReloadCore(string newCulture)
    {
        if (newCulture == _culture)
        {
            return;
        }

        _culture = newCulture;

        // 移除旧的本地化字典
        var app = Application.Current;
        if (app != null)
        {
            var dictList = app.Resources.MergedDictionaries;
            var toRemove = dictList.Where(IsLocalizationDictionary).ToList();
            foreach (var dict in toRemove)
            {
                dictList.Remove(dict);
            }
        }

        _preprocessedCultures.Clear();

        // 加载新语言字典
        LoadLocalizationDictionaries(newCulture);
        ApplyCultureToThread();

        // 通知订阅者刷新缓存的本地化文本
        LanguageChanged?.Invoke();
    }

    private static bool IsLocalizationDictionary(ResourceDictionary dict)
    {
        // 通过 Source 文件名判断本地化字典（路径固定为 Res\Localizations\xxx.xaml）
        var source = dict.Source?.OriginalString ?? string.Empty;
        return source.Replace('\\', '/').Contains("res/localizations/", StringComparison.OrdinalIgnoreCase);
    }

    private static void LoadLocalizationDictionaries(string culture)
    {
        if (culture == "pallas")
        {
            var dictionary = new ResourceDictionary {
                Source = new(@"Res\Localizations\zh-cn.xaml", UriKind.Relative),
            };
            foreach (var key in dictionary.Keys)
            {
                dictionary[key] = GetPallasString();
            }

            Application.Current.Resources.MergedDictionaries.Add(dictionary);
            return;
        }

        string[] cultureList = culture switch {
            "zh-cn" => [culture],
            "zh-tw" => ["zh-cn", culture],
            "en-us" => ["zh-cn", culture],
            _ => ["zh-cn", "en-us", culture],
        };

        foreach (var cur in cultureList)
        {
            var dictionary = new ResourceDictionary {
                Source = new($@"Res\Localizations\{cur}.xaml", UriKind.Relative),
            };
            _preprocessedCultures.Add(cur);
            PreprocessDictionary(dictionary, cur);
            Application.Current.Resources.MergedDictionaries.Add(dictionary);
            if (cur == culture)
            {
                break;
            }
        }
    }

    private static void ApplyCultureToThread()
    {
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

        if (TryLookupStringInternal(key, out var value, culture))
        {
            return value;
        }

        return $"{{{{ {key} }}}}";
    }

    /// <summary>
    /// Gets a localized string as-is, without unescaping <c>\n</c> or <c>\\</c>, so the result matches what XAML DynamicResource shows for the same key.
    /// Intended for strings shared between XAML tooltips and code-behind dialogs that contain literal backslashes.
    /// </summary>
    /// <param name="key">The key of the string.</param>
    /// <param name="culture">The language of the string</param>
    /// <returns>The raw string.</returns>
    public static string GetRawString(string key, string? culture = null)
    {
        if (_culture == "pallas")
        {
            return GetPallasString();
        }

        if (TryLookupStringInternal(key, out var value, culture, unescape: false))
        {
            return value;
        }

        return $"{{{{ {key} }}}}";
    }

    /// <summary>
    /// Try get a localized string. Returns false when the key is not present in resources.
    /// </summary>
    /// <param name="key">The key of the string.</param>
    /// <param name="value">The localized value when found.</param>
    /// <param name="culture">Optional culture to lookup.</param>
    /// <returns>True if a localized string is found; otherwise false.</returns>
    public static bool TryGetString(string key, out string value, string? culture = null)
    {
        if (_culture == "pallas")
        {
            value = GetPallasString();
            return true;
        }

        return TryLookupStringInternal(key, out value, culture);
    }

    private static bool TryLookupStringInternal(string key, out string value, string? culture = null, bool unescape = true)
    {
        value = string.Empty;

        if (!string.IsNullOrEmpty(culture))
        {
            try
            {
                var dictionary = new ResourceDictionary {
                    Source = new($@"Res\Localizations\{culture}.xaml", UriKind.Relative),
                };

                if (_preprocessedCultures.Add(culture))
                {
                    PreprocessDictionary(dictionary, culture);
                }

                if (dictionary.Contains(key))
                {
                    value = unescape ? Regex.Unescape(dictionary[key]?.ToString() ?? string.Empty) : dictionary[key]?.ToString() ?? string.Empty;
                    return true;
                }
            }
            catch
            {
                // ignore and fallback to merged dictionaries
            }
        }

        var dictList = Application.Current?.Resources?.MergedDictionaries;
        if (dictList != null)
        {
            for (int i = dictList.Count - 1; i >= 0; --i)
            {
                var dict = dictList[i];
                if (dict.Contains(key))
                {
                    value = unescape ? Regex.Unescape(dict[key]?.ToString() ?? string.Empty) : dict[key]?.ToString() ?? string.Empty;
                    return true;
                }
            }
        }

        return false;
    }

    /// <summary>
    /// Check whether a translation exists for the given key.
    /// </summary>
    /// <param name="key">The localization key.</param>
    /// <param name="culture">Optional culture to lookup.</param>
    /// <returns>True if translation exists; otherwise false.</returns>
    public static bool HasTranslation(string key, string? culture = null)
    {
        return TryGetString(key, out _, culture);
    }

    /// <summary>
    /// Gets a formatted localized string.
    /// </summary>
    /// <param name="key">The key of the string.</param>
    /// <param name="args">The args of string.Format</param>
    /// <returns>The string.Format result.</returns>
    public static string GetStringFormat(string key, params object?[] args)
    {
        if (_culture == "pallas")
        {
            return GetPallasString();
        }

        var rawString = GetString(key);
        return string.Format(rawString, args);
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

        // key 取到右花括号为止，不限定字符集：既有引用含 . 与 @（如 UserAdditional.Add、MiniGame@ALL@xxx），\w 无法覆盖
        var result = Regex.Replace(input, @"\{key=([^}]+)\}", match => {
            var innerKey = match.Groups[1].Value;
            var innerValue = GetString(innerKey, culture);
            return ResolveNestedKeys(innerKey, innerValue, culture, visited);
        });

        visited.Pop();
        return result;
    }

    private static readonly string[] _pallasChars = ["💃", "🕺", "🍷", "🍸", "🍺", "🍻", "🥃", "🍶"];
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
        return CustomCultureInfo.Name.ToLowerInvariant() switch {
            "zh-cn" => $"{version}{dateTime:#MMdd}",
            "zh-tw" => $"{version}{dateTime:#MMdd}",
            "en-us" => $"{dateTime:dd/MM} {version}",
            _ => $"{dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern.Replace("yyyy", string.Empty).Trim('/', '.'))} {version}",
        };
    }

    public static string FormatVersion(string? version, DateTimeOffset dateTime)
    {
        dateTime = dateTime.ToLocalTime();
        return CustomCultureInfo.Name.ToLowerInvariant() switch {
            "zh-cn" => $"{version}{dateTime:#MMdd}",
            "zh-tw" => $"{version}{dateTime:#MMdd}",
            "en-us" => $"{dateTime:dd/MM} {version}",
            _ => $"{dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern.Replace("yyyy", string.Empty).Trim('/', '.'))} {version}",
        };
    }

    public static string FormatDateTime(DateTime dateTime) => CustomCultureInfo.Name.ToLowerInvariant() switch {
        "en-us" => dateTime.ToString("yyyy/MM/dd"),
        _ => dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern),
    };

    public static string FormatDateTime(DateTimeOffset dateTime) => CustomCultureInfo.Name.ToLowerInvariant() switch {
        "en-us" => dateTime.ToString("yyyy/MM/dd"),
        _ => dateTime.ToString(CustomCultureInfo.DateTimeFormat.ShortDatePattern),
    };
}
