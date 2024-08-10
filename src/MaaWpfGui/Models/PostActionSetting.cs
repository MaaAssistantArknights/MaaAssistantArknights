// <copyright file="PostActionSetting.cs" company="MaaAssistantArknights">
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
using System.Windows.Controls;
using System.Windows.Input;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Stylet;

namespace MaaWpfGui.Models;

public class PostActionSetting : PropertyChangedBase
{
    [Flags]
    public enum PostActions
    {
        /// <summary>
        /// 无动作
        /// </summary>
        None = 0,

        /// <summary>
        /// 退出明日方舟
        /// </summary>
        ExitArknights = 1,

        /// <summary>
        /// 返回模拟器首页
        /// </summary>
        BackToAndroidHome = 2,

        /// <summary>
        /// 退出模拟器
        /// </summary>
        ExitEmulator = 4,

        /// <summary>
        /// 退出MAA
        /// </summary>
        ExitSelf = 8,

        /// <summary>
        /// 如果没有其他MAA
        /// </summary>
        IfNoOtherMaa = 16,

        /// <summary>
        /// 休眠
        /// </summary>
        Hibernate = 32,

        /// <summary>
        /// 关机
        /// </summary>
        Shutdown = 64,
    }

    private PostActions _postActions;

    private bool _once;

    public bool Once
    {
        get => _once;
        set
        {
            if (!SetAndNotify(ref _once, value))
            {
                return;
            }

            if (!value)
            {
                SaveActions();
            }

            ActionTitle = value ? string.Format("{0} ({1})", LocalizationHelper.GetString("PostActions"), LocalizationHelper.GetString("Once")) : LocalizationHelper.GetString("PostActions");
            RefreshDescription();
        }
    }

    private bool _exitArknights;

    public bool ExitArknights
    {
        get => _exitArknights;
        set
        {
            if (!SetAndNotify(ref _exitArknights, value))
            {
                return;
            }

            if (value)
            {
                BackToAndroidHome = false;
            }

            UpdatePostAction(PostActions.ExitArknights, value);
        }
    }

    private bool _backToAndroidHome;

    public bool BackToAndroidHome
    {
        get => _backToAndroidHome;
        set
        {
            if (!SetAndNotify(ref _backToAndroidHome, value))
            {
                return;
            }

            if (value)
            {
                ExitArknights = false;
            }

            UpdatePostAction(PostActions.BackToAndroidHome, value);
        }
    }

    private bool _exitEmulator;

    public bool ExitEmulator
    {
        get => _exitEmulator;
        set
        {
            if (!SetAndNotify(ref _exitEmulator, value))
            {
                return;
            }

            if (value)
            {
                ExitArknights = false;
                BackToAndroidHome = false;
            }
            else
            {
                IfNoOtherMaa = false;
            }

            UpdatePostAction(PostActions.ExitEmulator, value);
        }
    }

    private bool _exitSelf;

    public bool ExitSelf
    {
        get => _exitSelf;
        set
        {
            if (!SetAndNotify(ref _exitSelf, value))
            {
                return;
            }

            if (!value)
            {
                IfNoOtherMaa = false;
            }

            UpdatePostAction(PostActions.ExitSelf, value);
        }
    }

    private bool _ifNoOtherMaa;

    public bool IfNoOtherMaa
    {
        get => _ifNoOtherMaa;
        set
        {
            if (!SetAndNotify(ref _ifNoOtherMaa, value))
            {
                return;
            }

            if (value)
            {
                ExitEmulator = true;
                ExitSelf = true;
            }

            UpdatePostAction(PostActions.IfNoOtherMaa, value);
        }
    }

    private bool _hibernate;

    public bool Hibernate
    {
        get => _hibernate;
        set
        {
            if (!SetAndNotify(ref _hibernate, value))
            {
                return;
            }

            if (value)
            {
                Shutdown = false;
            }
            else if (!Shutdown)
            {
                IfNoOtherMaa = false;
            }

            UpdatePostAction(PostActions.Hibernate, value);
        }
    }

    private bool _shutdown;

    public bool Shutdown
    {
        get => _shutdown;
        set
        {
            if (!SetAndNotify(ref _shutdown, value))
            {
                return;
            }

            if (value)
            {
                ExitArknights = false;
                BackToAndroidHome = false;
                Hibernate = false;
            }
            else if (!Hibernate)
            {
                IfNoOtherMaa = false;
            }

            UpdatePostAction(PostActions.Shutdown, value);
        }
    }

    public static PostActionSetting Current { get; } = new();

    private string _actionTitle = LocalizationHelper.GetString("PostActions");

    public string ActionTitle
    {
        get => _actionTitle;
        private set => SetAndNotify(ref _actionTitle, value);
    }

    private string _actionDescription = string.Empty;

    public string ActionDescription
    {
        get => _actionDescription;
        private set => SetAndNotify(ref _actionDescription, value);
    }

    public void PostActionsAndOnce(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton != MouseButton.Right)
        {
            return;
        }

        CheckBox checkBox = (CheckBox)sender;
        Once = !checkBox.IsChecked ?? false;
        checkBox.IsChecked = !checkBox.IsChecked;
    }

    private void RefreshDescription()
    {
        List<string> actions = [];

        if (BackToAndroidHome)
        {
            actions.Add(LocalizationHelper.GetString("BackToAndroidHome"));
        }

        if (ExitArknights)
        {
            actions.Add(LocalizationHelper.GetString("ExitArknights"));
        }

        if (ExitEmulator)
        {
            actions.Add(LocalizationHelper.GetString("ExitEmulator"));
        }

        if (ExitSelf)
        {
            actions.Add(LocalizationHelper.GetString("ExitSelf"));
        }

        var prefix = IfNoOtherMaa ? LocalizationHelper.GetString("IfNoOtherMaa") : string.Empty;
        if (Hibernate)
        {
            actions.Add(prefix + LocalizationHelper.GetString("Hibernate"));
        }

        if (Shutdown)
        {
            actions.Add(prefix + LocalizationHelper.GetString("Shutdown"));
        }

        ActionDescription = actions.Count == 0
            ? LocalizationHelper.GetString("DoNothing")
            : string.Join(" -> ", actions);
    }

    private void UpdatePostAction(PostActions postActions, bool value)
    {
        if (value)
        {
            _postActions |= postActions;
        }
        else
        {
            _postActions &= ~postActions;
        }

        RefreshDescription();
        SaveActions();
    }

    private void SaveActions()
    {
        if (!_once)
        {
            ConfigurationHelper.SetValue(ConfigurationKeys.PostActions, JsonConvert.SerializeObject(_postActions));
        }
    }

    public void LoadPostActions()
    {
        _postActions = JsonConvert.DeserializeObject<PostActions>(ConfigurationHelper.GetValue(ConfigurationKeys.PostActions, "0"));
        ExitArknights = _postActions.HasFlag(PostActions.ExitArknights);
        BackToAndroidHome = _postActions.HasFlag(PostActions.BackToAndroidHome);
        ExitEmulator = _postActions.HasFlag(PostActions.ExitEmulator);
        ExitSelf = _postActions.HasFlag(PostActions.ExitSelf);
        IfNoOtherMaa = _postActions.HasFlag(PostActions.IfNoOtherMaa);
        Hibernate = _postActions.HasFlag(PostActions.Hibernate);
        Shutdown = _postActions.HasFlag(PostActions.Shutdown);
        Once = false;
    }

    private PostActionSetting()
    {
        LoadPostActions();
        RefreshDescription();
    }
}
