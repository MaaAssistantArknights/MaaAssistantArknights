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
using System.ComponentModel;
using System.Linq;
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Constants;
using MaaWpfGui.Models;
using Stylet;

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
            }
        }

        public int UnlockedCount => Achievements.Count(a => a.Value.IsUnlocked);

        public static AchievementTrackerHelper Instance { get; } = new();

        private AchievementTrackerHelper()
        {
            Load();
            InitializeAchievements();
            Instances.MainWindowManager.WindowRestored += (_, _) =>
            {
                TryShowPendingGrowls();
            };
        }

        private void Load()
        {
            _achievements = JsonDataHelper.Get("Achievement", new Dictionary<string, Achievement>()) ?? new Dictionary<string, Achievement>();
        }

        public void Save()
        {
            Achievements = _achievements
                .OrderByDescending(kv => kv.Value.IsUnlocked) // 已解锁优先
                .ThenBy(kv => kv.Value.IsHidden) // 隐藏排后面
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

            Save();
        }

        private static string GetGroupPrefix(string id)
        {
            // 例如 SanitySpender1 => SanitySpender
            return new(id.TakeWhile(char.IsLetter).ToArray());
        }

        public void Unlock(string id)
        {
            if (!_achievements.TryGetValue(id, out var achievement) || achievement.IsUnlocked)
            {
                return;
            }

            achievement.IsUnlocked = true;
            achievement.UnlockedTime = DateTime.UtcNow;
            Save();

            NotifyOfPropertyChange(nameof(UnlockedCount));

            var growlInfo = new GrowlInfo
            {
                IsCustom = true,
                Message = $"{LocalizationHelper.GetString("AchievementCelebrate")}: {achievement.Title}\n{achievement.Description}",
                StaysOpen = true,
                IconKey = "HangoverGeometry",
                IconBrushKey = achievement.IsHidden ? "HiddenMedalBrush" : "NormalMedalBrush",
            };
            ShowInfo(growlInfo);
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

        public void AddProgress(string id, int amount)
        {
            if (!_achievements.TryGetValue(id, out var achievement))
            {
                return;
            }

            achievement.Progress += amount;
            CheckProgressUnlock(achievement);
            Save();
        }

        public void AddProgressToGroup(string groupPrefix, int amount)
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

        public bool IsUnlocked(string id) => _achievements.TryGetValue(id, out var a) && a.IsUnlocked;

        public Achievement? Get(string id) => _achievements.GetValueOrDefault(id);

        private void InitializeAchievements()
        {
            var achievements = new[]
            {
                // 基础使用类
                new Achievement { Id = AchievementIds.SanitySpender1, Target = 10 }, // 刷理智次数
                new Achievement { Id = AchievementIds.SanitySpender2, Target = 100 },
                new Achievement { Id = AchievementIds.SanitySpender3, Target = 1000 },
                new Achievement { Id = AchievementIds.SanitySaver1, Target = 1 }, // 刷理智次数
                new Achievement { Id = AchievementIds.SanitySaver2, Target = 10 },
                new Achievement { Id = AchievementIds.SanitySaver3, Target = 50 },

                new Achievement { Id = AchievementIds.FirstLaunch }, // 首次启动
                new Achievement { Id = AchievementIds.SanityExpire, Target = 8 }, // 单次消耗 8 瓶快过期的理智药
                new Achievement { Id = AchievementIds.OverLimitAgent, Target = 100, IsHidden = true }, // 单次代理 100 关

                // 功能探索类
                new Achievement { Id = AchievementIds.ScheduleMaster1, Target = 1 }, // 定时执行
                new Achievement { Id = AchievementIds.ScheduleMaster2, Target = 100 },
                new Achievement { Id = AchievementIds.MirrorChyanFirstUse, IsHidden = true }, // 第一次成功使用 MirrorChyan 下载
                new Achievement { Id = AchievementIds.MirrorChyanCdkError, IsHidden = true }, // MirrorChyan CDK 错误

                // 自动战斗
                new Achievement { Id = AchievementIds.MapOutdated }, // 提示需要更新地图资源
                new Achievement { Id = AchievementIds.UseCopilot1, Target = 1 }, // 自动战斗
                new Achievement { Id = AchievementIds.UseCopilot2, Target = 10 },
                new Achievement { Id = AchievementIds.UseCopilot3, Target = 100 },

                // 搞笑/梗类成就
                new Achievement { Id = AchievementIds.QuickCloser, IsHidden = true }, // 快速关闭弹窗
                new Achievement { Id = AchievementIds.TacticalRetreat, IsHidden = true }, // 停止任务
                new Achievement { Id = AchievementIds.RealGacha, IsHidden = true }, // 真正的抽卡
                new Achievement { Id = AchievementIds.PeekScreen, IsHidden = true }, // 窥屏
                new Achievement { Id = AchievementIds.CustomizationMaster, IsHidden = true }, // 自定义背景
                new Achievement { Id = AchievementIds.Martian, IsHidden = true }, // 90 天没更新
                new Achievement { Id = AchievementIds.RecruitNoSixStar, Target = 100 }, // 公招中累计 100 次没出现六星tag
                new Achievement { Id = AchievementIds.RecruitNoSixStarStreak, Target = 100, IsHidden = true }, // 公招中连续 100 次没出现六星tag

                // BUG 相关
                new Achievement { Id = AchievementIds.CongratulationError, IsHidden = true }, // 喜报
                new Achievement { Id = AchievementIds.UnexpectedCrash, IsHidden = true }, // 不速之客
                new Achievement { Id = AchievementIds.ProblemFeedback }, // 问题反馈

                // 彩蛋类
                new Achievement { Id = AchievementIds.Rules, IsHidden = true }, // 我会一直注视着你
                new Achievement { Id = AchievementIds.VersionClick, IsHidden = true }, // 这也能点？
            };

            foreach (var achievement in achievements)
            {
                RegisterAchievement(achievement);
            }
        }
    }
}
