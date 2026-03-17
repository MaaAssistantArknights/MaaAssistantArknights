using Avalonia;
using Avalonia.Controls;

namespace MAAUnified.App.Controls;

public partial class TooltipHint : UserControl
{
    public static readonly StyledProperty<string?> TipProperty =
        AvaloniaProperty.Register<TooltipHint, string?>(nameof(Tip));

    public static readonly StyledProperty<string> GlyphTextProperty =
        AvaloniaProperty.Register<TooltipHint, string>(nameof(GlyphText), "?");

    public static readonly StyledProperty<Thickness> GlyphMarginProperty =
        AvaloniaProperty.Register<TooltipHint, Thickness>(nameof(GlyphMargin), new Thickness(0));

    public TooltipHint()
    {
        InitializeComponent();
        UpdateTooltipAvailability();
    }

    public string? Tip
    {
        get => GetValue(TipProperty);
        set => SetValue(TipProperty, value);
    }

    public string GlyphText
    {
        get => GetValue(GlyphTextProperty);
        set => SetValue(GlyphTextProperty, value);
    }

    public Thickness GlyphMargin
    {
        get => GetValue(GlyphMarginProperty);
        set => SetValue(GlyphMarginProperty, value);
    }

    protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
    {
        base.OnPropertyChanged(change);
        if (change.Property == TipProperty)
        {
            UpdateTooltipAvailability();
        }
    }

    private void UpdateTooltipAvailability()
    {
        if (GlyphHost is null)
        {
            return;
        }

        ToolTip.SetServiceEnabled(GlyphHost, !string.IsNullOrWhiteSpace(Tip));
    }
}
