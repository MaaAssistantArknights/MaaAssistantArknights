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

using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

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
        private bool _fight;
        private bool _mall;
        private bool _award;
        private bool _roguelike;
        private bool _reclamation;
        private bool _postAction;
        private bool _custom;

        public bool WakeUp { get => _startUp; set => SetAndNotify(ref _startUp, value); }

        public bool Recruiting { get => _recruit; set => SetAndNotify(ref _recruit, value); }

        public bool Base { get => _infrast; set => SetAndNotify(ref _infrast, value); }

        public bool Combat { get => _fight; set => SetAndNotify(ref _fight, value); }

        public bool Mall { get => _mall; set => SetAndNotify(ref _mall, value); }

        public bool Mission { get => _award; set => SetAndNotify(ref _award, value); }

        public bool AutoRoguelike { get => _roguelike; set => SetAndNotify(ref _roguelike, value); }

        public bool Reclamation { get => _reclamation; set => SetAndNotify(ref _reclamation, value); }

        public bool AfterAction { get => _postAction; set => SetAndNotify(ref _postAction, value); }

        public bool Custom { get => _custom; set => SetAndNotify(ref _custom, value); }

        public static TaskSettingVisibilityInfo Instance { get; } = new();

        // 长草任务当前选中
        public int CurrentIndex
        {
            get
            {
                if (ConfigFactory.CurrentConfig.TaskSelectedIndex < 0 || ConfigFactory.CurrentConfig.TaskSelectedIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
                {
                    ConfigFactory.CurrentConfig.TaskSelectedIndex = 0;
                }

                return ConfigFactory.CurrentConfig.TaskSelectedIndex;
            }

            set
            {
                ConfigFactory.CurrentConfig.TaskSelectedIndex = value;
                NotifyOfPropertyChange();
            }
        }

        [PropertyDependsOn(nameof(CurrentIndex))]
        public bool IsTaskRunning => Instances.AsstProxy.TasksStatus.TryGetValue(ConfigFactory.CurrentConfig.TaskQueue[CurrentIndex].TaskId, out var status) && status.Status == TaskStatus.InProgress;

        public static BaseTask CurrentTask => ConfigFactory.CurrentConfig.TaskQueue[Instance.CurrentIndex];
        public void Set(string taskName, bool enable)
        {
            bool ret = false;
            if (Guide && enable)
            {
                _currentEnableSetting = taskName;
                enable = false;
            }

            switch (taskName)
            {
                case "WakeUp":
                    WakeUp = enable;
                    SetRunning("StartUp");
                    break;
                case "Recruiting":
                    Recruiting = enable;
                    SetRunning("Recruit");
                    break;
                case "Base":
                    Base = enable;
                    SetRunning("Infrast");
                    break;
                case "Combat":
                    Combat = enable;
                    SetRunning("Fight");
                    break;
                case "Mall":
                    Mall = enable;
                    SetRunning("Mall");
                    break;
                case "Mission":
                    Mission = enable;
                    SetRunning("Award");
                    break;
                case "AutoRoguelike":
                    AutoRoguelike = enable;
                    SetRunning("Roguelike");
                    break;
                case "Reclamation":
                    Reclamation = enable;
                    SetRunning("Reclamation");
                    break;
                case "AfterAction":
                    AfterAction = enable;
                    break;
                case "Custom":
                    Custom = enable;
                    SetRunning("Custom");
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

            if (enable && !ret) // 如果切换到的不是当前运行任务
            {
                IsCurrentTaskRunning = false;
            }

            void SetRunning(in string task)
            {
                if (enable && CurrentTask.StartsWith(task))
                {
                    IsCurrentTaskRunning = true;
                    ret = true;
                }
            }
        }

        private string _currentTask = string.Empty;

        // 重构前的临时过渡
        public string CurrentTask
        {
            get => _currentTask;
            set
            {
                _currentTask = value;
                if (string.IsNullOrEmpty(value))
                {
                    IsCurrentTaskRunning = false;
                    return;
                }

                bool ret = false;
                CheckTask("StartUp", _startUp);
                CheckTask("Recruit", _recruit);
                CheckTask("Infrast", _infrast);
                CheckTask("Fight", _fight);
                CheckTask("Mall", _mall);
                CheckTask("Award", _award);
                CheckTask("Roguelike", _roguelike);
                CheckTask("Reclamation", _reclamation);
                CheckTask("Custom", _custom);
                if (!ret) // 如果没有匹配上任何任务
                {
                    IsCurrentTaskRunning = false;
                }

                void CheckTask(string taskName, bool taskIsShown)
                {
                    if (taskIsShown && value.StartsWith(taskName))
                    {
                        IsCurrentTaskRunning = true;
                        ret = true;
                    }
                }
            }
        }

        private bool _isCurrentTaskRunning;

        /// <summary> Gets or sets a value indicating whether 当前选中的任务是否正在运行 </summary>
        public bool IsCurrentTaskRunning
        {
            get => _isCurrentTaskRunning;
            set => SetAndNotify(ref _isCurrentTaskRunning, value);
        }

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

        private bool _guide = ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, 0) < SettingsViewModel.GuideMaxStep;

        public bool Guide
        {
            get => _guide;
            set
            {
                SetAndNotify(ref _guide, value);
                if (!value)
                {
                    var index = 0;
                    foreach (var task in ConfigFactory.CurrentConfig.TaskQueue)
                    {
                        if (task.TaskType == TaskType.Fight)
                        {
                            Set(index, true);
                            return;
                        }

                        index++;
                    }

                    Set(0, true);
                }
            }
        }
    }
}
