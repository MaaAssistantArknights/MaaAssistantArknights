// <copyright file="TouchModeExtensions.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Extensions;

public static class TouchModeExtensions
{
    public static string ToCustomString(this TouchMode touchMode)
    {
        return touchMode switch {
            TouchMode.MiniTouch => "minitouch",
            TouchMode.MaaTouch => "maatouch",
            TouchMode.Adb => "adb",
            TouchMode.MaaFwAdb => "MaaFwAdb",
            TouchMode.MumuExtras => "MumuExtras",
            _ => throw new ArgumentOutOfRangeException(nameof(touchMode), touchMode, null),
        };
    }
}
