// <copyright file="StageManager.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;
using Stylet;

namespace MaaWpfGui.Services;

/// <summary>
/// Stage manager
/// </summary>
public class StageManager
{
    private const string StageApi = "gui/StageActivityV2.json";
    private const string TasksApi = "resource/tasks.json";

    private static readonly ILogger _logger = Log.ForContext<StageManager>();

    // data
    private Dictionary<string, StageInfo> _stages = [];

    // mini game entries exposed from StageActivityV2 (richer model including Tip/TipKey)
    private List<MiniGameEntry> _miniGameEntries = InitializeDefaultMiniGameEntries();

    /// <summary>
    /// Initializes a new instance of the <see cref="StageManager"/> class.
    /// </summary>
    public StageManager()
    {
        UpdateStageLocal();
    }

    /// <summary>
    /// Gets mini game entries parsed from StageActivityV2 (richer model including Tip/TipKey).
    /// </summary>
    public IReadOnlyList<MiniGameEntry> MiniGameEntries => _miniGameEntries.AsReadOnly();

    public void UpdateStageLocal()
    {
        MergePermanentAndActivityStages(LoadLocalStages());
    }

    public async Task UpdateStageWeb()
    {
        // 清理旧的缓存文件
        string cacheAllFileDownloadComplete = PathsHelper.CacheDir + "allFileDownloadComplete.json";
        string lastUpdateTime = PathsHelper.CacheDir + "LastUpdateTime.json";
        string stageAndTasksUpdateTime = PathsHelper.CacheDir + "stageAndTasksUpdateTime.json";
        var filesToClean = new[] { cacheAllFileDownloadComplete, lastUpdateTime, stageAndTasksUpdateTime };

        foreach (var file in filesToClean)
        {
            try
            {
                if (File.Exists(file))
                {
                    File.Delete(file);
                }
            }
            catch
            {
                continue;
            }
        }

        var webStages = await LoadWebStages();
        if (webStages is null)
        {
            return;
        }

        MergePermanentAndActivityStages(webStages);

        _ = Execute.OnUIThreadAsync(() => {
            var growlInfo = new GrowlInfo {
                IsCustom = true,
                Message = LocalizationHelper.GetString("ApiUpdateSuccess"),
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            };
            Growl.Info(growlInfo);
        });
    }

    private static string GetClientType()
    {
        var clientType = SettingsViewModel.GameSettings.ClientType;

        // 官服和B服使用同样的资源
        if (clientType is "Bilibili" or "")
        {
            clientType = "Official";
        }

        return clientType;
    }

    private static JObject LoadLocalStages()
    {
        JObject activity = Instances.MaaApiService.LoadApiCache(StageApi);
        return activity;
    }

    /*
    private static async Task<bool> CheckWebUpdate()
    {
        // Check if we need to update from the web
        const string StageAndTasksUpdateTime = "stageAndTasksUpdateTime.json";
        const string AllFileDownloadCompleteFile = "allFileDownloadComplete.json";
        JObject localLastUpdatedJson = Instances.MaaApiService.LoadApiCache(StageAndTasksUpdateTime);
        JObject allFileDownloadCompleteJson = Instances.MaaApiService.LoadApiCache(AllFileDownloadCompleteFile);
        JObject webLastUpdatedJson = await Instances.MaaApiService.RequestMaaApiWithCache(StageAndTasksUpdateTime).ConfigureAwait(false);

        if (localLastUpdatedJson?["timestamp"] == null || webLastUpdatedJson?["timestamp"] == null)
        {
            return true;
        }

        long localTimestamp = localLastUpdatedJson["timestamp"]!.ToObject<long>();
        long webTimestamp = webLastUpdatedJson["timestamp"]!.ToObject<long>();
        bool allFileDownloadComplete = allFileDownloadCompleteJson?["allFileDownloadComplete"]?.ToObject<bool>() ?? false;
        return webTimestamp > localTimestamp || !allFileDownloadComplete;
    }
    */

