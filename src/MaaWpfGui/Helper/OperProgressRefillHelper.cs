// <copyright file="OperProgressRefillHelper.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

public static class OperProgressRefillHelper
{
    private const int MinimumSamples = 200;

    private static readonly TimeSpan _requestTimeout = TimeSpan.FromSeconds(15);

    private static readonly TimeSpan _maximumCacheAge = TimeSpan.FromDays(1);

    private static readonly ILogger _logger = Log.ForContext(typeof(OperProgressRefillHelper));

    public static async Task<Dictionary<string, List<AsstOperProgressTask.RefillStage>>> LoadAsync(CancellationToken token = default)
    {
        var clientType = SettingsViewModel.GameSettings.ClientType;
        var server = clientType switch {
            ClientType.Official or ClientType.Bilibili => "CN",
            ClientType.EN => "US",
            ClientType.JP => "JP",
            ClientType.KR => "KR",
            _ => null,
        };

        token.ThrowIfCancellationRequested();
        var stageManager = Instances.StageManager;
        var now = DateTimeOffset.UtcNow;
        var gameTime = now.UtcDateTime.ToYjDateTime();
        var nextGameDay = now.Add(gameTime.Date.AddDays(1) - gameTime);
        var routes = BuildResourceRoutes(now, nextGameDay, gameTime.DayOfWeek, stageManager);

        // Fixed resource routes do not need Penguin data. Do not borrow another server's material statistics.
        if (server == null)
        {
            _logger.Information("No refill statistics are available for client {ClientType}", clientType);
            return routes;
        }

        try
        {
            var materialIds = LoadMaterialIds(clientType);
            if (materialIds.Count == 0)
            {
                return routes;
            }

            token.ThrowIfCancellationRequested();
            var stages = await FetchArrayAsync("stages", server, token);
            if (stages == null)
            {
                return routes;
            }

            var matrix = await FetchArrayAsync("result/matrix", server, token);
            if (matrix == null)
            {
                return routes;
            }

            foreach (var (itemId, candidates) in BuildRoutes(stages, matrix, materialIds, server, now, nextGameDay, gameTime.DayOfWeek, stageManager))
            {
                routes.TryAdd(itemId, candidates);
            }

            return routes;
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to prepare operator material refill routes");
            return routes;
        }
    }

    private static Dictionary<string, List<AsstOperProgressTask.RefillStage>> BuildResourceRoutes(
        DateTimeOffset now,
        DateTimeOffset nextGameDay,
        DayOfWeek dayOfWeek,
        StageManager stageManager)
    {
        Dictionary<string, List<AsstOperProgressTask.RefillStage>> routes = [];
        void AddRoute(string itemId, string stage)
        {
            if (!stageManager.IsStageInStageList(stage))
            {
                return;
            }

            var stageInfo = stageManager.GetStageInfo(stage);
            if (!stageInfo.IsStageOpen(dayOfWeek))
            {
                return;
            }

            var deadline = nextGameDay.ToUnixTimeSeconds();
            if (stageInfo.Activity is { BeingOpen: true } activity)
            {
                deadline = Math.Min(deadline, new DateTimeOffset(DateTime.SpecifyKind(activity.UtcExpireTime, DateTimeKind.Utc)).ToUnixTimeSeconds());
            }

            if (deadline > now.ToUnixTimeSeconds())
            {
                routes[itemId] = [new(stage, deadline)];
            }
        }

        // These resource rewards need no drop statistics or estimated quantity.
        AddRoute("4001", "CE-6");
        AddRoute("4006", "AP-5");
        AddRoute("2004", "LS-6");

        foreach (var stage in stageManager.GetStageList())
        {
            if (stage.Activity is not { IsResourceCollection: true } || stage.DropGroups == null)
            {
                continue;
            }

            for (var group = 0; group < stage.DropGroups.Count; ++group)
            {
                // PR-X-1 stores both tiers: group zero belongs to -1 and group one to -2.
                var code = stage.Value.StartsWith("PR-", StringComparison.Ordinal)
                    ? stage.Value[..^1] + (group + 1)
                    : stage.Value;
                foreach (var itemId in stage.DropGroups[group])
                {
                    AddRoute(itemId, code);
                }
            }
        }

        return routes;
    }

    private static HashSet<string> LoadMaterialIds(ClientType clientType)
    {
        var path = clientType is ClientType.Official or ClientType.Bilibili
            ? Path.Combine(PathsHelper.ResourceDir, "item_index.json")
            : Path.Combine(PathsHelper.ResourceDir, "global", clientType.ToCustomString(), "resource", "item_index.json");
        var items = JObject.Parse(File.ReadAllText(path));
        HashSet<string> materialIds = [];
        foreach (var item in items.Properties())
        {
            if (item.Value["formula"] is not JObject formula)
            {
                continue;
            }

            // Reuse all existing recipes, including dual-chip ingredients.
            materialIds.Add(item.Name);
            foreach (var ingredient in formula.Properties())
            {
                if (items[ingredient.Name]?["classifyType"]?.Value<string>() == "MATERIAL")
                {
                    materialIds.Add(ingredient.Name);
                }
            }
        }

        return materialIds;
    }

