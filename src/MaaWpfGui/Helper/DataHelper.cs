// <copyright file="DataHelper.cs" company="MaaAssistantArknights">
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
#nullable enable

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using MaaWpfGui.Constants;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper
{
    public static class DataHelper
    {
        // 储存角色信息的字典
        public static Dictionary<string, CharacterInfo> Characters { get; } = new();

        public static HashSet<string> CharacterNames { get; } = new();

        static DataHelper()
        {
            const string FilePath = "resource/battle_data.json";
            if (!File.Exists(FilePath))
            {
                return;
            }

            string jsonText = File.ReadAllText(FilePath);
            var characterData = JsonConvert.DeserializeObject<Dictionary<string, CharacterInfo>>(JObject.Parse(jsonText)["chars"]?.ToString() ?? string.Empty) ?? new();

            var characterNames_Lang_Add = GetCharacterNamesAddAction(ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage));
            var characterNames_client_Add = GetCharacterNamesAddAction(ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty));
            foreach ((var key, var value) in characterData)
            {
                Characters.Add(key, value);
                if (!key.StartsWith("char_"))
                {
                    continue;
                }

                characterNames_Lang_Add.Invoke(value);
                characterNames_client_Add.Invoke(value);
            }
        }

        private static Action<CharacterInfo> GetCharacterNamesAddAction(string str)
        {
            if (str == "zh-cn" || str == "Official" || str == "Bilibili")
            {
                return v => CharacterNames.Add(v.Name ?? string.Empty);
            }

            if (str == "zh-tw" || str == "txwy")
            {
                return v => CharacterNames.Add(v.NameTw ?? string.Empty);
            }

            if (str == "en-us" || str == "YoStarEN")
            {
                return v => CharacterNames.Add(v.NameEn ?? string.Empty);
            }

            if (str == "ja-jp" || str == "YoStarJP")
            {
                return v => CharacterNames.Add(v.NameJp ?? string.Empty);
            }

            if (str == "ko-kr" || str == "YoStarKR")
            {
                return v => CharacterNames.Add(v.NameKr ?? string.Empty);
            }

            return v => CharacterNames.Add(v.Name ?? string.Empty);
        }

        public static CharacterInfo? GetCharacterByNameOrAlias(string characterName)
        {
            return Characters.Values.FirstOrDefault(
                character => new[]
                    {
                        character.Name,
                        character.NameEn,
                        character.NameJp,
                        character.NameKr,
                        character.NameTw,
                    }.Any(name => name?.Equals(characterName, StringComparison.OrdinalIgnoreCase) ?? false));
        }

        public static string? GetLocalizedCharacterName(string? characterName, string? language = null)
        {
            if (string.IsNullOrEmpty(characterName))
            {
                return null;
            }

            var characterInfo = GetCharacterByNameOrAlias(characterName);
            if (characterInfo?.Name == null)
            {
                return null;
            }

            language ??= Instances.SettingsViewModel.Language;

            return language switch
            {
                "zh-cn" => characterInfo.Name,
                "zh-tw" => characterInfo.NameTw ?? characterInfo.Name,
                "en-us" => characterInfo.NameEn ?? characterInfo.Name,
                "ja-jp" => characterInfo.NameJp ?? characterInfo.Name,
                "ko-kr" => characterInfo.NameKr ?? characterInfo.Name,
                _ => characterInfo.Name,
            };
        }

        public class CharacterInfo
        {
            [JsonProperty("name")]
            public string? Name { get; set; }

            [JsonProperty("name_en")]
            public string? NameEn { get; set; }

            [JsonProperty("name_jp")]
            public string? NameJp { get; set; }

            [JsonProperty("name_kr")]
            public string? NameKr { get; set; }

            [JsonProperty("name_tw")]
            public string? NameTw { get; set; }

            [JsonProperty("position")]
            public string? Position { get; set; }

            [JsonProperty("profession")]
            public string? Profession { get; set; }

            [JsonProperty("rangeId")]
            public List<string>? RangeId { get; set; }

            [JsonProperty("rarity")]
            public int Rarity { get; set; }
        }
    }
}
