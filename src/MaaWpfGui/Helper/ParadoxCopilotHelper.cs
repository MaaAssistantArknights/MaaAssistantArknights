// <copyright file="ParadoxCopilotHelper.cs" company="MaaAssistantArknights">
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

#nullable enable
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper;

public static class ParadoxCopilotHelper
{
    public record Candidate(int Id, string Stage, string FileName);

    public static async Task<List<Candidate>> DownloadAsync(string directory, CancellationToken token)
    {
        var levels = await GetAsync(MaaUrls.PrtsPlusLevels, token).ConfigureAwait(false) as JArray
            ?? throw new InvalidDataException("Invalid PRTS level response");
        var stages = levels.OfType<JObject>()
            .Where(level => (string?)level["cat_one"] == "悖论模拟")
            .Select(level => (string?)level["stage_id"])
            .OfType<string>()
            .Where(stage => stage.StartsWith("mem_", StringComparison.Ordinal))
            .ToHashSet(StringComparer.Ordinal);
        var summaries = new List<JObject>();
        for (int page = 1; ; ++page)
        {
            token.ThrowIfCancellationRequested();
            var payload = await GetAsync($"{MaaUrls.PrtsPlusCopilotQuery}?page={page}&limit=100&document={Uri.EscapeDataString("悖论模拟")}&type=PRTS&orderBy=hot&desc=true", token).ConfigureAwait(false);
            if (payload["data"] is not JArray data)
            {
                throw new InvalidDataException("Invalid PRTS query response");
            }

            summaries.AddRange(data.OfType<JObject>());
            if (payload.Value<bool?>("has_next") != true)
            {
                break;
            }

            if (data.Count == 0 || page >= 1000)
            {
                throw new InvalidDataException("PRTS pagination did not terminate");
            }
        }

        var selected = SelectCandidates(summaries, stages);
        Directory.CreateDirectory(directory);
        var downloaded = new ConcurrentDictionary<int, Candidate>();
        await Parallel.ForEachAsync(selected, new ParallelOptions { MaxDegreeOfParallelism = 4, CancellationToken = token }, async (summary, cancellation) => {
            int id = summary.Value<int>("id");
            string stage = ParseContent(summary)!.Value<string>("stage_name")!;
            string path = Path.GetFullPath(Path.Combine(directory, $"{id}.json"));
            JObject? content = null;
            if (File.Exists(path) && DateTime.UtcNow - File.GetLastWriteTimeUtc(path) < TimeSpan.FromDays(1))
            {
                try
                {
                    content = JObject.Parse(await File.ReadAllTextAsync(path, cancellation).ConfigureAwait(false));
                }
                catch (JsonException)
                {
                    // Corrupt or interrupted cache files are fetched again.
                }
                catch (IOException)
                {
                }
            }

            if (!IsValidContent(content, stage))
            {
                var detail = await GetAsync(MaaUrls.PrtsPlusCopilotGet + id, cancellation).ConfigureAwait(false);
                if (detail is not JObject job || !IsPublic(job))
                {
                    return;
                }

                content = ParseContent(job);
                if (content is null || content.Value<string>("stage_name") != stage)
                {
                    return;
                }

                // Valid low-rarity jobs may have no actions (deploy nothing).
                content["actions"] ??= new JArray();
                if (!IsValidContent(content, stage))
                {
                    return;
                }

                string temporary = path + ".tmp";
                try
                {
                    await File.WriteAllTextAsync(temporary, content.ToString(Formatting.Indented), cancellation).ConfigureAwait(false);
                    File.Move(temporary, path, overwrite: true);
                }
                finally
                {
                    File.Delete(temporary);
                }
            }

            downloaded[id] = new Candidate(id, stage, path);
        }).ConfigureAwait(false);

        // Concurrent downloads must not change the preference order.
        return selected.Select(job => job.Value<int>("id"))
            .Where(downloaded.ContainsKey).Select(id => downloaded[id]).ToList();
    }

    internal static List<JObject> SelectCandidates(IEnumerable<JObject> jobs, HashSet<string> stages)
    {
        return jobs.Where(IsPublic)
            .Where(job => job["id"]?.Type == JTokenType.Integer && job.Value<long>("id") is > 0 and <= int.MaxValue)
            .Select(job => (Job: job, Stage: ParseContent(job)?.Value<string>("stage_name")))
            .Where(item => item.Stage is not null && stages.Contains(item.Stage))
            .DistinctBy(item => item.Job.Value<int>("id"))
            .GroupBy(item => item.Stage, StringComparer.Ordinal)
            .OrderBy(group => group.Key, StringComparer.Ordinal)
            .SelectMany(group => group
                .OrderBy(item => item.Job.Value<bool?>("not_enough_rating") ?? true)
                .ThenByDescending(item => item.Job.Value<int?>("rating_level") ?? 0)
                .ThenByDescending(item => item.Job.Value<double?>("rating_ratio") ?? 0)
                .ThenByDescending(item => item.Job.Value<double?>("hot_score") ?? 0)
                .ThenByDescending(item => item.Job.Value<long?>("views") ?? 0)
                .ThenBy(item => item.Job.Value<int>("id"))
                .Take(3).Select(item => item.Job))
            .ToList();
    }

    private static bool IsPublic(JObject job) => job.Value<bool?>("available") == true && job.Value<string>("status") == "PUBLIC";

    private static bool IsValidContent(JObject? content, string stage) =>
        content?.Value<string>("stage_name") == stage && content["actions"] is JArray;

    private static JObject? ParseContent(JObject job)
    {
        try
        {
            return job["content"]?.Type == JTokenType.String ? JObject.Parse(job.Value<string>("content")!) : null;
        }
        catch (JsonException)
        {
            return null;
        }
    }

    private static async Task<JToken> GetAsync(string url, CancellationToken token)
    {
        using var response = await Instances.HttpService.GetAsync(new Uri(url), token: token).ConfigureAwait(false);
        response.EnsureSuccessStatusCode();
        var body = JObject.Parse(await response.Content.ReadAsStringAsync(token).ConfigureAwait(false));
        if (body.Value<int?>("status_code") != 200 || body["data"] is not JToken data || data.Type == JTokenType.Null)
        {
            throw new InvalidDataException("PRTS Plus returned an unsuccessful response");
        }

        return data;
    }
}
