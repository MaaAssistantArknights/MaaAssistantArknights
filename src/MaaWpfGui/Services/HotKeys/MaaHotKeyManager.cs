// <copyright file="MaaHotKeyManager.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using System.Linq;
using System.Windows.Input;
using GlobalHotKey;
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Services.HotKeys
{
    public class MaaHotKeyManager : IMaaHotKeyManager
    {
        private readonly Dictionary<MaaHotKeyAction, MaaHotKey> _actionHotKeyMapping =
            new Dictionary<MaaHotKeyAction, MaaHotKey>();

        private const string HotKeyConfigName = "HotKeys";

        public MaaHotKeyManager()
        {
            Instances.HotKeyManager.KeyPressed += HotKeyManagerPressed;

            foreach (var kvPair in GetPersistentHotKeys())
            {
                TryRegister(kvPair.Key, kvPair.Value);
            }
        }

        public bool TryRegister(MaaHotKeyAction action, MaaHotKey hotKey)
        {
            InternalUnregister(action);

            var hotKeyOwner = _actionHotKeyMapping.FirstOrDefault(x => x.Value != null && x.Value.Equals(hotKey));

            if (hotKeyOwner.Value != null)
            {
                return false;
            }

            try
            {
                Instances.HotKeyManager.Register(hotKey);
                _actionHotKeyMapping[action] = hotKey;
            }
            catch
            {
                return false;
            }

            try
            {
                PersistHotKeys();
            }
            catch
            {
                // ignored
            }

            return true;
        }

        public void Unregister(MaaHotKeyAction action)
        {
            InternalUnregister(action);

            try
            {
                PersistHotKeys();
            }
            catch
            {
                // ignored
            }
        }

        protected void InternalUnregister(MaaHotKeyAction action)
        {
            if (!_actionHotKeyMapping.ContainsKey(action) || _actionHotKeyMapping[action] == null)
            {
                return;
            }

            Instances.HotKeyManager.Unregister(_actionHotKeyMapping[action]);
            _actionHotKeyMapping[action] = null;
        }

        public MaaHotKey GetOrNull(MaaHotKeyAction action)
        {
            return _actionHotKeyMapping.ContainsKey(action) ? _actionHotKeyMapping[action] : null;
        }

        private void HotKeyManagerPressed(object sender, KeyPressedEventArgs e)
        {
            var action = _actionHotKeyMapping.Where(x => x.Value.Equals(e.HotKey)).Select(x => x.Key).FirstOrDefault();
            Instances.MaaHotKeyActionHandler.HandleKeyPressed(action);
        }

        private Dictionary<MaaHotKeyAction, MaaHotKey> GetPersistentHotKeys()
        {
            var hotkeysString = ConfigurationHelper.GetValue(HotKeyConfigName, null);

            return hotkeysString is null
                ? CreateInitialHotKeys()
                : JsonConvert.DeserializeObject<Dictionary<MaaHotKeyAction, MaaHotKey>>(hotkeysString);
        }

        private Dictionary<MaaHotKeyAction, MaaHotKey> CreateInitialHotKeys()
        {
            var hotKeys = new Dictionary<MaaHotKeyAction, MaaHotKey>
            {
                {
                    MaaHotKeyAction.ShowGui,
                    new MaaHotKey(Key.M, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt)
                },
                {
                    MaaHotKeyAction.LinkStart,
                    new MaaHotKey(Key.L, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt)
                },
            };

            return hotKeys;
        }

        private void PersistHotKeys()
        {
            ConfigurationHelper.SetValue(HotKeyConfigName, JsonConvert.SerializeObject(_actionHotKeyMapping));
        }
    }
}
