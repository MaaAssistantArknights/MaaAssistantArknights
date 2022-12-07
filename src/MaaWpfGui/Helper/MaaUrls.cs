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

        public static List<string> QqGroups { get; } = new List<string>
        {
            "https://jq.qq.com/?k=xRh6gqQZ",
            "https://jq.qq.com/?k=u9i2n7Sb",
            "https://jq.qq.com/?k=mKdOnhWV",
            "https://jq.qq.com/?k=h6FGp0qD",
            "https://jq.qq.com/?k=To6b6H6m",
            "https://jq.qq.com/?k=K8t6W7HZ",
            "https://jq.qq.com/?k=gGRc2Rlw",
            "https://jq.qq.com/?k=NOpUOidq",
        };

        public static string LatestQqGroup => QqGroups.Last();
    }
}
