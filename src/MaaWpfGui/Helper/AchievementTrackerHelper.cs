// <copyright file="AchievementTrackerHelper.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models;
using Newtonsoft.Json.Linq;
using Stylet;
using static MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Helper
{
    public class AchievementTrackerHelper : PropertyChangedBase
    {
        private Dictionary<string, Achievement> _achievements = new();

        public Dictionary<string, Achievement> Achievements
        {
            get => _achievements;
            set
            {
                SetAndNotify(ref _achievements, value);
                NotifyOfPropertyChange(nameof(UnlockedCount));
                NotifyOfPropertyChange(nameof(VisibleAchievements));
            }
        }

        // 已解锁及非隐藏的未解锁成就
        public Dictionary<string, Achievement> VisibleAchievements
        {
            get => Achievements
                .Where(kv => kv.Value.IsUnlocked || !kv.Value.IsHidden)
                .ToDictionary(kv => kv.Key, kv => kv.Value);
        }

        public int UnlockedCount => Achievements.Count(a => a.Value.IsUnlocked);

        public static AchievementTrackerHelper Instance { get; } = new();

        private AchievementTrackerHelper()
        {
            InitializeAchievements();
            Load();

            // 保存一下新注册的成就，保证 data 里能找到
            Save();
            Instances.MainWindowManager.WindowRestored += (_, _) =>
            {
                TryShowPendingGrowls();
            };
        }

        private void Load()
        {
            var loadAchievements = JsonDataHelper.Get("Achievement", new Dictionary<string, Achievement>()) ?? new Dictionary<string, Achievement>();

            // 从中读取 IsUnlocked UnlockedTime Progress CustomData 等有关成就状态的信息，覆盖 _achievements 里现有的默认值
            foreach (var (id, saved) in loadAchievements)
            {
                if (!_achievements.TryGetValue(id, out var achievement))
                {
                    // EDIT：存个屁，不存了
                    // 有可能是高版本移到低版本来的，虽然有点抽象，但存一下
                    // RegisterAchievement(saved);
                    continue;
                }

                achievement.IsUnlocked = saved.IsUnlocked;
                achievement.UnlockedTime = saved.UnlockedTime;
                achievement.Progress = saved.Progress;
                achievement.CustomData = saved.CustomData;
            }
        }

        public void Save()
        {
            Achievements = _achievements
                .OrderByDescending(kv => kv.Value.IsUnlocked) // 已解锁优先
                .ThenBy(kv => !kv.Value.IsNewUnlock) // 新解锁的排前面
                .ThenBy(kv => kv.Value.IsHidden) // 隐藏排后面
                .ThenBy(kv => kv.Value.Category) // 按类别分组
                .ThenBy(kv => kv.Value.Id) // 最后按 Id
                .ToDictionary(kv => kv.Key, kv => kv.Value);

            JsonDataHelper.Set("Achievement", _achievements);
        }

        public void RegisterAchievement(Achievement achievement)
        {
            if (!_achievements.TryAdd(achievement.Id, achievement))
            {
                return;
            }

            // 尝试从相同组的成就中找最大已达成进度
            var similarProgress = _achievements.Values
                .Where(a => a.Id != achievement.Id && a.Id.StartsWith(GetGroupPrefix(achievement.Id)))
                .Select(a => a.Progress)
                .DefaultIfEmpty(0)
                .Max();

            // 如果新成就进度是0，且存在类似成就的进度，就追溯赋值
            if (achievement.Progress == 0 && similarProgress > 0)
            {
                achievement.Progress = similarProgress;
                CheckProgressUnlock(achievement);
            }
        }

        private static string GetGroupPrefix(string id)
        {
            // 例如 SanitySpender1 => SanitySpender
            return new(id.TakeWhile(char.IsLetter).ToArray());
        }

        public void Unlock(string id, bool staysOpen = true)
        {
            if (!_achievements.TryGetValue(id, out var achievement) || achievement.IsUnlocked)
            {
                return;
            }

            achievement.IsUnlocked = true;
            achievement.UnlockedTime = DateTime.UtcNow;
            achievement.IsNewUnlock = true;
            Save();

            var growlInfo = new GrowlInfo
            {
                IsCustom = true,
                Message = $"{LocalizationHelper.GetString("AchievementCelebrate")}: {achievement.Title}\n{achievement.Description}",
                StaysOpen = staysOpen,
                IconKey = "HangoverGeometry",
                IconBrushKey = achievement.MedalBrushKey,
            };
            ShowInfo(growlInfo);
        }

        public async Task UnlockAll()
        {
            foreach (var achievement in _achievements.Values.Where(a => !a.IsUnlocked))
            {
                await Task.Delay(100);
                Unlock(achievement.Id, false);
            }
        }

        public void Lock(string id)
        {
            if (!_achievements.TryGetValue(id, out var achievement))
            {
                return;
            }

            achievement.UnlockedTime = null;
            achievement.Progress = 0;
            achievement.CustomData = null;
            achievement.IsNewUnlock = false;

            if (achievement.IsUnlocked)
            {
                achievement.IsUnlocked = false;
                NotifyOfPropertyChange(nameof(UnlockedCount));
            }

            Save();
        }

        public void LockAll()
        {
            foreach (var achievement in _achievements.Values)
            {
                Lock(achievement.Id);
            }
        }

        private static readonly List<GrowlInfo> _pending = [];

        public static void ShowInfo(GrowlInfo info)
        {
            Execute.OnUIThread(() =>
            {
                var win = Instances.MainWindowManager.GetWindowIfVisible();
                if (win == null)
                {
                    _pending.Add(info);
                    return;
                }

                Growl.Info(info);
            });
        }

        public static void TryShowPendingGrowls()
        {
            foreach (var info in _pending)
            {
                Growl.Info(info);
            }

            _pending.Clear();
        }

        private void CheckProgressUnlock(Achievement achievement)
        {
            if (achievement.IsUnlocked ||
                !achievement.Target.HasValue ||
                achievement.Progress < achievement.Target.Value)
            {
                return;
            }

            Unlock(achievement.Id);
        }

        public void AddProgress(string id, int amount = 1)
        {
            if (!_achievements.TryGetValue(id, out var achievement))
            {
                return;
            }

            achievement.Progress += amount;
            CheckProgressUnlock(achievement);
            Save();
        }

        public void AddProgressToGroup(string groupPrefix, int amount = 1)
        {
            foreach (var achievement in _achievements.Values.Where(achievement => achievement.Id.StartsWith(groupPrefix)))
            {
                AddProgress(achievement.Id, amount);
            }
        }

        public void SetProgress(string id, int progress)
        {
            if (!_achievements.TryGetValue(id, out var achievement))
            {
                return;
            }

            achievement.Progress = progress;
            CheckProgressUnlock(achievement);
            Save();
        }

        public void SetProgressToGroup(string groupPrefix, int progress)
        {
            foreach (var achievement in _achievements.Values.Where(achievement => achievement.Id.StartsWith(groupPrefix)))
            {
                SetProgress(achievement.Id, progress);
            }
        }

        public Dictionary<string, JToken> GetAchievementCustomData(string id)
        {
            if (!_achievements.TryGetValue(id, out var achievement) || achievement.CustomData == null)
            {
                return new();
            }

            return achievement.CustomData;
        }

        public JToken? GetAchievementCustomData(string id, string key)
        {
            if (!_achievements.TryGetValue(id, out var achievement) || achievement.CustomData == null)
            {
                return null;
            }

            achievement.CustomData.TryGetValue(key, out var value);
            return value;
        }

        public void SetAchievementCustomData(string id, string key, JToken value)
        {
            if (!_achievements.TryGetValue(id, out var achievement))
            {
                return;
            }

            achievement.CustomData ??= new();
            achievement.CustomData[key] = value;
            Save();
        }

        public bool IsUnlocked(string id) => _achievements.TryGetValue(id, out var a) && a.IsUnlocked;

        public Achievement? Get(string id) => _achievements.GetValueOrDefault(id);

        #region 工厂函数

        private static Achievement BasicUsage(string id, int? target = null, bool isHidden = false)
           => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.BasicUsage };

        private static Achievement FeatureExploration(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.FeatureExploration };

        private static Achievement AutoBattle(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.AutoBattle };

        private static Achievement Humor(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.Humor };

        private static Achievement BugRelated(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.BugRelated };

        private static Achievement Behavior(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.Behavior };

        private static Achievement EasterEgg(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.EasterEgg };

        private static Achievement Rare(string id, int? target = null, bool isHidden = false)
            => new() { Id = id, Target = target, IsHidden = isHidden, Category = AchievementCategory.Rare };

        #endregion

        private readonly Achievement[] _allAchievements =
        [

            #region 基础使用类
            BasicUsage(id: AchievementIds.SanitySpender1, target: 10), // 刷理智次数
            BasicUsage(id: AchievementIds.SanitySpender2, target: 100),
            BasicUsage(id: AchievementIds.SanitySpender3, target: 1000),

            BasicUsage(id: AchievementIds.SanitySaver1, target: 1), // 刷理智次数
            BasicUsage(id: AchievementIds.SanitySaver2, target: 10),
            BasicUsage(id: AchievementIds.SanitySaver3, target: 50),

            BasicUsage(id: AchievementIds.FirstLaunch), // 首次启动
            BasicUsage(id: AchievementIds.SanityExpire, target: 8), // 单次消耗 8 瓶快过期的理智药
            BasicUsage(id: AchievementIds.OverLimitAgent, target: 100, isHidden: true), // 单次代理 100 关

            BasicUsage(id: AchievementIds.RoguelikeGamePass1, target: 1), // 使用牛牛通关肉鸽
            BasicUsage(id: AchievementIds.RoguelikeGamePass2, target: 5),
            BasicUsage(id: AchievementIds.RoguelikeGamePass3, target: 10),
            BasicUsage(id: AchievementIds.RoguelikeRetreat, target: 100), // 牛牛放弃探索 100 次
            BasicUsage(id: AchievementIds.RoguelikeGoldMax, target: 999), // 肉鸽源石锭到达 999

            #endregion

            #region 功能探索类
            FeatureExploration(id: AchievementIds.ScheduleMaster1, target: 1), // 定时执行
            FeatureExploration(id: AchievementIds.ScheduleMaster2, target: 100),

            FeatureExploration(id: AchievementIds.MirrorChyanFirstUse, isHidden: true), // 第一次成功使用 MirrorChyan 下载
            FeatureExploration(id: AchievementIds.MirrorChyanCdkError, isHidden: true), // MirrorChyan CDK 错误

            FeatureExploration(id: AchievementIds.PioneerTest), // 将 MAA 更新至公测版
            FeatureExploration(id: AchievementIds.PioneerSuperTest, isHidden: true), // 将 MAA 更新至内测版（隐藏）
            FeatureExploration(id: AchievementIds.PioneerDebug, isHidden: true), // 使用未发布版本的 MAA（隐藏）

            FeatureExploration(id: AchievementIds.MosquitoLeg, target: 5), // 使用「借助战打 OF-1」功能超过 5 次

            #endregion

            #region 自动战斗
            AutoBattle(id: AchievementIds.UseCopilot1, target: 1), // 自动战斗
            AutoBattle(id: AchievementIds.UseCopilot2, target: 10),
            AutoBattle(id: AchievementIds.UseCopilot3, target: 100),
            AutoBattle(id: AchievementIds.CopilotLikeGiven1, target: 1), // 点赞 1 次
            AutoBattle(id: AchievementIds.CopilotLikeGiven2, target: 10), // 点赞 10 次
            AutoBattle(id: AchievementIds.CopilotLikeGiven3, target: 50), // 点赞 50 次
            AutoBattle(id: AchievementIds.CopilotError), // 代理作战出现失误

            AutoBattle(id: AchievementIds.MapOutdated, isHidden: true), // 提示需要更新地图资源
            AutoBattle(id: AchievementIds.Irreplaceable, isHidden: true), // 自动编队缺少至少两名干员
            #endregion

            #region 搞笑/梗类成就
            Humor(id: AchievementIds.SnapshotChallenge999, isHidden: true), // 平均截图用时超过 800ms（高 ping 战士）
            Humor(id: AchievementIds.SnapshotChallenge800, isHidden: true), // 平均截图用时在 400ms 到 800ms 之间（是不是有点太慢了）
            Humor(id: AchievementIds.SnapshotChallenge400), // 平均截图用时小于 400ms（截图挑战 · Normal）
            Humor(id: AchievementIds.SnapshotChallenge100), // 平均截图用时小于 100ms（截图挑战 · Fast）
            Humor(id: AchievementIds.SnapshotChallenge10), // 平均截图用时小于 10ms（截图挑战 · Ultra）

            Humor(id: AchievementIds.QuickCloser, isHidden: true), // 快速关闭弹窗
            Humor(id: AchievementIds.TacticalRetreat, isHidden: true), // 停止任务
            Humor(id: AchievementIds.RealGacha, isHidden: true), // 真正的抽卡
            Humor(id: AchievementIds.PeekScreen, isHidden: true), // 窥屏
            Humor(id: AchievementIds.CustomizationMaster, isHidden: true), // 自定义背景
            Humor(id: AchievementIds.Martian, isHidden: true), // 90 天没更新
            Humor(id: AchievementIds.RecruitNoSixStar, target: 100), // 公招中累计 100 次没出现六星tag
            Humor(id: AchievementIds.RecruitNoSixStarStreak, target: 100, isHidden: true), // 公招中连续 100 次没出现六星tag
            Humor(id: AchievementIds.AfkWatcher), // 窗口尺寸最小化后长时间不操作
            Humor(id: AchievementIds.TaskStartCancel), // 在开始任务后马上又停止

            #endregion

            #region BUG 相关
            BugRelated(id: AchievementIds.CongratulationError, isHidden: true), // 喜报
            BugRelated(id: AchievementIds.UnexpectedCrash, isHidden: true), // 不速之客
            BugRelated(id: AchievementIds.ProblemFeedback), // 问题反馈
            BugRelated(id: AchievementIds.CdnTorture, target: 3), // 下载资源失败超过3次
            #endregion

            #region 习惯 行为 时长类
            Behavior(id: AchievementIds.MissionStartCount, target: 3), // 一天内开始任务超过 3 次
            Behavior(id: AchievementIds.LongTaskTimeout), // 触发超时提醒
            Behavior(id: AchievementIds.ProxyOnline3Hours, isHidden: true), // 使用代理功能连续在线超过 3 小时

            Behavior(id: AchievementIds.UseDaily1, target: 7), // 连续使用时间
            Behavior(id: AchievementIds.UseDaily2, target: 30),
            Behavior(id: AchievementIds.UseDaily3, target: 365),

            Behavior(id: AchievementIds.AprilFools, isHidden: true), // 愚人节
            Behavior(id: AchievementIds.MidnightLaunch, isHidden: true), // 0~4 点
            Behavior(id: AchievementIds.LunarNewYear, isHidden: true), // 春节

            #endregion

            #region 彩蛋类
            EasterEgg(id: AchievementIds.Rules, isHidden: true), // 我会一直注视着你
            EasterEgg(id: AchievementIds.VersionClick, isHidden: true), // 这也能点？

            Rare(id: AchievementIds.Lucky, isHidden: true), // 启动 MAA 时有极小概率触发
            #endregion
        ];

        private void InitializeAchievements()
        {
            foreach (var achievement in _allAchievements)
            {
                RegisterAchievement(achievement);
            }
        }

        #region 带有 CustomData 的辅助函数

        public void MissionStartCountAdd()
        {
            const string Id = AchievementIds.MissionStartCount;
            const string Key = AchievementIds.MissionStartCountCustomDataKey;

            var today = DateTime.UtcNow.ToYjDate().Date;
            DateTime? lastDate = GetAchievementCustomData(AchievementIds.MissionStartCount, Key)?.ToObject<DateTime>();

            if (lastDate.HasValue && lastDate.Value == today)
            {
                AddProgress(Id);
            }
            else
            {
                SetAchievementCustomData(Id, Key, JToken.FromObject(today));
                SetProgress(Id, 1);
            }
        }

        public void UseDailyAdd()
        {
            // group 是不注册的，从第一个成就取 CustomData
            const string Id = AchievementIds.UseDaily1;
            const string GroupId = AchievementIds.UseDailyGroup;
            const string Key = AchievementIds.UseDailyCustomDataKey;

            var today = DateTime.UtcNow.ToYjDate().Date;
            DateTime? lastDate = GetAchievementCustomData(Id, Key)?.ToObject<DateTime>();
            if (lastDate.HasValue)
            {
                var delta = (today - lastDate.Value).TotalDays;

                switch (delta)
                {
                    case 1:
                        AddProgressToGroup(GroupId);
                        break;
                    case > 1:
                        SetProgressToGroup(GroupId, 1);
                        break;
                }
            }
            else
            {
                SetProgressToGroup(GroupId, 1);
            }

            SetAchievementCustomData(Id, Key, JToken.FromObject(today));
        }

        #endregion
    }
}
