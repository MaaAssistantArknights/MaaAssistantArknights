// <copyright file="ItemListHelper.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Windows.Media.Imaging;
using MaaWpfGui.Constants;
using MaaWpfGui.Models;
using Serilog;

namespace MaaWpfGui.Helper
{
    public static class ItemListHelper
    {
        public static Dictionary<string, ArkItem> ArkItems { get; }

        private static readonly ILogger _logger = Log.ForContext("SourceContext", "ItemListHelper");

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
                    filename = Path.Combine(Directory.GetCurrentDirectory(), "resource", "global", DataHelper.ClientDirectoryMapper[language], "resource", "item_index.json");
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

        /// <summary>
        /// 获取当前语言下的物品名称 / Get the name of the item in the current language
        /// </summary>
        /// <param name="itemId">物品 id / Item id</param>
        /// <returns></returns>
        public static string GetItemName(string itemId)
        {
            return ArkItems.TryGetValue(itemId, out var item)
                ? item.Name
                : itemId;
        }

        /// <summary>
        /// 获取对应物品的图标 / Get the icon of the corresponding item
        /// </summary>
        /// <param name="itemId">物品 id / Item id</param>
        /// <returns></returns>
        public static BitmapImage GetItemImage(string itemId)
        {
            var imagePath = Path.Combine(Environment.CurrentDirectory, $"resource/template/items/{itemId}.png");
            return File.Exists(imagePath)
                ? new BitmapImage(new Uri(imagePath, UriKind.RelativeOrAbsolute))
                : new BitmapImage();
        }
    }
}
