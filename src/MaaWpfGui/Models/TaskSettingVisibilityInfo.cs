// <copyright file="TaskSettingVisibilityInfo.cs" company="MaaAssistantArknights">
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

        private bool _startUp;
        private bool _recruit;
        private bool _infrast;
        private bool _combat;
        private bool _mall;
        private bool _award;
        private bool _roguelike;
        private bool _reclamation;
        private bool _postAction;
        private bool _custom;

        public bool WakeUp { get => _startUp; set => SetAndNotify(ref _startUp, value); }

        public bool Recruiting { get => _recruit; set => SetAndNotify(ref _recruit, value); }

        public bool Base { get => _infrast; set => SetAndNotify(ref _infrast, value); }

        public bool Combat { get => _combat; set => SetAndNotify(ref _combat, value); }

        public bool Mall { get => _mall; set => SetAndNotify(ref _mall, value); }

        public bool Mission { get => _award; set => SetAndNotify(ref _award, value); }

        public bool AutoRoguelike { get => _roguelike; set => SetAndNotify(ref _roguelike, value); }

        public bool Reclamation { get => _reclamation; set => SetAndNotify(ref _reclamation, value); }

        public bool AfterAction { get => _postAction; set => SetAndNotify(ref _postAction, value); }

        public bool Custom { get => _custom; set => SetAndNotify(ref _custom, value); }

        public static TaskSettingVisibilityInfo Instance { get; } = new();

        // 长草任务当前选中
        public int CurrentIndex { get; set; }

        public void Set(string taskName, bool enable)
        {
            if (Guide && enable)
            {
                _currentEnableSetting = taskName;
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
                case "Reclamation":
                    Reclamation = enable;
                    break;
                case "AfterAction":
                    AfterAction = enable;
                    break;
                case "Custom":
                    Custom = enable;
                    break;
            }

            EnableAdvancedSettings = false;
            if (Mission || WakeUp || AfterAction)
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

        private string _currentEnableSetting;

        private bool _guide = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, "0")) < SettingsViewModel.GuideMaxStep;

        public bool Guide
        {
            get => _guide;
            set
            {
                SetAndNotify(ref _guide, value);
                Set(_currentEnableSetting, !value);
            }
        }

        #region 双入口设置可见性

        private bool _customInfrastPlanShowInFightSettings = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanShowInFightSettings, bool.FalseString));

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
