// <copyright file="DateTimeExtension.cs" company="MaaAssistantArknights">
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

#nullable enable
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Extensions
{
    public static class DateTimeExtension
    {
        private const int YjDayStartHour = 4;

        private static string ClientType => ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty);

        private static readonly Dictionary<string, int> _clientTypeTimezone = new()
        {
            { string.Empty, 8 },
            { "Official", 8 },
            { "Bilibili", 8 },
            { "txwy", 8 },
            { "YoStarEN", -7 },
            { "YoStarJP", 9 },
            { "YoStarKR", 9 },
        };

        public static DateTime ToYjDateTime(this DateTime dt)
        {
            return dt.AddHours(_clientTypeTimezone[ClientType] - YjDayStartHour);
        }

        public static DateTime ToYjDate(this DateTime dt)
        {
            return dt.ToYjDateTime().Date;
        }

        public static string ToFormattedString(this DateTime dt)
        {
            return dt.ToString("yyyy/MM/dd HH:mm:ss", DateTimeFormatInfo.InvariantInfo);
        }

        public static bool IsAprilFoolsDay(this DateTime dt)
        {
            return dt is { Month: 4, Day: 1 };
        }

        public static CultureInfo CustomCultureInfo => LocalizationHelper.CustomCultureInfo;

        public static string ToLocalTimeString(this DateTime dt, string? format = null)
        {
            return string.IsNullOrEmpty(format)
                ? dt.ToLocalTime().ToString($"{CustomCultureInfo.DateTimeFormat.ShortDatePattern} HH:mm:ss", CustomCultureInfo)
                : dt.ToLocalTime().ToString(format, CustomCultureInfo);
        }

        public static DateTime ToDateTime(this System.Runtime.InteropServices.ComTypes.FILETIME filetime)
        {
            return DateTime.FromFileTime(((long)filetime.dwHighDateTime << 32) | (uint)filetime.dwLowDateTime);
        }
    }
}
