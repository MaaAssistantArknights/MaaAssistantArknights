// <copyright file="PropertyDependsOnAttribute.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Utilities;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false)]
public class PropertyDependsOnAttribute : Attribute
{
    public PropertyDependsOnAttribute(string propertyName)
    {
        PropertyNames = [propertyName];
    }

    public PropertyDependsOnAttribute(string propertyName, params string[] propertyNames)
    {
        PropertyNames = [propertyName, .. propertyNames];
    }

    public string[] PropertyNames { get; init; }
}
