using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using MeoAsstGui.MaaHotKeys;

namespace MeoAsstGui
{
    public partial class HotKeyEditorUserControl : UserControl
    {
        public static readonly DependencyProperty HotKeyProperty =
            DependencyProperty.Register(nameof(HotKey), typeof(MaaHotKey),
                typeof(HotKeyEditorUserControl),
                new FrameworkPropertyMetadata(default(MaaHotKey),
                    FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public MaaHotKey HotKey
        {
            get => (MaaHotKey)GetValue(HotKeyProperty);
            set => SetValue(HotKeyProperty, value);
        }

        public HotKeyEditorUserControl()
        {
            InitializeComponent();
        }

        private void HotKeyTextBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            // Don't let the event pass further
            // because we don't want standard textbox shortcuts working
            e.Handled = true;

            // Get modifiers and key data
            var modifiers = Keyboard.Modifiers;
            var key = e.Key;

            // When Alt is pressed, SystemKey is used instead
            if (key == Key.System)
            {
                key = e.SystemKey;
            }

            // Remove hotkey if no modifiers
            if (modifiers == ModifierKeys.None)
            {
                HotKey = null;
                return;
            }

            // If no actual key was pressed - return
            if (key == Key.LeftCtrl ||
                key == Key.RightCtrl ||
                key == Key.LeftAlt ||
                key == Key.RightAlt ||
                key == Key.LeftShift ||
                key == Key.RightShift ||
                key == Key.LWin ||
                key == Key.RWin ||
                key == Key.Clear ||
                key == Key.OemClear ||
                key == Key.Apps)
            {
                return;
            }

            // Update the value
            HotKey = new MaaHotKey(key, modifiers);
        }
    }
}
