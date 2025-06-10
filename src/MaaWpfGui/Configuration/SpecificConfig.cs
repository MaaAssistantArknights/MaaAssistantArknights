// <copyright file="SpecificConfig.cs" company="MaaAssistantArknights">
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

using System.Text.Json.Serialization;
using ObservableCollections;

namespace MaaWpfGui.Configuration
{
    public class SpecificConfig
    {
        [JsonInclude]
        public ObservableDictionary<string, int> InfrastOrder { get; private set; } = new ObservableDictionary<string, int>();

        [JsonInclude]
        public ObservableDictionary<string, int> TaskQueueOrder { get; private set; } = new ObservableDictionary<string, int>();

        [JsonInclude]
        public ObservableDictionary<string, bool> DragItemIsChecked { get; private set; } = new ObservableDictionary<string, bool>();
    }
}
