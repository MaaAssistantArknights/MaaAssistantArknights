// <copyright file="SelectableGenericCombinedData.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// The <see cref="GenericCombinedData{TValueType}"/> with an additional selectable state.
/// </summary>
/// <typeparam name="TValueType">The type of value.</typeparam>
public class SelectableGenericCombinedData<TValueType> : GenericCombinedData<TValueType>
{
    public SelectableGenericCombinedData()
    {
    }

    public SelectableGenericCombinedData(string name, TValueType value)
        : base(name, value)
    {
    }

    /// <summary>
    /// Gets or sets a value indicating whether this item is selectable.
    /// </summary>
    public bool IsEnabled { get; set => SetAndNotify(ref field, value); } = true;
}
