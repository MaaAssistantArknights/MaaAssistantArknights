// <copyright file="ClientTypeExtension.cs" company="MaaAssistantArknights">
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
using System.Text;
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Extensions;

public static class ClientTypeExtension
{
    public const string Official = "Official";
    public const string Bilibili = "Bilibili";
    public const string EN = "YoStarEN";
    public const string JP = "YoStarJP";
    public const string KR = "YoStarKR";
    public const string Txwy = "txwy";

    public static string ToCustomString(this ClientType clientType)
    {
        return clientType switch {
            ClientType.Official => Official,
            ClientType.Bilibili => Bilibili,
            ClientType.EN => EN,
            ClientType.JP => JP,
            ClientType.KR => KR,
            ClientType.Txwy => Txwy,
            _ => throw new ArgumentOutOfRangeException(nameof(clientType), clientType, null),
        };
    }

    public static string ToGameWindowName(this ClientType clientType)
    {
        return clientType switch {
            ClientType.Official or ClientType.Bilibili => "明日方舟",
            ClientType.EN => "Arknights",
            ClientType.JP => "アークナイツ",
            ClientType.KR => "명일방주",
            ClientType.Txwy => "明日方舟",
            _ => throw new ArgumentOutOfRangeException(nameof(clientType), clientType, null),
        };
    }
}
