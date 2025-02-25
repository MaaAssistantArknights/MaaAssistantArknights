// <copyright file="AsstBaseTask.cs" company="MaaAssistantArknights">
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
using System;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

[JsonObject(MemberSerialization.OptIn)]
public class AsstBaseTask
{
    public virtual AsstTaskType TaskType { get; private set; }

    public virtual (AsstTaskType TaskType, JObject Params) Serialize()
    {
        throw new NotImplementedException();
    }
}