    private static Dictionary<string, List<AsstOperProgressTask.RefillStage>> BuildRoutes(
        JArray stages,
        JArray matrix,
        HashSet<string> materialIds,
        string server,
        DateTimeOffset now,
        DateTimeOffset nextGameDay,
        DayOfWeek dayOfWeek,
        StageManager stageManager)
    {
        Dictionary<string, (string Stage, double ApCost, long Deadline, HashSet<string> PrimaryDropItems)> availableStages = [];
        foreach (var stage in stages.OfType<JObject>())
        {
            var id = stage["stageId"]?.Value<string>();
            var code = stage["code"]?.Value<string>();
            var apCost = ReadNumber(stage["apCost"]);
            if (string.IsNullOrEmpty(id) || string.IsNullOrEmpty(code) || apCost is not > 0 ||
                stage["existence"]?[server] is not JObject existence || existence["exist"]?.Value<bool>() != true ||
                !IsCurrentInterval(existence["openTime"], existence["closeTime"], now))
            {
                continue;
            }

            var deadline = nextGameDay.ToUnixTimeSeconds();
            if (ReadNumber(existence["closeTime"]) is double closeTime && closeTime > 0)
            {
                deadline = Math.Min(deadline, (long)(closeTime / 1000));
            }

            string navigationStage;
            if (stage["stageType"]?.Value<string>() == "MAIN")
            {
                var match = Regex.Match(code, @"^[A-Z]{0,3}(\d{1,2})-\d{1,2}$", RegexOptions.CultureInvariant);
                if (!match.Success || code.StartsWith('H') ||
                    !(id.StartsWith("main_", StringComparison.Ordinal) || id.StartsWith("sub_", StringComparison.Ordinal) || id.StartsWith("tough_", StringComparison.Ordinal)))
                {
                    continue;
                }

                int chapter = int.Parse(match.Groups[1].Value, System.Globalization.CultureInfo.InvariantCulture);
                navigationStage = chapter >= 10
                    ? code + (id.StartsWith("tough_", StringComparison.Ordinal) ? "-HARD" : "-NORMAL")
                    : code;
            }
            else
            {
                // Activity codes must be in MAA's current navigation list. Unknown codes are not permanent stages.
                if (!stageManager.IsStageInStageList(code))
                {
                    continue;
                }

                var stageInfo = stageManager.GetStageInfo(code);
                if (!stageInfo.IsStageOpen(dayOfWeek) || stageInfo.Activity is not { IsResourceCollection: false })
                {
                    continue;
                }

                if (stageInfo.Activity is { BeingOpen: true } activity)
                {
                    deadline = Math.Min(deadline, new DateTimeOffset(DateTime.SpecifyKind(activity.UtcExpireTime, DateTimeKind.Utc)).ToUnixTimeSeconds());
                }

                navigationStage = code;
            }

            if (deadline > now.ToUnixTimeSeconds())
            {
                var primaryDropItems = new HashSet<string>(
                    (stage["dropInfos"] as JArray ?? []).OfType<JObject>()
                        .Where(drop => drop["dropType"]?.Value<string>() is "NORMAL_DROP" or "SPECIAL_DROP")
                        .Select(drop => drop["itemId"]?.Value<string>())
                        .OfType<string>(),
                    StringComparer.Ordinal);
                availableStages[id] = (navigationStage, apCost.Value, deadline, primaryDropItems);
            }
        }

        Dictionary<string, List<(AsstOperProgressTask.RefillStage Route, double Cost)>> ranked = [];
        foreach (var row in matrix.OfType<JObject>())
        {
            var itemId = row["itemId"]?.Value<string>();
            var stageId = row["stageId"]?.Value<string>();
            var times = ReadNumber(row["times"]);
            var quantity = ReadNumber(row["quantity"]);
            if (itemId == null || !materialIds.Contains(itemId) || stageId == null ||
                !availableStages.TryGetValue(stageId, out var stage) || times is null or < MinimumSamples ||
                quantity is not > 0 || !IsCurrentInterval(row["start"], row["end"], now) ||
                !stage.PrimaryDropItems.Contains(itemId))
            {
                continue;
            }

            if (!ranked.TryGetValue(itemId, out var routes))
            {
                routes = [];
                ranked.Add(itemId, routes);
            }

            routes.Add((new(stage.Stage, stage.Deadline), stage.ApCost * times.Value / quantity.Value));
        }

        return ranked.ToDictionary(
            item => item.Key,
            item => item.Value.OrderBy(route => route.Cost)
                .ThenBy(route => route.Route.Stage, StringComparer.Ordinal)
                .SelectMany(route => ExpandDifficultyVariants(route.Route))
                .DistinctBy(route => route.Stage).ToList());
    }

