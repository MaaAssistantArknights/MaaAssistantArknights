// <copyright file="Types.cs" company="MaaAssistantArknights">
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

#pragma warning disable CS0649, SA1307, SA1602, SA1649
using System;
using System.Runtime.InteropServices;

namespace MaaWpfGui.WineCompat.FontConfig;

internal enum FcResult
{
    Match,
    NoMatch,
    TypeMismatch,
    NoId,
    OutOfMemory,
}

internal enum FcMatchKind
{
    Pattern,
    Font,
    Scan,
    FcMatchKindEnd,
    FcMatchKindBegin = Pattern,
}

internal unsafe struct FcFontSet
{
    public int nfont;
    public int sfont;
    public FcPattern** fonts;
}

internal struct FcConfig
{
}

internal struct FcPattern
{
}

internal struct FcCharSet
{
}

internal enum FcBool
{
    False,
    True,
    DontCare,
}
