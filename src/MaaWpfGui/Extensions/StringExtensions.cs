// <copyright file="StringExtensions.cs" company="MaaAssistantArknights">
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

using System;
using System.Security.Cryptography;
using System.Text;

namespace MaaWpfGui.Extensions
{
    public static class StringExtensions
    {
        /// <summary>
        /// Mask the string with a visible start and end.
        /// </summary>
        /// <param name="input">The string to be masked.</param>
        /// <param name="visibleStartLength">The number of characters to remain visible at the start of the string.</param>
        /// <param name="visibleEndLength">The number of characters to remain visible at the end of the string.</param>
        /// <param name="maskChar">The character used for masking.</param>
        /// <returns>A masked string with the specified number of visible characters at the start and end.</returns>
        public static string Mask(this string input, int visibleStartLength = 3, int visibleEndLength = 3, char maskChar = '*')
        {
            if (input.Length <= visibleStartLength + visibleEndLength)
            {
                return new(maskChar, input.Length);
            }

            var maskLength = input.Length - visibleStartLength - visibleEndLength;
            var maskedPart = new string(maskChar, maskLength);
            return input[..visibleStartLength] + maskedPart + input[^visibleEndLength..];
        }

        /// <summary>
        /// Get the stable hash of the string.
        /// </summary>
        /// <param name="input">input text</param>
        /// <returns>stable hash</returns>
        public static string StableHash(this string input)
        {
            var hash = SHA256.HashData(Encoding.UTF8.GetBytes(input));
            return BitConverter.ToString(hash).Replace("-", string.Empty);
        }
    }
}
