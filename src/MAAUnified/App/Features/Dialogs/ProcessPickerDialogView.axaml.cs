using System.Collections.ObjectModel;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class ProcessPickerDialogView : Window
{
    private readonly ObservableCollection<ProcessPickerItem> _items = [];

    public ProcessPickerDialogView()
    {
        InitializeComponent();
        ProcessList.ItemsSource = _items;
    }

    public void ApplyRequest(ProcessPickerDialogRequest request)
    {
        Title = request.Title;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
        _items.Clear();
        foreach (var item in request.Items.OrderByDescending(i => i.IsPrimary).ThenBy(i => i.DisplayName, StringComparer.OrdinalIgnoreCase))
        {
            _items.Add(item);
        }

        if (!string.IsNullOrWhiteSpace(request.SelectedId))
        {
            ProcessList.SelectedItem = _items.FirstOrDefault(i => string.Equals(i.Id, request.SelectedId, StringComparison.Ordinal));
        }

        ProcessList.SelectedItem ??= _items.FirstOrDefault();
    }

    public ProcessPickerDialogPayload? BuildPayload()
    {
        if (ProcessList.SelectedItem is not ProcessPickerItem selected)
        {
            return null;
        }

        return new ProcessPickerDialogPayload(selected.Id, selected.DisplayName);
    }

    private void OnRefreshClick(object? sender, RoutedEventArgs e)
    {
        var selected = (ProcessList.SelectedItem as ProcessPickerItem)?.Id;
        var ordered = _items
            .OrderByDescending(i => i.IsPrimary)
            .ThenBy(i => i.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToList();
        _items.Clear();
        foreach (var item in ordered)
        {
            _items.Add(item);
        }

        ProcessList.SelectedItem = _items.FirstOrDefault(i => string.Equals(i.Id, selected, StringComparison.Ordinal)) ?? _items.FirstOrDefault();
    }

    private void OnConfirmClick(object? sender, RoutedEventArgs e)
    {
        if (BuildPayload() is null)
        {
            return;
        }

        Close(DialogReturnSemantic.Confirm);
    }

    private void OnCancelClick(object? sender, RoutedEventArgs e)
    {
        Close(DialogReturnSemantic.Cancel);
    }
}
