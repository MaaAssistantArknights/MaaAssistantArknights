// <copyright file="ItemListHelper.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Text.Json;
using MaaWpfGui.Constants;
using MaaWpfGui.Models;
using Serilog;

namespace MaaWpfGui.Helper
{
    public static class ItemListHelper
    {
        public static Dictionary<string, ArkItem> ArkItems { get; }

        private static readonly ILogger _logger = Log.ForContext("SourceContext", "ItemListHelper");

        private static readonly Dictionary<string, string> _clientDirectoryMapper = new Dictionary<string, string>
        {
            { "zh-tw", "txwy" },
            { "en-us", "YoStarEN" },
            { "ja-jp", "YoStarJP" },
            { "ko-kr", "YoStarKR" },
        };

        static ItemListHelper()
        {
            var language = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);
            string filename = string.Empty;
            switch (language)
            {
                case "zh-cn":
                    filename = Path.Combine(Directory.GetCurrentDirectory(), "resource", "item_index.json");
                    break;

                case "pallas":
                    break;

                default:
                    filename = Path.Combine(Directory.GetCurrentDirectory(), "resource", "global", _clientDirectoryMapper[language], "resource", "item_index.json");
                    break;
            }

            var tempItems = new Dictionary<string, ArkItem>();

            if (File.Exists(filename) is false)
            {
                ArkItems = tempItems;
                _logger.Warning("Item list file not found: {Filename}", filename);
                return;
            }

            try
            {
                _logger.Information("Loading item list from {filename}", filename);
                var jsonStr = File.ReadAllText(filename);
                tempItems = JsonSerializer.Deserialize<Dictionary<string, ArkItem>>(jsonStr);
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to load item list from {filename}", filename);
            }

            ArkItems = tempItems ?? new Dictionary<string, ArkItem>();
        }

        public static string GetItemName(string itemId)
        {
            return ArkItems.ContainsKey(itemId)
                ? ArkItems[itemId].Name
                : itemId;
        }
    }
}
