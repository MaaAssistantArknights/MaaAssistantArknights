// <copyright file="AwardTask.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Models.CoreTask;

namespace MaaWpfGui.Configuration.MaaTask
{
    public class AwardTask : BaseTask
    {
        /// <summary>
        /// Gets or sets a value indicating whether 领取日常奖励
        /// </summary>
        public bool Award { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether 领取邮件奖励
        /// </summary>
        public bool Mail { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 领取每日免费一抽
        /// </summary>
        public bool FreeGacha { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 领取幸运墙合成玉
        /// </summary>
        public bool Orundum { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 领挖矿合成玉
        /// </summary>
        public bool Mining { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 5周年特殊月卡
        /// </summary>
        public bool SpecialAccess { get; set; }

        public override JObject SerializeJsonTask()
        {
            return new JObject
            {
                ["award"] = Award,
                ["mail"] = Mail,
                ["recruit"] = FreeGacha,
                ["orundum"] = Orundum,
                ["mining"] = Mining,
                ["specialaccess"] = SpecialAccess,
            };
        }
    }
}
