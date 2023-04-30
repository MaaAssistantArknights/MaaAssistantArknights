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
using MaaWpfGui.Helper;

namespace MaaWpfGui.Constants
{
    public static class MaaUrls
    {
        public const string MaaPlus = "https://www.maa.plus";

        public const string Bilibili = "https://space.bilibili.com/3493274731940507";

        public const string GitHub = "https://github.com/MaaAssistantArknights/MaaAssistantArknights";

        public const string GitHubIssues = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues";

        public const string Telegram = "https://t.me/+Mgc2Zngr-hs3ZjU1";

        public const string PrtsPlus = "https://prts.plus";

        public const string MapPrts = "https://map.ark-nights.com/areas?coord_override=maa";

        public const string CustomInfrastGenerator = "https://yituliu.site/riicCal/";

        public const string QqGroups = "https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html";

        public const string QQchannel = "https://pd.qq.com/s/4j1ju9z47";

        private static string language => ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);

        public static string HelpUri => $"https://maa.plus/docs/{_helpUrl[language]}";

        public static string OverseasAdaptation => $"https://maa.plus/docs/{_overseasAdaptation[language]}";

        public static string NewIssueUri => language switch
        {
            "zh-cn" => "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new?assignees=&labels=bug&template=cn-bug-report.yaml",
            _ => "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new?assignees=&labels=bug&template=en-bug-report.yaml",
        };

        private static readonly Dictionary<string, string> _overseasAdaptation = new Dictionary<string, string>
        {
            { "zh-cn", "2.5-外服适配教程.html" },
            { "en-us", "en-us/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "ja-jp", "ja-jp/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "ko-kr", "ko-kr/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "zh-tw", "zh-tw/2.5-外服適配教程.html" },
            { "pallas", "KeepDrinking" },
        };

        private static readonly Dictionary<string, string> _helpUrl = new Dictionary<string, string>
        {
            { "zh-cn", "1.2-常见问题.html" },
            { "en-us", "en-us/1.2-FAQ.html" },
            { "ja-jp", "ja-jp/1.2-よくある質問.html" },
            { "ko-kr", "ko-kr/1.2-FAQ.html" },
            { "zh-tw", "zh-tw/1.2-常見問題.html" },
            { "pallas", "KeepDrinking" },
        };
    }
}
