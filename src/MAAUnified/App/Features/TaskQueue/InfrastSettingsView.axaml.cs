using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.TaskQueue;

public partial class InfrastSettingsView : UserControl
{
    public InfrastSettingsView()
    {
        InitializeComponent();
        DataContextChanged += (_, _) => SyncModeCombo();
    }

    private InfrastModuleViewModel? VM => DataContext as InfrastModuleViewModel;

    private void SyncModeCombo()
    {
        if (VM is null)
        {
            return;
        }

        ModeCombo.SelectedIndex = VM.Mode switch
        {
            10000 => 1,
            20000 => 2,
            _ => 0,
        };
    }

    private void OnModeSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (VM is null || sender is not ComboBox combo)
        {
            return;
        }

        VM.Mode = combo.SelectedIndex switch
        {
            1 => 10000,
            2 => 20000,
            _ => 0,
        };
    }

    private async void OnReloadPlansClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ReloadPlansAsync();
        }
    }

    private void OnSelectAllFacilityClick(object? sender, RoutedEventArgs e)
    {
        VM?.SelectAllFacility();
    }

    private void OnClearFacilityClick(object? sender, RoutedEventArgs e)
    {
        VM?.ClearFacility();
    }
}
