using System.Text;
using System.Windows.Input;
using GlobalHotKey;

namespace MeoAsstGui.MaaHotKeys
{
    public class MaaHotKey : HotKey
    {
        public MaaHotKey()
        {
        }

        public MaaHotKey(Key key, ModifierKeys modifiers)
            : base(key, modifiers)
        {
        }

        public override string ToString()
        {
            var str = new StringBuilder();

            if (Modifiers.HasFlag(ModifierKeys.Control))
            {
                str.Append("Ctrl + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Shift))
            {
                str.Append("Shift + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Alt))
            {
                str.Append("Alt + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Windows))
            {
                str.Append("Win + ");
            }

            str.Append(Key);

            return str.ToString();
        }
    }
}
