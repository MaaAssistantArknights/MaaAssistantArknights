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

        public const string BilibiliVideo = "https://www.bilibili.com/video/";

        public const string GitHub = "https://github.com/MaaAssistantArknights/MaaAssistantArknights";

        public const string GitHubIssues = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues";

        public const string Telegram = "https://t.me/+Mgc2Zngr-hs3ZjU1";

        public const string PrtsPlus = "https://prts.plus";

        public const string PrtsPlusCopilotGet = "https://prts.maa.plus/copilot/get/";

        public const string PrtsPlusCopilotRating = "https://prts.maa.plus/copilot/rating";

        public const string MapPrts = "https://map.ark-nights.com/areas?coord_override=maa";

        public const string CustomInfrastGenerator = "https://yituliu.site/tools/maa";

        public const string MaaApi = "https://ota.maa.plus/MaaAssistantArknights/api/";

        public const string MaaResourceApi = "https://ota.maa.plus/MaaAssistantArknights/MaaAssistantArknights/";
        public const string AnnMirrorResourceApi = "https://maa-ota.annangela.cn/MaaAssistantArknights/MaaAssistantArknights/";
        public const string S3ResourceApi = "https://s3.maa-org.net:25240/maa-resources/MaaAssistantArknights/MaaAssistantArknights/";
        public const string R2ResourceApi = "https://maa.r2.imgg.dev/MaaAssistantArknights/MaaAssistantArknights/";

        public const string QqGroups = "https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html";

        public const string QqChannel = "https://pd.qq.com/s/4j1ju9z47";

        private static string Language => ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);

        public static string HelpUri => $"https://maa.plus/docs/{_helpUrl[Language]}";

        public static string OverseasAdaptation => $"https://maa.plus/docs/{_overseasAdaptation[Language]}";

        public static string NewIssueUri => Language switch
        {
            "zh-cn" => "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new?assignees=&labels=bug&template=cn-bug-report.yaml",
            _ => "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new?assignees=&labels=bug&template=en-bug-report.yaml",
        };

        private static readonly Dictionary<string, string> _overseasAdaptation = new Dictionary<string, string>
        {
            { "zh-cn", "开发文档/外服适配教程.html" },
            { "en-us", "en-us/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "ja-jp", "ja-jp/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "ko-kr", "ko-kr/2.5-OVERSEAS_CLIENTS_ADAPTATION.html" },
            { "zh-tw", "zh-tw/2.5-外服適配教程.html" },
            { "pallas", "KeepDrinking.html" },
        };

        private static readonly Dictionary<string, string> _helpUrl = new Dictionary<string, string>
        {
            { "zh-cn", "用户手册/常见问题.html" },
            { "en-us", "en-us/1.2-FAQ.html" },
            { "ja-jp", "ja-jp/1.2-よくある質問.html" },
            { "ko-kr", "ko-kr/1.2-FAQ.html" },
            { "zh-tw", "zh-tw/1.2-常見問題.html" },
            { "pallas", "KeepDrinking.html" },
        };

        public const string RemoteControlDocument = "https://maa.plus/docs/协议文档/远程控制协议.html";
    }
}
