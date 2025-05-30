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
using HandyControl.Controls;
using HandyControl.Data;
using MaaWpfGui.Models;

namespace MaaWpfGui.Helper
{
    public class AchievementTrackerHelper
    {
        private Dictionary<string, Achievement> _achievements = new();

        public IReadOnlyDictionary<string, Achievement> Achievements => _achievements;

        public static AchievementTrackerHelper Instance { get; } = new();

        private AchievementTrackerHelper()
        {
            Load();
            InitializeAchievements();
        }

        private void Load()
        {
            _achievements = JsonDataHelper.Get("Achievement", new Dictionary<string, Achievement>()) ?? new Dictionary<string, Achievement>();
        }

        private void Save()
        {
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
                if (achievement is { IsUnlocked: false, Target: not null } &&
                    achievement.Progress >= achievement.Target.Value)
                {
                    Unlock(achievement.Id);
                }
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

            var growlInfo = new GrowlInfo
            {
                IsCustom = true,
                Message = $"{LocalizationHelper.GetString("AchievementCelebrate")}{achievement.Title}\n\n{achievement.Description}\n\n{LocalizationHelper.GetString("AchievementConditions")}{achievement.Conditions}",
                StaysOpen = true,
                IconKey = "HangoverGeometry",
                IconBrushKey = "PallasBrush",
            };
            Growl.Info(growlInfo);
        }

        public void AddProgress(string id, int amount)
        {
            if (!_achievements.TryGetValue(id, out var achievement) || achievement.IsUnlocked)
            {
                return;
            }

            achievement.Progress += amount;
            if (achievement.Target.HasValue && achievement.Progress >= achievement.Target.Value)
            {
                Unlock(id);
            }
            else
            {
                Save();
            }
        }

        public void AddProgressToGroup(string groupPrefix, int amount)
        {
            foreach (var achievement in _achievements.Values.Where(achievement => achievement.Id.StartsWith(groupPrefix)))
            {
                AddProgress(achievement.Id, amount);
            }
        }

        public bool IsUnlocked(string id) => _achievements.TryGetValue(id, out var a) && a.IsUnlocked;

        public Achievement? Get(string id) => _achievements.GetValueOrDefault(id);

        private void InitializeAchievements()
        {
            var achievements = new[]
            {
                // 基础使用类
                new Achievement { Id = "SanitySpender1", Target = 10 }, // 刷理智次数
                new Achievement { Id = "SanitySpender2", Target = 100 },

                // 搞笑/梗类成就
                new Achievement { Id = "QuickCloser" }, // 快速关闭弹窗
            };

            foreach (var achievement in achievements)
            {
                RegisterAchievement(achievement);
            }
        }
    }
}
