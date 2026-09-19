// <copyright file="MaaHotKeyManager.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Windows.Input;
using GlobalHotKey;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Services.HotKeys;

public class MaaHotKeyManager : IMaaHotKeyManager
{
    private readonly Dictionary<MaaHotKeyAction, MaaHotKey> _actionHotKeyMapping = [];

    // 注册失败（被占用/与其他条目重复）的条目仍保留在 mapping 中作为当前配置，
    // 由该集合区分 ｢已生效｣ 与 ｢未生效但保留｣ ，避免编辑框显示与配置文件脱节
    private readonly HashSet<MaaHotKeyAction> _failedRegistrations = [];

    public MaaHotKeyManager()
    {
        Instances.HotKeyManager.KeyPressed += HotKeyManagerPressed;

        foreach (var kvPair in GetPersistentHotKeys())
        {
            // 清空过的热键在配置中是 null 条目，没有可注册的内容，跳过以免误入失败标记
            if (kvPair.Value is null)
            {
                continue;
            }

            TryRegister(kvPair.Key, kvPair.Value);
        }
    }

    public MaaHotKeyRegistrationResult TryRegister(MaaHotKeyAction action, MaaHotKey hotKey)
    {
        InternalUnRegister(action);

        var hotKeyOwner = _actionHotKeyMapping.FirstOrDefault(x => x.Value != null && x.Value.Equals(hotKey));

        if (hotKeyOwner.Value != null)
        {
            _actionHotKeyMapping[action] = hotKey;
            _failedRegistrations.Add(action);
            PersistSilently();
            return MaaHotKeyRegistrationResult.DuplicateHotKey;
        }

        try
        {
            Instances.HotKeyManager.Register(hotKey);
            _actionHotKeyMapping[action] = hotKey;
        }
        catch
        {
            _actionHotKeyMapping[action] = hotKey;
            _failedRegistrations.Add(action);
            PersistSilently();
            return MaaHotKeyRegistrationResult.OccupiedByOtherApp;
        }

        PersistSilently();
        return MaaHotKeyRegistrationResult.Success;
    }

    public void UnRegister(MaaHotKeyAction action)
    {
        InternalUnRegister(action);

        PersistSilently();
    }

    public void Release()
    {
        foreach (var kvPair in _actionHotKeyMapping)
        {
            InternalUnRegister(kvPair.Key);
        }
    }

    public bool IsRegistrationFailed(MaaHotKeyAction action)
    {
        return _failedRegistrations.Contains(action);
    }

    private void InternalUnRegister(MaaHotKeyAction action)
    {
        bool wasFailed = _failedRegistrations.Remove(action);

        if (!_actionHotKeyMapping.TryGetValue(action, out var value) || value == null)
        {
            return;
        }

        // 失败保留的条目没有注册到系统，不能对其调用 Unregister
        if (!wasFailed)
        {
            Instances.HotKeyManager.Unregister(value);
        }

        _actionHotKeyMapping[action] = null;
    }

    public MaaHotKey GetOrNull(MaaHotKeyAction action)
    {
        return _actionHotKeyMapping.GetValueOrDefault(action);
    }

    private void HotKeyManagerPressed(object sender, KeyPressedEventArgs e)
    {
        // 失败保留的条目未注册到系统，不会触发回调，排除以免与生效条目同组合时误派发
        var action = _actionHotKeyMapping
            .Where(x => x.Value != null && x.Value.Equals(e.HotKey) && !_failedRegistrations.Contains(x.Key))
            .Select(x => x.Key)
            .FirstOrDefault();
        Instances.MaaHotKeyActionHandler.HandleKeyPressed(action);
    }

    private static Dictionary<MaaHotKeyAction, MaaHotKey> GetPersistentHotKeys()
    {
        var hotKeys = new Dictionary<MaaHotKeyAction, MaaHotKey>(ConfigFactory.Root.Gui.HotKeys);
        return hotKeys == null || hotKeys.Count == 0 ? CreateInitialHotKeys() : hotKeys;
    }

    private static Dictionary<MaaHotKeyAction, MaaHotKey> CreateInitialHotKeys()
    {
        var hotKeys = new Dictionary<MaaHotKeyAction, MaaHotKey>
        {
            {
                MaaHotKeyAction.ShowGui, new MaaHotKey(Key.M, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt)
            },
            {
                MaaHotKeyAction.LinkStart, new MaaHotKey(Key.L, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt)
            },
        };

        return hotKeys;
    }

    private void PersistHotKeys()
    {
        ConfigFactory.Root.Gui.HotKeys = new(_actionHotKeyMapping);
    }

    private void PersistSilently()
    {
        try
        {
            PersistHotKeys();
        }
        catch
        {
            // ignored
        }
    }
}
