// <copyright file="Utils.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui
{
    public static class Utils
    {
        static Utils()
        {
            var language = ViewStatusStorage.Get("GUI.Localization", Localization.DefaultLanguage);
            string filename = string.Empty;
            if (language == "zh-cn")
            {
                filename = Directory.GetCurrentDirectory() + "\\resource\\item_index.json";
            }
            else if (language == "pallas")
            {
                // DoNothing
            }
            else
            {
                Dictionary<string, string> client = new Dictionary<string, string>
                {
                    { "zh-tw", "txwy" },
                    { "en-us", "YoStarEN" },
                    { "ja-jp", "YoStarJP" },
                    { "ko-kr", "YoStarKR" },
                };
                filename = Directory.GetCurrentDirectory() + "\\resource\\global\\" + client[language] + "\\resource\\item_index.json";
            }

            if (File.Exists(filename))
            {
                try
                {
                    _itemList = (JObject)JsonConvert.DeserializeObject(File.ReadAllText(filename));
                }
                catch
                {
                    // DoNothing
                }
            }
        }

        private static string _clientType = ViewStatusStorage.Get("Start.ClientType", string.Empty);

        public static string ClientType { get => _clientType; set => _clientType = value; }

        private static readonly Dictionary<string, int> _clientTypeTimeOffsetMap = new Dictionary<string, int>
        {
            { string.Empty, 4 },
            { "Official", 4 },
            { "Bilibili", 4 },
            { "txwy", 4 },
            { "YoStarEN", 4 },
            { "YoStarJP", 5 },
            { "YoStarKR", 4 },
        };

        /// <summary>
        /// 获取yj历时间
        /// </summary>
        /// <returns>yj历时间</returns>
        public static DateTime GetYJTimeNow()
        {
            return DateTime.UtcNow.AddHours(_clientTypeTimeOffsetMap[ClientType]);
        }

        /// <summary>
        /// 获取yj历时间字符串
        /// </summary>
        /// <returns>yj历时间的字符串表示形式</returns>
        public static string GetYJTimeNowString()
        {
            return GetYJTimeNow().ToString("yyyy/MM/dd HH:mm:ss");
        }

        /// <summary>
        /// 获取yj历日期
        /// </summary>
        /// <returns>yj历日期</returns>
        public static DateTime GetYJTimeDate()
        {
            return GetYJTimeNow().Date;
        }

        /// <summary>
        /// 获取yj历日期字符串
        /// </summary>
        /// <returns>yj历日期的字符串表示形式</returns>
        public static string GetYJTimeDateString()
        {
            return GetYJTimeDate().ToString("yyyy/MM/dd HH:mm:ss");
        }

        /// <summary>
        /// 将日期转换为yj历日期
        /// </summary>
        /// <param name="dt"></param>
        /// <returns>yj历格式的时间</returns>
        public static DateTime ToYJTime(DateTime dt)
        {
            return dt.AddHours(_clientTypeTimeOffsetMap[ClientType]);
        }

        private static readonly JObject _itemList = new JObject();

        public static JObject GetItemList() => _itemList;

        public static string GetItemName(string id)
        {
            if (_itemList.ContainsKey(id))
            {
                return _itemList[id]["name"].ToString();
            }
            else
            {
                return id;
            }
        }
    }
}
