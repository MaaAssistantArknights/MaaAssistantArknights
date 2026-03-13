using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;
using Avalonia.Interactivity;

namespace MAAUnified.App.Controls;

public partial class VerticalSpinNumberBox : UserControl
{
    public static readonly StyledProperty<int> MinimumProperty =
        AvaloniaProperty.Register<VerticalSpinNumberBox, int>(nameof(Minimum), 0);

    public static readonly StyledProperty<int> MaximumProperty =
        AvaloniaProperty.Register<VerticalSpinNumberBox, int>(nameof(Maximum), int.MaxValue);

    public static readonly StyledProperty<int> IncrementProperty =
        AvaloniaProperty.Register<VerticalSpinNumberBox, int>(nameof(Increment), 1);

    public static readonly StyledProperty<int> ValueProperty =
        AvaloniaProperty.Register<VerticalSpinNumberBox, int>(
            nameof(Value),
            defaultValue: 0,
            defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<string> FormatStringProperty =
        AvaloniaProperty.Register<VerticalSpinNumberBox, string>(nameof(FormatString), "F0");

    static VerticalSpinNumberBox()
    {
        MinimumProperty.Changed.AddClassHandler<VerticalSpinNumberBox>((box, _) => box.CoerceValueWithinRange());
        MaximumProperty.Changed.AddClassHandler<VerticalSpinNumberBox>((box, _) => box.CoerceValueWithinRange());
        ValueProperty.Changed.AddClassHandler<VerticalSpinNumberBox>((box, _) => box.CoerceValueWithinRange());
    }

    public VerticalSpinNumberBox()
    {
        InitializeComponent();
    }

    public int Minimum
    {
        get => GetValue(MinimumProperty);
        set => SetValue(MinimumProperty, value);
    }

    public int Maximum
    {
        get => GetValue(MaximumProperty);
        set => SetValue(MaximumProperty, value);
    }

    public int Increment
    {
        get => GetValue(IncrementProperty);
        set => SetValue(IncrementProperty, value);
    }

    public int Value
    {
        get => GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    public string FormatString
    {
        get => GetValue(FormatStringProperty);
        set => SetValue(FormatStringProperty, value);
    }

    private void OnIncreaseClick(object? sender, RoutedEventArgs e)
    {
        Step(+1);
    }

    private void OnDecreaseClick(object? sender, RoutedEventArgs e)
    {
        Step(-1);
    }

    private void Step(int direction)
    {
        if (!IsEnabled || direction == 0)
        {
            return;
        }

        var step = Increment <= 0 ? 1 : Increment;
        var stepped = (long)Value + ((long)step * direction);
        SetCurrentValue(ValueProperty, ClampToRange(stepped));
    }

    private void CoerceValueWithinRange()
    {
        var clamped = ClampToRange(Value);
        if (clamped != Value)
        {
            SetCurrentValue(ValueProperty, clamped);
        }
    }

    private int ClampToRange(long value)
    {
        var min = Math.Min(Minimum, Maximum);
        var max = Math.Max(Minimum, Maximum);
        if (value < min)
        {
            return min;
        }

        if (value > max)
        {
            return max;
        }

        return (int)value;
    }
}