    private static async Task<JObject?> LoadWebStages()
    {
        var clientType = GetClientType();

        var activityTask = Instances.MaaApiService.RequestMaaApiWithCache(StageApi, false);
        var tasksTask = Instances.MaaApiService.RequestMaaApiWithCache(TasksApi, false);

        await Task.WhenAll(activityTask, tasksTask);

        var activityJson = await activityTask;
        var tasksJson = await tasksTask;
        JObject? globalTasksJson = null;
        if (clientType != "Official" && tasksJson != null)
        {
            var tasksPath = "resource/global/" + clientType + '/' + TasksApi;

            // Download the client specific resources only when the Official ones are successfully downloaded so that the client specific resource version is the actual version
            // TODO: There may be an issue when the CN resource is loaded from cache (e.g. network down) while global resource is downloaded (e.g. network up again)
            // var tasksJsonClient = fromWeb ? WebService.RequestMaaApiWithCache(tasksPath) : WebService.RequestMaaApiWithCache(tasksPath);
            globalTasksJson = await Instances.MaaApiService.RequestMaaApiWithCache(tasksPath, false);
        }

        if (activityJson is null || tasksJson is null)
        {
            return null;
        }

        if (clientType != "Official" && globalTasksJson is null)
        {
            return null;
        }

        await Task.Run(() => {
            _ = Instances.AsstProxy.LoadResourceWhenIdleAsync();
        });
        return activityJson;
    }

    private void MergePermanentAndActivityStages(JObject? activity)
    {
        var tempStage = InitializeDefaultStages();

        var clientType = GetClientType();

        bool isDebugVersion = Instances.VersionUpdateDialogViewModel.IsDebugVersion();
        bool curVerParsed = TryParseVersion(VersionUpdateSettingsUserControlModel.CoreVersion, out var curVersionObj);

        // bool curResourceVerParsed = SemVersion.TryParse(
        //    tasksJsonClient?["ResourceVersion"]?.ToString() ?? tasksJson?["ResourceVersion"]?.ToString() ?? string.Empty,
        //    SemVersionStyles.AllowLowerV, out var curResourceVersionObj);
        var resourceCollection = InitializeResourceCollection(activity?[clientType]?["resourceCollection"]);

        if (activity?[clientType] != null && (isDebugVersion || (curVerParsed && curVersionObj != null)))
        {
            ParseActivityStages(activity[clientType], tempStage, curVerParsed, curVersionObj, isDebugVersion);
        }

        AddPermanentStages(tempStage, resourceCollection);

        _stages = tempStage;

        // parse mini-game tasks from activity json if provided (use helper to populate temp list)
        var tempMiniGames = InitializeDefaultMiniGameEntries();
        ParseMiniGameEntries(activity?[clientType], tempMiniGames, curVerParsed, curVersionObj, isDebugVersion);
        _miniGameEntries = tempMiniGames;
    }

    private static Dictionary<string, StageInfo> InitializeDefaultStages()
    {
        return new()
        {
            // 「当前/上次」关卡导航
            { string.Empty, new() { Display = LocalizationHelper.GetString("DefaultStage"), Value = string.Empty } },

            // 周一和周日的关卡提示
            { "Pormpt1", new() { Tip = LocalizationHelper.GetString("Pormpt1"), OpenDaysOfWeek = [DayOfWeek.Monday], IsHidden = true } },
            { "Pormpt2", new() { Tip = LocalizationHelper.GetString("Pormpt2"), OpenDaysOfWeek = [DayOfWeek.Sunday], IsHidden = true } },
        };
    }

    private static List<MiniGameEntry> InitializeDefaultMiniGameEntries()
    {
        var entries = new List<MiniGameEntry>
        {
            new() { Display = LocalizationHelper.GetString("MiniGameNameSsStore"), Value = "SS@Store@Begin", TipKey = "MiniGameNameSsStoreTip" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameGreenTicketStore"), Value = "GreenTicket@Store@Begin", TipKey = "MiniGameNameGreenTicketStoreTip" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameYellowTicketStore"), Value = "YellowTicket@Store@Begin", TipKey = "MiniGameNameYellowTicketStoreTip" },
            new() { Display = LocalizationHelper.GetString("MiniGameNameRAStore"), Value = "RA@Store@Begin", TipKey = "MiniGameNameRAStoreTip" },
            new() { Display = LocalizationHelper.GetString("MiniGame@SecretFront"), Value = "MiniGame@SecretFront", TipKey = "MiniGame@SecretFrontTip" },
        };

