// <copyright file="GenericCombData{TValueType}.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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

namespace MeoAsstGui
{
    /// <summary>
    /// Generic combinated data class.
    /// </summary>
    /// <typeparam name="TValueType">The type of value.</typeparam>
    public class GenericCombData<TValueType>
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
