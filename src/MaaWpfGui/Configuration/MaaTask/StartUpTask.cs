// <copyright file="StartUpTask.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Models.MaaTask;

namespace MaaWpfGui.Configuration.MaaTask
{
    public class StartUpTask : BaseTask
    {
        public string? AccountName { get; set; }

        public override JObject SerializeJsonTask()
        {
            var taskParams = new JObject()
            {
                ["client_type"] = "cn",
                ["start_game_enabled"] = false,
            };

            if (!string.IsNullOrEmpty(AccountName))
            {
                taskParams["account_name"] = AccountName;
            }

            return taskParams;
        }
    }
}
