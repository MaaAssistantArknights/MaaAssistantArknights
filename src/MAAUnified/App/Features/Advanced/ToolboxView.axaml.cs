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
}
