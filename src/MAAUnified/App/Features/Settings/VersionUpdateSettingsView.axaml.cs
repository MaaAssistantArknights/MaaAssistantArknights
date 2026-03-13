using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class VersionUpdateSettingsView : UserControl
{
    public VersionUpdateSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnCheckUpdateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.CheckVersionUpdateAsync();
        }
    }

    private async void OnUpdateResourceClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ManualUpdateResourceAsync();
        }
    }

    private async void OnRefreshResourceInfoClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RefreshVersionUpdateResourceInfoAsync();
        }
    }

    private async void OnOpenChangelogClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenVersionUpdateChangelogAsync();
        }
    }

    private async void OnOpenResourceRepositoryClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenVersionUpdateResourceRepositoryAsync();
        }
    }

    private async void OnOpenMirrorChyanClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenVersionUpdateMirrorChyanAsync();
        }
    }

    private async void OnCopyMirrorChyanCdkClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || string.IsNullOrWhiteSpace(VM.VersionUpdateMirrorChyanCdk))
        {
            return;
        }

        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.Clipboard is null)
        {
            return;
        }

        await topLevel.Clipboard.SetTextAsync(VM.VersionUpdateMirrorChyanCdk);
    }
}
