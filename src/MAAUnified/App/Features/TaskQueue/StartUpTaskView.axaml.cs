using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.TaskQueue;

public partial class StartUpTaskView : UserControl
{
    public StartUpTaskView()
    {
        InitializeComponent();
    }

    private StartUpTaskModuleViewModel? VM => DataContext as StartUpTaskModuleViewModel;

    private async void OnAccountSwitchManualRunClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RunAccountSwitchManualAsync();
        }
    }

    private async void OnSelectAdbPathClick(object? sender, RoutedEventArgs e)
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
                Title = vm.Texts.GetOrDefault("StartUp.Select", "Select"),
            });
        var selected = files.FirstOrDefault();
        if (selected is null)
        {
            return;
        }

        var path = selected.TryGetLocalPath();
        if (!string.IsNullOrWhiteSpace(path))
        {
            vm.AdbPath = path;
        }
    }
}
