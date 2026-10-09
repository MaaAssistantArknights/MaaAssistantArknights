// <copyright file="PerformanceMigrationConverter.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration.Converter.Specific;

// Migrate the per-profile Performance section into the global Gui tree once, before normal
// deserialization. GPU preference and software rendering are machine-wide, so only the current
// profile's value survives; per-profile values of inactive profiles are dropped as they may
// conflict with no merge semantics. Keeping migration here also covers inactive profiles and
// backup restoration. No-op when neither legacy location exists, hence idempotent.
internal sealed class PerformanceMigrationConverter : JsonConverter<Root>
{
    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var node = JsonNode.Parse(ref reader);
        if (node is not JsonObject root)
        {
            return node?.Deserialize<Root>(WithoutThisConverter(options));
        }

        if (root["Configurations"] is JsonObject configurations)
        {
            string? currentName = null;
            if (root["Current"] is JsonValue current && current.TryGetValue<string>(out var currentNameValue))
            {
                currentName = currentNameValue;
            }

            foreach (var (name, configNode) in configurations)
            {
                if (configNode?["Gui"] is not JsonObject gui || gui["Performance"] is not JsonObject)
                {
                    continue;
                }

                if (name == currentName && root["Gui"] is JsonObject globalGui)
                {
                    globalGui["Performance"] = gui["Performance"]!.DeepClone();
                }

                gui["Performance"] = null;
            }
        }

        if (root["Gui"] is JsonObject global && global["IgnoreBadModulesAndUseSoftwareRendering"] is JsonValue softwareRendering)
        {
            var performance = global["Performance"] as JsonObject ?? [];
            performance["IgnoreBadModulesAndUseSoftwareRendering"] = softwareRendering.DeepClone();
            global["Performance"] = performance;
            global["IgnoreBadModulesAndUseSoftwareRendering"] = null;
        }

        return root.Deserialize<Root>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private static JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var copy = new JsonSerializerOptions(options);
        for (var index = copy.Converters.Count - 1; index >= 0; --index)
        {
            if (copy.Converters[index] is PerformanceMigrationConverter)
            {
                copy.Converters.RemoveAt(index);
            }
        }

        return copy;
    }
}
