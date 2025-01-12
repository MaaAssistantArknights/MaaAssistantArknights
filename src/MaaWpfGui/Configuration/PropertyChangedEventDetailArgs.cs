// <copyright file="PropertyChangedEventDetailArgs.cs" company="MaaAssistantArknights">
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
#nullable enable
using System.ComponentModel;

namespace MaaWpfGui.Configuration
{
    // https://github.com/dotnet/runtime/issues/27252
    public class PropertyChangedEventDetailArgs : PropertyChangedEventArgs
    {
        public PropertyChangedEventDetailArgs(string propertyName, object oldValue, object newValue)
            : base(propertyName)
        {
            OldValue = oldValue;
            NewValue = newValue;
        }

        // ReSharper disable once UnusedAutoPropertyAccessor.Global
        public object OldValue { get; private set; }

        public object NewValue { get; private set; }
    }
}
