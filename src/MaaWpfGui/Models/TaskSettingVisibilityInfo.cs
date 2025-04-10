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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
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
        private bool _fight;
        private bool _mall;
        private bool _mission;
        private bool _autoRoguelike;
        private bool _reclamation;
        private bool _afterAction;
        private bool _custom;

        public bool WakeUp { get => _wakeUp; set => SetAndNotify(ref _wakeUp, value); }

        public bool Recruiting { get => _recruiting; set => SetAndNotify(ref _recruiting, value); }

        public bool Base { get => _base; set => SetAndNotify(ref _base, value); }

        public bool Combat { get => _fight; set => SetAndNotify(ref _fight, value); }

        public bool Mall { get => _mall; set => SetAndNotify(ref _mall, value); }

        public bool Mission { get => _mission; set => SetAndNotify(ref _mission, value); }

        public bool AutoRoguelike { get => _autoRoguelike; set => SetAndNotify(ref _autoRoguelike, value); }

        public bool Reclamation { get => _reclamation; set => SetAndNotify(ref _reclamation, value); }

        public bool AfterAction { get => _afterAction; set => SetAndNotify(ref _afterAction, value); }

        public bool Custom { get => _custom; set => SetAndNotify(ref _custom, value); }

        public static TaskSettingVisibilityInfo Instance { get; } = new();

        public int CurrentIndex { get; set; }

        public void Set(int taskIndex, bool enable)
        {
            if (Guide && enable)
            {
                // TODO Config修复引导
                // _currentEnableSetting = taskName;
                enable = false;
            }

            if (enable)
            {
                CurrentIndex = taskIndex;
            }

            if (enable || ConfigFactory.CurrentConfig.TaskQueue[taskIndex].GetType() != ConfigFactory.CurrentConfig.TaskQueue[CurrentIndex].GetType())
            {
                var task = ConfigFactory.CurrentConfig.TaskQueue[taskIndex];
                if (task is StartUpTask)
                {
                    WakeUp = enable;
                }
                else if (task is RecruitTask)
                {
                    Recruiting = enable;
                }
                else if (task is InfrastTask)
                {
                    Base = enable;
                }
                else if (task is FightTask)
                {
                    Combat = enable;
                }
                else if (task is MallTask)
                {
                    Mall = enable;
                }
                else if (task is AwardTask)
                {
                    Mission = enable;
                }
                else if (task is RoguelikeTask)
                {
                    AutoRoguelike = enable;
                }
                else if (task is ReclamationTask)
                {
                    Reclamation = enable;
                }
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

            if (enable)
            {
                // Instances.SettingsViewModel.Refresh();
                Instances.TaskQueueViewModel.RefreshTaskModel(ConfigFactory.CurrentConfig.TaskQueue[taskIndex]);
            }
        }

        public void SetPostAction(bool value)
        {
            /*WakeUp = false;
            Recruiting = false;
            Base = false;
            Combat = false;
            Mall = false;
            Mission = false;
            AutoRoguelike = false;
            Reclamation = false;
            Custom = false;*/
            AfterAction = value;
        }

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

            var list = Instances.TaskQueueViewModel.TaskItemViewModels;
            if (enable && list is not null)
            {
                for (int index = 0; index < list.Count; index++)
                {
                    var item = Instances.TaskQueueViewModel.TaskItemViewModels[index];
                    if (enable && item.OriginalName == taskName)
                    {
                        CurrentIndex = index;
                        Instances.TaskQueueViewModel.RefreshTaskModel(ConfigFactory.CurrentConfig.TaskQueue[index]);
                    }
                }
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
