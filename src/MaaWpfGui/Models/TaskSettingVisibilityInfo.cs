// <copyright file="TaskSettingVisibilityInfo.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.Models
{
    /// <summary>
    /// The task setting enable info.
    /// </summary>
    public class TaskSettingVisibilityInfo : PropertyChangedBase
    {
        public const string DefaultVisibleTaskSetting = "Combat";

        private bool _wakeUp;
        private bool _recruiting;
        private bool _base;
        private bool _combat;
        private bool _mall;
        private bool _mission;
        private bool _autoRoguelike;

        public bool WakeUp { get => _wakeUp; set => SetAndNotify(ref _wakeUp, value); }

        public bool Recruiting { get => _recruiting; set => SetAndNotify(ref _recruiting, value); }

        public bool Base { get => _base; set => SetAndNotify(ref _base, value); }

        public bool Combat { get => _combat; set => SetAndNotify(ref _combat, value); }

        public bool Mall { get => _mall; set => SetAndNotify(ref _mall, value); }

        public bool Mission { get => _mission; set => SetAndNotify(ref _mission, value); }

        public bool AutoRoguelike { get => _autoRoguelike; set => SetAndNotify(ref _autoRoguelike, value); }

        public static TaskSettingVisibilityInfo Current { get; } = new TaskSettingVisibilityInfo();

        public void Set(string taskName, bool enable)
        {
            if (Guide && enable)
            {
                currentEnableSetting = taskName;
                enable = false;
            }

            switch (taskName)
            {
                case "WakeUp":
                    WakeUp = enable;
                    break;
                case "Recruiting":
                    Recruiting = enable;
                    break;
                case "Base":
                    Base = enable;
                    break;
                case "Combat":
                    Combat = enable;
                    break;
                case "Mall":
                    Mall = enable;
                    break;
                case "Mission":
                    Mission = enable;
                    break;
                case "AutoRoguelike":
                    AutoRoguelike = enable;
                    break;
            }

            EnableAdvancedSettings = false;
            if (Mission || WakeUp)
            {
                AdvancedSettingsVisibility = false;
            }
            else
            {
                AdvancedSettingsVisibility = true;
            }
        }

        private bool _enableAdvancedSettings;

        public bool EnableAdvancedSettings
        {
            get => _enableAdvancedSettings;
            set => SetAndNotify(ref _enableAdvancedSettings, value);
        }

        private bool _advancedSettingsVisibility;

        public bool AdvancedSettingsVisibility
        {
            get => _advancedSettingsVisibility;
            set => SetAndNotify(ref _advancedSettingsVisibility, value);
        }

        private string currentEnableSetting;

        private bool _guide = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, "0")) < SettingsViewModel.GuideMaxStep;

        public bool Guide
        {
            get => _guide;
            set
            {
                SetAndNotify(ref _guide, value);
                Set(currentEnableSetting, !value);
            }
        }

        #region 双入口设置可见性

        private bool _customInfrastPlanShowInFightSettings = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanShowInFightSettings, false.ToString()));

        public bool CustomInfrastPlanShowInFightSettings
        {
            get => _customInfrastPlanShowInFightSettings;
            set
            {
                SetAndNotify(ref _customInfrastPlanShowInFightSettings, value);
                ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanShowInFightSettings, value.ToString());
            }
        }

        #endregion
    }
}
