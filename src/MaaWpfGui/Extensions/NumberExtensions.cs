// <copyright file="NumberExtensions.cs" company="MaaAssistantArknights">
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
namespace MaaWpfGui.Extensions
{
    public static class NumberExtensions
    {
        public static string FormatNumber(this int n) => n switch
        {
            >= 1_000_000_000 => $"{n / 1_000_000_000.0:#.#}B",
            >= 1_000_000 => $"{n / 1_000_000.0:#.#}M",
            >= 1_000 => $"{n / 1_000.0:#.#}k",
            _ => $"{n}",
        };

        public static string FormatNumber(this long n) => n switch
        {
            >= 1_000_000_000 => $"{n / 1_000_000_000.0:#.#}B",
            >= 1_000_000 => $"{n / 1_000_000.0:#.#}M",
            >= 1_000 => $"{n / 1_000.0:#.#}k",
            _ => $"{n}",
        };
    }
}
