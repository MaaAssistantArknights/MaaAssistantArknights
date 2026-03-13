using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.TaskQueue;

public partial class InfrastSettingsView : UserControl
{
    public InfrastSettingsView()
    {
        InitializeComponent();
    }

    private InfrastModuleViewModel? VM => DataContext as InfrastModuleViewModel;

    private async void OnReloadPlansClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ReloadPlansAsync();
        }
    }

    private async void OnSelectCustomFileClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.StorageProvider is not { CanOpen: true } storageProvider)
        {
            return;
        }

        var files = await storageProvider.OpenFilePickerAsync(
            new FilePickerOpenOptions
            {
                AllowMultiple = false,
                Title = vm.Texts.GetOrDefault("Infrast.Select", "Select"),
                FileTypeFilter =
                [
                    new FilePickerFileType("json")
                    {
                        Patterns = ["*.json"],
                    },
                ],
            });
        var selected = files.FirstOrDefault();
        if (selected is null)
        {
            return;
        }

        var path = selected.TryGetLocalPath();
        if (!string.IsNullOrWhiteSpace(path))
        {
            vm.SelectCustomFile(path);
            await vm.ReloadPlansAsync();
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
