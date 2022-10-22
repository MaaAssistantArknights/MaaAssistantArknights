using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Input;
using GlobalHotKey;
using Newtonsoft.Json;
using StyletIoC;

namespace MeoAsstGui.MaaHotKeys
{
    public class MaaHotKeyManager : IMaaHotKeyManager
    {
        private readonly Dictionary<MaaHotKeyAction, MaaHotKey> _actionHotKeyMapping =
            new Dictionary<MaaHotKeyAction, MaaHotKey>();

        private const string HotKeyConfigName = "HotKeys";

        private readonly HotKeyManager _hotKeyManager;
        private readonly IMaaHotKeyActionHandler _actionHandler;

        public MaaHotKeyManager(IContainer container)
        {
            _hotKeyManager = container.Get<HotKeyManager>();
            _actionHandler = container.Get<IMaaHotKeyActionHandler>();
            _hotKeyManager.KeyPressed += HotKeyManagerPressed;

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
                _hotKeyManager.Register(hotKey);
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

            _hotKeyManager.Unregister(_actionHotKeyMapping[action]);
            _actionHotKeyMapping[action] = null;
        }

        public MaaHotKey GetOrNull(MaaHotKeyAction action)
        {
            return _actionHotKeyMapping.ContainsKey(action) ? _actionHotKeyMapping[action] : null;
        }

        private void HotKeyManagerPressed(object sender, KeyPressedEventArgs e)
        {
            var action = _actionHotKeyMapping.Where(x => x.Value.Equals(e.HotKey)).Select(x => x.Key).FirstOrDefault();
            _actionHandler.HandleKeyPressed(action);
        }

        private Dictionary<MaaHotKeyAction, MaaHotKey> GetPersistentHotKeys()
        {
            var hotkeysString = ViewStatusStorage.Get(HotKeyConfigName, null);

            return hotkeysString is null
                ? CreateInitialHotKeys()
                : JsonConvert.DeserializeObject<Dictionary<MaaHotKeyAction, MaaHotKey>>(hotkeysString);
        }

        private Dictionary<MaaHotKeyAction, MaaHotKey> CreateInitialHotKeys()
        {
            var hotKeys = new Dictionary<MaaHotKeyAction, MaaHotKey>();

            hotKeys.Add(MaaHotKeyAction.ShowGui,
                new MaaHotKey(Key.M, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt));

            hotKeys.Add(MaaHotKeyAction.LinkStart,
                new MaaHotKey(Key.L, ModifierKeys.Control | ModifierKeys.Shift | ModifierKeys.Alt));

            return hotKeys;
        }

        private void PersistHotKeys()
        {
            ViewStatusStorage.Set(HotKeyConfigName, JsonConvert.SerializeObject(_actionHotKeyMapping));
        }
    }
}
