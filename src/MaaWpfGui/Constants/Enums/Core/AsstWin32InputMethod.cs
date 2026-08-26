// <copyright file="AsstWin32InputMethod.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums.Core;

#pragma warning disable SA1602 // Enumeration items should be documented
// 遵循 AsstCaller.h 中的定义，确保与 AsstCaller.h 中的枚举值对应
public enum AsstWin32InputMethod
{
    Seize = 1,
    SendMessageWithCursorPos = 32,
    SendMessageWithWindowPos = 128,
}
#pragma warning restore SA1602 // Enumeration items should be documented
