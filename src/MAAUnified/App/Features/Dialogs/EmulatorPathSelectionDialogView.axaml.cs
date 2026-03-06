using System.Collections.ObjectModel;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class EmulatorPathSelectionDialogView : Window
{
    private readonly ObservableCollection<string> _paths = [];

    public EmulatorPathSelectionDialogView()
    {
        InitializeComponent();
        PathList.ItemsSource = _paths;
        PathList.SelectionChanged += OnPathSelectionChanged;
    }

    public void ApplyRequest(EmulatorPathDialogRequest request)
    {
        Title = request.Title;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
        _paths.Clear();
        foreach (var path in request.CandidatePaths.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            _paths.Add(path);
        }

        var selected = request.SelectedPath;
        PathList.SelectedItem = string.IsNullOrWhiteSpace(selected)
            ? _paths.FirstOrDefault()
            : _paths.FirstOrDefault(p => string.Equals(p, selected, StringComparison.OrdinalIgnoreCase));
        PathInput.Text = selected ?? (PathList.SelectedItem as string) ?? string.Empty;
    }

    public EmulatorPathDialogPayload? BuildPayload()
    {
        var selected = (PathInput.Text ?? string.Empty).Trim();
        if (selected.Length == 0)
        {
            return null;
        }

        return new EmulatorPathDialogPayload(selected);
    }

    private void OnPathSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (PathList.SelectedItem is string selected)
        {
            PathInput.Text = selected;
        }
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
