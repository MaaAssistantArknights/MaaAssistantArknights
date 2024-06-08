// <copyright file="GenericCombinedData.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Utilities.ValueType
{
    /// <summary>
    /// Generic combined data class.
    /// </summary>
    /// <typeparam name="TValueType">The type of value.</typeparam>
    public class GenericCombinedData<TValueType>
    {
        /// <summary>
        /// Gets or sets the name displayed.
        /// </summary>
        public string Display { get; set; }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public TValueType Value { get; set; }
    }
}
