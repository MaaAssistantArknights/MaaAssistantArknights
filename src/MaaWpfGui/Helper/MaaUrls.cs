// <copyright file="MaaUrls.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Linq;

namespace MaaWpfGui
{
    public static class MaaUrls
    {
        public static string MaaPlus => "https://www.maa.plus";

        public static string GitHub => "https://github.com/MaaAssistantArknights/MaaAssistantArknights";

        public static string GitHubIssues => $"{GitHub}/issues";

        public static string Telegram => "https://t.me/+Mgc2Zngr-hs3ZjU1";

        public static string PrtsPlus => "https://prts.plus";

        public static string MapPrts => "https://map.ark-nights.com/areas?coord_override=maa";

        public static string CustomInfrastGenerator => "https://yituliu.site/riicCal";

        public static string QqGroups => "https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html";

        public static string QQchannel => "https://pd.qq.com/s/4j1ju9z47";

        private static readonly Dictionary<string, string> _helpUrl = new Dictionary<string, string>
        {
            { "zh-cn", "1.2-常见问题.md" },
            { "en-us", "en-us/1.2-FAQ.md" },
            { "ja-jp", "ja-jp/1.2-よくある質問.md" },
            { "ko-kr", "ko-kr/1.2-FAQ.md" },
            { "zh-tw", "zh-tw/1.2-常見問題.md" },
        };

        public static string HelpUri
        {
            get
            {
                var language = ViewStatusStorage.Get("GUI.Localization", Localization.DefaultLanguage);
                return $"https://maa.plus/docs/{_helpUrl[language]}";
            }
        }
    }
}