    private static IEnumerable<AsstOperProgressTask.RefillStage> ExpandDifficultyVariants(AsstOperProgressTask.RefillStage route)
    {
        yield return route;

        // Chapter 15+ shares drop statistics between normal and six-star modes, but replay availability is separate.
        var match = Regex.Match(route.Stage, @"^[A-Z]{0,3}(\d{1,2})-\d{1,2}-NORMAL$", RegexOptions.CultureInvariant);
        if (match.Success && int.Parse(match.Groups[1].Value, System.Globalization.CultureInfo.InvariantCulture) >= 15)
        {
            yield return new(route.Stage[..^"-NORMAL".Length] + "-HARD", route.ValidUntilUtc);
        }
    }

    private static bool IsCurrentInterval(JToken? start, JToken? end, DateTimeOffset now)
    {
        var timestamp = now.ToUnixTimeMilliseconds();
        var startValue = ReadNumber(start);
        var endValue = ReadNumber(end);
        if ((start?.Type is not (null or JTokenType.Null) && !startValue.HasValue) ||
            (end?.Type is not (null or JTokenType.Null) && !endValue.HasValue))
        {
            return false;
        }

        return (startValue is not > 0 || startValue <= timestamp) &&
            (endValue is not > 0 || endValue > timestamp);
    }

    private static double? ReadNumber(JToken? token)
    {
        if (token?.Type is not (JTokenType.Integer or JTokenType.Float))
        {
            return null;
        }

        var value = token.Value<double>();
        return double.IsFinite(value) ? value : null;
    }

    private static async Task<JArray?> FetchArrayAsync(string endpoint, string server, CancellationToken token)
    {
        var cachePath = Path.Combine(PathsHelper.CacheDir, "penguin", endpoint.Replace('/', '_') + "_" + server + ".json");
        JArray? ParseArray(string body)
        {
            var json = JToken.Parse(body);
            return endpoint == "stages" ? json as JArray : json["matrix"] as JArray;
        }

        JArray? cached = null;
        if (File.Exists(cachePath))
        {
            try
            {
                cached = ParseArray(await File.ReadAllTextAsync(cachePath, token).ConfigureAwait(false));
            }
            catch (Exception ex) when (ex is IOException or JsonException)
            {
                _logger.Warning(ex, "Failed to read Penguin Statistics cache {Path}", cachePath);
            }
        }

        foreach (var domain in new[] { MaaUrls.PenguinIoDomain }.Concat(MaaUrls.PenguinBackupDomains))
        {
            token.ThrowIfCancellationRequested();
            var url = domain + "/PenguinStats/api/v2/" + endpoint + "?server=" + server;
            using var requestCancellation = CancellationTokenSource.CreateLinkedTokenSource(token);
            requestCancellation.CancelAfter(_requestTimeout);
            try
            {
                using var response = await ETagCache.FetchResponseWithEtag(url, cached == null, requestCancellation.Token).ConfigureAwait(false);
                token.ThrowIfCancellationRequested();
                if (response?.StatusCode == HttpStatusCode.NotModified && cached != null)
                {
                    File.SetLastWriteTimeUtc(cachePath, DateTime.UtcNow);
                    return cached;
                }

                if (response?.StatusCode != HttpStatusCode.OK)
                {
                    continue;
                }

                var body = await response.Content.ReadAsStringAsync(requestCancellation.Token).ConfigureAwait(false);
                var result = ParseArray(body);
                if (result == null)
                {
                    continue;
                }

                Directory.CreateDirectory(Path.GetDirectoryName(cachePath)!);
                await File.WriteAllTextAsync(cachePath, body).ConfigureAwait(false);
                ETagCache.Set(response, url);
                return result;
            }
            catch (OperationCanceledException) when (!token.IsCancellationRequested)
            {
                _logger.Warning("Timed out while fetching Penguin Statistics response from {Url}", url);
            }
            catch (Exception ex) when (ex is IOException or JsonException or HttpRequestException)
            {
                _logger.Warning(ex, "Failed to read or cache Penguin Statistics response from {Url}", url);
            }
        }

        token.ThrowIfCancellationRequested();
        return cached != null && DateTime.UtcNow - File.GetLastWriteTimeUtc(cachePath) <= _maximumCacheAge ? cached : null;
    }
}
