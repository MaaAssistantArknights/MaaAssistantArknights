using GlobalHotKey;

namespace MeoAsstGui.MaaHotKeys
{
    public interface IMaaHotKeyManager
    {
        bool TryRegister(MaaHotKeyAction action, MaaHotKey hotKey);

        void Unregister(MaaHotKeyAction action);

        MaaHotKey GetOrNull(MaaHotKeyAction action);
    }
}
