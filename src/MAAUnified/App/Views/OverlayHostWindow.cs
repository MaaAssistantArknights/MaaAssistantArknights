using Avalonia.Controls;
using Avalonia.Media;

namespace MAAUnified.App.Views;

public sealed class OverlayHostWindow : Window
{
    public OverlayHostWindow()
    {
        Width = 1;
        Height = 1;
        MinWidth = 1;
        MinHeight = 1;
        Topmost = true;
        ShowInTaskbar = false;
        CanResize = false;
        IsHitTestVisible = false;
        Focusable = false;
        SystemDecorations = SystemDecorations.None;
        Background = Brushes.Transparent;
        TransparencyLevelHint = new[]
        {
            WindowTransparencyLevel.Transparent,
            WindowTransparencyLevel.AcrylicBlur,
        };
        Opacity = 0.85;
    }
}
