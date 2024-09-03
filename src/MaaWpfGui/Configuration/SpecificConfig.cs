// <copyright file="SpecificConfig.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using ObservableCollections;
using static MaaWpfGui.Models.MaaTask;

namespace MaaWpfGui.Configuration
{
    public class SpecificConfig
    {
        // ReSharper disable once AutoPropertyCanBeMadeGetOnly.Global
        [JsonInclude]
        public GUI GUI { get; private set; } = new();

        [JsonInclude]
        public ObservableDictionary<string, int> InfrastOrder { get; private set; } = [];

        [JsonInclude]
        public ObservableList<BaseTask> TaskQueue { get; private set; } = [];

        [JsonInclude]
        public ObservableDictionary<string, bool> DragItemIsChecked { get; private set; } = [];

        public object ToolBox { get; private set; } = new(); // 抽卡、扫仓库、自动战斗
        public object 外部通知 { get; private set; } = new();
        public object Cache { get; private set; } = new(); // 理智、当天打OF-1
    }
}
