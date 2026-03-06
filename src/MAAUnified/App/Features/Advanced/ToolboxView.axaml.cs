using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Toolbox;

namespace MAAUnified.App.Features.Advanced;

public partial class ToolboxView : UserControl
{
    public ToolboxView()
    {
        InitializeComponent();
    }

    private ToolboxPageViewModel? VM => DataContext as ToolboxPageViewModel;

    private async void OnExecuteClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ExecuteCurrentToolAsync();
        }
    }

    private async void OnExecuteSuccessPresetClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        VM.ApplySuccessPresetForCurrentTool();
        await VM.ExecuteCurrentToolAsync();
    }

    private async void OnExecuteFailurePresetClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        VM.ApplyFailurePresetForCurrentTool();
        await VM.ExecuteCurrentToolAsync();
    }
}
