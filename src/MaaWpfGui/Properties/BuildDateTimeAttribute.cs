// <copyright file="BuildDateTimeAttribute.cs" company="MaaAssistantArknights">
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

using System;

namespace MaaWpfGui.Properties
{
    [AttributeUsage(AttributeTargets.Assembly)]
    public class BuildDateTimeAttribute(string date) : Attribute
    {
        public DateTime BuildDateTime { get; } = DateTime.ParseExact(date, "O", null);
    }
}
