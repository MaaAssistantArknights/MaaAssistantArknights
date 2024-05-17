// <copyright file="Types.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MAA project
// Copyright (C) 2021 MistEO and Contributors
// </copyright>
using System;
using System.Runtime.InteropServices;

namespace MaaWpfGui.WineCompat.FontConfig;

internal enum FcResult
{
    Match,
    NoMatch,
    TypeMismatch,
    NoId,
    OutOfMemory
}

internal enum FcMatchKind
{
    Pattern,
    Font,
    Scan,
    FcMatchKindEnd,
    FcMatchKindBegin = Pattern
}

internal unsafe struct FcFontSet
{
    public int nfont;
    public int sfont;
    public FcPattern** fonts;
}

internal struct FcConfig { }
internal struct FcPattern { }
internal struct FcCharSet { }

internal enum FcBool
{
    False,
    True,
    DontCare
}
