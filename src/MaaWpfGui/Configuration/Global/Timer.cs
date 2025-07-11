// <copyright file="Timer.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;

namespace MaaWpfGui.Configuration.Global
{
    public class Timer
    {
        [JsonConstructor]
        public Timer(bool enable, string config, int hour, int minute)
        {
            Enable = enable;
            Config = config;
            Hour = hour;
            Minute = minute;
        }

        public Timer()
        {
            Enable = false;
            Config = ConfigFactory.Root.Current;
            Hour = 0;
            Minute = 0;
        }

        [JsonInclude]
        public bool Enable { get; set; }

        [JsonInclude]
        public string Config { get; set; }

        [JsonInclude]
        public int Hour { get; set; }

        [JsonInclude]
        public int Minute { get; set; }
    }
}
