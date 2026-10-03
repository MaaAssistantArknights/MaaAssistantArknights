// <copyright file="ParadoxCopilotTests.cs" company="MaaAssistantArknights">
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

using System.Net;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Xunit;

namespace MaaWpfGui.Tests;

public sealed class ParadoxCopilotTests : IDisposable
{
    private const string Stage = "mem_beagle_1";
    private readonly string _directory = Path.Combine(Path.GetTempPath(), "maa-paradox-tests-" + Guid.NewGuid());

    private static JObject Job(int id, string stage = Stage, int rating = 1, bool unrated = false) => new() {
        ["id"] = id, ["available"] = true, ["status"] = "PUBLIC",
        ["content"] = new JObject { ["stage_name"] = stage }.ToString(),
        ["not_enough_rating"] = unrated, ["rating_level"] = rating,
    };

    private static HttpResponseMessage Response(JToken payload) => new(HttpStatusCode.OK) {
        Content = new StringContent(new JObject { ["status_code"] = 200, ["data"] = payload }.ToString()),
    };

    private static HttpResponseMessage Levels() => Response(new JArray(
        new JObject { ["cat_one"] = "悖论模拟", ["stage_id"] = Stage },
        new JObject { ["cat_one"] = "主线", ["stage_id"] = "mem_fake_1" }));

    private static HttpResponseMessage Query(params JObject[] jobs) => Response(new JObject {
        ["data"] = new JArray(jobs), ["has_next"] = false,
    });

    [Fact]
    public void RankingFiltersAndLimitsEachStage()
    {
        var invalid = Job(99);
        invalid["content"] = "not json";
        var privateJob = Job(98);
        privateJob["status"] = "PRIVATE";
        var unavailable = Job(97);
        unavailable["available"] = false;
        var actual = ParadoxCopilotHelper.SelectCandidates(
            [Job(1, rating: 99, unrated: true), Job(2, rating: 2), Job(3, rating: 3),
             Job(4, rating: 4), Job(4, rating: 4), Job(5, "mem_fake_1"), invalid, privateJob, unavailable],
            [Stage]);
        Assert.Equal([4, 3, 2], actual.Select(job => job.Value<int>("id")));
    }

    [Fact]
    public void RankingUsesRatioPopularityAndViewsInOrder()
    {
        var jobs = Enumerable.Range(1, 5).Select(id => Job(id)).ToArray();
        jobs[0]["rating_ratio"] = 0.9;
        jobs[1]["rating_ratio"] = 0.8;
        jobs[2]["hot_score"] = 10;
        jobs[3]["views"] = 1000;
        jobs[4]["hot_score"] = 10;
        jobs[4]["views"] = 1;
        Assert.Equal([1, 2, 5], ParadoxCopilotHelper.SelectCandidates(jobs, [Stage]).Select(job => job.Value<int>("id")));
    }

    [Fact]
    public async Task DownloadsDetailsAndPreservesRankingDespiteCompletionOrder()
    {
        Instances.HttpService.Handler = async (uri, token) => {
            if (uri.AbsolutePath == "/arknights/level") { return Levels(); }
            if (uri.AbsolutePath == "/copilot/query") { return Query(Job(1, rating: 3), Job(2, rating: 2)); }
            int id = int.Parse(uri.Segments.Last());
            await Task.Delay(id == 1 ? 60 : 1, token);
            var detail = Job(id);
            detail["content"] = new JObject {
                ["stage_name"] = Stage, ["actions"] = new JArray(new JObject { ["type"] = "SpeedUp" }),
            }.ToString();
            return Response(detail);
        };
        var jobs = await ParadoxCopilotHelper.DownloadAsync(_directory, default);
        Assert.Equal([1, 2], jobs.Select(job => job.Id));
        foreach (var job in jobs)
        {
            var content = JObject.Parse(await File.ReadAllTextAsync(job.FileName));
            Assert.Equal("SpeedUp", content["actions"]![0]!["type"]);
        }
    }

    [Fact]
    public async Task FollowsPaginationAndSkipsMismatchedOrUnavailableDetails()
    {
        int pages = 0;
        Instances.HttpService.Handler = (uri, _) => {
            if (uri.AbsolutePath == "/arknights/level") { return Task.FromResult(Levels()); }
            if (uri.AbsolutePath == "/copilot/query")
            {
                ++pages;
                return Task.FromResult(Response(new JObject {
                    ["data"] = new JArray(Job(pages)), ["has_next"] = pages == 1,
                }));
            }
            var job = uri.Segments.Last() == "1" ? Job(1, "mem_other_1") : Job(2);
            if (uri.Segments.Last() == "2") { job["available"] = false; }
            return Task.FromResult(Response(job));
        };
        Assert.Empty(await ParadoxCopilotHelper.DownloadAsync(_directory, default));
        Assert.Equal(2, pages);
    }

    [Fact]
    public async Task NormalizesEmptyActionsAndReusesValidatedCache()
    {
        int downloads = 0;
        Instances.HttpService.Handler = (uri, _) => {
            if (uri.AbsolutePath == "/arknights/level") { return Task.FromResult(Levels()); }
            if (uri.AbsolutePath == "/copilot/query") { return Task.FromResult(Query(Job(1))); }
            ++downloads;
            return Task.FromResult(Response(Job(1)));
        };
        var first = Assert.Single(await ParadoxCopilotHelper.DownloadAsync(_directory, default));
        Assert.Empty((JArray)JObject.Parse(await File.ReadAllTextAsync(first.FileName))["actions"]!);
        Assert.Single(await ParadoxCopilotHelper.DownloadAsync(_directory, default));
        Assert.Equal(1, downloads);
        await File.WriteAllTextAsync(first.FileName, "corrupt");
        Assert.Single(await ParadoxCopilotHelper.DownloadAsync(_directory, default));
        Assert.Equal(2, downloads);
        File.SetLastWriteTimeUtc(first.FileName, DateTime.UtcNow.AddDays(-2));
        Assert.Single(await ParadoxCopilotHelper.DownloadAsync(_directory, default));
        Assert.Equal(3, downloads);
    }

    [Fact]
    public async Task CancellationInterruptsDownloadAndDoesNotPublishCache()
    {
        using var cancellation = new CancellationTokenSource();
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        Instances.HttpService.Handler = async (uri, token) => {
            if (uri.AbsolutePath == "/arknights/level") { return Levels(); }
            if (uri.AbsolutePath == "/copilot/query") { return Query(Job(1)); }
            entered.SetResult();
            await Task.Delay(Timeout.Infinite, token);
            return Response(Job(1));
        };
        var pending = ParadoxCopilotHelper.DownloadAsync(_directory, cancellation.Token);
        await entered.Task.WaitAsync(TimeSpan.FromSeconds(5));
        cancellation.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => pending);
        Assert.Empty(Directory.GetFiles(_directory));
    }

    [Fact]
    public async Task RejectsNonTerminatingPagination()
    {
        Instances.HttpService.Handler = (uri, _) => Task.FromResult(uri.AbsolutePath == "/arknights/level"
            ? Levels() : Response(new JObject { ["data"] = new JArray(), ["has_next"] = true }));
        await Assert.ThrowsAsync<InvalidDataException>(() => ParadoxCopilotHelper.DownloadAsync(_directory, default));
    }

    public void Dispose()
    {
        if (Directory.Exists(_directory)) { Directory.Delete(_directory, recursive: true); }
    }
}
