using System.Collections;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Templates;
using Avalonia.Data;
using Avalonia.Input;
using Avalonia.Interactivity;

namespace MAAUnified.App.Controls;

public sealed class CheckComboBoxSelectionCommittedEventArgs(object? selectedItem) : EventArgs
{
    public object? SelectedItem { get; } = selectedItem;
}

public partial class CheckComboBox : UserControl
{
    public static readonly StyledProperty<string> HeaderTextProperty =
        AvaloniaProperty.Register<CheckComboBox, string>(nameof(HeaderText), string.Empty);

    public static readonly StyledProperty<string> TextProperty =
        AvaloniaProperty.Register<CheckComboBox, string>(
            nameof(Text),
            string.Empty,
            defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<string> WatermarkProperty =
        AvaloniaProperty.Register<CheckComboBox, string>(nameof(Watermark), string.Empty);

    public static readonly StyledProperty<bool> IsEditableProperty =
        AvaloniaProperty.Register<CheckComboBox, bool>(nameof(IsEditable), false);

    public static readonly StyledProperty<object?> DropDownContentProperty =
        AvaloniaProperty.Register<CheckComboBox, object?>(nameof(DropDownContent));

    public static readonly StyledProperty<IEnumerable?> ItemsSourceProperty =
        AvaloniaProperty.Register<CheckComboBox, IEnumerable?>(nameof(ItemsSource));

    public static readonly StyledProperty<object?> SelectedItemProperty =
        AvaloniaProperty.Register<CheckComboBox, object?>(
            nameof(SelectedItem),
            defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<IDataTemplate?> ItemTemplateProperty =
        AvaloniaProperty.Register<CheckComboBox, IDataTemplate?>(nameof(ItemTemplate));

    public static readonly StyledProperty<bool> IsTreeModeProperty =
        AvaloniaProperty.Register<CheckComboBox, bool>(nameof(IsTreeMode), false);

    public static readonly StyledProperty<bool> IsDropDownOpenProperty =
        AvaloniaProperty.Register<CheckComboBox, bool>(
            nameof(IsDropDownOpen),
            defaultValue: false,
            defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<double> MaxDropDownHeightProperty =
        AvaloniaProperty.Register<CheckComboBox, double>(nameof(MaxDropDownHeight), 280d);

    public CheckComboBox()
    {
        InitializeComponent();
        DropDownPopup.PlacementTarget = ShellBorder;

        this.GetObservable(IsEditableProperty).Subscribe(_ => UpdateVisualState());
        this.GetObservable(ItemsSourceProperty).Subscribe(_ => UpdateVisualState());
        this.GetObservable(IsTreeModeProperty).Subscribe(_ => UpdateVisualState());
        UpdateVisualState();
    }

    public event EventHandler<CheckComboBoxSelectionCommittedEventArgs>? SelectionCommitted;

    public event EventHandler<EventArgs>? EditorCommitted;

    public string HeaderText
    {
        get => GetValue(HeaderTextProperty);
        set => SetValue(HeaderTextProperty, value);
    }

    public string Text
    {
        get => GetValue(TextProperty);
        set => SetValue(TextProperty, value);
    }

    public string Watermark
    {
        get => GetValue(WatermarkProperty);
        set => SetValue(WatermarkProperty, value);
    }

    public bool IsEditable
    {
        get => GetValue(IsEditableProperty);
        set => SetValue(IsEditableProperty, value);
    }

    public object? DropDownContent
    {
        get => GetValue(DropDownContentProperty);
        set => SetValue(DropDownContentProperty, value);
    }

    public IEnumerable? ItemsSource
    {
        get => GetValue(ItemsSourceProperty);
        set => SetValue(ItemsSourceProperty, value);
    }

    public object? SelectedItem
    {
        get => GetValue(SelectedItemProperty);
        set => SetValue(SelectedItemProperty, value);
    }

    public IDataTemplate? ItemTemplate
    {
        get => GetValue(ItemTemplateProperty);
        set => SetValue(ItemTemplateProperty, value);
    }

    public bool IsTreeMode
    {
        get => GetValue(IsTreeModeProperty);
        set => SetValue(IsTreeModeProperty, value);
    }

    public bool IsDropDownOpen
    {
        get => GetValue(IsDropDownOpenProperty);
        set => SetValue(IsDropDownOpenProperty, value);
    }

    public double MaxDropDownHeight
    {
        get => GetValue(MaxDropDownHeightProperty);
        set => SetValue(MaxDropDownHeightProperty, value);
    }

    private void UpdateVisualState()
    {
        EditableTextBox.IsVisible = IsEditable;
        HeaderTextBlock.IsVisible = !IsEditable;

        var useTreeMode = IsTreeMode;
        var useItemsSourceMode = !useTreeMode && ItemsSource is not null;
        var useCustomContentMode = !useTreeMode && ItemsSource is null;

        TreeModeView.IsVisible = useTreeMode;
        FlatListBox.IsVisible = useItemsSourceMode;
        CustomContentPresenter.IsVisible = useCustomContentMode;
    }

    private void OnShellPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (IsEditable)
        {
            return;
        }

        if (e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
        {
            var point = e.GetPosition(ShellBorder);
            if (point.X >= ShellBorder.Bounds.Width - ToggleButton.Bounds.Width)
            {
                return;
            }

            TogglePopup();
            e.Handled = true;
        }
    }

    private void OnToggleButtonClick(object? sender, RoutedEventArgs e)
    {
        TogglePopup();
    }

    private void OnPopupClosed(object? sender, EventArgs e)
    {
        IsDropDownOpen = false;
    }

    private void OnFlatListBoxSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (SelectedItem is null)
        {
            return;
        }

        CommitSelection(SelectedItem);
    }

    private void OnTreeViewSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (TreeModeView.SelectedItem is not { } selectedItem)
        {
            return;
        }

        if (!CanCommitSelection(selectedItem))
        {
            return;
        }

        SelectedItem = selectedItem;
        CommitSelection(selectedItem);
        TreeModeView.SelectedItem = null;
    }

    private void OnEditableTextBoxLostFocus(object? sender, RoutedEventArgs e)
    {
        EditorCommitted?.Invoke(this, EventArgs.Empty);
    }

    private void OnEditableTextBoxKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key != Key.Enter)
        {
            return;
        }

        e.Handled = true;
        EditorCommitted?.Invoke(this, EventArgs.Empty);
    }

    private void TogglePopup()
    {
        IsDropDownOpen = !IsDropDownOpen;
    }

    private void CommitSelection(object selectedItem)
    {
        IsDropDownOpen = false;
        SelectionCommitted?.Invoke(this, new CheckComboBoxSelectionCommittedEventArgs(selectedItem));
    }

    private static bool CanCommitSelection(object? item)
    {
        if (item is null)
        {
            return false;
        }

        if (TryReadBoolProperty(item, "CanSelect", out var canSelect))
        {
            return canSelect;
        }

        if (TryReadBoolProperty(item, "IsFolder", out var isFolder))
        {
            return !isFolder;
        }

        return true;
    }

    private static bool TryReadBoolProperty(object instance, string propertyName, out bool value)
    {
        value = false;
        var property = instance.GetType().GetProperty(propertyName, BindingFlags.Instance | BindingFlags.Public);
        if (property?.PropertyType != typeof(bool))
        {
            return false;
        }

        value = (bool)property.GetValue(instance)!;
        return true;
    }
}
