// <copyright file="Timer.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
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

using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration
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

        [JsonInclude]
        public bool Enable { get; }

        [JsonInclude]
        public string Config { get; }

        [JsonInclude]
        public int Hour { get; }

        [JsonInclude]
        public int Minute { get; }
    }
}
