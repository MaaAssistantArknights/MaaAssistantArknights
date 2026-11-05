// <copyright file="InfrastPresetImport.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

public sealed class InfrastPresetImport
{
    private static readonly Dictionary<string, int> Capacities = new() {
        ["control"] = 5, ["meeting"] = 2, ["manufacture"] = 3, ["trading"] = 3,
        ["power"] = 1, ["hire"] = 1, ["recycle"] = 2,
    };

    [JsonProperty("rooms")]
    public List<Room> Rooms { get; } = [];

    [JsonIgnore]
    public List<string> PlanNames { get; } = [];

    [JsonIgnore]
    public List<string> Skipped { get; } = [];

    [JsonIgnore]
    public int QueueCount => Rooms.Sum(room => room.Queues.Count(queue => queue.Count > 0));

    public static InfrastPresetImport Parse(string json)
    {
        var document = JObject.Parse(json);
        if (document["plans"] is not JArray { Count: 3 } plans)
        {
            throw new JsonSerializationException("Exactly three plans are required.");
        }

        var result = new InfrastPresetImport();
        var roomsById = new Dictionary<(string Facility, int Index), Room>();
        for (int shift = 0; shift < 3; ++shift)
        {
            if (plans[shift] is not JObject plan || plan["rooms"] is not JObject rooms)
            {
                throw new JsonSerializationException("Each plan must contain rooms.");
            }

            result.PlanNames.Add(plan["name"]?.ToString().Replace('\n', ' ').Replace('\r', ' ') ?? $"{shift + 1}");
            var used = new HashSet<string>(StringComparer.Ordinal);
            foreach (var facility in rooms.Properties())
            {
                if (facility.Value is not JArray entries || entries.Any(entry => entry is not JObject))
                {
                    throw new JsonSerializationException($"Invalid rooms: {facility.Name}");
                }

                if (facility.Name is "dormitory" or "processing" or "training")
                {
                    if (entries.Count > 0 && !result.Skipped.Contains(facility.Name))
                    {
                        result.Skipped.Add(facility.Name);
                    }

                    continue;
                }

                if (!Capacities.TryGetValue(facility.Name, out var capacity))
                {
                    throw new JsonSerializationException($"Unknown facility: {facility.Name}");
                }

                for (int index = 0; index < entries.Count; ++index)
                {
                    var entry = (JObject)entries[index];
                    foreach (var flag in new[] { "skip", "autofill", "use_operator_groups" })
                    {
                        if (entry[flag] is { Type: not JTokenType.Boolean })
                        {
                            throw new JsonSerializationException($"Invalid flag: {flag}");
                        }
                    }

                    if (entry.Value<bool?>("skip") == true)
                    {
                        result.Skipped.Add($"{shift + 1}: {facility.Name} {index + 1}");
                        continue;
                    }

                    if (entry["candidates"] is { } candidates && candidates is not JArray)
                    {
                        throw new JsonSerializationException("Candidates must be an array.");
                    }

                    if (entry.Value<bool?>("autofill") == true || entry.Value<bool?>("use_operator_groups") == true
                        || entry["candidates"] is JArray { Count: > 0 })
                    {
                        throw new JsonSerializationException($"A fixed operator list is required: {facility.Name} {index + 1}");
                    }

                    if (entry["operators"] is not JArray names || names.Count == 0)
                    {
                        if (entry["operators"] != null && entry["operators"] is not JArray)
                        {
                            throw new JsonSerializationException("Operators must be an array.");
                        }

                        result.Skipped.Add($"{shift + 1}: {facility.Name} {index + 1}");
                        continue;
                    }

                    if (names.Count > capacity || names.Any(name => name.Type != JTokenType.String || string.IsNullOrWhiteSpace(name.Value<string>())))
                    {
                        throw new JsonSerializationException($"Invalid operators: {facility.Name} {index + 1}");
                    }

                    var operators = names.Select(name => name.Value<string>()!.Trim()).ToList();
                    if (operators.Any(name => !used.Add(name)))
                    {
                        throw new JsonSerializationException($"An operator appears twice in shift {shift + 1}.");
                    }

                    if (!roomsById.TryGetValue((facility.Name, index), out var room))
                    {
                        room = new Room { Facility = facility.Name, Index = index };
                        roomsById.Add((facility.Name, index), room);
                        result.Rooms.Add(room);
                    }

                    room.Queues[shift] = operators;
                }
            }
        }

        if (result.QueueCount == 0)
        {
            throw new JsonSerializationException("No preset operators to import.");
        }

        return result;
    }

    public sealed class Room
    {
        [JsonProperty("facility")]
        public string Facility { get; init; } = string.Empty;

        [JsonProperty("index")]
        public int Index { get; init; }

        [JsonProperty("queues")]
        public List<List<string>> Queues { get; } = [[], [], []];
    }
}
