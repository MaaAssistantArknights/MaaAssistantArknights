using System.Collections.ObjectModel;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class AchievementListDialogView : Window
{
    private readonly ObservableCollection<AchievementListItem> _visibleItems = [];
    private IReadOnlyList<AchievementListItem> _allItems = [];

    public AchievementListDialogView()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
        AchievementList.ItemsSource = _visibleItems;
    }

    public void ApplyRequest(AchievementListDialogRequest request)
    {
        Title = request.Title;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
        _allItems = request.Items;
        FilterInput.Text = request.InitialFilter ?? string.Empty;
        ApplyFilter(FilterInput.Text ?? string.Empty);
    }

    public AchievementListDialogPayload BuildPayload()
    {
        var selectedIds = (AchievementList.SelectedItems ?? Array.Empty<object>())
            .OfType<AchievementListItem>()
            .Select(item => item.Id)
            .Distinct(StringComparer.Ordinal)
            .ToArray();
        return new AchievementListDialogPayload(FilterInput.Text ?? string.Empty, selectedIds);
    }

    private void ApplyFilter(string filter)
    {
        _visibleItems.Clear();
        var normalized = (filter ?? string.Empty).Trim();
        IEnumerable<AchievementListItem> filtered = _allItems;
        if (normalized.Length > 0)
        {
            filtered = filtered.Where(item =>
                item.Title.Contains(normalized, StringComparison.OrdinalIgnoreCase)
                || item.Description.Contains(normalized, StringComparison.OrdinalIgnoreCase)
                || item.Status.Contains(normalized, StringComparison.OrdinalIgnoreCase));
        }

        foreach (var item in filtered)
        {
            _visibleItems.Add(item);
        }
    }

    private void OnFilterChanged(object? sender, TextChangedEventArgs e)
    {
        ApplyFilter(FilterInput.Text ?? string.Empty);
    }

    private void OnConfirmClick(object? sender, RoutedEventArgs e)
    {
        Close(DialogReturnSemantic.Confirm);
    }

    private void OnCancelClick(object? sender, RoutedEventArgs e)
    {
        Close(DialogReturnSemantic.Cancel);
    }
}
