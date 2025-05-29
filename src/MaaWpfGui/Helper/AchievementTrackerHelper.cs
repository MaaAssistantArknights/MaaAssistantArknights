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

            Save();
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
                Message = $"{LocalizationHelper.GetString("AchievementCelebrate")}\n{achievement.Title}\n{achievement.Description}",
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
            RegisterAchievement(new()
            {
                Id = "SanitySpender1",
                Target = 500,
            });

            RegisterAchievement(new()
            {
                Id = "SanitySpender2",
                Target = 1000,
            });

            RegisterAchievement(new()
            {
                Id = "QuickCloser",
            });
        }
    }
}
