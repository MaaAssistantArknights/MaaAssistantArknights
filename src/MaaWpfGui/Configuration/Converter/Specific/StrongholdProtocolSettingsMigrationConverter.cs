// <copyright file="StrongholdProtocolSettingsMigrationConverter.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Linq;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Configuration.Single.Settings;

namespace MaaWpfGui.Configuration.Converter.Specific;

internal sealed class StrongholdProtocolSettingsMigrationConverter : JsonConverter<Root>
{
    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var root = JsonNode.Parse(ref reader);
        if (root is JsonObject document && document[nameof(Root.Configurations)] is JsonObject configurations)
        {
            foreach (var (_, configuration) in configurations)
            {
                if (configuration is JsonObject profile)
                {
                    Migrate(profile);
                }
            }
        }

        return root.Deserialize<Root>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options)
        => JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private static void Migrate(JsonObject profile)
    {
        // 每个档案共用一套策略，按任务队列顺序取首份旧配置。
        if (profile[nameof(Single.SpecificConfig.TaskQueue)] is not JsonArray tasks ||
            tasks.OfType<JsonObject>().FirstOrDefault(task =>
                task["$type"] is JsonValue type && type.TryGetValue<string>(out var name) && name == nameof(SeasonalTask)) is not { } seasonal)
        {
            return;
        }

        profile[nameof(Single.SpecificConfig.Toolbox)] ??= new JsonObject();
        if (profile[nameof(Single.SpecificConfig.Toolbox)] is not JsonObject toolbox)
        {
            return;
        }

        toolbox[nameof(Toolbox.MiniGame)] ??= new JsonObject();
        if (toolbox[nameof(Toolbox.MiniGame)] is not JsonObject miniGame ||
            miniGame[nameof(MiniGame.StrongholdProtocol)] is not null)
        {
            return;
        }

        var settings = new JsonObject { [nameof(StrongholdProtocolSettings.Strategy)] = nameof(StrongholdProtocolStrategy.Covenant) };
        foreach (var property in new[] { nameof(StrongholdProtocolSettings.CoreMajorBond), nameof(StrongholdProtocolSettings.CoreMinorBond), nameof(StrongholdProtocolSettings.UpgradePlan) })
        {
            if (seasonal[property] is { } value)
            {
                settings[property] = value.DeepClone();
            }
        }

        miniGame[nameof(MiniGame.StrongholdProtocol)] = settings;
    }

    private JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var result = new JsonSerializerOptions(options);
        result.Converters.Remove(this);
        return result;
    }
}
