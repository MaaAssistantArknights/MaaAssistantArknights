using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Copilot;

namespace MAAUnified.App.Features.Advanced;

public partial class CopilotView : UserControl
{
    public CopilotView()
    {
        InitializeComponent();
    }

    private CopilotPageViewModel? VM => DataContext as CopilotPageViewModel;

    private async void OnImportFileClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ImportFromFileAsync();
        }
    }

    private async void OnImportClipboardClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ImportFromClipboardAsync(VM.FilePath);
        }
    }

    private async void OnStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartAsync();
        }
    }

    private async void OnStopClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StopAsync();
        }
    }

    private async void OnAddItemClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.AddEmptyTaskAsync();
        }
    }

    private async void OnRemoveItemClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RemoveSelectedAsync();
        }
    }

    private async void OnClearClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ClearAllAsync();
        }
    }

    private async void OnLikeClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SendLikeAsync(true);
        }
    }
}