        return entries;
    }

    private static MiniGameEntry? ParseMiniGameEntry(JToken? token)
    {
        if (token == null)
        {
            return null;
        }

        try
        {
            if (token.Type == JTokenType.String)
            {
                var val = token.ToString();
                return new MiniGameEntry { Display = val, Value = val };
            }

            var display = token["Display"]?.ToString();
            var displayKey = token["DisplayKey"]?.ToString();
            var value = token["Value"]?.ToString() ?? token["value"]?.ToString();
            var tip = token["Tip"]?.ToString();
            var tipKey = token["TipKey"]?.ToString();
            var minimumRequired = token["MinimumRequired"]?.ToString();

            string finalDisplay = string.Empty;
            if (!string.IsNullOrEmpty(display))
            {
                finalDisplay = display;
            }
            else if (!string.IsNullOrEmpty(displayKey))
            {
                if (LocalizationHelper.TryGetString(displayKey, out var loc))
                {
                    finalDisplay = loc;
                }
                else if (!string.IsNullOrEmpty(value))
                {
                    finalDisplay = value;
                }
                else
                {
                    finalDisplay = displayKey;
                }
            }
            else if (!string.IsNullOrEmpty(value))
            {
                finalDisplay = value;
            }
            else
            {
                finalDisplay = token.ToString();
            }

            if (string.IsNullOrEmpty(value))
            {
                value = finalDisplay;
            }

            // parse optional activity times on the same level as Display (UtcStartTime/UtcExpireTime)
            DateTime utcStart = DateTime.MinValue;
            DateTime utcExpire = DateTime.MinValue;
            var startStr = token["UtcStartTime"]?.ToString();
            var expireStr = token["UtcExpireTime"]?.ToString();
            if (!string.IsNullOrEmpty(startStr))
            {
                try
                {
                    utcStart = ParseDateTime(token, "UtcStartTime");
                }
                catch
                {
                    utcStart = DateTime.MinValue;
                }
            }

            if (!string.IsNullOrEmpty(expireStr))
            {
                try
                {
                    utcExpire = ParseDateTime(token, "UtcExpireTime");
                }
                catch
                {
                    utcExpire = DateTime.MinValue;
                }
            }

            return new MiniGameEntry {
                Display = finalDisplay,
                DisplayKey = displayKey,
                Value = value,
                Tip = tip,
                TipKey = tipKey,
                MinimumRequired = minimumRequired,
                UtcStartTime = utcStart,
                UtcExpireTime = utcExpire,
            };
        }
        catch
        {
            return null;
        }
    }

    private static bool TryParseVersion(string? version, out SemVersion? versionObj)
    {
        return SemVersion.TryParse(version, SemVersionStyles.AllowLowerV, out versionObj);
    }

    private static DateTime ParseDateTime(JToken? token, string key)
    {
        return DateTime.ParseExact(token?[key]?.ToString() ?? string.Empty, "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture)
            .AddHours(-(token?["TimeZone"]?.ToObject<int>() ?? 0));
    }

    private static StageActivityInfo InitializeResourceCollection(JToken? resourceCollectionData)
    {
        if (resourceCollectionData == null)
        {
            return new() { IsResourceCollection = true };
        }

        // 资源全开放活动
        return new() {
            IsResourceCollection = true,
            Tip = resourceCollectionData["Tip"]?.ToString(),
            UtcStartTime = ParseDateTime(resourceCollectionData, "UtcStartTime"),
            UtcExpireTime = ParseDateTime(resourceCollectionData, "UtcExpireTime"),
        };
    }

    private static void ParseActivityStages(JToken? clientData, Dictionary<string, StageInfo> tempStage, bool curVerParsed, SemVersion? curVersionObj, bool isDebugVersion)
    {
        try
        {
            var sideToken = clientData?["sideStoryStage"];
            if (sideToken == null || sideToken.Type != JTokenType.Object)
            {
                return;
            }

            // 新格式：sideStoryStage 为对象，按活动分组，组内包含 Activity 与 Stages 数组
            foreach (var prop in sideToken.Children<JProperty>())
            {
                var group = prop.Value;

                JToken? activityToken = group["Activity"] ?? group["activity"];
                var groupMinReqStr = group["MinimumRequired"]?.ToString() ?? group["minimumRequired"]?.ToString();

                JToken? stagesToken = group["Stages"] ?? group["stages"];
                if (stagesToken == null || stagesToken.Type != JTokenType.Array)
                {
                    // invalid group format, skip
                    continue;
                }

                foreach (var stageObj in stagesToken)
                {
                    var minReqStr = stageObj["MinimumRequired"]?.ToString() ?? groupMinReqStr;
                    if (!TryParseVersion(minReqStr, out var minRequiredObj))
                    {
                        continue;
                    }

                    bool unsupportedStages = !isDebugVersion && curVerParsed && curVersionObj!.CompareSortOrderTo(minRequiredObj) < 0;

                    var stageInfo = CreateStageInfo(stageObj, unsupportedStages, minRequiredObj, activityToken);
                    tempStage.TryAdd(stageInfo.Display, stageInfo);
                }
            }
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to parse Cache Stage resources");
        }
    }

    private static void ParseMiniGameEntries(JToken? clientData, List<MiniGameEntry> tempMiniGames, bool curVerParsed, SemVersion? curVersionObj, bool isDebugVersion)
    {
        if (clientData == null)
        {
            return;
        }

        var miniGameToken = clientData["miniGame"];
        if (miniGameToken == null)
        {
            return;
        }

        var parsedEntries = new List<MiniGameEntry>();

        if (miniGameToken.Type == JTokenType.Array)
        {
            foreach (var item in miniGameToken)
            {
                var entry = ParseMiniGameEntry(item);
                if (entry == null)
                {
                    continue;
                }

                var minReqStr = entry.MinimumRequired;
                if (!string.IsNullOrEmpty(minReqStr))
                {
                    if (!TryParseVersion(minReqStr, out var minReqObj))
                    {
                        entry.Tip = LocalizationHelper.GetString("LowVersion") + '\n' + LocalizationHelper.GetString("MinimumRequirements") + " " + minReqStr;
                        entry.TipKey = null;
                    }
                    else
                    {
                        bool unsupported = !isDebugVersion && curVerParsed && curVersionObj!.CompareSortOrderTo(minReqObj) < 0;
                        if (unsupported)
                        {
                            entry.Tip = LocalizationHelper.GetString("LowVersion") + '\n' + LocalizationHelper.GetString("MinimumRequirements") + " " + minReqStr;
                            entry.TipKey = null;
                        }
                    }
                }

                if (entry.BeingOpen)
                {
                    parsedEntries.Add(entry);
                }
            }
        }
        else
        {
            var entry = ParseMiniGameEntry(miniGameToken);
            if (entry != null)
            {
                var minReqStr = entry.MinimumRequired;
                if (!string.IsNullOrEmpty(minReqStr))
                {
                    if (!TryParseVersion(minReqStr, out var minReqObj))
                    {
                        entry.Tip = LocalizationHelper.GetString("LowVersion") + '\n' + LocalizationHelper.GetString("MinimumRequirements") + " " + minReqStr;
                        entry.TipKey = null;
                    }
                    else
                    {
                        bool unsupported = !isDebugVersion && curVerParsed && curVersionObj!.CompareSortOrderTo(minReqObj) < 0;
                        if (unsupported)
                        {
                            entry.Tip = LocalizationHelper.GetString("LowVersion") + '\n' + LocalizationHelper.GetString("MinimumRequirements") + " " + minReqStr;
                            entry.TipKey = null;
                        }
                    }
                }

                if (entry.BeingOpen)
                {
                    parsedEntries.Add(entry);
                }
            }
        }

        if (parsedEntries.Count > 0)
        {
            tempMiniGames.InsertRange(0, parsedEntries);
        }
    }

    private static StageInfo CreateStageInfo(JToken? stageObj, bool unsupportedStages, SemVersion? minRequiredObj, JToken? activityToken)
    {
        activityToken = stageObj?["Activity"] ?? activityToken;

        var display = stageObj?["Display"]?.ToString() ?? string.Empty;

        var value = stageObj?["Value"]?.ToString() ?? string.Empty;

        var drop = unsupportedStages
            ? LocalizationHelper.GetString("LowVersion") + '\n' + LocalizationHelper.GetString("MinimumRequirements") + minRequiredObj
            : stageObj?["Drop"]?.ToString();

        DateTime utcStart = DateTime.MinValue;
        DateTime utcExpire = DateTime.MinValue;
        if (activityToken != null)
        {
            var startStr = activityToken["UtcStartTime"]?.ToString();
            var expireStr = activityToken["UtcExpireTime"]?.ToString();
            if (!string.IsNullOrEmpty(startStr))
            {
                try
                {
                    utcStart = ParseDateTime(activityToken, "UtcStartTime");
                }
                catch
                {
                    utcStart = DateTime.MinValue;
                }
            }

            if (!string.IsNullOrEmpty(expireStr))
            {
                try
                {
                    utcExpire = ParseDateTime(activityToken, "UtcExpireTime");
                }
                catch
                {
                    utcExpire = DateTime.MinValue;
                }
            }
        }

        var activity = new StageActivityInfo {
            Tip = activityToken?["Tip"]?.ToString(),
            StageName = activityToken?["StageName"]?.ToString(),
            UtcStartTime = utcStart,
            UtcExpireTime = utcExpire,
        };

        return new StageInfo(display, value, drop, activity);
    }

    private static void AddPermanentStages(Dictionary<string, StageInfo> tempStage, StageActivityInfo resourceCollection)
    {
        var permanentStages = new Dictionary<string, StageInfo>
        {
            // 主线关卡
            { "1-7", new() { Display = "1-7", Value = "1-7" } },
            { "R8-11", new() { Display = "R8-11", Value = "R8-11" } },
            { "12-17-HARD", new() { Display = "12-17-HARD", Value = "12-17-HARD" } },

            // 资源本
            { "CE-6", new("CE-6", "CETip", [DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection) },
            { "AP-5", new("AP-5", "APTip", [DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection) },
            { "CA-5", new("CA-5", "CATip", [DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday], resourceCollection) },
            { "LS-6", new("LS-6", "LSTip", [], resourceCollection) },
            { "SK-5", new("SK-5", "SKTip", [DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday], resourceCollection) },

            // 剿灭模式
            { "Annihilation", new() { Display = LocalizationHelper.GetString("AnnihilationMode"), Value = "Annihilation" } },

            // 芯片本 - dropGroups 格式：[[PR-X-1的掉落], [PR-X-2的掉落]]
            { "PR-A-1", new("PR-A-1", "PR-ATip", [DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday], resourceCollection, [["3231", "3261"], ["3232", "3262"]]) },
            { "PR-A-2", new("PR-A-2", string.Empty, [DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday], resourceCollection) },
            { "PR-B-1", new("PR-B-1", "PR-BTip", [DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday], resourceCollection, [["3251", "3241"], ["3252", "3242"]]) },
            { "PR-B-2", new("PR-B-2", string.Empty, [DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday], resourceCollection) },
            { "PR-C-1", new("PR-C-1", "PR-CTip", [DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection, [["3211", "3271"], ["3212", "3272"]]) },
            { "PR-C-2", new("PR-C-2", string.Empty, [DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection) },
            { "PR-D-1", new("PR-D-1", "PR-DTip", [DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection, [["3221", "3281"], ["3222", "3282"]]) },
            { "PR-D-2", new("PR-D-2", string.Empty, [DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday], resourceCollection) },
        };

        foreach (var kvp in permanentStages)
        {
            tempStage.TryAdd(kvp.Key, kvp.Value);
        }
    }

    /// <summary>
    /// Gets stage by name
    /// </summary>
    /// <param name="stage">Stage name</param>
    /// <returns>Stage info</returns>
    public StageInfo GetStageInfo(string stage)
    {
        if (_stages.TryGetValue(stage, out var stageInfo))
        {
            return stageInfo;
        }

        // If stage matches pattern of two letters + '-' + 1-2 digits (e.g. "UR-5", "AD-10"),
        // treat it as an expired activity stage instead of a permanent stage.
        if (!string.IsNullOrEmpty(stage) && Regex.IsMatch(stage, "^[A-Za-z]{2}-\\d{1,2}$"))
        {
            var expiredActivity = new StageActivityInfo {
                UtcStartTime = DateTime.MinValue,
                UtcExpireTime = DateTime.MinValue,
            };
            return new StageInfo(stage, stage, null, expiredActivity);
        }

        // Fallback: treat as permanent stage
        return new() { Display = stage, Value = stage };
    }

    public bool IsStageInStageList(string stage)
    {
        return _stages.ContainsKey(stage);
    }

    public void AddUnOpenStage(string stage)
    {
        var unopenActivity = new StageActivityInfo {
            UtcStartTime = DateTime.MinValue,
            UtcExpireTime = DateTime.MinValue,
        };
        _stages.Add(stage, new StageInfo(stage, stage, null, unopenActivity));
    }

    /// <summary>
    /// Determine whether stage is open
    /// </summary>
    /// <param name="stage">Stage name</param>
    /// <param name="dayOfWeek">Current day of week</param>
    /// <returns>Whether stage is open</returns>
    public bool IsStageOpen(string? stage, DayOfWeek dayOfWeek)
    {
        return stage is not null && GetStageInfo(stage).IsStageOpen(dayOfWeek);
    }

    /// <summary>
    /// Gets open stage tips at specified day of week
    /// </summary>
    /// <param name="dayOfWeek">Day of week</param>
    /// <returns>Open stages</returns>
    public string GetStageTips(DayOfWeek dayOfWeek)
    {
        var lines = new List<string>();
        var shownSideStories = new HashSet<string>();
        bool resourceTipShown = false;
        DateTime now = DateTime.UtcNow;

        foreach (var stage in _stages.Values.Where(s => s.IsStageOpen(dayOfWeek)))
        {
            var activity = stage.Activity;

            // Resource collection tip - only show one
            if (!resourceTipShown && activity is { IsResourceCollection: true, BeingOpen: true })
            {
                // 插入到第一行
                lines.Insert(0, $"｢{activity.Tip}｣ {LocalizationHelper.GetString("DaysLeftOpen")}{GetDaysLeftText(activity.UtcExpireTime, now)}");
                resourceTipShown = true;
            }

            // Side story tips
            if (!string.IsNullOrEmpty(activity?.StageName) && shownSideStories.Add(activity.StageName))
            {
                lines.Add($"｢{activity.StageName}｣ {LocalizationHelper.GetString("DaysLeftOpen")}{GetDaysLeftText(activity.UtcExpireTime, now)}");
            }

            // Side story Drop item tips
            if (!string.IsNullOrEmpty(stage.Drop))
            {
                var str = $"{stage.Value}: {ItemListHelper.GetItemName(stage.Drop) ?? stage.Drop}";
                var count = Instances.ToolboxViewModel?.DepotResult?.FirstOrDefault(d => d.Id == stage.Drop)?.DisplayCount;
                if (!string.IsNullOrEmpty(count) && count != "-1")
                {
                    str += $" ({LocalizationHelper.GetString("Inventory")} {count})";
                }

                lines.Add(str);
            }

            // Normal stage tips
            if (!string.IsNullOrEmpty(stage.Tip))
            {
                lines.Add(stage.Tip);
            }

            // Drop groups tips (for chip stages like PR-A-1/2)
            if (stage.DropGroups != null && stage.DropGroups.Count > 0)
            {
                var normalizedCountGroups = stage.DropGroups
                    .Select(dropGroup => dropGroup
                        .Select(dropId => {
                            var depotItem = Instances.ToolboxViewModel?.DepotResult?.FirstOrDefault(d => d.Id == dropId);
                            return depotItem is { Count: > 0 } ? depotItem.Count : 0;
                        })
                        .ToList())
                    .ToList();

                bool hasPositiveCount = normalizedCountGroups.SelectMany(group => group).Any(count => count > 0);
                if (hasPositiveCount)
                {
                    var groupTexts = normalizedCountGroups
                        .Select(group => string.Join(" & ", group.Select(count => count.FormatNumber(false))))
                        .ToList();

                    string inventoryLabel = LocalizationHelper.GetString("Inventory");
                    string inventoryText = $" ({inventoryLabel} {string.Join(" / ", groupTexts)})";
                    lines.Add(inventoryText);
                }
            }
        }

        return lines.Count > 0 ? string.Join(Environment.NewLine, lines) : string.Empty;
    }

    /// <summary>
    /// Gets the days left text
    /// </summary>
    /// <param name="expireTime">活动结束时间</param>
    /// <param name="now">当前时间</param>
    /// <returns>活动剩余日期</returns>
    private static string GetDaysLeftText(DateTime expireTime, DateTime now)
    {
        int daysLeft = (expireTime - now).Days;
        return daysLeft > 0 ? daysLeft.ToString() : LocalizationHelper.GetString("LessThanOneDay");
    }

    /// <summary>
    /// Gets open stage list at specified day of week
    /// </summary>
    /// <param name="dayOfWeek">Day of week</param>
    /// <returns>Open stage list</returns>
    public IEnumerable<StageInfo> GetStageList(DayOfWeek dayOfWeek)
    {
        return _stages.Values.Where(stage => !stage.IsHidden && stage.IsStageOpen(dayOfWeek));
    }

    /// <summary>
    /// Gets all open or will open stage list
    /// </summary>
    /// <returns>Open or will open stage list</returns>
    public IEnumerable<StageInfo> GetStageList()
    {
        return _stages.Values.Where(stage => !stage.IsHidden && stage.IsStageOpenOrWillOpen());
    }
}
