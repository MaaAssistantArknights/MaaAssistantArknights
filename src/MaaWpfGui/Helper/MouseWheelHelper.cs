using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MaaWpfGui.Helper
{
    public static class MouseWheelHelper
    {
        public static void RouteMouseWheelToParent(object sender, MouseWheelEventArgs e)
        {
            if (e.Handled)
            {
                return;
            }

            e.Handled = true;
            var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta)
            {
                RoutedEvent = UIElement.MouseWheelEvent,
            };
            var parent = ((Control)sender).Parent as UIElement;
            parent?.RaiseEvent(eventArg);
        }

        public static void DisableComboBoxScrollWhenClosed(object sender, MouseWheelEventArgs e)
        {
            if (sender is ComboBox { IsDropDownOpen: false })
            {
                e.Handled = true; // 阻止滚动
            }
        }
    }
}
