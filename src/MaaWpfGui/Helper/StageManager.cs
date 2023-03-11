// <copyright file="StageManager.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Semver;
using Stylet;
using StyletIoC;

namespace MaaWpfGui
{
    /// <summary>
    /// Stage manager
    /// </summary>
    public class StageManager
    {
        [DllImport("MaaCore.dll")]
        private static extern IntPtr AsstGetVersion();

        // model references
        private readonly TaskQueueViewModel _taskQueueViewModel;

        // datas
        private Dictionary<string, StageInfo> _stages;

        /// <summary>
        /// Initializes a new instance of the <see cref="StageManager"/> class.
        /// </summary>
        /// <param name="container">The IoC container.</param>
        public StageManager(IContainer container)
        {
            _taskQueueViewModel = container.Get<TaskQueueViewModel>();
            UpdateStage(false);

            Execute.OnUIThread(async () =>
            {
                var task = Task.Run(() =>
                {
                    UpdateStage(true);
                });
                await task;
                if (_taskQueueViewModel != null)
                {
                    _taskQueueViewModel.UpdateDatePrompt();
                    _taskQueueViewModel.UpdateStageList(true);
                }
            });
        }

        public void UpdateStage(bool fromWeb)
        {
            if (fromWeb)
            {
                // Check if we need to update from the web
                string lastUpdateTimeFile = "lastUpdateTime.json";
                JObject localLastUpdatedJson = WebService.LoadApiCache(lastUpdateTimeFile);
                JObject webLastUpdatedJson = WebService.RequestMaaApiWithCache(lastUpdateTimeFile);
                if (localLastUpdatedJson != null && webLastUpdatedJson != null)
                {
                    long localTimestamp = localLastUpdatedJson["timestamp"].ToObject<long>();
                    long webTimestamp = webLastUpdatedJson["timestamp"].ToObject<long>();
                    if (webTimestamp <= localTimestamp)
                    {
                        return;
                    }
                }
            }

            var tempStage = new Dictionary<string, StageInfo>
            {
                // 这里会被 “剩余理智” 复用，第一个必须是 string.Empty 的
                // 「当前/上次」关卡导航
                { string.Empty, new StageInfo { Display = Localization.GetString("DefaultStage"), Value = string.Empty } },
            };

            var clientType = ViewStatusStorage.Get("Start.ClientType", string.Empty);

            // 官服和B服使用同样的资源
            if (clientType == "Bilibili" || clientType == string.Empty)
            {
                clientType = "Official";
            }

            // Download the activities
            var stageApi = "gui/StageActivity.json";
            JObject activity = fromWeb ? WebService.RequestMaaApiWithCache(stageApi) : WebService.LoadApiCache(stageApi);

            // Download the tasks resources into cache so MaaCore can load them later
            var tasksPath = "resource/tasks.json";
            JObject tasksJson = fromWeb ? WebService.RequestMaaApiWithCache(tasksPath) : WebService.LoadApiCache(tasksPath);

            if (clientType != "Official" && tasksJson != null)
            {
                tasksPath = "resource/global/" + clientType + '/' + tasksPath;

                // Download the client specific resources only when the Official ones are successfully downloaded so that the client specific resource version is the actual version
                // TODO: There may be an issue when the CN resource is loaded from cache (e.g. network down) while global resource is downloaded (e.g. network up again)
                // var tasksJsonClient = fromWeb ? WebService.RequestMaaApiWithCache(tasksPath) : WebService.RequestMaaApiWithCache(tasksPath);
                _ = fromWeb ? WebService.RequestMaaApiWithCache(tasksPath) : WebService.RequestMaaApiWithCache(tasksPath);
            }

            bool isDebugVersion = Marshal.PtrToStringAnsi(AsstGetVersion()) == "DEBUG VERSION";
            bool curVerParsed = SemVersion.TryParse(Marshal.PtrToStringAnsi(AsstGetVersion()), SemVersionStyles.AllowLowerV, out var curVersionObj);

            // bool curResourceVerParsed = SemVersion.TryParse(
            //    tasksJsonClient?["ResourceVersion"]?.ToString() ?? tasksJson?["ResourceVersion"]?.ToString() ?? string.Empty,
            //    SemVersionStyles.AllowLowerV, out var curResourceVersionObj);
            var resourceCollection = new StageActivityInfo()
            {
                IsResourceCollection = true,
            };

            static DateTime GetDateTime(JToken keyValuePairs, string key)
                => DateTime.ParseExact(keyValuePairs[key].ToString(),
                   "yyyy/MM/dd HH:mm:ss",
                   CultureInfo.InvariantCulture).AddHours(-Convert.ToInt32(keyValuePairs?["TimeZone"].ToString() ?? "0"));

            if (activity?[clientType] != null)
            {
                try
                {
                    // 资源全开放活动
                    var resourceCollectionData = activity[clientType]["resourceCollection"];
                    if (resourceCollectionData != null)
                    {
                        resourceCollection.Tip = resourceCollectionData["Tip"]?.ToString();
                        resourceCollection.UtcStartTime = GetDateTime(resourceCollectionData, "UtcStartTime");
                        resourceCollection.UtcExpireTime = GetDateTime(resourceCollectionData, "UtcExpireTime");
                    }

                    // 活动关卡
                    foreach (var stageObj in activity[clientType]["sideStoryStage"] ?? Enumerable.Empty<JToken>())
                    {
                        // 现在只有导航，暂不判断版本
                        // MinimumResourceRequired is not necessarily provided in json, in which case it is ok even if there are no cached resources
                        // bool minResourceRequiredParsed = SemVersion.TryParse(stageObj?["MinimumResourceRequired"]?.ToString() ?? string.Empty, SemVersionStyles.AllowLowerV, out var minResourceRequiredObj);
                        bool minRequiredParsed = SemVersion.TryParse(stageObj?["MinimumRequired"]?.ToString() ?? string.Empty, SemVersionStyles.AllowLowerV, out var minRequiredObj);

                        var stageInfo = new StageInfo();

                        // && (!minResourceRequiredParsed || curResourceVerParsed))
                        if (isDebugVersion || (curVerParsed && minRequiredParsed))
                        {
                            // Debug Version will be considered satisfying min version requirement, but the resource version needs a comparison
                            if (!isDebugVersion)
                            {
                                // &&(!minResourceRequiredParsed || curResourceVersionObj.CompareSortOrderTo(minResourceRequiredObj) < 0)
                                if (curVersionObj.CompareSortOrderTo(minRequiredObj) < 0)
                                {
                                    if (!tempStage.ContainsKey(Localization.GetString("UnsupportedStages")))
                                    {
                                        stageInfo = new StageInfo
                                        {
                                            Display = Localization.GetString("UnsupportedStages"),
                                            Value = Localization.GetString("UnsupportedStages"),
                                            Drop = Localization.GetString("LowVersion"),
                                            Activity = new StageActivityInfo()
                                            {
                                                Tip = stageObj["Activity"]?["Tip"]?.ToString(),
                                                StageName = stageObj["Activity"]?["StageName"]?.ToString(),
                                                UtcStartTime = GetDateTime(stageObj["Activity"], "UtcStartTime"),
                                                UtcExpireTime = GetDateTime(stageObj["Activity"], "UtcExpireTime"),
                                            },
                                        };
                                        if (!stageInfo.Activity.IsExpired)
                                        {
                                            tempStage.Add(stageInfo.Display, stageInfo);
                                        }
                                    }

                                    continue;
                                }
                            }
                        }
                        else
                        {
                            continue;
                        }

                        stageInfo = new StageInfo
                        {
                            Display = stageObj?["Display"]?.ToString() ?? string.Empty,
                            Value = stageObj["Value"].ToString(),
                            Drop = stageObj?["Drop"]?.ToString(),
                            Activity = new StageActivityInfo()
                            {
                                Tip = stageObj["Activity"]?["Tip"]?.ToString(),
                                StageName = stageObj["Activity"]?["StageName"]?.ToString(),
                                UtcStartTime = GetDateTime(stageObj["Activity"], "UtcStartTime"),
                                UtcExpireTime = GetDateTime(stageObj["Activity"], "UtcExpireTime"),
                            },
                        };

                        if (!stageInfo.Activity.IsExpired)
                        {
                            tempStage.Add(stageInfo.Display, stageInfo);
                        }
                    }
                }
                catch (Exception e)
                {
                    Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                }
            }

            foreach (var kvp in new Dictionary<string, StageInfo>
            {
                // 主线关卡
                { "1-7", new StageInfo { Display = "1-7", Value = "1-7" } },

                // 资源本
                { "CE-6", new StageInfo("CE-6", "CETip", new[] { DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },
                { "AP-5", new StageInfo("AP-5", "APTip", new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },
                { "CA-5", new StageInfo("CA-5", "CATip", new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday }, resourceCollection) },
                { "LS-6", new StageInfo("LS-6", "LSTip", new DayOfWeek[] { }, resourceCollection) },

                // 碳本没做导航，只显示关卡提示
                { "SK-5", new StageInfo("SK-5", "SKTip", new[] { DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday }, resourceCollection) { IsHidden = true } },

                // 剿灭模式
                { "Annihilation", new StageInfo { Display = Localization.GetString("Annihilation"), Value = "Annihilation" } },

                // 芯片本
                { "PR-A-1", new StageInfo("PR-A-1", "PR-ATip", new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }, resourceCollection) },
                { "PR-A-2", new StageInfo("PR-A-2", string.Empty, new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }, resourceCollection) },
                { "PR-B-1", new StageInfo("PR-B-1", "PR-BTip", new[] { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }, resourceCollection) },
                { "PR-B-2", new StageInfo("PR-B-2", string.Empty, new[] { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }, resourceCollection) },
                { "PR-C-1", new StageInfo("PR-C-1", "PR-CTip", new[] { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },
                { "PR-C-2", new StageInfo("PR-C-2", string.Empty, new[] { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },
                { "PR-D-1", new StageInfo("PR-D-1", "PR-DTip", new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },
                { "PR-D-2", new StageInfo("PR-D-2", string.Empty, new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }, resourceCollection) },

                // 周一和周日的关卡提示
                { "Pormpt1", new StageInfo { Tip = Localization.GetString("Pormpt1"), OpenDays = new[] { DayOfWeek.Monday }, IsHidden = true } },
                { "Pormpt2", new StageInfo { Tip = Localization.GetString("Pormpt2"), OpenDays = new[] { DayOfWeek.Sunday }, IsHidden = true } },
            })
            {
                tempStage.Add(kvp.Key, kvp.Value);
            }

            _stages = tempStage;
        }

        /// <summary>
        /// Gets stage by name
        /// </summary>
        /// <param name="stage">Stage name</param>
        /// <returns>Stage info</returns>
        public StageInfo GetStageInfo(string stage)
        {
            _stages.TryGetValue(stage, out var stageInfo);
            return stageInfo;
        }

        /// <summary>
        /// Determine whether stage is open
        /// </summary>
        /// <param name="stage">Stage name</param>
        /// <param name="dayOfWeek">Current day of week</param>
        /// <returns>Whether stage is open</returns>
        public bool IsStageOpen(string stage, DayOfWeek dayOfWeek)
        {
            if (stage == null)
            {
                return false;
            }

            return GetStageInfo(stage)?.IsStageOpen(dayOfWeek) == true;
        }

        /// <summary>
        /// Gets open stage tips at specified day of week
        /// </summary>
        /// <param name="dayOfWeek">Day of week</param>
        /// <returns>Open stages</returns>
        public string GetStageTips(DayOfWeek dayOfWeek)
        {
            var builder = new StringBuilder();
            var sideStoryFlag = true;
            foreach (var item in _stages)
            {
                if (item.Value.IsStageOpen(dayOfWeek))
                {
                    if (sideStoryFlag && !string.IsNullOrEmpty(item.Value.Activity?.StageName))
                    {
                        DateTime dateTime = DateTime.UtcNow;
                        var daysleftopen = (item.Value.Activity.UtcExpireTime - dateTime).Days;
                        builder.AppendLine(item.Value.Activity.StageName
                            + " "
                            + Localization.GetString("Daysleftopen")
                            + (daysleftopen > 0 ? daysleftopen.ToString() : Localization.GetString("LessThanOneDay")));
                        sideStoryFlag = false;
                    }

                    if (!string.IsNullOrEmpty(item.Value.Tip))
                    {
                        builder.AppendLine(item.Value.Tip);
                    }

                    if (!string.IsNullOrEmpty(item.Value.Drop))
                    {
                        builder.AppendLine(item.Value.Display + ": " + Utils.GetItemName(item.Value.Drop));
                    }
                }
            }

            return builder.ToString();
        }

        /// <summary>
        /// Gets open stage list at specified day of week
        /// </summary>
        /// <param name="dayOfWeek">Day of week</param>
        /// <returns>Open stage list</returns>
        public IEnumerable<CombData> GetStageList(DayOfWeek dayOfWeek)
        {
            return _stages.Values.Where(stage => !stage.IsHidden && stage.IsStageOpen(dayOfWeek));
        }

        /// <summary>
        /// Gets all stage list
        /// </summary>
        /// <returns>All stage list</returns>
        public IEnumerable<CombData> GetStageList()
        {
            return _stages.Values.Where(stage => !stage.IsHidden);
        }
    }
}
